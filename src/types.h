#pragma once

#include <stdint.h>

/* Inicializador para Date */
#define DATE_INIT         {(uint16_t)0,(uint8_t)0,(uint8_t)0}
/* Inicializador para WeatherInfo */
#define WEATHER_INFO_INIT {{(char)0},(char)0,(float)(0.0F)}
/* Inicializador para Request */
#define REQUEST_INIT      {(char*)0,(char*)0,(int)0,(int)0}

/* Estructura para recibir fechas */
typedef struct _Date
{
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
} Date;

/* Signos */
typedef enum _AstroSign
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

/* Estructura para información del horóscopo */
typedef struct _AstroInfo
{
  char sign;
  char sign_compat;
  char date_from[10];
  char date_to[10];
  char color[10];
  char mood[20];
} AstroInfo;

/* Condiciones del clima */
typedef enum _WeatherCond {
  W_CLEAR,
  W_CLOUD,
  W_MIST,
  W_RAIN,
  W_SHOWERS,
  W_SNOW,
  N_CONDITIONS
} WeatherCond;

/* Estructura para información del clima */
typedef struct _WeatherInfo {
  char  date[11];
  char  cond;
  float temp;
} WeatherInfo;

/* Estructura para solicitud/respuesta entre cliente y servidor  */
typedef struct
{
  void *send;
  void *recv;
  int   send_len;
  int   recv_len;
} Request;
