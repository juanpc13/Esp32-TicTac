/*
 * secrets.example.h  -  PLANTILLA, este archivo SI se sube al repo
 * -------------------------------------------------------------
 * Si acabas de clonar el proyecto:
 *
 *   1. Copia este archivo como "secrets.h" en la misma carpeta:
 *        cp src/secrets.example.h src/secrets.h       (Linux / macOS)
 *        copy src\secrets.example.h src\secrets.h     (Windows)
 *
 *   2. Pon tus propias redes WiFi abajo.
 *
 * secrets.h esta en .gitignore, asi que tus claves no se suben.
 * Nunca escribas credenciales reales en ESTE archivo.
 * -------------------------------------------------------------
 */

#pragma once

struct RedWifi {
  const char *ssid;
  const char *pass;
};

// Tantas redes como quieras. Se conecta a la de mejor senal de
// las que encuentre disponibles (WiFiMulti).
static const RedWifi REDES_WIFI[] = {
  {"TU_RED_WIFI",      "TU_CLAVE"},
  {"OTRA_RED_OPCIONAL", "OTRA_CLAVE"},
};
