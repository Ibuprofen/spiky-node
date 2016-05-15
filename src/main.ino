#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <SD.h>

#include "Secrets.h"

/* Soft AP network parameters */
IPAddress apIP(192, 168, 5, 1);
IPAddress netMsk(255, 255, 255, 0);

// mac address
uint8_t MAC_array[6];
char MAC_char[18];

// UDP
WiFiUDP UDP;
#define UDP_PORT 8888

// LED
const double BRIGHTNESS = 0.1;//1.0;
#define NUM_LEDS     30  // maximum LED node number we can receive
#define NUM_NODES NUM_LEDS
uint8_t incoming_leds[NUM_LEDS * 3] = {0};
uint8_t incoming_state = 0; // 0=none, 1=node
uint8_t incoming_index = 0;
uint8_t incoming_led;
uint8_t incoming_red;
uint8_t incoming_green;
uint8_t incoming_blue;
uint8_t outgoing_leds[NUM_LEDS * 3] = {0};

// fallback animation
unsigned long lastHeardDataAt = millis();
int globalHue = 0;

// SD
File root;

// func declarations
void printRoot();
void printDirectory(File dir);
void readFile(File* file);
size_t readField(File* file, char* str, size_t size, char* delim);
bool isCSVFile(char* filename);
void startFrame();
void fadeNext();
void setLedColorHSV(int h, double s, double v);
void setColor(int r, int g, int b);

// string to lowercase
char *strlwr(char *str)
{
  unsigned char *p = (unsigned char *)str;

  while (*p) {
     *p = tolower(*p);
      p++;
  }

  return str;
}

void setup() {

  Serial.begin(115200);
  Serial1.begin(115200);  // TX1 is GPIO2

  delay(100); // iono

  // WIFI Access Point  --------------------------------------------------------
  Serial.println();
  Serial.print("Configuring access point... ");

  WiFi.enableAP(true);
  WiFi.mode(WIFI_AP);

  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();

  Serial.print("AP IP address: ");
  Serial.println(myIP);

  Serial.print("MAC address: ");
  WiFi.macAddress(MAC_array);
  for (int i = 0; i < sizeof(MAC_array); ++i){
    sprintf(MAC_char,"%s%02x:",MAC_char,MAC_array[i]);
  }
  Serial.println(MAC_char);


  // UDP  ----------------------------------------------------------------------
  while (!UDP.begin(UDP_PORT)) {
    delay(250);
  }
  Serial.println("UDP ready");

  // SD
  Serial.print("Initializing SD card...");

  // put the SD card on the HSPI pins D5-D8
  if (!SD.begin()) {
    Serial.println("initialization failed!");
    //return;
  } else {
    Serial.println("initialization done.");
  }

  //printRoot();
}

void loop() {

  int avail = UDP.parsePacket();
  while (avail > 0) {

    lastHeardDataAt = millis();

    unsigned char c = UDP.read();

    if (c == '@') {
      incoming_state = 1;
      incoming_index = 0;
    } else if (c == '#') {

      startFrame();

      memcpy(outgoing_leds, incoming_leds, sizeof(incoming_leds));

      Serial1.write(outgoing_leds, sizeof(outgoing_leds));

      Serial1.flush();

      incoming_state = 0;

    } else {
      if (incoming_state == 1) {

        // decode incoming LED data as it arrives
        if (c >= '0' && c <= '9') {
          int n = c - '0';
          switch (incoming_index) {
          case  0: incoming_led   = n * 100; break;
          case  1: incoming_led   += n * 10; break;
          case  2: incoming_led   += n;      break;
          case  3: incoming_red   = n * 100; break;
          case  4: incoming_red   += n * 10; break;
          case  5: incoming_red   += n;      break;
          case  6: incoming_green = n * 100; break;
          case  7: incoming_green += n * 10; break;
          case  8: incoming_green += n;      break;
          case  9: incoming_blue  = n * 100; break;
          case 10: incoming_blue  += n * 10; break;
          case 11: incoming_blue  += n;      break;
          default: incoming_index = 0;       break;
          }
          incoming_index++;
          if (incoming_index >= 12) {
            if (incoming_led < NUM_LEDS) {
              n = incoming_led * 3;
              incoming_leds[n+0] = incoming_red;
              incoming_leds[n+1] = incoming_green;
              incoming_leds[n+2] = incoming_blue;
            }
            incoming_index = 0;
          }
        }
      }
    }

    avail = avail - 1;
  }

  // nothing heard in a while.. do SOMETHING
  if (lastHeardDataAt + 10 * 1000 < millis()) {
    fadeNext();
    delay(10);
  }

  // allow the ESP to do what it do
  yield();
}


void printRoot() {
  root = SD.open("/");

  printDirectory(root);

  //Serial.println("done!");
}


void printDirectory(File dir) {
  while (true) {

    File file = dir.openNextFile();
    if (!file) {
      Serial.println("No more files. rewinding.");
      //dir.rewindDirectory();
      // no more files
      break;
    } else {
      readFile(&file);
      //Serial.println("Closing file.");
      file.close();
    }
  }
}


void readFile(File* file) {

  Serial.print(file->name());

  if (file && !file->isDirectory()) {

    if ( !isCSVFile(file->name() ) ) {
      //Serial.print("Not a csv file: ");
      //Serial.println(file->name());
      return;
    }

    // files have sizes, directories do not
    Serial.print("\t\t");
    Serial.println(file->size(), DEC);

    size_t n;      // Length of returned field with delimiter.
    char str[5];  // Must hold longest field with delimiter and zero byte.
    uint8_t frame[NUM_NODES * 3] = {0}; // r, g, b
    int column = 0;
    bool eol = false; // end of line

    // Read the file and print fields.
    while (true) {
      n = readField(file, str, sizeof(str), ",\n");

      // done if Error or at EOF.
      if (n == 0) break;

      // Print the type of delimiter.
      if (str[n-1] == ',' || str[n-1] == '\n') {
        ////Serial.print(str[n-1] == ',' ? F("comma: ") : F("endl:  "));

        if (str[n-1] == '\n') {
          eol = true;
        }

        // Remove the delimiter.
        str[n-1] = 0;
      } else {
        // At eof, too long, or read error.  Too long is error.
        //Serial.print(file->available() ? F("error: ") : F("eof:   "));
      }

      // Print the field.
      /*//Serial.print(str);
      //Serial.print("\t");
      //Serial.println(atoi(str));*/

      frame[column] = atoi(str);

      // frame is complete
      if (eol) {

        // send frame
        startFrame();
        Serial1.write(frame, sizeof(frame));
        Serial1.flush();

        delay(10);

        eol = false;
        column = 0;
      } else {
        column++;
      }

    }

    //Serial.println("File read complete.");
    yield();
  }

}


/*
 * Read a file one field at a time.
 *
 * file - File to read.
 * str - Character array for the field.
 * size - Size of str array.
 * delim - String containing field delimiters.
 * return - length of field including terminating delimiter.
 * Note, the last character of str will not be a delimiter if
 * a read error occurs, the field is too long, or the file
 * does not end with a delimiter.  Consider this an error
 * if not at end-of-file.
 *
 */
size_t readField(File* file, char* str, size_t size, char* delim) {
  char ch;
  size_t n = 0;
  while ((n + 1) < size && file->read(&ch, 1) == 1) {
    // Delete CR.
    if (ch == '\r') {
      continue;
    }
    str[n++] = ch;
    if (strchr(delim, ch)) {
        break;
    }
  }
  str[n] = '\0';
  return n;
}


bool isCSVFile(char* filename) {
  int8_t len = strlen(filename);
  bool result;
  if (  strstr(strlwr(filename + (len - 4)), ".csv")
     || strstr(strlwr(filename + (len - 4)), ".cs")
     // and anything else you want
    ) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

void startFrame() {
  Serial1.begin(38400);
  Serial1.write(0); // start of frame message
  Serial1.flush();
  delay(1); // ESP specific
  Serial1.begin(115200);
  Serial1.write(0);
}


void setColor(int r, int g, int b) {

  startFrame();

  for (int i=1; i <= NUM_LEDS; i++) {
    Serial1.write(int(r * BRIGHTNESS));
    Serial1.write(int(b * BRIGHTNESS));
    Serial1.write(int(g * BRIGHTNESS));
  }

  Serial1.flush();
}

void setLedColorHSV(int h, double s, double v) {
  //this is the algorithm to convert from RGB to HSV
  double r=0;
  double g=0;
  double b=0;

  double hf=h/60.0;

  int i=(int)hf;
  double f = h/60.0 - i;
  double pv = v * (1 - s);
  double qv = v * (1 - s*f);
  double tv = v * (1 - s * (1 - f));

  switch (i)
  {
  case 0: //rojo dominante
    r = v;
    g = tv;
    b = pv;
    break;
  case 1: //verde
    r = qv;
    g = v;
    b = pv;
    break;
  case 2:
    r = pv;
    g = v;
    b = tv;
    break;
  case 3: //azul
    r = pv;
    g = qv;
    b = v;
    break;
  case 4:
    r = tv;
    g = pv;
    b = v;
    break;
  case 5: //rojo
    r = v;
    g = pv;
    b = qv;
    break;
  }

  //set each component to a integer value between 0 and 255
  int red=constrain((int)255*r,0,255);
  int green=constrain((int)255*g,0,255);
  int blue=constrain((int)255*b,0,255);

  setColor(red,green,blue);
}

void fadeNext() {
  int hue = globalHue + 1;
  if (hue == 360) {
    hue = 0;
  }
  setLedColorHSV(hue, 1, 1); //We are using Saturation and Value constant at 1
  globalHue = hue;
}
