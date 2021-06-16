#pragma once
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <SPI.h>
#include <WiFi.h>

#include "utils.hpp"
#include "Message.hpp"
#include "Devices.hpp"
#include "keys.h"

FirebaseAuth auth;
FirebaseConfig config;
FirebaseData DB;

void database_setup() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  /*
  config.service_account.data.client_email = CLIENT_EMAIL;
  config.service_account.data.client_id = CLIENT_ID;
  config.service_account.data.private_key = PRIVATE_KEY;
  config.service_account.data.private_key_id = PRIVATE_KEY_ID;
  config.service_account.data.project_id = PROJECT_ID;
  */
  auth.token.uid = DATABASE_UID;
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;

  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);
  Serial.print("Connecting to firebase");
  while (!Firebase.ready()) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
}

void sync_time() {
  Serial.println("Tymesync...");
  bool retry = true;
  while (retry) {
    timeval tv_now;
    gettimeofday(&tv_now, NULL);
    Serial.print("Local: sec=");
    Serial.print(tv_now.tv_sec);
    Serial.print(" usec=");
    Serial.println(tv_now.tv_usec);
    if (!Firebase.RTDB.setTimestamp(&DB, "/timestamp"))
      continue;
    retry = false;
    uint64_t datenow = DB.doubleData();
    uint64_t sec = datenow / 1000L;
    uint64_t usec = datenow % 1000L * 1000L;
    Serial.print("Server: sec=");
    Serial.print(sec);
    Serial.print(" usec=");
    Serial.println(usec);
    tv_now.tv_sec = sec;
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
  }
}

#define HARDWARE_PARSE_FROG 0
#define HARDWARE_PARSE_SENSOR 1
#define HARDWARE_PARSE_PORT 2
#define HARDWARE_PARSE_INTERVAL 3
#define HARDWARE_PARSE_FINISH 4

void load_hardware_configuration(Multifrog& multifrog) {
  Serial.println("Loading hardware configuration...");
  String path = "/users/";
  path += USER_ID;
  path += "/hardware/";
  path += DEVICE_UUID;
  if (Firebase.RTDB.getString(&DB, path.c_str())) {
    Serial.println("Loaded:");
    String str = DB.stringData();
    char* data = (char*)str.c_str();
    char* end = data + str.length();
    uint8_t state = HARDWARE_PARSE_FROG;
    uint8_t uuid[16];
    uint8_t port;
    unsigned long interval;
    Frog* frog;
    while (data < end && state != HARDWARE_PARSE_FINISH) {
      switch (state) {
        case HARDWARE_PARSE_FROG:
          if (end - data < 36) {
            state = HARDWARE_PARSE_FINISH;
            break;
          }
          uuid_parse(data, uuid);
          Serial.print("Frog ");
          Serial.write(data, 36);
          Serial.println();
          data += 36;
          frog = multifrog.add_frog(uuid);
          if (data < end && *data++ == ':')
            state = HARDWARE_PARSE_SENSOR;
          else
            state = HARDWARE_PARSE_FINISH;
          break;
        case HARDWARE_PARSE_SENSOR:
          if (end - data < 36) {
            state = HARDWARE_PARSE_FINISH;
            break;
          }
          uuid_parse(data, uuid);
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
        case HARDWARE_PARSE_FINISH:
          break;
      }
    }
  } else {
    Serial.println("Error!");
    Serial.println(DB.errorReason());
  }
}