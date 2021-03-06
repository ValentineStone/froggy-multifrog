#include <Arduino.h>
#include <ESPRandom.h>
#include <LinkedList.h>

#include "Devices.hpp"
#include "Firebase.hpp"
#include "Message.hpp"
#include "Network.hpp"
#include "keys.h"
#include "utils.hpp"

bool     REQUESTING_NOW         = false;
uint64_t LAST_REQUESTED         = 0;
uint16_t LAST_REQUESTED_TIMEOUT = 5000;
uint64_t LAST_TIMESYNC          = 0;
uint16_t LAST_TIMESYNC_TIMEOUT  = 10000;
uint16_t CURRENT_i              = 0;
uint16_t UPDATE_RETRY_MAX       = 10;

#define HC12 Serial2

Multifrog MULTIFROG(DEVICE_UUID, DEVICE_ID);

struct Network: public NetworkBase {
  using NetworkBase::NetworkBase;
  void handleMessage() {
    Serial.println("Message to me");
    switch (inbound.message_id) {
      case MESSAGE_ID_VALUE: {
        Serial.println("Got readings");
        Frog* frog = MULTIFROG.get_frog(inbound.from_id);
        if (!frog) {
          Serial.println("Unknown frog");
          return;
        }
        Reading* reading = (Reading*) inbound.payload;
        Sensor*  sensor  = frog->get_sensor(reading->port);
        if (!sensor) {
          Serial.print("Unknown sensor ");
          Serial.println(reading->port);
          return;
        }
        Serial.print("On port ");
        Serial.print(reading->port);
        Serial.print(" value=");
        Serial.println(reading->value);
        sensor->reading_timestamp = date_now();
        sensor->last_read         = millis();
        sensor->reading           = reading->value;
        REQUESTING_NOW            = false;

        digitalWrite(BUILTIN_LED, LOW);
        for (uint8_t retry = 0; retry <= UPDATE_RETRY_MAX; retry++) {
          if (retry == UPDATE_RETRY_MAX) ESP.restart();
          if (set_reading(sensor)) {
            Serial.println("Updated value");
            digitalWrite(BUILTIN_LED, HIGH);
            break;
          } else
            Serial.println("Could not update value");
        }
        return;
      }
    }
  }
  void handleBadMessage() {
    // Serial.println("Got BAD message");
  }
  void handleTimeout() {
    // Serial.println("Discarded broken message");
  }
  void handleAnyMessage() {
    /// Serial.println("Got message");
  }
  void handleSent(uint8_t* msg, uint8_t len) {
    /*
    Serial.print("Sent message: [ ");
    for (int i = 0; i < len; i++) {
      if (msg[i] < 16) Serial.print("0");
      Serial.print((int) msg[i], HEX);
      Serial.print(" ");
    }
    Serial.println("]");
    */
  }
  void handleBroadcast() {
    // Serial.println("Got Broadcast message");
  }
};

Network network(HC12, MULTIFROG.id);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  HC12.begin(9600);

  Serial.print("Device UUID: ");
  Serial.println(MULTIFROG.uuid);
  Serial.print("Device ID: ");
  Serial.println(MULTIFROG.id);

  Serial.println("Connecting to firebase...");
  if (!firebase_setup()) {
    Serial.println("Could not connect, restarting...");
    ESP.restart();
  }
  Serial.println("Loading hardware configuration...");
  if (!load_hardware_configuration(MULTIFROG)) {
    Serial.println("Could not load config, restarting...");
    ESP.restart();
  }
  if (!sync_time()) {
    Serial.println("Could not sync time, restarting...");
    ESP.restart();
  }
  digitalWrite(BUILTIN_LED, HIGH);
}

void loop() {
  network.loop();

  if (millis() - LAST_TIMESYNC > LAST_TIMESYNC_TIMEOUT) {
    if (!sync_time(false)) {
      Serial.println("Could not sync time, restarting...");
      ESP.restart();
    } else {
      LAST_TIMESYNC = millis();
    }
  }

  if (REQUESTING_NOW) {
    if (millis() - LAST_REQUESTED > LAST_REQUESTED_TIMEOUT)
      REQUESTING_NOW = false;
    else
      return;
  }

  auto count = MULTIFROG.sensors.size();
  for (int i = 0; i < count; CURRENT_i = (++i + CURRENT_i) % count) {
    Sensor* sensor = MULTIFROG.sensors.get(CURRENT_i);
    auto    now    = millis();
    bool    unread = sensor->last_requested == 0;
    bool mustRead  = unread || now - sensor->last_requested > sensor->interval;
    if (mustRead) {
      REQUESTING_NOW         = true;
      LAST_REQUESTED         = now;
      sensor->last_requested = now;
      network.send(
        sensor->frog->id, MESSAGE_ID_REQUEST_VALUE, 1, &sensor->port);
      return;
    }
  }
}