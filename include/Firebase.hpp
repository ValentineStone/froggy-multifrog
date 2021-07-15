#pragma once
#include <Arduino.h>
#include <ArduinoHttpClient.h>

#include "Ethernet.hpp"
#include "GSM.hpp"
#include "WiFi.hpp"

#include "Devices.hpp"
#include "keys.h"

#define WAIT_FOR_ETHERNET 1000
#define WAIT_FOR_GSM      15000
#define WAIT_FOR_WIFI     15000

HttpClient* http_client = nullptr;

bool firebase_setup() {
  if (ethernet_setup(WAIT_FOR_ETHERNET))
    http_client = &eth_http_client;
  else if (gsm_setup(WAIT_FOR_GSM))
    http_client = &gsm_http_client;
  else if (wifi_setup(WAIT_FOR_WIFI))
    http_client = &wifi_http_client;
  return http_client != nullptr;
}

bool set_reading(Sensor* sensor) {
  char time[30];
  sprintf(time, "%lld", sensor->reading_timestamp);
  String path = "/readings/";
  path += sensor->uuid;
  path += "/";
  path += time;
  String path_full = path + ".json?auth=" + String(DATABASE_SECRET);
  String value     = String(sensor->reading);
  http_client->put(path_full, "application/json", value);
  bool success = http_client->responseBody().equals(value);
  return success;
}

uint64_t get_timestamp() {
  http_client->put(
    "/timestamp.json?auth=" + String(DATABASE_SECRET),
    "application/json",
    "{\".sv\":\"timestamp\"}");
  String   body      = http_client->responseBody();
  uint64_t timestamp = strtoull(body.c_str(), nullptr, 10);
  return timestamp;
}

String get_config() {
  String path = "/users/";
  path += USER_ID;
  path += "/hardware/";
  path += DEVICE_UUID;
  path += ".json?auth=" + String(DATABASE_SECRET);
  http_client->get(path);
  String json = http_client->responseBody();
  if (json.length() == 0 || json[0] != '"')
    return "";
  else
    return json.substring(1, -1);
}

bool sync_time() {
  Serial.println("Tymesync...");
  timeval tv_now;
  gettimeofday(&tv_now, NULL);
  Serial.print("Local: sec=");
  Serial.print(tv_now.tv_sec);
  Serial.print(" usec=");
  Serial.println(tv_now.tv_usec);
  uint64_t datenow = get_timestamp();
  if (datenow == 0) return false;
  uint64_t sec  = datenow / 1000L;
  uint64_t usec = datenow % 1000L * 1000L;
  Serial.print("Server: sec=");
  Serial.print(sec);
  Serial.print(" usec=");
  Serial.println(usec);
  tv_now.tv_sec  = sec;
  tv_now.tv_usec = usec;
  settimeofday(&tv_now, nullptr);
  timeval tv_new;
  gettimeofday(&tv_new, NULL);
  Serial.print("Adjusted: sec=");
  Serial.print(tv_new.tv_sec);
  Serial.print(" usec=");
  Serial.print(tv_new.tv_usec);
  Serial.print(" date_now=");
  Serial.println(date_now());
  return true;
}

#define HARDWARE_PARSE_FROG     0
#define HARDWARE_PARSE_FROG_ID  1
#define HARDWARE_PARSE_SENSOR   2
#define HARDWARE_PARSE_PORT     3
#define HARDWARE_PARSE_INTERVAL 4
#define HARDWARE_PARSE_FINISH   5

bool load_hardware_configuration(Multifrog& multifrog) {
  String config = get_config();
  if (!config.isEmpty()) {
    Serial.println("Loaded:");
    char*   data  = (char*) config.c_str();
    char*   end   = data + config.length();
    uint8_t state = HARDWARE_PARSE_FROG;

    Frog*    frog;
    uint8_t  id;
    char*    uuid;
    uint8_t  port;
    uint64_t interval;

    while (data < end && state != HARDWARE_PARSE_FINISH) {
      switch (state) {
        case HARDWARE_PARSE_FROG:
          if (end - data < 36) {
            state = HARDWARE_PARSE_FINISH;
            break;
          }
          uuid = data;
          Serial.print("Frog ");
          Serial.write(uuid, 36);
          data += 36;
          if (data < end && *data++ == ':')
            state = HARDWARE_PARSE_FROG_ID;
          else
            state = HARDWARE_PARSE_FINISH;
          break;
        case HARDWARE_PARSE_FROG_ID:
          id = strtoul(data, &data, 10);
          Serial.print(" id=");
          Serial.println(id);
          frog = multifrog.add_frog(uuid, id);
          if (*data++ == ':')
            state = HARDWARE_PARSE_SENSOR;
          else
            state = HARDWARE_PARSE_FINISH;
          break;
        case HARDWARE_PARSE_SENSOR:
          if (end - data < 36) {
            state = HARDWARE_PARSE_FINISH;
            break;
          }
          uuid = data;
          Serial.print("  Sensor ");
          Serial.write(data, 36);
          data += 36;
          if (data < end && *data++ == ':')
            state = HARDWARE_PARSE_PORT;
          else
            state = HARDWARE_PARSE_FINISH;
          break;
        case HARDWARE_PARSE_PORT:
          port = strtoul(data, &data, 10);
          Serial.print(" port=");
          Serial.print(port);
          if (*data++ == ':')
            state = HARDWARE_PARSE_INTERVAL;
          else
            state = HARDWARE_PARSE_FINISH;
          break;
        case HARDWARE_PARSE_INTERVAL:
          interval = strtoul(data, &data, 10);
          Serial.print(" int=");
          Serial.println(interval);
          frog->add_sensor(uuid, port, interval);
          if (*data == ':')
            state = HARDWARE_PARSE_SENSOR;
          else if (*data == ';')
            state = HARDWARE_PARSE_FROG;
          else
            state = HARDWARE_PARSE_FINISH;
          data++;
          break;
        case HARDWARE_PARSE_FINISH: break;
      }
    }
    return true;
  } else
    return false;
}