// WiFi + UDP
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <ArduinoJson.h>

bool openState;

// Helper Functions
void readJson();

WiFiUDP udp;
JsonDocument json;

char ssid[] = "UNDEFINED";
char pass[] = "";
//int udpPort = UNDEFINED;
char* udpAdd = "UNDEFINED";

void setup(){

}

void loop() {

}

void bridgePivot(){
  //if want open, open

  //if want close, close
}

void controlSys(bool openState){
  if(openState){
    //booms down
    //lights
  }
  else(){
    //booms up
    //lights
  }
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