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
