#pragma once
#include <Arduino.h>
#include "utils.hpp"
#define MAX_PAYLOAD_LENGTH 64

#define MESSAGE_ID_SET_UUID 0
#define MESSAGE_ID_REQUEST_CONFIG 1
#define MESSAGE_ID_CONFIG 2
#define MESSAGE_ID_REQUEST_READINGS 3
#define MESSAGE_ID_READINGS 4

struct __attribute__((packed)) Message {
  uint8_t __A = 'A';
  uint8_t __T = 'T';
  uint8_t message_id;
  uint8_t from_id[16];
  uint8_t to_id[16];
  uint8_t payload_length;
  uint8_t payload[MAX_PAYLOAD_LENGTH];
  uint8_t checksum;

  void make(uint8_t _message_id,
            uint8_t* _from_id,
            uint8_t* _to_id,
            uint8_t _payload_length,
            uint8_t* _payload,
            uint8_t _checksum) {
    message_id = _message_id;
    for (uint8_t i = 0; i < 16; i++) {
      from_id[i] = _from_id[i];
      to_id[i] = _to_id[i];
    }
    payload_length = _payload_length;
    for (uint8_t i = 0; i < payload_length; i++)
      payload[i] = _payload[i];
  }

  uint8_t calculate_checksum() {
    uint8_t crc = _crc_ibutton_update(0, message_id);
    for (uint8_t i = 0; i < 16; i++)
      crc = _crc_ibutton_update(crc, from_id[i]);
    for (uint8_t i = 0; i < 16; i++)
      crc = _crc_ibutton_update(crc, to_id[i]);
    crc = _crc_ibutton_update(crc, payload_length);
    for (uint8_t i = 0; i < payload_length; i++)
      crc = _crc_ibutton_update(crc, payload[i]);
    return crc;
  }

  uint8_t packed_length() { return 2 + 1 + 16 + 16 + 1 + payload_length + 1; }
  uint8_t* pack() {
    checksum = calculate_checksum();
    if (payload_length < MAX_PAYLOAD_LENGTH)
      payload[payload_length] = checksum;
    return (uint8_t*)this;
  }
};

#define PARSED_MESSAGE 1
#define PARSED_BAD_MESSAGE -1
#define PARSED_NOTHING 0

#define WAITING_FOR_A 0
#define WAITING_FOR_T 1
#define WAITING_FOR_MESSAGE_ID 2
#define WAITING_FOR_FROM_ID 3
#define WAITING_FOR_TO_ID 4
#define WAITING_FOR_PAYLOAD_LENGTH 5
#define WAITING_FOR_PAYLOAD 6
#define WAITING_FOR_CHECKSUM 7

struct Parser {
  uint8_t state = WAITING_FOR_A;
  uint8_t read_counter;
  unsigned long last_updated;
  Message& message;

  Parser(Message& _inbound_message) : message(_inbound_message) {}

  int8_t parse(uint8_t byte) {
    last_updated = millis();
    read_counter += 1;
    switch (state) {
      case WAITING_FOR_A:
        if (byte == 'A')
          state = WAITING_FOR_T;
        break;
      case WAITING_FOR_T:
        if (byte == 'T')
          state = WAITING_FOR_MESSAGE_ID;
        else
          state = WAITING_FOR_A;
        break;
      case WAITING_FOR_MESSAGE_ID:
        message.message_id = byte;
        state = WAITING_FOR_FROM_ID;
        read_counter = 0;
        break;

      case WAITING_FOR_FROM_ID:
        message.from_id[read_counter - 1] = byte;
        if (read_counter == 16) {
          state = WAITING_FOR_TO_ID;
          read_counter = 0;
        }
        break;
      case WAITING_FOR_TO_ID:
        message.to_id[read_counter - 1] = byte;
        if (read_counter == 16) {
          state = WAITING_FOR_PAYLOAD_LENGTH;
          read_counter = 0;
        }
        break;
      case WAITING_FOR_PAYLOAD_LENGTH:
        message.payload_length = byte;
        if (message.payload_length > MAX_PAYLOAD_LENGTH) {
          state = WAITING_FOR_A;
          return PARSED_BAD_MESSAGE;
        }
        read_counter = 0;
        if (message.payload_length)
          state = WAITING_FOR_PAYLOAD;
        else
          state = WAITING_FOR_CHECKSUM;
      case WAITING_FOR_PAYLOAD:
        message.payload[read_counter - 1] = byte;
        if (read_counter == message.payload_length)
          state = WAITING_FOR_CHECKSUM;
        break;
      case WAITING_FOR_CHECKSUM:
        message.checksum = byte;
        state = WAITING_FOR_A;
        if (message.checksum == message.calculate_checksum())
          return PARSED_MESSAGE;
        else
          return PARSED_BAD_MESSAGE;
    }
    return PARSED_NOTHING;
  }

  uint8_t attempt_timeout() {
    if (state != WAITING_FOR_A && millis() - last_updated > 1000) {
      state = WAITING_FOR_A;
      return 1;
    }
    return 0;
  }
};
