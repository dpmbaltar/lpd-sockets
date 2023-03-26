#pragma once

#include <stdint.h>

/* Inicializador para Date */
#define DATE_INIT { (uint16_t)0, (uint8_t)0, (uint8_t)0 }

/* Tipo de datos para recepción de fechas del cliente */
typedef struct
{
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
} Date;
