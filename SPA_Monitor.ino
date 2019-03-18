// *** OTA Specific ***
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// ********************

#include <WiFiClient.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

#define GPIO_IN ((volatile uint32_t*) 0x60000318)     //GPIO Read Register
#define GPIO_OUT ((volatile uint32_t*) 0x60000300)    //GPIO Write Register

// Private.h contains the ssid and password as a temporary measure until a config page is added
#include "Private.h"
//const char* ssid = "TestSSID";
//const char* password = "TestPASS";

uint16_t buf = 0;                                     //16-Bit buffer

char segments[4];
uint8_t allSegs = 0;

uint8_t btnRequest;
uint8_t btnCount;

bool flushSync = false;                               //True when display reset, used to sync buffer
bool btnSync = false;                                 //True during button frame

void readSegment(int seg) {
  uint16_t digit = buf & 13976;                       //AND buf with mask to strip off values, like buzzer, etc.
  char sx = -1;
  
  if (digit == 16)    sx = '0';
  if (digit == 9368)  sx = '1';
  if (digit == 520)   sx = '2';
  if (digit == 136)   sx = '3';
  if (digit == 9344)  sx = '4';
  if (digit == 4224)  sx = '5';
  if (digit == 4096)  sx = '6';
  if (digit == 1176)  sx = '7';
  if (digit == 0)     sx = '8';
  if (digit == 1152)  sx = '9';

  if (digit == 4624)  sx = 'C';
  if (digit == 5632)  sx = 'F';
  if (digit == 4608)  sx = 'E';

  if (seg != -1)
    segments[seg] = sx;

  //allSegs++;
}

uint8_t ledStates;

void readLEDStates() {
    ledStates = 0;

    if (bitRead(buf, 0) == 0) bitSet(ledStates, 0); //Power
    if (bitRead(buf, 10) == 0) bitSet(ledStates, 1); //Bubbles
    if (bitRead(buf, 9) == 0) bitSet(ledStates, 2); //Heater A
    
//    if (bitRead(buf, 0) == 1) bitSet(ledStates, 3); //
//    if (bitRead(buf, 0) == 1) bitSet(ledStates, 4); //
}

void writeButtonPress() {
  
}

String ledstat() {
  String message = "";
  message += "Power: ";
  message += (bitRead(ledStates, 0) == 1)?"ON" : "OFF";
  message += "\n";
  message += "Bubbles: ";
  message += (bitRead(ledStates, 1) == 1)?"ON" : "OFF";
  message += "\n";
  message += "Heater A: ";
  message += (bitRead(ledStates, 2) == 1)?"ON" : "OFF";
  message += "\n";
  
  return message;
}

void handleRoot() {
  server.send(200, "text/plain", "<body>hello from esp8266!</body>");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

/******************************************************************************************/
//16-Bit Shift Register

const int CLK = 13;
const int LAT = 12;
const int DAT = 14;

uint8_t clkCount = 0;

bool outputMode = false;
bool outputEnabled = false;

//Just clock the data bits into the buffer
void ICACHE_RAM_ATTR handleClock() {  
  clkCount++;
  buf = buf << 1;                                 //Shift buffer along
  if (bitRead(*GPIO_IN, DAT) == 1) bitSet(buf, 0); //Flip data bit in buffer if needed.
}

//
void ICACHE_RAM_ATTR handleLatch() {
  if (bitRead(*GPIO_IN, DAT) == 0) {
    //pinMode(DAT, INPUT);
  } else {
    if(flushSync) {
  
      /*
       * Filter
       * Heater
       * ?FC
       * Down
       * Bubble
       * Power
       * ?Up
       * ?
       */

  
      //Expensive?
      /*if ((buf | 0x100) == 0xFFFD) {}
      if ((buf | 0x100) == 0x7FFF) {}
      if ((buf | 0x100) == 0xEFFF) {}
      if ((buf | 0x100) == 0xFF7F) {}
      if ((buf | 0x100) == 0xFFF7) {}
      if ((buf | 0x100) == 0xFEFF) {
          //if (bitRead(ledStates, 0) == 0) {
            //pinMode(DAT, OUTPUT);
            //digitalWrite(DAT, LOW);
          //}
        }
      if ((buf | 0x100) == 0xDFFF) {}
      if ((buf | 0x100) == 0xBFFE) {}*/
  
    /*
      if (bitRead(buf, 1) == 0)) {}
      if (bitRead(buf, 15) == 0)) {}
      if (bitRead(buf, 12) == 0)) {}
      if (bitRead(buf, 7) == 0)) {}
      if (bitRead(buf, 3) == 0)) {}
      if (bitRead(buf, 10) == 0)) {}
      if (bitRead(buf, 13) == 0)) {}
      if (bitRead(buf, 0) == 0)) {}
      */
    
      
      /*if (btnCount > 7) {
        btnSync = false;
        btnCount = 0;
      }
    
      if (btnSync) {
        if (bitRead(btnRequest,btnCount) == 1 && clkCount == 8) {
          //pinMode(DAT, OUTPUT);
          //digitalWrite(DAT, HIGH);
          //bitSet(*GPIO_OUT, DAT);
          bitClear(btnRequest,btnCount);
          Serial.println(btnCount);
        } else {
          pinMode(DAT, INPUT);
        }
        btnCount++;
      }*/
      if (clkCount == 16) {
        //Decode display if valid
        if (bitRead(buf, 6) == 0)   readSegment(0);
        if (bitRead(buf, 5) == 0)   readSegment(1);
        if (bitRead(buf, 11) == 0)  readSegment(2);
        if (bitRead(buf, 2) == 0)   readSegment(3);
        if (bitRead(buf, 14) == 0)  readLEDStates();
      }
      
      flushSync = false; //((buf | 0xF00) == 0xFFFF); //If idle, we can use to mark a sync.
      buf = 0;
      clkCount = 0;
  
    //  if (allSegs > 4) {
      //  allSegs = 0;
        //detachInterrupt(digitalPinToInterrupt(CLK));
    //    detachInterrupt(digitalPinToInterrupt(LAT));
     // }
    } else {
      if ((buf | 0xF00) == 0xFFFF) { //If idle, we can use to mark a sync.
        flushSync = true;
        buf = 0;
        clkCount = 0;
      }
    }
  }
}

//0xFEFF Suggests Idle
//0xFFFF Suggests Button is pressed

/******************************************************************************************/

void initSegs() {
  attachInterrupt(digitalPinToInterrupt(CLK), handleClock, RISING);
  attachInterrupt(digitalPinToInterrupt(LAT), handleLatch, CHANGE);
}

void setup() {
  Serial.begin(115200);
  Serial.println("");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int dotCount;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
    if (dotCount > 1) {
      dotCount = -1;
      Serial.print("\b\b\b");
    }
    dotCount++;
  }
  Serial.println("");
  Serial.println("Intex Spa WiFi Controller");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("INTEX_Spa");
  WiFi.hostname("INTEX_Spa");
  MDNS.begin("INTEX_Spa");
 
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  server.on("/", handleRoot);

  server.on("/initsegs", []() {
    initSegs();
    server.send(200, "text/plain", "OK");
  });

  server.on("/reboot", []() {
    server.send(200, "text/plain", "Rebooting...");
    ESP.reset();
  });
  
  server.on("/digits", []() {
    String message = segments;
    message += "\n";
    message += ledstat();
    server.send(200, "text/plain", message);
  });

  server.onNotFound(handleNotFound);

  server.begin();
  
  segments[0] = '0';
  segments[1] = '0';
  segments[2] = '0';
  segments[3] = 'C';
  segments[4] = '\0';

  //Configure shift register
  pinMode(CLK, INPUT);
  pinMode(LAT, INPUT);
  pinMode(DAT, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(CLK), handleClock, RISING);
  attachInterrupt(digitalPinToInterrupt(LAT), handleLatch, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(LAT), handleLatch, RISING);
}

uint32_t mLastTime = 0;
uint32_t mTimeSeconds = 0;

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  MDNS.update();
  
  yield();
}
