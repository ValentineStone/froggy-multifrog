#pragma once
#include <Arduino.h>
#include <LinkedList.h>
#include "Message.hpp"
#include "utils.hpp"

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
  uint8_t device_id;

  Message inbound;
  Message outbound;
  Parser parser{inbound};

  int last_avaliable = 0;
  LinkedList<ForSend*> send_queue;

  NetworkBase(Stream& _stream, uint8_t _device_id)
      : stream(_stream), device_id(_device_id) {}

  void loop() {
    bool new_available = stream.available() != last_avaliable;
    if (new_available) {
      last_avaliable = stream.available();
      if (!send_queue.size())
        parse();
    } else {
      if (send_queue.size())
        send();
      parse();
    }
  }

  void send() {
    while (send_queue.size()) {
      ForSend* for_send = send_queue.shift();
      stream.write(for_send->buff, for_send->len);
      handleSent(for_send->buff, for_send->len);
      delete for_send;
    }
  }

  void parse() {
    while (stream.available()) {
      auto result = parser.parse(stream.read());
      last_avaliable = stream.available();
      if (result) {
        if (result == PARSED_MESSAGE) {
          if (inbound.to_id == device_id) {
            handleMessage();
          } else if (inbound.to_id == 255) {
            handleBroadcast();
          }
        } else {
          handleBadMessage();
        }
      }
    }
  }

  void prepare(uint8_t _to_id,
               uint8_t _message_id,
               uint8_t _payload_length,
               uint8_t* _payload) {
    outbound.message_id = _message_id;
    outbound.to_id = _to_id;
    outbound.from_id = device_id;
    outbound.payload_length = _payload_length;
    for (uint8_t i = 0; i < _payload_length; i++)
      outbound.payload[i] = _payload[i];
  }

  void send(uint8_t _to_id,
            uint8_t _message_id,
            uint8_t _payload_length,
            uint8_t* _payload) {
    prepare(_to_id, _message_id, _payload_length, _payload);
    send_queue.add(new ForSend(outbound.pack(), outbound.packed_length()));
  }

  void respond(uint8_t _message_id, uint8_t _payload_len, uint8_t* _payload) {
    send(inbound.from_id, _message_id, _payload_len, _payload);
  }

  void broadcast(uint8_t _message_id, uint8_t _payload_len, uint8_t* _payload) {
    send(255, _message_id, _payload_len, _payload);
  }

  virtual void handleMessage() = 0;
  virtual void handleBroadcast() = 0;
  virtual void handleBadMessage() = 0;
  virtual void handleSent(uint8_t*, uint8_t) = 0;
};