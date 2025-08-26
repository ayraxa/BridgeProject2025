// WiFi + UDP
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <ArduinoJson.h>

// Helper Functions
void readJson();
void encoderISR();

WiFiUDP udp;
JsonDocument json;

char ssid[] = "UNDEFINED";
char pass[] = "";
//int udpPort = UNDEFINED;
char* udpAdd = "UNDEFINED";

void setup(){
    pinMode(13, OUTPUT);
}

void loop() {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(100);
}

void readJson() {
  int packetSize = udp.parsePacket();
  if(packetSize) {
    int size = udp.read(readBuffer, packetSize);
    //readBuffer[size] = '\0';
    Serial.println(readBuffer);
    Serial.println(speed);
    deserializeJson(json, readBuffer);

    const uint8_t buffer[50] = "recieved!";
    udp.beginPacket(udpAdd, udpPort);
    udp.write(buffer, 11);
    udp.endPacket();
  }
}