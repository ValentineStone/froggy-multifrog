#pragma once
#include <Arduino.h>
#include "utils.hpp"
#define MAX_PAYLOAD_LENGTH 64

#define MESSAGE_ID_REQUEST_VALUE 0
#define MESSAGE_ID_VALUE 1

struct __attribute__((packed)) Message {
  uint8_t magic = '!';
  uint8_t message_id;
  uint8_t from_id;
  uint8_t to_id;
  uint8_t payload_length;
  uint8_t payload[MAX_PAYLOAD_LENGTH];
  uint8_t checksum;

  void make(uint8_t _message_id,
            uint8_t _from_id,
            uint8_t _to_id,
            uint8_t _payload_length,
            uint8_t* _payload,
            uint8_t _checksum) {
    message_id = _message_id;
    from_id = _from_id;
    to_id = _to_id;
    payload_length = _payload_length;
    for (uint8_t i = 0; i < payload_length; i++)
      payload[i] = _payload[i];
  }

  uint8_t calculate_checksum() {
    uint8_t crc = _crc_ibutton_update(0, message_id);
    crc = _crc_ibutton_update(0, from_id);
    crc = _crc_ibutton_update(0, to_id);
    crc = _crc_ibutton_update(crc, payload_length);
    for (uint8_t i = 0; i < payload_length; i++)
      crc = _crc_ibutton_update(crc, payload[i]);
    return crc;
  }

  uint8_t packed_length() { return 1 + 1 + 1 + 1 + 1 + payload_length + 1; }
  uint8_t* pack() {
    checksum = calculate_checksum();
    if (payload_length < MAX_PAYLOAD_LENGTH)
      payload[payload_length] = checksum;
    return (uint8_t*)this;
  }
};

#define PARSED_MESSAGE 1
#define PARSED_BAD_MESSAGE 2
#define PARSED_NOTHING 0

#define WAITING_FOR_MAGIC 0
#define WAITING_FOR_MESSAGE_ID 1
#define WAITING_FOR_FROM_ID 2
#define WAITING_FOR_TO_ID 3
#define WAITING_FOR_PAYLOAD_LENGTH 4
#define WAITING_FOR_PAYLOAD 5
#define WAITING_FOR_CHECKSUM 6

struct Parser {
 private:
  Message& message;
  uint8_t read_counter = 0;

 public:
  uint8_t state = WAITING_FOR_MAGIC;

  Parser(Message& _inbound_message) : message(_inbound_message) {}

  uint8_t parse(uint8_t byte) {
    switch (state) {
      case WAITING_FOR_MAGIC:
        if (byte == '!')
          state = WAITING_FOR_MESSAGE_ID;
        break;
      case WAITING_FOR_MESSAGE_ID:
        message.message_id = byte;
        state = WAITING_FOR_FROM_ID;
        break;
      case WAITING_FOR_FROM_ID:
        message.from_id = byte;
        state = WAITING_FOR_TO_ID;
        break;
      case WAITING_FOR_TO_ID:
        message.to_id = byte;
        state = WAITING_FOR_PAYLOAD_LENGTH;
        break;
      case WAITING_FOR_PAYLOAD_LENGTH:
        message.payload_length = byte;
        if (message.payload_length > MAX_PAYLOAD_LENGTH) {
          state = WAITING_FOR_MAGIC;
          return PARSED_BAD_MESSAGE;
        }
        read_counter = 0;
        if (message.payload_length)
          state = WAITING_FOR_PAYLOAD;
        else
          state = WAITING_FOR_CHECKSUM;
        break;
      case WAITING_FOR_PAYLOAD:
        message.payload[read_counter++] = byte;
        if (read_counter == message.payload_length)
          state = WAITING_FOR_CHECKSUM;
        break;
      case WAITING_FOR_CHECKSUM:
        message.checksum = byte;
        state = WAITING_FOR_MAGIC;
        if (message.checksum == message.calculate_checksum())
          return PARSED_MESSAGE;
        else
          return PARSED_BAD_MESSAGE;
    }
    return PARSED_NOTHING;
  }
};
