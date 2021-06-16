#include <Arduino.h>
#include <ESPRandom.h>
#include <LinkedList.h>

#include "Database.hpp"
#include "Message.hpp"
#include "Network.hpp"
#include "Sensors.hpp"
#include "utils.hpp"

#define HC12 Serial2
#define GSM Serial1

FirebaseData db;
String MULTIFROG_PATH;

#define FROG_UPDATE_TIMEOUT 1000

struct Frog {
  uint8_t frog_id[16];
  Config config;
  float* readings = nullptr;
  String* sensor_ids = nullptr;
  unsigned long last_requested_readings = 0;
  Frog(uint8_t* _frog_id) { memcpy(frog_id, _frog_id, 16); }
  void setConfig(void* _config) {
    memcpy(&config, _config, sizeof(config));
    readings = new float[config.count];
    if (!sensor_ids)
      setSensorIds();
  }
  void setReadings(float* _readings) {
    memcpy(readings, _readings, sizeof(float) * config.count);
  }
  void setSensorIds() {
    String frog_path = MULTIFROG_PATH + "/" + uuid_stringify(frog_id) + "/";
    sensor_ids = new String[config.count];
    for (int i = 0; i < config.count; i++) {
      uint8_t buff[16];
      ESPRandom::uuid(buff);
      sensor_ids[i] = ESPRandom::uuidToString(buff);
      String path = frog_path + sensor_ids[i];
      // Firebase.RTDB.setInt(&db, path.c_str(), i);
    }
    String newpath = frog_path + "new";
    // Firebase.RTDB.deleteNode(&db, newpath.c_str());
  }
  void setSensorIds(String data) {
    FirebaseJson json;
    size_t len = json.iteratorBegin(data.c_str());
    sensor_ids = new String[len];
    String key, value;
    int type = 0;
    for (size_t i = 0; i < len; i++) {
      json.iteratorGet(i, type, key, value);
      sensor_ids[value.toInt()] = key;
    }
    json.iteratorEnd();
  }
  ~Frog() {
    if (readings)
      delete[] readings;
    if (sensor_ids)
      delete[] sensor_ids;
  }
};

uint8_t DEVICE_ID[17];
LinkedList<Frog*> frogs;

Frog* getFrog(uint8_t* frog_id) {
  Frog* frog;
  for (int i = 0; i < frogs.size(); i++) {
    frog = frogs.get(i);
    if (uuid_equals(frog->frog_id, frog_id))
      return frog;
  }
  return nullptr;
}

struct Network : public NetworkBase {
  using NetworkBase::NetworkBase;
  void handleMessage() {
    Serial.println("Message to me");
    switch (parser.message.message_id) {
      case MESSAGE_ID_CONFIG: {
        Serial.println("Got config");
        Frog* frog = getFrog(inbound.from_id);
        if (!frog) {
          Serial.println("From unknown device");
          return;
        }
        frog->setConfig(parser.message.payload);
        for (uint8_t i = 0; i < frog->config.count; i++) {
          if (frog->config.types[i] == SENSOR_TYPE_TEMPERATURE)
            Serial.println(String("Sensor ") + i + String("is temperature"));
          else if (frog->config.types[i] == SENSOR_TYPE_VOLTAGE)
            Serial.println(String("Sensor ") + i + String("is voltage"));
        }
        frog->last_requested_readings = millis();
        respond(MESSAGE_ID_REQUEST_READINGS, 0, nullptr);
        return;
      }
      case MESSAGE_ID_READINGS: {
        Serial.println("Got readings");
        Frog* frog = getFrog(inbound.from_id);
        if (!frog) {
          Serial.println("From unknown device");
          return;
        }
        frog->setReadings((float*)inbound.payload);
        for (uint8_t i = 0; i < frog->config.count; i++) {
          if (frog->config.types[i] == SENSOR_TYPE_TEMPERATURE)
            Serial.println(String("Sensor ") + i + String("is temperature = ") +
                           frog->readings[i]);
          else if (frog->config.types[i] == SENSOR_TYPE_VOLTAGE)
            Serial.println(String("Sensor ") + i + String("is voltage = ") +
                           frog->readings[i]);
        }
        return;
      }
    }
  }
  void handleBadMessage() { Serial.println("Got BAD message"); }
  void handleTimeout() { Serial.println("Discarded broken message"); }
  void handleAnyMessage() { Serial.println("Got message"); }
  void handleSent(uint8_t* packed, uint8_t packed_length) {
    Serial.println("Message sent");
  }
  void handleBroadcast() { Serial.println("Got Broadcast message"); }
};

Network network(HC12, DEVICE_ID);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  EEPROM.begin(64);
  Serial.begin(115200);
  HC12.begin(9600);

  DEVICE_ID[16] = 0;
  read_device_uuid(DEVICE_ID);
  Serial.print("Device UUID: ");
  print_uuid(Serial, DEVICE_ID);
  Serial.println();

  database_setup();
}

void database_loop();
void frogs_loop();

unsigned long dataMillis = 0;
int count = 0;

void loop() {
  network.loop();
  database_loop();
  frogs_loop();
  if (Firebase.ready() && millis() - dataMillis > 5000) {
    dataMillis = millis();
    Serial.printf("Set int... %s\n", Firebase.RTDB.setInt(&db, "/test/int", count++) ? "ok" : db.errorReason().c_str());
  }
}

void handleNewFrog(uint8_t*);
void handleFrog(uint8_t*, String);
bool needs_init = true;
void database_loop() {
  if (needs_init) {
    needs_init = false;
    digitalWrite(LED_BUILTIN, HIGH);

    MULTIFROG_PATH = "/users/";
    MULTIFROG_PATH += USER_ID;
    MULTIFROG_PATH += "/multifrogs/";
    MULTIFROG_PATH += uuid_stringify(DEVICE_ID);

    Serial.println("Reading... " + MULTIFROG_PATH);
    if (Firebase.RTDB.getJSON(&db, MULTIFROG_PATH.c_str())) {
      auto json = db.jsonObject();
      size_t len = json.iteratorBegin();
      String key, value;
      int type = 0;
      for (size_t i = 0; i < len; i++) {
        json.iteratorGet(i, type, key, value);
        if (value.length() && value[0] == '{') {
          Serial.println("Frog " + key);
          uint8_t frog_id[16];
          uuid_parse(key, frog_id);
          if (value.indexOf("\"new\"") > 0)
            handleNewFrog(frog_id);
          else {
            handleFrog(frog_id, value.c_str());
          }
        }
      }
      json.iteratorEnd();
    } else
      Serial.println("Error!");
    Serial.println(db.errorReason());
  }
}

void handleNewFrog(uint8_t* frog_id) {
  Frog* frog = new Frog(frog_id);
  frogs.add(frog);
  network.send(frog->frog_id, MESSAGE_ID_REQUEST_CONFIG, 0, nullptr);
}

void handleFrog(uint8_t* frog_id, String data) {
  Frog* frog = new Frog(frog_id);
  frog->setSensorIds(data);
  frogs.add(frog);
  network.send(frog->frog_id, MESSAGE_ID_REQUEST_CONFIG, 0, nullptr);
}

void frogs_loop() {
  auto now = millis();
  Frog* frog;
  for (int i = 0; i < frogs.size(); i++) {
    frog = frogs.get(i);
    if (frog->sensor_ids &&
        now - frog->last_requested_readings > FROG_UPDATE_TIMEOUT) {
      frog->last_requested_readings = millis();
      network.send(frog->frog_id, MESSAGE_ID_REQUEST_READINGS, 0, nullptr);

      /*
      for (int i = 0; i < frog->config.count; i++) {
        String frog_path = MULTIFROG_PATH + "/" + uuid_stringify(frog->frog_id);
        auto sensor_path = frog_path + "/" + frog->sensor_ids[i];
        Firebase.RTDB.setFloat(&db, sensor_path.c_str(), frog->readings[i]);
      }
      */
    }
  }
}