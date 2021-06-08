#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "keys.h"

#define Serial0 Serial

FirebaseData fbdb;
FirebaseAuth auth;
FirebaseConfig config;

void setup() {
  Serial0.begin(115200);
  Serial0.printf("Ready");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  config.service_account.data.client_email = CLIENT_EMAIL;
  config.service_account.data.client_id = CLIENT_ID;
  config.service_account.data.private_key = PRIVATE_KEY;
  config.service_account.data.private_key_id = PRIVATE_KEY_ID;
  config.service_account.data.project_id = PROJECT_ID;
  auth.token.uid = DATABASE_UID;
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);
}

unsigned long dataMillis = 0;
int count = 0;
void loop() {
  while (Serial0.available())
    Serial2.write(Serial0.read());
  while (Serial2.available())
    Serial0.write(Serial2.read());
  
  if (Firebase.ready() && millis() - dataMillis > 5000) {
    dataMillis = millis();
    Serial.printf("Set int... %s\n", Firebase.RTDB.setInt(&fbdb, "/test/int", count++) ? "ok" : fbdb.errorReason().c_str());
  }
}