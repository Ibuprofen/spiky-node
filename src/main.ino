#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <SD.h>

#include "Secrets.h"

/* Soft AP network parameters */
IPAddress apIP(192, 168, 5, 1);
IPAddress netMsk(255, 255, 255, 0);

// UDP
WiFiUDP UDP;
#define UDP_PORT 8888

// SD
File root;

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


  // SD
  Serial.print("Initializing SD card...");

  // put the SD card on the HSPI pins D5-D8
  if (!SD.begin()) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  printRoot();

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


void printRoot() {
  root = SD.open("/");

  printDirectory(root, 0);

  Serial.println("done!");
}


void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (!entry) {
      Serial.println("No more files. rewinding.");
      //dir.rewindDirectory();
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
      // re-open the file for reading:
      //myFile = SD.open(entry.name());
      if (entry) {

        // read from the file until there's nothing else in it:
        while (entry.available()) {
          //Serial1.write(entry.read());
          entry.read();
          yield(); // prevent wdt
        }
        // close the file:
        entry.close();
        Serial.println("File read. Delaying...");
      } else {
        // if the file didn't open, print an error:
        Serial.println("error opening file");
      }


    }
    //entry.close();
  }
}
