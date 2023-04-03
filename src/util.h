/**
 * @file util.h
 * @author Diego Pablo Matias Baltar (diego.baltar@est.fi.uncoma.edu.ar)
 * @brief Funciones de utilidad
 * @version 0.1
 * @date 2023-04-02
 */
#pragma once

#include <glib.h>
#include <json-glib/json-glib.h>

/** Itera sobre los datos para mostrar cada byte en el formato dado. */
#define printf_bytes(format,data,length) \
  for (int i = 0; i < length; i++) \
    printf(format, (unsigned char)(data)[i]); \
  printf("\n");

/** Itera sobre los datos para mostrar cada byte en 2 dígitos hexadecimales. */
#define printx_bytes(data,length) \
  printf_bytes("%02x ",data,length)

/**
 * Transforma una fecha en formato ISO a una instancia de GDate.
 *
 * @param data una fecha en formato ISO (YYYY-MM-DD)
 * @return una instancia de GDate, o NULL si la fecha no es válida. Una vez
 * utilizado el resultado, debe liberarse con g_date_free().
 */
GDate *parse_date(const char *data);

/**
 * Transforma una cadena en formato JSON a una instancia de JsonNode.
 *
 * @param data los datos en formato JSON
 * @param length la longitud de los datos
 * @param parser una instancia de JsonParser
 * @return una instancia de JsonNode, o NULL si el formato es incorrecto. Una
 * vez utilizado el resultado, debe liberarse con json_node_free().
 */
JsonNode *parse_json(const char *data, int length, JsonParser *parser);
