#pragma once
#include <Arduino.h>

#include "utils.hpp"

struct Sensor {
  Sensor(Frog* _frog): frog(_frog) {}
  Frog* frog;
  char uuid[37];
  uint8_t port;
  uint64_t interval;
  float reading;
  uint64_t reading_timestamp = 0;
  unsigned long last_read = 0;
  unsigned long last_requested = 0;
};

struct Frog {
  char uuid[37];
  uint8_t id;
  LinkedList<Sensor*> sensors;
  Multifrog* multifrog;
  Frog(Multifrog* _multifrog): multifrog(_multifrog) {}
  Sensor* add_sensor(char* uuid, uint8_t port, uint64_t interval) {
    Sensor* sensor = new Sensor(this);
    memcpy(sensor->uuid, uuid, 36);
    uuid[36] = 0;
    sensor->port = port;
    sensor->interval = interval;
    sensors.add(sensor);
    multifrog->sensors.add(sensor);
    return sensor;
  }
  Sensor* get_sensor(uint8_t port) {
    auto sensor_count = sensors.size();
    for (int i = 0; i < sensor_count; i++) {
      Sensor* sensor = sensors.get(i);
      if (sensor->port == port)
        return sensor;
    }
    return nullptr;
  }
};

struct Multifrog {
  char uuid[37];
  uint8_t id;
  LinkedList<Frog*> frogs;
  LinkedList<Sensor*> sensors;
  Multifrog(const char* _uuid, uint8_t _id) {
    memcpy(uuid, _uuid, 36);
    uuid[36] = 0;
    id = _id;
  }
  Frog* add_frog(char* uuid, uint8_t id) {
    Frog* frog = new Frog(this);
    memcpy(frog->uuid, uuid, 36);
    uuid[36] = 0;
    frog->id = id;
    frogs.add(frog);
    return frog;
  }
  Frog* get_frog(uint8_t frog_id) {
    auto frog_count = frogs.size();
    for (int i = 0; i < frog_count; i++) {
      Frog* frog = frogs.get(i);
      if (frog->id == frog_id)
        return frog;
    }
    return nullptr;
  }
  Sensor* get_sensor(uint8_t port) {
    auto frog_count = frogs.size();
    for (int i = 0; i < frog_count; i++) {
      Frog* frog = frogs.get(i);
      auto sensor_count = frog->sensors.size();
      for (int i = 0; i < sensor_count; i++) {
        Sensor* sensor = frog->sensors.get(i);
        if (sensor->port == port)
          return sensor;
      }
    }
    return nullptr;
  }
};

struct __attribute__((packed)) Reading {
  uint8_t port;
  float value;
};