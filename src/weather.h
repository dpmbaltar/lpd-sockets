#pragma once

/* Inicializador para WeatherInfo */
#define WEATHER_INFO_INIT { {(char)0}, (char)0, (float)(0.0F) }

/* Condición del clima */
typedef enum _WeatherCond {
  WC_CLEAR,
  WC_CLOUD,
  WC_MIST,
  WC_RAIN,
  WC_SHOWERS,
  WC_SNOW,
  N_CONDITIONS
} WeatherCond;

/* Información del clima */
typedef struct _WeatherInfo {
  char  date[11];
  char  cond;
  float temp;
} WeatherInfo;
