#pragma once

#define printf_bytes(format,data,length) \
  for (int i = 0; i < length; i++) \
    printf(format, (unsigned char)(data)[i]); \
  printf("\n");

#define printx_bytes(data,length) printf_bytes("%02x ",data,length)
