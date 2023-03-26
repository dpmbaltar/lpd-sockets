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

#define printf_bytes(format,data,length) \
  for (int i = 0; i < length; i++) \
    printf(format, (unsigned char)(data)[i]); \
  printf("\n");

#define printx_bytes(data,length) printf_bytes("%02x ",data,length)
