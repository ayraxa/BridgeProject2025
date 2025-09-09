#include <WiFi.h>
#include <arduino-timer.h>

// network config
const char* ssid = "Engg2k3k";
const char* password = "password";
const int serverPort = 1234;

WiFiServer server(serverPort);
WiFiClient client;

// pin definition
const int trafLedGreen = 27;   //change pins 
const int trafLedYellow = 26;
const int trafLedRed = 25;
const int pedLedGreen = 0;
const int pedLedRed = 0;
const int endStopOpen = 0;
const int endStopClosed = 0;
const int beamBreak1 = 0;
const int beamBreak2 = 0;
const int motorPin = 0;
const int servoPin = 0;
const int piezoPin = 0;

// variables
int ledState = 0;   //0 = off, 1 = green, 2 = yellow/flashing red, 3 = full red
int beam1State;
int beam2State;
int esOpenState;
int esClosedState;
int beamCounter; //entered = TRUE, exited = FALSE
bool bridgeState; //open = TRUE, closed = False
auto timer = timer_create_default();

void setup() {
  // Pin in/out allocation
  pinMode(trafLedGreen, OUTPUT);
  pinMode(trafLedYellow, OUTPUT);
  pinMode(trafLedYellow, OUTPUT);
  pinMode(pedLedGreen, OUTPUT);
  pinMode(pedLedRed, OUTPUT);
  pinMode(piezoPin, OUTPUT);
  pinMode(endStopOpen, INPUT);
  pinMode(endStopClosed, INPUT);
  pinMode(beamBreak1, INPUT);
  pinMode(beamBreak2, INPUT);

  Serial.begin(115200);
  delay(1000);

  // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n Wi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start TCP server
  server.begin();
  Serial.print("TCP server started on port ");
  Serial.println(serverPort);
}

void loop() {
  // Check for incoming client connection
  if (!client || !client.connected()) {
    client = server.available();
    if (client) {
      Serial.println("java client connected!");
      client.println("Hello from ESP32!\n");
      while () {
        main();
      }
    }
  }
  delay(100);
}

void main(){
  beamBreakCheck();
  while(beamCounter != 0){
    timer.every(1000, beamBreakCheck());
    bridgeOpen();
  }
  bridgeClose();
  timer.tick;
}

void trafficLight(){
  if(ledState==0)    {    //off
      digitalWrite(trafLedGreen,LOW);
      digitalWrite(trafLedYellow,LOW);
      digitalWrite(trafLedRed,LOW);

  }
  else if(ledState==1)    {   //green
      digitalWrite(trafLedGreen,HIGH);
      digitalWrite(trafLedYellow,LOW);
      digitalWrite(trafLedRed,LOW);

  }
  else if(ledState==2)    {    //yellow/flashing red
      digitalWrite(trafLedGreen,LOW);
      digitalWrite(trafLedYellow,HIGH);
      digitalWrite(trafLedRed,LOW);

          }
  else if(ledState==3)    {    //full red
      digitalWrite(trafLedGreen,LOW);
      digitalWrite(trafLedYellow,LOW);
      digitalWrite(trafLedRed,HIGH);

  }
}
              

bool endStopOpenCheck() {
  if(esOpenState == LOW){
    return true;
  }
  if(esClosedState == LOW){
    return false;
  }
}


void beamBreakCheck() {
  if (beam1State == LOW){
    beamCounter += 1;
  }
  if(beam2State == LOW){
    beamCounter -= 1;
  }
}

void bridgeOpen(){
  if(!endStopOpenCheck){

  }
  bridgeState = true;
}

void bridgeClose(){
  if(endStopOpenCheck){

  }
  bridgeState = false;
}