#pragma once
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <SPI.h>
#include <WiFi.h>

#include "utils.hpp"
#include "Message.hpp"
#include "keys.h"

FirebaseAuth auth;
FirebaseConfig config;

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