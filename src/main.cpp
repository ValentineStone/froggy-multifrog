#include <Arduino.h>
#include <ESPRandom.h>
#include <LinkedList.h>

#include "Database.hpp"
#include "Devices.hpp"
#include "Message.hpp"
#include "Network.hpp"
#include "keys.h"
#include "utils.hpp"

bool REQUESTING_NOW = false;
uint64_t LAST_REQUESTED = 0;
uint16_t LAST_REQUESTED_TIMEOUT = 1000;

#define HC12 Serial2
#define GSM Serial1

Multifrog MULTIFROG(DEVICE_UUID, DEVICE_ID);

struct Network : public NetworkBase {
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
        Reading reading = *(Reading*)inbound.payload;
        Sensor* sensor = frog->get_sensor(reading.port);
        if (!sensor) {
          Serial.print("Unknown sensor ");
          Serial.println(reading.port);
          return;
        }
        Serial.print("On port ");
        Serial.print(reading.port);
        Serial.print(" value=");
        Serial.println(reading.value);
        sensor->reading_timestamp = date_now();
        sensor->last_read = millis();
        sensor->reading = reading.value;
        sensor->fresh = true;
        REQUESTING_NOW = false;
        return;
      }
    }
  }
  void handleBadMessage() { Serial.println("Got BAD message"); }
  void handleTimeout() { Serial.println("Discarded broken message"); }
  void handleAnyMessage() { Serial.println("Got message"); }
  void handleSent(uint8_t* msg, uint8_t len) { Serial.println("Sent message"); }
  void handleBroadcast() { Serial.println("Got Broadcast message"); }
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

  database_setup();
  load_hardware_configuration(MULTIFROG);
  sync_time();
  digitalWrite(BUILTIN_LED, HIGH);
}

void loop() {
  network.loop();
  // loop over frogs
  auto frog_count = MULTIFROG.frogs.size();
  for (int i = 0; i < frog_count; i++) {
    Frog* frog = MULTIFROG.frogs.get(i);
    auto sensor_count = frog->sensors.size();
    for (int i = 0; i < sensor_count; i++) {
      Sensor* sensor = frog->sensors.get(i);
      if (sensor->fresh) {
        sensor->fresh = false;
        char time[30];
        sprintf(time, "%lld", sensor->reading_timestamp);
        String path = "/readings/";
        path += sensor->uuid;
        path += "/";
        path += time;
        if (!Firebase.RTDB.setFloatAsync(&DB, path.c_str(), sensor->reading)) {
          Serial.print("Error setting ");
          Serial.println(path);
          Serial.println(DB.errorReason());
        }
      }
    }
  }
  if (REQUESTING_NOW) {
    if (millis() - LAST_REQUESTED > LAST_REQUESTED_TIMEOUT)
      REQUESTING_NOW = false;
    else
      return;
  }
  for (int i = 0; i < frog_count; i++) {
    Frog* frog = MULTIFROG.frogs.get(i);
    auto sensor_count = frog->sensors.size();
    for (int i = 0; i < sensor_count; i++) {
      Sensor* sensor = frog->sensors.get(i);
      auto now = millis();
      bool unread = sensor->last_requested == 0;
      bool mustRead = unread || now - sensor->last_requested > sensor->interval;
      if (mustRead) {
        REQUESTING_NOW = true;
        LAST_REQUESTED = now;
        sensor->last_requested = now;
        network.send(frog->id, MESSAGE_ID_REQUEST_VALUE, 1, &sensor->port);
        return;
      }
    }
  }
}