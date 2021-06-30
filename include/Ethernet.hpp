#pragma once

#include <Ethernet.h>
#include <SSLClient.h>
#include <ArduinoHttpClient.h>

#include "keys.h"
#include "trust_anchors.h"

#define ETHERNET_CS_PIN 5

// Set the static IP address to use if the DHCP fails to assign
// IPAddress ip(192, 168, 0, 177);
// IPAddress myDns(192, 168, 0, 1);

EthernetClient eth_client;
SSLClient  eth_ssl_client(eth_client, TAs, (size_t) TAs_NUM, RANDOM_PIN);
HttpClient eth_http_client(eth_ssl_client, DATABASE_DOMAIN, 443);

bool ethernet_setup(uint64_t wait_for) {
  Serial.println("Initializing Ethernet...");
  Ethernet.init(ETHERNET_CS_PIN);
  if (!Ethernet.begin(ETHERNET_MAC, wait_for)) {
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Hardware not connected...");
      return false;
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Cable not connected...");
      return false;
    }
    // Ethernet.begin(ETHERNET_MAC, ip, myDns);
  }
  // give the Ethernet module a second to initialize:
  delay(1000); //////TRASH//////TRASH//////TRASH//////TRASH//////TRASH//////TRAS

  Serial.print("Ethernet connected: ");
  Serial.println(Ethernet.localIP());
  eth_http_client.connectionKeepAlive();
  return true;
}

void ethernet_celanup() {
  eth_http_client.stop();
  eth_client.stop();
}
