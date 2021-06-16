#pragma once
#include <Arduino.h>
#include <EEPROM.h>

uint8_t _crc_ibutton_update(uint8_t crc, uint8_t data) {
  crc = crc ^ data;
  for (uint8_t i = 0; i < 8; i++)
    if (crc & 0x01)
      crc = (crc >> 1) ^ 0x8C;
    else
      crc >>= 1;
  return crc;
}

uint8_t __hex_buffer[3] = {0, 0, 0};
char* hex(uint8_t a) {
  uint8_t lo = a & 0b00001111;
  uint8_t hi = a >> 4;
  __hex_buffer[0] = hi < 10 ? hi + 48 : hi - 10 + 97;
  __hex_buffer[1] = lo < 10 ? lo + 48 : lo - 10 + 97;
  return (char*)__hex_buffer;
}

void print_uuid(Stream& stream, uint8_t* uuid) {
  for (uint8_t i = 0; i < 16; i++) {
    if (i == 4 || i == 6 || i == 8 || i == 10)
      stream.print('-');
    stream.print(hex(uuid[i]));
  }
}

String uuid_stringify(uint8_t* uuid) {
  String str;
  for (uint8_t i = 0; i < 16; i++) {
    if (i == 4 || i == 6 || i == 8 || i == 10)
      str += "-";
    str += hex(uuid[i]);
  }
  return str;
}

void uuid_parse(String& uuid, uint8_t* parsed) {
  for (uint8_t i = 0, j = 0; i < uuid.length(); i++, j++) {
    if (uuid[i] == '-') {
      j--;
      continue;
    }
    uint8_t dec = uuid[i] - (uuid[i] > 57 ? ('a' - 10) : '0');
    if (j % 2 == 0) {
      parsed[j / 2] = dec << 4;
    } else {
      parsed[j / 2] += dec;
    }
  }
}

bool uuid_equals(uint8_t* a, uint8_t* b) {
  for (uint8_t i = 0; i < 16; i++)
    if (a[i] != b[i])
      return false;
  return true;
};

bool uuid_null(uint8_t* a) {
  for (uint8_t i = 0; i < 16; i++)
    if (a[i])
      return false;
  return true;
};

#define EEPROM_UUID_OFFSET 0

bool read_device_uuid(uint8_t* id) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < 16; i++) {
    id[i] = EEPROM.read(EEPROM_UUID_OFFSET + i);
    crc = _crc_ibutton_update(crc, id[i]);
  }
  uint8_t crc_stored = EEPROM.read(EEPROM_UUID_OFFSET + 16);
  if (crc_stored == crc)
    return true;
  else {
    for (uint8_t i = 0; i < 16; i++)
      id[i] = 0;
    return false;
  }
}

void write_device_uuid(uint8_t* id) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < 16; i++) {
    EEPROM.write(EEPROM_UUID_OFFSET + i, id[i]);
    crc = _crc_ibutton_update(crc, id[i]);
  }
  EEPROM.write(EEPROM_UUID_OFFSET + 16, crc);
  EEPROM.commit();
}