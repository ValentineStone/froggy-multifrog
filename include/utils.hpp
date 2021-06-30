#pragma once
#include <Arduino.h>

uint8_t _crc_ibutton_update(uint8_t crc, uint8_t data) {
  crc = crc ^ data;
  for (uint8_t i = 0; i < 8; i++)
    if (crc & 0x01)
      crc = (crc >> 1) ^ 0x8C;
    else
      crc >>= 1;
  return crc;
}
int64_t date_now() {
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  return (int64_t)tv_now.tv_sec * 1000L + (int64_t)tv_now.tv_usec / 1000L;
}