/**
 * @file types.h
 * @author Diego Pablo Matias Baltar (diego.baltar@est.fi.uncoma.edu.ar)
 * @brief Estructuras utilizadas para el manejo de datos
 * @version 0.1
 * @date 2023-03-29
 *
 * Contiene las estructuras para el manejo interno de la información del clima y
 * el horóscopo.
 */
#pragma once

#include <stdint.h>

/** Signos del horóscopo */
typedef enum
{
  S_ARIES,
  S_TAURUS,
  S_GEMINI,
  S_CANCER,
  S_LEO,
  S_VIRGO,
  S_LIBRA,
  S_SCORPIO,
  S_SAGITTARIUS,
  S_CAPRICORN,
  S_AQUARIUS,
  S_PISCES,
  N_SIGNS
} AstroSign;

/** Información del horóscopo */
typedef struct
{
  uint8_t sign;             /**< Signo (0-11) */
  uint8_t sign_compat;      /**< Signo compatible (0-11) */
  char    date_range[2][6]; /**< Rango de fechas, i.e. {"MM-DD", "MM-DD"} */
  char    mood[242];        /**< Estado */
} AstroInfo;

/** Condiciones del clima */
typedef enum {
  W_CLEAR,
  W_CLOUD,
  W_MIST,
  W_RAIN,
  W_SHOWERS,
  W_SNOW,
  N_CONDITIONS
} WeatherCond;

/** Información del clima */
typedef struct {
  char  date[11]; /**< Fecha en formato ISO, i.e. "YYYY-MM-DD" */
  char  cond;     /**< Condición del clima @see WeatherCond */
  float temp;     /**< Temperatura (*C) */
} WeatherInfo;
