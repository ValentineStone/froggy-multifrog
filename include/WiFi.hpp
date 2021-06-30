#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <SSLClient.h>
#include <ArduinoHttpClient.h>

#include "keys.h"

WiFiClient wifi_client;
SSLClient  wifi_ssl_client(wifi_client, TAs, (size_t) TAs_NUM, RANDOM_PIN);
HttpClient wifi_http_client(wifi_ssl_client, DATABASE_DOMAIN, 443);

bool wifi_setup(uint64_t wait_for) {
  Serial.println("Initializing WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint64_t started = millis();
  Serial.print("Waiting for connection ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    if (wait_for && millis() - started > wait_for) {
      Serial.println(" timed out");
      return false;
    }
    delay(1000);
  }
  Serial.println();
  wifi_http_client.connectionKeepAlive();
  Serial.println("Wifi connected");
  return true;
}

void wifi_cleanup() {
  wifi_http_client.stop();
  wifi_client.stop();
}