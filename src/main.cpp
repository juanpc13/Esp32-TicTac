/*
 * Esp32-TicTac
 * -------------------------------------------------------------
 * Base limpia con actualizacion por aire (ArduinoOTA) y botones
 * por tabla de configuracion. Compila para las dos placas:
 *
 *                   ESP32-S2 Saola-1      ESP32 clasico (TicTac)
 *   NeoPixel        GPIO3                 GPIO14
 *   Boton           GPIO4 a GND           GPIO4 a VCC
 *                   pull-up, activo LOW   pull-down, activo HIGH
 *   Hostname mDNS   tictac-s2.local       tictac-esp32.local
 *
 * La seleccion es automatica segun el env de platformio.ini: el
 * core define CONFIG_IDF_TARGET_ESP32S2 al compilar para el S2.
 *
 * Gestos del boton (configurables en CONFIG_BOTONES, abajo):
 *   1 clic   -> siguiente modo del NeoPixel
 *   2 clics  -> siguiente nivel de brillo
 *   3+ clics -> imprime el estado por serie
 *   larga    -> apaga / enciende el NeoPixel
 *
 * Las credenciales WiFi van en src/secrets.h, que NO se sube al
 * repo. Si acabas de clonar: copia src/secrets.example.h.
 *
 * IP por DHCP: se imprime al arrancar y cada 60 s, y se anuncia
 * por mDNS. El OTA no lleva password.
 *
 * Sube FW_VERSION en cada OTA para confirmar que la placa
 * realmente se actualizo.
 * -------------------------------------------------------------
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <OneButton.h>

#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #error "Falta src/secrets.h -- copia src/secrets.example.h y pon tus redes WiFi"
#endif

// ====================== CONFIGURACION ========================

#define FW_VERSION "0.2.0"

// ---- Diferencias de hardware entre las dos placas ----
#if CONFIG_IDF_TARGET_ESP32S2
  #define PIN_NEO          3
  #define PIN_BOTON        4
  #define BOTON_ACTIVO_BAJO  true    // pin a GND
  #define BOTON_PULLUP       true    // pull-up interno del ESP
  #define OTA_HOSTNAME     "tictac-s2"
#else   // ESP32 clasico: la placa del TicTac
  #define PIN_NEO          14
  #define PIN_BOTON        4
  #define BOTON_ACTIVO_BAJO  false   // pin a VCC
  #define BOTON_PULLUP       false   // pull-down en la placa
  #define OTA_HOSTNAME     "tictac-esp32"
#endif

#define NUM_PIXELS   1

// ====================== ESTADO GLOBAL ========================

WiFiMulti wifiMulti;
Adafruit_NeoPixel pixels(NUM_PIXELS, PIN_NEO, NEO_GRB + NEO_KHZ800);

enum Modo : uint8_t {
  MODO_ESTADO = 0,   // color segun estado de la conexion
  MODO_ROJO,
  MODO_VERDE,
  MODO_AZUL,
  MODO_ARCOIRIS,
  MODO_APAGADO,
  MODO_TOTAL
};

static Modo modo = MODO_ESTADO;

static const uint8_t NIVELES_BRILLO[] = {10, 40, 120, 255};
#define NUM_NIVELES (sizeof(NIVELES_BRILLO) / sizeof(NIVELES_BRILLO[0]))
static uint8_t indiceBrillo = 1;

static bool otaEnCurso        = false;
static uint8_t otaProgreso    = 0;     // 0..100
static bool otaConError       = false;
static uint32_t otaErrorDesde = 0;

static uint32_t ultimoChequeoWifi = 0;
static uint32_t ultimoRefrescoLed = 0;

// ====================== NEOPIXEL =============================

static void pintar(uint8_t r, uint8_t g, uint8_t b) {
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
}

// Parpadeo bloqueante corto, solo para avisos puntuales
static void destello(uint8_t r, uint8_t g, uint8_t b, uint8_t veces, uint16_t ms) {
  for (uint8_t i = 0; i < veces; i++) {
    pintar(r, g, b);
    delay(ms);
    pintar(0, 0, 0);
    delay(ms);
  }
}

// Onda triangular 0..255 para respiracion y parpadeos suaves
static uint8_t onda(uint16_t periodoMs) {
  uint32_t fase  = millis() % periodoMs;
  uint32_t mitad = periodoMs / 2;
  return (fase < mitad) ? (fase * 255 / mitad)
                        : ((periodoMs - fase) * 255 / mitad);
}

static void actualizarLed() {
  if (millis() - ultimoRefrescoLed < 20) return;   // ~50 fps
  ultimoRefrescoLed = millis();

  pixels.setBrightness(NIVELES_BRILLO[indiceBrillo]);

  // La actualizacion OTA siempre manda sobre el modo elegido
  if (otaEnCurso) {
    uint8_t v = map(otaProgreso, 0, 100, 0, 255);  // azul -> verde
    pintar(0, v, 255 - v);
    return;
  }
  if (otaConError) {
    // El aviso rojo dura 5 s y luego vuelve al modo normal, para no
    // quedar enganchado si una subida falla
    if (millis() - otaErrorDesde < 5000) {
      pintar(onda(300), 0, 0);
      return;
    }
    otaConError = false;
  }

  switch (modo) {
    case MODO_ESTADO:
      if (WiFi.status() == WL_CONNECTED) {
        pintar(0, onda(3000) / 2 + 20, 0);         // verde respirando
      } else {
        pintar(onda(500), onda(500) / 3, 0);       // naranja parpadeante
      }
      break;

    case MODO_ROJO:  pintar(255, 0, 0); break;
    case MODO_VERDE: pintar(0, 255, 0); break;
    case MODO_AZUL:  pintar(0, 0, 255); break;

    case MODO_ARCOIRIS: {
      uint16_t matiz = (uint16_t)((millis() * 10) % 65536);
      pixels.setPixelColor(0, pixels.gamma32(pixels.ColorHSV(matiz)));
      pixels.show();
      break;
    }

    case MODO_APAGADO:
    default: pintar(0, 0, 0); break;
  }
}

// ====================== BOTONES ==============================
// Un boton = una fila en CONFIG_BOTONES. La tabla es dato puro
// (vive en flash); el array botones[] guarda el estado en RAM.
// Agregar un boton es agregar una fila, no escribir codigo.

struct Boton;
typedef void (*AccionBoton)(Boton &);

struct ConfigBoton {
  uint8_t     pin;
  bool        activoEnBajo;      // true = el pin va a GND al pulsar
  bool        pullupInterno;     // false si la placa trae pull-down
  const char *nombre;            // aparece en los mensajes por serie
  uint16_t    msAntirrebote;     // ignora rebotes del contacto
  uint16_t    msEntreClics;      // ventana para encadenar clics
  uint16_t    msPulsacionLarga;  // a partir de aqui es pulsacion larga
  AccionBoton alClic;            // 1 clic
  AccionBoton alDobleClic;       // 2 clics
  AccionBoton alMultiClic;       // 3 o mas, ver ob.getNumberClicks()
  AccionBoton alPulsacionLarga;  // salta al cruzar msPulsacionLarga
};

struct Boton {
  OneButton          ob;
  const ConfigBoton *cfg;
};

// ---- Acciones disponibles ----

static void accSiguienteModo(Boton &b) {
  modo = (Modo)((modo + 1) % MODO_TOTAL);
  Serial.printf("[%s] 1 clic -> modo %u\n", b.cfg->nombre, (unsigned)modo);
}

static void accSiguienteBrillo(Boton &b) {
  indiceBrillo = (indiceBrillo + 1) % NUM_NIVELES;
  Serial.printf("[%s] 2 clics -> brillo %u\n",
                b.cfg->nombre, NIVELES_BRILLO[indiceBrillo]);
  destello(255, 255, 255, 1, 60);
}

static void accImprimirEstado(Boton &b) {
  Serial.printf("[%s] %d clics -> v%s | %s | IP %s | %s.local | RSSI %d dBm\n",
                b.cfg->nombre, b.ob.getNumberClicks(), FW_VERSION,
                WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(),
                OTA_HOSTNAME, WiFi.RSSI());
}

static void accApagarEncender(Boton &b) {
  modo = (modo == MODO_APAGADO) ? MODO_ESTADO : MODO_APAGADO;
  Serial.printf("[%s] pulsacion larga -> %s\n", b.cfg->nombre,
                (modo == MODO_APAGADO) ? "apagado" : "encendido");
}

// ---- Tabla de configuracion: esto es lo que se edita ----

static const ConfigBoton CONFIG_BOTONES[] = {
  { PIN_BOTON, BOTON_ACTIVO_BAJO, BOTON_PULLUP, "principal",
    50,    // antirrebote
    400,   // ventana entre clics
    800,   // umbral de pulsacion larga
    accSiguienteModo,
    accSiguienteBrillo,
    accImprimirEstado,
    accApagarEncender },
};

#define NUM_BOTONES (sizeof(CONFIG_BOTONES) / sizeof(CONFIG_BOTONES[0]))
static Boton botones[NUM_BOTONES];

// ---- Puentes entre OneButton y la tabla ----
// OneButton entrega un void*; aqui se recupera el Boton y se
// despacha a la accion que diga su configuracion.

static void puenteClic(void *p) {
  Boton &b = *(Boton *)p;
  if (b.cfg->alClic) b.cfg->alClic(b);
}
static void puenteDobleClic(void *p) {
  Boton &b = *(Boton *)p;
  if (b.cfg->alDobleClic) b.cfg->alDobleClic(b);
}
static void puenteMultiClic(void *p) {
  Boton &b = *(Boton *)p;
  if (b.cfg->alMultiClic) b.cfg->alMultiClic(b);
}
static void puentePulsacionLarga(void *p) {
  Boton &b = *(Boton *)p;
  if (b.cfg->alPulsacionLarga) b.cfg->alPulsacionLarga(b);
}

static void configurarBotones() {
  for (size_t i = 0; i < NUM_BOTONES; i++) {
    const ConfigBoton *c = &CONFIG_BOTONES[i];
    Boton &b = botones[i];
    b.cfg = c;

    b.ob.setup(c->pin, c->pullupInterno ? INPUT_PULLUP : INPUT, c->activoEnBajo);
    b.ob.setDebounceMs(c->msAntirrebote);
    b.ob.setClickMs(c->msEntreClics);
    b.ob.setPressMs(c->msPulsacionLarga);

    if (c->alClic)           b.ob.attachClick(puenteClic, &b);
    if (c->alDobleClic)      b.ob.attachDoubleClick(puenteDobleClic, &b);
    if (c->alMultiClic)      b.ob.attachMultiClick(puenteMultiClic, &b);
    if (c->alPulsacionLarga) b.ob.attachLongPressStart(puentePulsacionLarga, &b);

    Serial.printf("[BOTON] \"%s\" en GPIO%u, activo en %s\n",
                  c->nombre, c->pin, c->activoEnBajo ? "LOW" : "HIGH");
  }
}

static void leerBotones() {
  for (size_t i = 0; i < NUM_BOTONES; i++) botones[i].ob.tick();
}

// ====================== WIFI + OTA ===========================

static void conectarWifi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(OTA_HOSTNAME);
  WiFi.setSleep(false);                 // mas estable durante el OTA

  // Redes de src/secrets.h: se conecta a la de mejor senal
  for (size_t i = 0; i < sizeof(REDES_WIFI) / sizeof(REDES_WIFI[0]); i++) {
    wifiMulti.addAP(REDES_WIFI[i].ssid, REDES_WIFI[i].pass);
  }

  Serial.println("[WIFI] Buscando redes conocidas...");

  uint32_t limite = millis() + 20000;   // 20 s de espera
  while (wifiMulti.run() != WL_CONNECTED && millis() < limite) {
    actualizarLed();
    delay(10);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI] Conectado a \"%s\". IP: %s  RSSI: %d dBm\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(),
                  WiFi.RSSI());
    destello(0, 255, 0, 2, 100);
  } else {
    Serial.println("[WIFI] Sin conexion, se reintenta en segundo plano");
    destello(255, 0, 0, 3, 100);
  }
}

static void configurarOta() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPort(3232);
  // Sin password. Para exigirla: ArduinoOTA.setPassword("tuclave");
  // y agregar --auth=tuclave a upload_flags en platformio.ini

  ArduinoOTA.onStart([]() {
    otaEnCurso  = true;
    otaConError = false;
    otaProgreso = 0;
    const char *tipo = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
    Serial.printf("\n[OTA] Inicio de actualizacion (%s)\n", tipo);
  });

  ArduinoOTA.onProgress([](unsigned int recibido, unsigned int total) {
    otaProgreso = (total > 0) ? (uint8_t)(recibido * 100 / total) : 0;
    static uint8_t ultimo = 255;
    if (otaProgreso != ultimo) {
      ultimo = otaProgreso;
      Serial.printf("[OTA] %u%%\r", otaProgreso);
    }
    actualizarLed();
  });

  ArduinoOTA.onEnd([]() {
    otaEnCurso = false;
    Serial.println("\n[OTA] Completado, reiniciando...");
    destello(0, 255, 0, 3, 80);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaEnCurso    = false;
    otaConError   = true;
    otaErrorDesde = millis();
    Serial.printf("\n[OTA] Error %u: ", error);
    switch (error) {
      case OTA_AUTH_ERROR:    Serial.println("autenticacion"); break;
      case OTA_BEGIN_ERROR:   Serial.println("inicio");        break;
      case OTA_CONNECT_ERROR: Serial.println("conexion");      break;
      case OTA_RECEIVE_ERROR: Serial.println("recepcion");     break;
      case OTA_END_ERROR:     Serial.println("cierre");        break;
      default:                Serial.println("desconocido");   break;
    }
  });

  ArduinoOTA.begin();
  Serial.printf("[OTA] Listo en %s (%s.local) puerto 3232\n",
                WiFi.localIP().toString().c_str(), OTA_HOSTNAME);
}

// Reconexion no bloqueante por si se cae el AP
static void vigilarWifi() {
  if (millis() - ultimoChequeoWifi < 5000) return;
  ultimoChequeoWifi = millis();

  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("[WIFI] Desconectado, reintentando...");
    return;
  }

  // Recordatorio de la IP cada 60 s: con DHCP puede cambiar, y asi
  // aparece aunque abras el monitor mucho despues del arranque
  static uint32_t ultimoAviso = 0;
  if (millis() - ultimoAviso >= 60000) {
    ultimoAviso = millis();
    Serial.printf("[WIFI] v%s | %s | IP: %s | %s.local | RSSI: %d dBm\n",
                  FW_VERSION, WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str(),
                  OTA_HOSTNAME, WiFi.RSSI());
  }
}

// ====================== SETUP / LOOP =========================

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("\n\n=== Esp32-TicTac | %s | firmware v%s | build %s %s ===\n",
                OTA_HOSTNAME, FW_VERSION, __DATE__, __TIME__);

  pixels.begin();
  pixels.setBrightness(NIVELES_BRILLO[indiceBrillo]);
  pixels.clear();
  pixels.show();
  destello(0, 0, 255, 1, 150);          // azul = arrancando

  configurarBotones();
  conectarWifi();
  configurarOta();
}

void loop() {
  ArduinoOTA.handle();
  leerBotones();
  actualizarLed();
  vigilarWifi();
}
