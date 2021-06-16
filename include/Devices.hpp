#pragma once
#include <Arduino.h>

#include "utils.hpp"

struct Sensor {
  uint8_t id[16];
  uint8_t port;
  uint64_t interval;
  float reading;
  uint64_t reading_timestamp = 0;
  unsigned long last_read = 0;
  unsigned long last_requested = 0;
  bool fresh = false;
};

struct Frog {
  uint8_t id[16];
  LinkedList<Sensor*> sensors;
  Sensor* add_sensor(uint8_t* sensor_id, uint8_t port, uint64_t interval) {
    Sensor* sensor = new Sensor();
    memcpy(sensor->id, sensor_id, 16);
    sensor->port = port;
    sensor->interval = interval;
    sensors.add(sensor);
    return sensor;
  }
  Sensor* get_sensor(uint8_t* sensor_id) {
    auto sensor_count = sensors.size();
    for (int i = 0; i < sensor_count; i++) {
      Sensor* sensor = sensors.get(i);
      if (uuid_equals(sensor->id, sensor_id))
        return sensor;
    }
    return nullptr;
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
  uint8_t id[16];
  LinkedList<Frog*> frogs;
  Frog* add_frog(uint8_t* frog_id) {
    Frog* frog = new Frog();
    memcpy(frog->id, frog_id, 16);
    frogs.add(frog);
    return frog;
  }
  Frog* get_frog(uint8_t* frog_id) {
    auto frog_count = frogs.size();
    for (int i = 0; i < frog_count; i++) {
      Frog* frog = frogs.get(i);
      if (uuid_equals(frog->id, frog_id))
        return frog;
    }
    return nullptr;
  }
  Sensor* get_sensor(uint8_t* sensor_id) {
    auto frog_count = frogs.size();
    for (int i = 0; i < frog_count; i++) {
      Frog* frog = frogs.get(i);
      auto sensor_count = frog->sensors.size();
      for (int i = 0; i < sensor_count; i++) {
        Sensor* sensor = frog->sensors.get(i);
        if (uuid_equals(sensor->id, sensor_id))
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