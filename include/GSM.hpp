#pragma once

#define TINY_GSM_MODEM_SIM800
#define SIM800 Serial1

#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <SSLClient.h>
#include <TinyGsmClient.h>

#include "keys.h"
#include "trust_anchors.h"

struct A {
  public:
  void g() {}
};

TinyGsm       modem(SIM800);
TinyGsmClient gsm_client(modem);
SSLClient     ssl_client(gsm_client, TAs, (size_t) TAs_NUM, /*rand_pin*/ 25);
HttpClient    http_client(ssl_client, DATABASE_DOMAIN, 443);

void gsm_setup() {
  SIM800.begin(115200, SERIAL_8N1, 27, 26);
  Serial.println("Initializing GSM...");
  modem.restart();
}

void gsm_loop() {
  Serial.print("Waiting for network... ");
  if (!modem.waitForNetwork()) {
    Serial.println("fail");
    delay(1000);
    return;
  }
  Serial.println("success");

  Serial.print("Connecting GPRS... ");
  if (!modem.gprsConnect(GSM_APN)) {
    Serial.println("fail");
    delay(1000);
    return;
  }
  Serial.println("success");

  Serial.print("Patching data... ");

  http_client.connectionKeepAlive();
  if (http_client.patch(
    "/public.json?auth=" + String(DATABASE_SECRET),
    "application/json",
    "{\"time\":\"" + modem.getGSMDateTime(DATE_FULL) + "\"}"
  )) {
    Serial.println("fail");
    delay(1000);
    return;
  }
  Serial.println("success");

  int status = http_client.responseStatusCode();
  Serial.print("Response status code: ");
  Serial.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  Serial.println("Response Headers:");
  while (http_client.headerAvailable()) {
    String headerName  = http_client.readHeaderName();
    String headerValue = http_client.readHeaderValue();
    Serial.println("    " + headerName + " : " + headerValue);
  }

  int length = http_client.contentLength();
  if (length >= 0)
    Serial.println(String("Content length is: ") + length);
  if (http_client.isResponseChunked())
    Serial.println("The response is chunked");

  String body = http_client.responseBody();
  Serial.println("Response:");
  Serial.println(body);

  Serial.print("Body length is: ");
  Serial.println(body.length());

  http_client.stop();
  Serial.println("Server disconnected");

  modem.gprsDisconnect();
  Serial.println("GPRS disconnected");
  while (true) delay(1000);
}