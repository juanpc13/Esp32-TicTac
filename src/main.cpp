/*
 * Esp32-TicTac
 * -------------------------------------------------------------
 * Actualizacion por aire (ArduinoOTA), botones por tabla de
 * configuracion y un menu en una OLED 96x16 por I2C.
 *
 *                   ESP32-S2 Saola-1      ESP32 clasico (TicTac)
 *   NeoPixel        GPIO3                 GPIO14
 *   Boton           GPIO4 a GND           GPIO4 a VCC
 *                   pull-up, activo LOW   pull-down, activo HIGH
 *   OLED SDA        GPIO19                GPIO21
 *   OLED SCL        GPIO20                GPIO22
 *   Hostname mDNS   tictac-s2.local       tictac-esp32.local
 *
 * La seleccion es automatica segun el env de platformio.ini: el
 * core define CONFIG_IDF_TARGET_ESP32S2 al compilar para el S2.
 *
 * OJO con el S2: GPIO19/20 son tambien D-/D+ del USB nativo. Por eso
 * se compila con ARDUINO_USB_CDC_ON_BOOT=0 (ver platformio.ini), se
 * sube por OTA y el monitor serie sale por el puerto UART de la
 * placa, no por el conector USB del chip.
 *
 * Gestos del boton (tabla CONFIG_BOTONES, abajo):
 *   1 toque   -> abre el menu / baja al siguiente item
 *   2 toques  -> activa el item marcado (en el inicio, cambia modo)
 *   larga     -> sale del menu / apaga-enciende el NeoPixel
 *   3+ toques -> vuelca el estado completo por serie
 *
 * Si la pantalla no responde en el I2C el firmware sigue igual:
 * solo pierde el menu, el LED y el OTA no se enteran.
 *
 * El panel es un 96x16, no un 128x32: se midio con el patron de
 * prueba (ver DIAGNOSTICO_OLED). Declararlo mal dejaba medio dibujo
 * fuera del vidrio. Son dos lineas de 16 caracteres.
 *
 * Las credenciales WiFi van en src/secrets.h, que NO se sube al
 * repo. Si acabas de clonar: copia src/secrets.example.h.
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
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <OneButton.h>

#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #error "Falta src/secrets.h -- copia src/secrets.example.h y pon tus redes WiFi"
#endif

// ====================== CONFIGURACION ========================

#define FW_VERSION "0.6.0"

// Patron de prueba al arrancar: numera cada franja de 8 px bajo
// cuatro configuraciones distintas, para medir que parte del bufer
// llega de verdad al vidrio. Dejar en 0 salvo que haya que repetirlo.
#define DIAGNOSTICO_OLED   0
#define MS_DIAGNOSTICO 168000

// ---- Diferencias de hardware entre las dos placas ----
#if CONFIG_IDF_TARGET_ESP32S2
  #define PIN_NEO          3
  #define PIN_BOTON        4
  #define BOTON_ACTIVO_BAJO  true    // pin a GND
  #define BOTON_PULLUP       true    // pull-up interno del ESP
  #define PIN_SDA          19        // = USB D-, libre porque el CDC esta apagado
  #define PIN_SCL          20        // = USB D+
  #define OTA_HOSTNAME     "tictac-s2"
#else   // ESP32 clasico: la placa del TicTac
  #define PIN_NEO          14
  #define PIN_BOTON        4
  #define BOTON_ACTIVO_BAJO  false   // pin a VCC
  #define BOTON_PULLUP       false   // pull-down en la placa
  #define PIN_SDA          21        // I2C por defecto del ESP32
  #define PIN_SCL          22
  #define OTA_HOSTNAME     "tictac-esp32"
#endif

#define NUM_PIXELS   1

// ---- Pantalla ----
// El panel de esta placa es un 96x16 (los de 0.69"), NO un 128x32.
// Se midio con el patron de prueba: declarandolo de 128x32 solo se
// veian 16 filas, y ademas las de abajo del bufer, porque la libreria
// manda COMSCANDEC y el barrido arranca por las lineas COM mas altas.
// En horizontal se cortaba a partir de la columna 96. Con el tamano
// real declarado se usa el vidrio entero y no hace falta recortar.
#define OLED_ANCHO        96
#define OLED_ALTO         16
#define OLED_RESET        -1     // sin pin de reset propio
#define OLED_DIR_A      0x3C     // la direccion habitual de estos modulos
#define OLED_DIR_B      0x3D     // algunos traen el puente movido

// Origen del dibujo. Con la geometria correcta se usa todo el bufer;
// quedan como constantes por si aparece otro panel con recorte.
#define OLED_Y0            0
#define OLED_ALTO_UTIL  OLED_ALTO

#define COLUMNAS_TEXTO (OLED_ANCHO / 6)    // 6 px por caracter en tamano 1
#define FILAS_MENU  (OLED_ALTO_UTIL / 8)   // lineas de menu que caben
#define MS_REFRESCO_OLED  80     // ~12 fps: suficiente y no satura el I2C
#define MS_REFRESCO_OTA  250     // durante la subida, menos trafico I2C
#define MS_CERRAR_MENU 15000     // el menu vuelve solo al inicio
#define MS_DORMIR_OLED 180000    // apaga el panel: los OLED se queman

// ====================== ESTADO GLOBAL ========================

WiFiMulti wifiMulti;
Adafruit_NeoPixel pixels(NUM_PIXELS, PIN_NEO, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306 oled(OLED_ANCHO, OLED_ALTO, &Wire, OLED_RESET);
WebServer servidorWeb(80);

enum Modo : uint8_t {
  MODO_ESTADO = 0,   // color segun estado de la conexion
  MODO_ROJO,
  MODO_VERDE,
  MODO_AZUL,
  MODO_ARCOIRIS,
  MODO_APAGADO,
  MODO_TOTAL
};

// Hasta 10 caracteres: es lo que cabe en la letra grande del inicio
static const char *NOMBRE_MODO[MODO_TOTAL] = {
  "Estado", "Rojo", "Verde", "Azul", "Arcoiris", "Apagado"
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

// ====================== PANTALLA Y MENU ======================
// Mismo criterio que los botones: el menu es una tabla de datos.
// Cada fila dice como se llama el item, que texto muestra a la
// derecha y que hace al activarlo con doble toque.

static bool hayOled     = false;      // false = la placa va sin pantalla
static bool oledDormida = false;
static uint8_t dirOled  = 0;          // 0x3C o 0x3D, la que haya contestado
static uint32_t ultimoRefrescoOled = 0;
static uint32_t ultimoToque        = 0;

enum Vista : uint8_t { VISTA_INICIO, VISTA_MENU };
static Vista vista     = VISTA_INICIO;
static uint8_t menuSel = 0;   // item marcado
static uint8_t menuTop = 0;   // primer item visible (ventana de scroll)

// ---- Lo que muestra cada item a la derecha ----

static void valModo(char *b, size_t n) {
  snprintf(b, n, "%s", NOMBRE_MODO[modo]);
}
static void valBrillo(char *b, size_t n) {
  snprintf(b, n, "%u%%", (unsigned)(NIVELES_BRILLO[indiceBrillo] * 100 / 255));
}
static void valRed(char *b, size_t n) {
  snprintf(b, n, "%s", WiFi.status() == WL_CONNECTED ? WiFi.SSID().c_str()
                                                     : "sin red");
}
static void valSenal(char *b, size_t n) {
  if (WiFi.status() == WL_CONNECTED) snprintf(b, n, "%ddBm", WiFi.RSSI());
  else                               snprintf(b, n, "--");
}
static void valIp(char *b, size_t n) {
  snprintf(b, n, "%s", WiFi.status() == WL_CONNECTED
                       ? WiFi.localIP().toString().c_str() : "---");
}
static void valOta(char *b, size_t n) {
  snprintf(b, n, "%s", OTA_HOSTNAME);
}
static void valVersion(char *b, size_t n) {
  snprintf(b, n, "v%s", FW_VERSION);
}
static void valTiempo(char *b, size_t n) {
  uint32_t s = millis() / 1000;
  if (s < 3600) snprintf(b, n, "%um%us", (unsigned)(s / 60), (unsigned)(s % 60));
  else          snprintf(b, n, "%uh%um", (unsigned)(s / 3600),
                                         (unsigned)((s % 3600) / 60));
}

// ---- Lo que hace cada item con el doble toque ----

static void actSiguienteModo() {
  modo = (Modo)((modo + 1) % MODO_TOTAL);
}
static void actSiguienteBrillo() {
  indiceBrillo = (indiceBrillo + 1) % NUM_NIVELES;
}
static void actReconectar() {
  // vigilarWifi() vuelve a levantar la conexion en menos de 5 s
  WiFi.disconnect();
}
static void actReiniciar() {
  Serial.println("[MENU] Reinicio pedido desde el menu");
  delay(200);
  ESP.restart();
}

// ---- Tabla del menu: esto es lo que se edita ----

struct ItemMenu {
  const char *titulo;
  void (*valor)(char *buf, size_t n);   // texto a la derecha (NULL = ninguno)
  void (*activar)();                    // doble toque (NULL = solo informa)
};

static const ItemMenu MENU[] = {
  { "Modo",      valModo,    actSiguienteModo   },
  { "Brillo",    valBrillo,  actSiguienteBrillo },
  { "Red",       valRed,     actReconectar      },
  { "Senal",     valSenal,   nullptr            },
  { "IP",        valIp,      nullptr            },
  { "OTA",       valOta,     nullptr            },
  { "Firmware",  valVersion, nullptr            },
  { "Tiempo",    valTiempo,  nullptr            },
  { "Reiniciar", nullptr,    actReiniciar       },
};

#define NUM_ITEMS_MENU (sizeof(MENU) / sizeof(MENU[0]))

// ---- Dibujo ----

static void dormirOled(bool dormir) {
  if (!hayOled || dormir == oledDormida) return;
  oled.ssd1306_command(dormir ? SSD1306_DISPLAYOFF : SSD1306_DISPLAYON);
  oledDormida = dormir;
}

// La fuente de la libreria mide 6x8 px por caracter en tamano 1
static void textoDerecha(const char *t, int16_t y, int16_t margen) {
  int16_t x = OLED_ANCHO - margen - (int16_t)strlen(t) * 6;
  if (x < 0) x = 0;
  oled.setCursor(x, y);
  oled.print(t);
}

// Aviso de dos lineas, se dibuja de inmediato
static void mensajeOled(const char *titulo, const char *detalle) {
  if (!hayOled) return;
  dormirOled(false);
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, OLED_Y0);
  oled.print(titulo);
  oled.setCursor(0, OLED_Y0 + 8);
  oled.print(detalle);
  oled.display();
  ultimoRefrescoOled = millis();
}

// Dos lineas de 8 px: modo + senal arriba, IP abajo
static void dibujarInicio() {
  char buf[24];

  oled.setTextSize(1);
  oled.setCursor(0, OLED_Y0);
  oled.print(NOMBRE_MODO[modo]);
  valSenal(buf, sizeof(buf));
  textoDerecha(WiFi.status() == WL_CONNECTED ? buf : "sin red", OLED_Y0, 0);

  valIp(buf, sizeof(buf));
  oled.setCursor(0, OLED_Y0 + 8);
  oled.print(buf);
}

static void dibujarMenu() {
  // Ventana deslizante: el item marcado siempre queda a la vista
  if (menuSel < menuTop)               menuTop = menuSel;
  if (menuSel >= menuTop + FILAS_MENU) menuTop = menuSel - FILAS_MENU + 1;

  oled.setTextSize(1);
  for (uint8_t f = 0; f < FILAS_MENU; f++) {
    uint8_t i = menuTop + f;
    if (i >= NUM_ITEMS_MENU) break;

    int16_t y    = OLED_Y0 + f * 8;
    bool marcado = (i == menuSel);
    if (marcado) oled.fillRect(0, y, OLED_ANCHO - 4, 8, SSD1306_WHITE);
    oled.setTextColor(marcado ? SSD1306_BLACK : SSD1306_WHITE);

    oled.setCursor(2, y);
    oled.print(MENU[i].titulo);

    if (MENU[i].valor) {
      char buf[22];
      buf[0] = 0;
      MENU[i].valor(buf, sizeof(buf));
      // Recorto el valor para que no pise al titulo en la misma fila
      size_t libre = COLUMNAS_TEXTO - 1 - strlen(MENU[i].titulo);
      if (strlen(buf) > libre) buf[libre] = 0;
      textoDerecha(buf, y, 6);
    }
  }
  oled.setTextColor(SSD1306_WHITE);

  // Barra de desplazamiento: recuerda que el menu sigue hacia abajo
  if (NUM_ITEMS_MENU > FILAS_MENU) {
    int16_t alto = OLED_ALTO_UTIL * FILAS_MENU / NUM_ITEMS_MENU;
    if (alto < 3) alto = 3;
    int16_t y = OLED_Y0 + OLED_ALTO_UTIL * menuTop / NUM_ITEMS_MENU;
    oled.fillRect(OLED_ANCHO - 2, y, 2, alto, SSD1306_WHITE);
  }
}

static void dibujarOta() {
  char buf[16];

  oled.setTextSize(1);
  oled.setCursor(0, OLED_Y0);
  if (otaConError && !otaEnCurso) {
    oled.print("OTA con error");
    return;
  }

  snprintf(buf, sizeof(buf), "OTA %u%%", (unsigned)otaProgreso);
  oled.print(buf);

  // Barra de progreso en la segunda linea de la franja visible
  int16_t y = OLED_Y0 + 9;
  oled.drawRect(0, y, OLED_ANCHO, 7, SSD1306_WHITE);
  oled.fillRect(1, y + 1, (OLED_ANCHO - 2) * otaProgreso / 100, 5, SSD1306_WHITE);
}

static void actualizarPantalla(bool forzar = false) {
  if (!hayOled) return;

  uint32_t ahora = millis();
  uint16_t cada  = otaEnCurso ? MS_REFRESCO_OTA : MS_REFRESCO_OLED;
  if (!forzar && ahora - ultimoRefrescoOled < cada) return;
  ultimoRefrescoOled = ahora;

  bool ota = otaEnCurso || otaConError;

  if (ota) {
    dormirOled(false);          // una subida siempre despierta la pantalla
  } else {
    // Sin tocar el boton: primero se cierra el menu, luego se apaga
    if (vista == VISTA_MENU && ahora - ultimoToque > MS_CERRAR_MENU) {
      vista = VISTA_INICIO;
    }
    if (!oledDormida && ahora - ultimoToque > MS_DORMIR_OLED) {
      dormirOled(true);
    }
  }

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  if (ota)                      dibujarOta();
  else if (vista == VISTA_MENU) dibujarMenu();
  else                          dibujarInicio();

  // Con el panel dormido se sigue componiendo el bufer pero no se
  // manda por I2C: asi el espejo web muestra siempre lo vigente y el
  // vidrio sigue apagado, que es de lo que se trata
  if (!oledDormida) oled.display();
}

static bool oledResponde(uint8_t dir) {
  Wire.beginTransmission(dir);
  return Wire.endTransmission() == 0;
}

static void iniciarPantalla() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  uint8_t dir = 0;
  if      (oledResponde(OLED_DIR_A)) dir = OLED_DIR_A;
  else if (oledResponde(OLED_DIR_B)) dir = OLED_DIR_B;

  if (!dir) {
    Serial.printf("[OLED] Nadie contesta en SDA=%d SCL=%d (probe 0x3C y 0x3D)\n",
                  PIN_SDA, PIN_SCL);
    Serial.println("[OLED] Sigo sin pantalla: revisa cableado y pull-ups");
    return;
  }

  // reset=false y periphBegin=false: el Wire ya quedo arrancado arriba
  // con los pines de esta placa, que no son los que elegiria la libreria
  if (!oled.begin(SSD1306_SWITCHCAPVCC, dir, false, false)) {
    Serial.println("[OLED] La SSD1306 contesta pero no arranco");
    return;
  }

  hayOled = true;
  dirOled = dir;
  oled.setTextWrap(false);
  Serial.printf("[OLED] %ux%u en 0x%02X (SDA %d, SCL %d)\n", OLED_ANCHO, OLED_ALTO, dir, PIN_SDA, PIN_SCL);
  mensajeOled("TicTac v" FW_VERSION, "iniciando...");
}

#if DIAGNOSTICO_OLED
// ---- Patron de prueba temporal ----
// Dibuja un marco y numera cada franja de 8 px. Alternando la
// configuracion de 32 y 64 filas se ve cual coincide con el panel:
// la buena muestra TODAS las franjas y el marco completo.

static Adafruit_SSD1306 oled64(OLED_ANCHO, 64, &Wire, OLED_RESET);

// Cada franja de 8 px se rotula con su letra de prueba y su numero
// ("A0", "A1"...). Asi, se vea la parte que se vea, queda claro que
// configuracion es y que franjas del bufer llegan al vidrio.
static void patronPrueba(Adafruit_SSD1306 &d, uint8_t filas, char letra) {
  d.clearDisplay();
  d.setTextWrap(false);
  d.setTextColor(SSD1306_WHITE);
  d.setTextSize(1);
  d.drawRect(0, 0, OLED_ANCHO, filas, SSD1306_WHITE);

  for (uint8_t i = 0; i < filas / 8; i++) {
    d.setCursor(4, i * 8);
    d.printf("%c%u", letra, i);
    // Repetida a la derecha por si el lado izquierdo queda fuera
    d.setCursor(OLED_ANCHO - 20, i * 8);
    d.printf("%c%u", letra, i);
  }
  d.display();
}

// Deja el panel configurado y manda los retoques que distinguen a
// cada candidata. offset: desplazamiento vertical. compins: mapeo de
// filas (0x02 secuencial, 0x12 alternado).
static void prepararPanel(Adafruit_SSD1306 &d, uint8_t offset, uint8_t compins) {
  d.begin(SSD1306_SWITCHCAPVCC, dirOled, false, false);
  if (offset) {
    d.ssd1306_command(SSD1306_SETDISPLAYOFFSET);
    d.ssd1306_command(offset);
  }
  if (compins) {
    d.ssd1306_command(SSD1306_SETCOMPINS);
    d.ssd1306_command(compins);
  }
}

// Espera sin bloquear el OTA, para poder reprogramar durante la prueba
static void esperarConOta(uint32_t ms) {
  uint32_t fin = millis() + ms;
  while ((int32_t)(millis() - fin) < 0) {
    ArduinoOTA.handle();
    actualizarLed();
    delay(5);
  }
}

static void diagnosticoPantalla() {
  if (!hayOled) return;
  Serial.println("[OLED] Patron de prueba: alternando 32 y 64 filas");

  uint32_t fin = millis() + MS_DIAGNOSTICO;
  while ((int32_t)(millis() - fin) < 0) {
    // A: 32 filas tal cual las configura la libreria (lo de ahora)
    prepararPanel(oled, 0, 0);
    patronPrueba(oled, 32, 'A');
    esperarConOta(7000);

    // B: 32 filas desplazadas 16 hacia abajo
    prepararPanel(oled, 16, 0);
    patronPrueba(oled, 32, 'B');
    esperarConOta(7000);

    // C: 32 filas con el mapeo de filas alternado
    prepararPanel(oled, 0, 0x12);
    patronPrueba(oled, 32, 'C');
    esperarConOta(7000);

    // D: panel tratado como 128x64 completo
    prepararPanel(oled64, 0, 0);
    patronPrueba(oled64, 64, 'D');
    esperarConOta(7000);
  }

  // Volver a dejar el panel como lo espera el resto del firmware
  prepararPanel(oled, 0, 0);
  oled.setTextWrap(false);
  Serial.println("[OLED] Fin del patron de prueba");
}
#endif

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
  AccionBoton alClic;            // 1 toque
  AccionBoton alDobleClic;       // 2 toques
  AccionBoton alMultiClic;       // 3 o mas, ver ob.getNumberClicks()
  AccionBoton alPulsacionLarga;  // salta al cruzar msPulsacionLarga
};

struct Boton {
  OneButton          ob;
  const ConfigBoton *cfg;
};

// ---- Acciones disponibles ----

// Todo gesto cuenta como actividad. Si la pantalla estaba dormida el
// gesto solo la despierta, para no cambiar nada a ciegas.
static bool despertarPantalla() {
  ultimoToque = millis();
  if (oledDormida) {
    dormirOled(false);
    actualizarPantalla(true);
    return true;
  }
  return false;
}

static void accNavegar(Boton &b) {
  if (despertarPantalla()) return;

  if (vista == VISTA_INICIO) {
    vista   = VISTA_MENU;                       // el toque abre el menu
    menuSel = 0;
    menuTop = 0;
  } else {
    menuSel = (menuSel + 1) % NUM_ITEMS_MENU;   // y dentro, baja de item
  }
  actualizarPantalla(true);
  Serial.printf("[%s] 1 toque -> %s\n", b.cfg->nombre,
                (vista == VISTA_MENU) ? MENU[menuSel].titulo : "inicio");
}

static void accActivar(Boton &b) {
  if (despertarPantalla()) return;

  if (vista == VISTA_INICIO) {
    // Sin menu abierto, el doble toque sigue siendo el cambio de modo
    actSiguienteModo();
    Serial.printf("[%s] 2 toques -> modo %s\n", b.cfg->nombre, NOMBRE_MODO[modo]);
  } else if (MENU[menuSel].activar) {
    MENU[menuSel].activar();
    Serial.printf("[%s] 2 toques -> activa \"%s\"\n",
                  b.cfg->nombre, MENU[menuSel].titulo);
  } else {
    Serial.printf("[%s] 2 toques -> \"%s\" es solo informativo\n",
                  b.cfg->nombre, MENU[menuSel].titulo);
  }
  actualizarPantalla(true);
}

static void accVolver(Boton &b) {
  if (despertarPantalla()) return;

  if (vista == VISTA_MENU) {
    vista = VISTA_INICIO;          // larga dentro del menu = salir
    Serial.printf("[%s] larga -> salgo del menu\n", b.cfg->nombre);
  } else {
    modo = (modo == MODO_APAGADO) ? MODO_ESTADO : MODO_APAGADO;
    Serial.printf("[%s] larga -> LED %s\n", b.cfg->nombre,
                  (modo == MODO_APAGADO) ? "apagado" : "encendido");
  }
  actualizarPantalla(true);
}

static void accImprimirEstado(Boton &b) {
  despertarPantalla();
  Serial.printf("[%s] %d toques -> v%s | %s | IP %s | %s.local | RSSI %d dBm\n",
                b.cfg->nombre, b.ob.getNumberClicks(), FW_VERSION,
                WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(),
                OTA_HOSTNAME, WiFi.RSSI());
}

// ---- Tabla de configuracion: esto es lo que se edita ----

static const ConfigBoton CONFIG_BOTONES[] = {
  { PIN_BOTON, BOTON_ACTIVO_BAJO, BOTON_PULLUP, "principal",
    50,    // antirrebote
    400,   // ventana entre clics
    800,   // umbral de pulsacion larga
    accNavegar,
    accActivar,
    accImprimirEstado,
    accVolver },
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
  mensajeOled("WiFi", "buscando red...");

  uint32_t limite = millis() + 20000;   // 20 s de espera
  while (wifiMulti.run() != WL_CONNECTED && millis() < limite) {
    actualizarLed();
    delay(10);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI] Conectado a \"%s\". IP: %s  RSSI: %d dBm\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(),
                  WiFi.RSSI());
    mensajeOled("WiFi ok", WiFi.localIP().toString().c_str());
    destello(0, 255, 0, 2, 100);
  } else {
    Serial.println("[WIFI] Sin conexion, se reintenta en segundo plano");
    mensajeOled("Sin red", "reintentando...");
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
    actualizarPantalla(true);
  });

  ArduinoOTA.onProgress([](unsigned int recibido, unsigned int total) {
    otaProgreso = (total > 0) ? (uint8_t)(recibido * 100 / total) : 0;
    static uint8_t ultimo = 255;
    if (otaProgreso != ultimo) {
      ultimo = otaProgreso;
      Serial.printf("[OTA] %u%%\r", otaProgreso);
    }
    actualizarLed();
    actualizarPantalla();        // se autolimita a uno cada 250 ms
  });

  ArduinoOTA.onEnd([]() {
    otaEnCurso  = false;
    otaProgreso = 100;
    Serial.println("\n[OTA] Completado, reiniciando...");
    mensajeOled("Listo", "reiniciando...");
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
    actualizarPantalla(true);
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

// ====================== ESPEJO WEB ===========================
// Sirve el MISMO bufer que se manda a la OLED, sin redibujar nada
// aparte: lo que se ve aqui es literalmente lo que tiene el vidrio.
//
//   /               la pantalla ampliada, se refresca sola
//   /pantalla.bmp   el bufer como imagen de 1 bit
//   /pantalla.txt   el bufer como texto, comodo desde la terminal
//   /estado.json    modo, brillo, red, version, tiempo encendido
//
// El bufer de la SSD1306 va por paginas de 8 filas: el pixel (x,y)
// es el bit (y & 7) del byte x + (y / 8) * ancho.

#define BMP_BYTES_FILA (((OLED_ANCHO + 31) / 32) * 4)   // filas alineadas a 4
#define BMP_OFFSET     (14 + 40 + 8)                    // cabeceras + paleta
#define BMP_TAM        (BMP_OFFSET + BMP_BYTES_FILA * OLED_ALTO)

static inline bool pixelEncendido(const uint8_t *buf, int x, int y) {
  return buf[x + (y / 8) * OLED_ANCHO] & (1 << (y & 7));
}

static void escribir16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}
static void escribir32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;         p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static void servirBmp() {
  if (!hayOled) { servidorWeb.send(503, "text/plain", "sin pantalla\n"); return; }

  static uint8_t bmp[BMP_TAM];
  memset(bmp, 0, sizeof(bmp));

  bmp[0] = 'B'; bmp[1] = 'M';
  escribir32(bmp + 2,  BMP_TAM);
  escribir32(bmp + 10, BMP_OFFSET);
  escribir32(bmp + 14, 40);                      // tamano de la cabecera DIB
  escribir32(bmp + 18, OLED_ANCHO);
  escribir32(bmp + 22, OLED_ALTO);               // positivo = filas de abajo arriba
  escribir16(bmp + 26, 1);                       // planos
  escribir16(bmp + 28, 1);                       // bits por pixel
  escribir32(bmp + 34, BMP_BYTES_FILA * OLED_ALTO);
  escribir32(bmp + 46, 2);                       // colores de la paleta
  escribir32(bmp + 50, 2);
  // Paleta BGRA: el bit 0 es el fondo, el 1 el pixel encendido
  bmp[54] = 0x00; bmp[55] = 0x0A; bmp[56] = 0x14; bmp[57] = 0x00;
  bmp[58] = 0xE0; bmp[59] = 0xF4; bmp[60] = 0xFF; bmp[61] = 0x00;

  const uint8_t *buf = oled.getBuffer();
  for (int y = 0; y < OLED_ALTO; y++) {
    uint8_t *fila = bmp + BMP_OFFSET + (uint32_t)(OLED_ALTO - 1 - y) * BMP_BYTES_FILA;
    for (int x = 0; x < OLED_ANCHO; x++) {
      if (pixelEncendido(buf, x, y)) fila[x >> 3] |= 0x80 >> (x & 7);
    }
  }

  servidorWeb.sendHeader("Cache-Control", "no-store");
  servidorWeb.send_P(200, "image/bmp", (const char *)bmp, sizeof(bmp));
}

static void servirTxt() {
  if (!hayOled) { servidorWeb.send(503, "text/plain", "sin pantalla\n"); return; }

  const uint8_t *buf = oled.getBuffer();
  String s;
  s.reserve((OLED_ANCHO + 1) * OLED_ALTO + 1);
  for (int y = 0; y < OLED_ALTO; y++) {
    for (int x = 0; x < OLED_ANCHO; x++) s += pixelEncendido(buf, x, y) ? '#' : '.';
    s += '\n';
  }
  servidorWeb.sendHeader("Cache-Control", "no-store");
  servidorWeb.send(200, "text/plain; charset=utf-8", s);
}

static void servirEstado() {
  char tiempo[16];
  valTiempo(tiempo, sizeof(tiempo));

  char json[512];
  snprintf(json, sizeof(json),
    "{\"version\":\"%s\",\"hostname\":\"%s\",\"modo\":\"%s\",\"brillo\":%u,"
    "\"red\":\"%s\",\"ip\":\"%s\",\"rssi\":%d,\"encendido\":\"%s\","
    "\"vista\":\"%s\",\"item\":\"%s\",\"pantalla\":{\"ancho\":%u,\"alto\":%u,"
    "\"presente\":%s,\"dormida\":%s}}",
    FW_VERSION, OTA_HOSTNAME, NOMBRE_MODO[modo],
    (unsigned)(NIVELES_BRILLO[indiceBrillo] * 100 / 255),
    WiFi.status() == WL_CONNECTED ? WiFi.SSID().c_str() : "",
    WiFi.localIP().toString().c_str(), WiFi.RSSI(), tiempo,
    vista == VISTA_MENU ? "menu" : "inicio",
    vista == VISTA_MENU ? MENU[menuSel].titulo : "",
    OLED_ANCHO, OLED_ALTO,
    hayOled ? "true" : "false", oledDormida ? "true" : "false");

  servidorWeb.sendHeader("Cache-Control", "no-store");
  servidorWeb.send(200, "application/json", json);
}

// Lo que de verdad se puede averiguar del panel. Poco: el SSD1306 no
// tiene registro de identificacion, asi que no hay modelo ni
// resolucion que preguntar (la resolucion es del vidrio, no del
// chip). Lo unico que devuelve al leer es un byte de estado cuyo
// bit 6 indica panel apagado. Lo demas es lo que sondeamos nosotros.
static void servirI2c() {
  String dispositivos;
  uint8_t encontrados = 0;
  for (uint8_t dir = 1; dir < 127; dir++) {
    Wire.beginTransmission(dir);
    if (Wire.endTransmission() == 0) {
      if (encontrados++) dispositivos += ",";
      dispositivos += "\"0x";
      if (dir < 16) dispositivos += "0";
      dispositivos += String(dir, HEX);
      dispositivos += "\"";
    }
  }

  int estado = -1;
  if (dirOled && Wire.requestFrom((int)dirOled, 1) == 1) estado = Wire.read();

  char json[512];
  snprintf(json, sizeof(json),
    "{\"sda\":%d,\"scl\":%d,\"frecuencia\":400000,"
    "\"dispositivos\":[%s],\"oled\":{\"direccion\":\"0x%02X\","
    "\"byte_estado\":%d,\"panel_encendido\":%s,"
    "\"ancho_configurado\":%u,\"alto_configurado\":%u},"
    "\"nota\":\"el SSD1306 no expone modelo ni resolucion: "
    "la geometria se midio dibujando, ver /test\"}",
    PIN_SDA, PIN_SCL, dispositivos.c_str(), dirOled, estado,
    (estado >= 0 && !(estado & 0x40)) ? "true" : "false",
    OLED_ANCHO, OLED_ALTO);

  servidorWeb.sendHeader("Cache-Control", "no-store");
  servidorWeb.send(200, "application/json", json);
}

// Patron de franjas numeradas bajo demanda: es la unica forma de
// comprobar la geometria, y asi no hace falta reprogramar para
// repetir la medicion si algun dia se cambia de panel.
static void servirTest() {
  if (!hayOled) { servidorWeb.send(503, "text/plain", "sin pantalla\n"); return; }

  dormirOled(false);
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.drawRect(0, 0, OLED_ANCHO, OLED_ALTO, SSD1306_WHITE);
  for (uint8_t i = 0; i < OLED_ALTO / 8; i++) {
    oled.setCursor(3, i * 8);
    oled.print(i);
  }
  oled.display();

  ultimoToque = millis();          // el dibujo normal vuelve al siguiente refresco
  servidorWeb.send(200, "text/plain",
    "Patron dibujado. Cada franja de 8 px lleva su numero y hay un\n"
    "marco alrededor del area completa. Si se ven todas las franjas\n"
    "y el marco cerrado, la geometria configurada es la correcta.\n"
    "Mira /pantalla.bmp para verlo, o el vidrio directamente.\n");
}

static const char PAGINA[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>TicTac</title><style>
:root{color-scheme:dark}
body{margin:0;min-height:100vh;display:flex;flex-direction:column;align-items:center;
justify-content:center;gap:18px;background:#0d1117;color:#c9d1d9;
font:14px/1.5 ui-monospace,SFMono-Regular,Consolas,monospace}
#marco{padding:14px;background:#000;border:1px solid #30363d;border-radius:10px}
img{display:block;image-rendering:pixelated;width:min(90vw,576px);height:auto}
#datos{display:grid;grid-template-columns:auto auto;gap:2px 14px;font-size:13px}
b{font-weight:400;color:#8b949e}
small{color:#8b949e}
</style></head><body>
<div id="marco"><img id="p" alt="pantalla"></div>
<div id="datos"></div>
<small id="pie"></small>
<script>
const campos=[["modo","Modo"],["brillo","Brillo %"],["red","Red"],["ip","IP"],
["rssi","RSSI dBm"],["encendido","Encendido"],["vista","Vista"],["item","Item"]];
async function tick(){
  document.getElementById('p').src='/pantalla.bmp?'+Date.now();
  try{
    const e=await (await fetch('/estado.json',{cache:'no-store'})).json();
    document.getElementById('datos').innerHTML=campos
      .filter(([k])=>e[k]!==''&&e[k]!==undefined)
      .map(([k,t])=>`<b>${t}</b><span>${e[k]}</span>`).join('');
    document.getElementById('pie').textContent=
      `${e.hostname} v${e.version} - ${e.pantalla.ancho}x${e.pantalla.alto}`+
      (e.pantalla.dormida?' - panel apagado, esto es el bufer':'');
  }catch(err){document.getElementById('pie').textContent='sin respuesta de la placa';}
}
tick();setInterval(tick,700);
</script></body></html>)HTML";

static void configurarWeb() {
  servidorWeb.on("/", []() {
    servidorWeb.send_P(200, "text/html; charset=utf-8", PAGINA);
  });
  servidorWeb.on("/pantalla.bmp", servirBmp);
  servidorWeb.on("/pantalla.txt", servirTxt);
  servidorWeb.on("/estado.json", servirEstado);
  servidorWeb.on("/i2c.json", servirI2c);
  servidorWeb.on("/test", servirTest);
  servidorWeb.onNotFound([]() {
    servidorWeb.send(404, "text/plain",
                     "rutas: /  /pantalla.bmp  /pantalla.txt  /estado.json"
                     "  /i2c.json  /test\n");
  });

  servidorWeb.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.printf("[WEB] http://%s/ (y %s.local)\n",
                WiFi.localIP().toString().c_str(), OTA_HOSTNAME);
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

  iniciarPantalla();
  configurarBotones();
  conectarWifi();
  configurarOta();
  configurarWeb();

#if DIAGNOSTICO_OLED
  diagnosticoPantalla();
#endif

  ultimoToque = millis();               // arranca el reloj de inactividad
}

void loop() {
  ArduinoOTA.handle();
  servidorWeb.handleClient();
  leerBotones();
  actualizarLed();
  actualizarPantalla();
  vigilarWifi();
}
