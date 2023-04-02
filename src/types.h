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

/** Inicializador para Date */
#define DATE_INIT         { 0, 0, 0 }
/** Inicializador para AstroInfo */
#define ASTRO_INFO_INIT   { 0, 0, {{0}}, {0} }
/** Inicializador para AstroQuery */
#define ASTRO_QUERY_INIT  { DATE_INIT, 0 }
/** Inicializador para WeatherInfo */
#define WEATHER_INFO_INIT { {0}, 0, 0.0F }
/** Inicializador para Request */
#define REQUEST_INIT      { 0L, 0L, 0, 0 }

/** Obtiene tamaño fijo de AstroInfo sin mood */
#define ASTRO_INFO_FSIZE(a) (sizeof(AstroInfo)-sizeof(((AstroInfo*)0)->mood))
/** Obtiene tamaño dinámico de AstroInfo según mood_len para mood */
#define ASTRO_INFO_DSIZE(a) (ASTRO_INFO_FSIZE(a)+((AstroInfo*)(a))->mood_len)

/** Estructura para fechas */
typedef struct
{
  uint16_t year;  /**< El año */
  uint8_t  month; /**< El mes */
  uint8_t  day;   /**< El día del mes */
} Date;

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

/** Solicitud datos del horóscopo */
typedef struct
{
  Date      date;
  AstroSign sign;
} AstroQuery;

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
  char  date[11];
  char  cond;
  float temp;
} WeatherInfo;

/** Estructura para solicitud/respuesta entre cliente y servidor */
typedef struct
{
  void *send;
  void *recv;
  int   send_len;
  int   recv_len;
} Request;
