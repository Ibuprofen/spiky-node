#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

#include "Secrets.h"

/* Soft AP network parameters */
IPAddress apIP(192, 168, 5, 1);
IPAddress netMsk(255, 255, 255, 0);

// UDP
WiFiUDP UDP;
#define UDP_PORT 8888


void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);

  // WIFI Access Point

  Serial.println();
  Serial.print("Configuring access point... ");

  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);


  // UDP
  while (!UDP.begin(UDP_PORT)) {
    delay(250);
  }
  Serial.println("UDP ready. ");

}

void loop() {
  delay(500);
  Serial.print('.');

  int avail = UDP.parsePacket();
  while (avail > 0) {
    unsigned char c = UDP.read();
    Serial.println(c);
    avail = avail - 1;
  }
}
