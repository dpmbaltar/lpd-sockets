#include <glib.h>
#include <string.h>

#include "weather.h"

#define MIN_TEMP    -25.0F
#define MAX_TEMP    50.0F
#define DATE_FORMAT "%Y-%m-%d"

static const char *weather_conditions[N_CONDITIONS] = {
  [WC_CLEAR]   = "Despejado",
  [WC_CLOUD]   = "Nublado",
  [WC_MIST]    = "Neblina",
  [WC_RAIN]    = "Lluvia",
  [WC_SHOWERS] = "Chubascos",
  [WC_SNOW]    = "Nieve",
};

int weather_get_info(WeatherInfo *weather_info, int day)
{
  g_return_val_if_fail(weather_info != NULL, -1);
  g_return_val_if_fail(day >= WEATHER_MIN_DAYS && day <= WEATHER_MAX_DAYS, -1);

  GDateTime *dt_now = g_date_time_new_now_local();
  GDateTime *dt = g_date_time_add_days(dt_now, day);
  GRand *rand = g_rand_new();

  char *date = g_date_time_format(dt, DATE_FORMAT);
  char cond = g_rand_int_range(rand, 0, N_CONDITIONS);
  float temp = g_rand_double_range(rand, MIN_TEMP, MAX_TEMP);

  memcpy(&weather_info->date, date, sizeof(((WeatherInfo*)0)->date));
  memcpy(&weather_info->cond, &cond, sizeof(cond));
  memcpy(&weather_info->temp, &temp, sizeof(temp));

  g_date_time_unref(dt_now);
  g_date_time_unref(dt);
  g_rand_free(rand);
  g_free(date);

  return 0;
}

const char *weather_cond_str(WeatherCond cond)
{
  g_return_val_if_fail(cond >= 0 && cond < N_CONDITIONS, NULL);
  return weather_conditions[cond];
}
