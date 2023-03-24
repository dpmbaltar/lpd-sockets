#pragma once

/* Mímino de días a partir de hoy */
#define WEATHER_MIN_DAYS  0
/* Máximo de días a partir de hoy */
#define WEATHER_MAX_DAYS  7
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

int             weather_get_info        (WeatherInfo *weather_info, int day);
const char     *weather_cond_str        (WeatherCond  cond);
