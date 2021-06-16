#pragma once
#include <Arduino.h>

#define SENSORS_ONE_WIRE_BUS 3
#define SENSORS_COUNT 4

#define SENSOR_TYPE_TEMPERATURE 0
#define SENSOR_TYPE_VOLTAGE 1

struct __attribute__((packed)) Config {
  uint8_t count = SENSORS_COUNT;
  uint8_t types[SENSORS_COUNT];
};

#if __has_include("DallasTemperature.h")
#include <DallasTemperature.h>

OneWire __oneWire(SENSORS_ONE_WIRE_BUS);
DallasTemperature __dallasTemperature(&__oneWire);

struct __attribute__((packed)) Sensors {
  Config config;
  float readings[SENSORS_COUNT];

  Sensors() {
    config.types[0] = SENSOR_TYPE_TEMPERATURE;
    config.types[1] = SENSOR_TYPE_TEMPERATURE;
    config.types[2] = SENSOR_TYPE_TEMPERATURE;
    config.types[3] = SENSOR_TYPE_TEMPERATURE;
  }

  void read() {
    for (int i = 0; i < SENSORS_COUNT; i++)
      readings[i] = __dallasTemperature.getTempCByIndex(i);
  }
  void loop() {}
};
#endif