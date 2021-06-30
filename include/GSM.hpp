#pragma once

#define TINY_GSM_MODEM_SIM800
#define SIM800 Serial1

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <SSLClient.h>
#include <ArduinoHttpClient.h>

#include "keys.h"
#include "trust_anchors.h"

TinyGsm       modem(SIM800);
TinyGsmClient gsm_client(modem);
SSLClient  gsm_ssl_client(gsm_client, TAs, (size_t) TAs_NUM, RANDOM_PIN);
HttpClient gsm_http_client(gsm_ssl_client, DATABASE_DOMAIN, 443);

bool gsm_setup(uint64_t wait_for) {
  Serial.println("Initializing GSM...");
  SIM800.begin(115200, SERIAL_8N1, 27, 26);
  modem.restart();
  Serial.println("Waiting for network...");
  if (!modem.waitForNetwork(wait_for)) {
    Serial.println("No network found");
    return false;
  }
  Serial.println("Connecting GPRS...");
  if (!modem.gprsConnect(GSM_APN)) {
    Serial.println("Could not connect to existing network");
    return false;
  }
  gsm_http_client.connectionKeepAlive();
  Serial.println("GSM connected");
  return true;
}

void gsm_cleanup() {
  gsm_http_client.stop();
  modem.gprsDisconnect();
}