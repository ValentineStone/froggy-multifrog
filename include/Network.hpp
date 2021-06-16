#pragma once
#include <Arduino.h>
#include <LinkedList.h>
#include "Message.hpp"
#include "utils.hpp"

#define SEND_BACKOFF_MIN 200
#define SEND_BACKOFF_MAX 1000
#define SEND_WAIT_SINCE_ACTIVE_MIN 200
#define SEND_WAIT_SINCE_ACTIVE_MAX 500

struct ForSend {
  uint8_t* buff;
  int8_t len;
  ForSend(uint8_t* _buff, int8_t _len) {
    len = _len;
    buff = new uint8_t[len];
    memcpy(buff, _buff, len);
  }
  ~ForSend() { delete[] buff; }
};

struct NetworkBase {
  Stream& stream;
  uint8_t* device_id;

  Message inbound;
  Message outbound;
  Parser parser{inbound};

  unsigned long medium_last_active;
  uint16_t backoff = 0;
  uint16_t wait_since_active =
      random(SEND_WAIT_SINCE_ACTIVE_MIN, SEND_WAIT_SINCE_ACTIVE_MAX);
  LinkedList<ForSend*> for_send_queue;

  NetworkBase(Stream& _stream, uint8_t* _device_id)
      : stream(_stream), device_id(_device_id) {}

  void loop() {
    auto now = millis();
    if (stream.available())
      medium_last_active = now;
    while (stream.available()) {
      auto result = parser.parse(stream.read());
      if (result) {
        if (result == PARSED_MESSAGE) {
          handleAnyMessage();
          if (uuid_equals(parser.message.to_id, device_id)) {
            handleMessage();
          } else if (uuid_null(parser.message.to_id)) {
            handleBroadcast();
          }
        } else {
          handleBadMessage();
        }
      }
    }
    if (parser.attempt_timeout())
      handleTimeout();
    if (for_send_queue.size()) {
      if (now - medium_last_active >= wait_since_active) {
        if (stream.available()) return;
        ForSend* for_send = for_send_queue.shift();
        stream.write(for_send->buff, for_send->len);
        handleSent(for_send->buff, for_send->len);
        delete for_send;
      }
    }
  }

  void prepare(uint8_t* _to_id,
               uint8_t _message_id,
               uint8_t _payload_length,
               uint8_t* _payload) {
    outbound.message_id = _message_id;
    for (uint8_t i = 0; i < 16; i++) {
      outbound.from_id[i] = device_id[i];
      outbound.to_id[i] = _to_id ? _to_id[i] : 0;
    }
    outbound.payload_length = _payload_length;
    for (uint8_t i = 0; i < _payload_length; i++)
      outbound.payload[i] = _payload[i];
  }

  void send(uint8_t* _to_id,
            uint8_t _message_id,
            uint8_t _payload_length,
            uint8_t* _payload) {
    prepare(_to_id, _message_id, _payload_length, _payload);
    sendRaw(outbound.pack(), outbound.packed_length());
  }

  void respond(uint8_t _message_id, uint8_t _payload_len, uint8_t* _payload) {
    send(inbound.from_id, _message_id, _payload_len, _payload);
  }

  void broadcast(uint8_t _message_id, uint8_t _payload_len, uint8_t* _payload) {
    send(nullptr, _message_id, _payload_len, _payload);
  }

  void sendRaw(uint8_t* _for_send, int8_t _for_send_length) {
    for_send_queue.add(new ForSend(_for_send, _for_send_length));
  }

  virtual void handleAnyMessage() = 0;
  virtual void handleMessage() = 0;
  virtual void handleBadMessage() = 0;
  virtual void handleTimeout() = 0;
  virtual void handleSent(uint8_t*, uint8_t) = 0;
  virtual void handleBroadcast() = 0;
};