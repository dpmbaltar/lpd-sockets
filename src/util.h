#pragma once

#include <stdint.h>

/* Inicializador para Date */
#define DATE_INIT { (uint16_t)0, (uint8_t)0, (uint8_t)0 }
/* Inicializador para Request */
#define REQUEST_INIT { (char*)0, (char*)0, (int)0, (int)0 }

/* Tipo de datos para recepción de fechas del cliente */
typedef struct
{
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
} Date;

/* Tipo de dato para manejar solicitud/respuesta entre cliente y servidor  */
typedef struct
{
  void *send;
  void *recv;
  int   send_len;
  int   recv_len;
} Request;
