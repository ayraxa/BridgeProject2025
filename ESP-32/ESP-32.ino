#include <WiFi.h>
#include <Arduino.h>
#include <arduino-timer.h>
#include <Servo.h>

Servo gateServo = Servo();

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
const int servoPin = 12;
const int piezoPin = 0;
const int trigPin = 0; //ultra
const int echoPin = 0; //ultra
int motorPinIn1 = 19;  //connect to in1
int motorPinIn2 = 18;  //connect to in2


// variables
int ledState = 0;   //0 = off, 1 = green, 2 = yellow/flashing red, 3 = full red
int beam1State, beam2State;
int esOpenState, esClosedState;
int beamCounter; //entered +=1, exited -=1
bool bridgeState; //open = TRUE, closed = False
auto timer = timer_create_default();
int servoSpeed = 1; //servo speed from 1-10 //dont exceed 100
int servoMaxUs = 2100;  //specs for SG90 are 2400/1500/600 //specs for FS90 should be 2100/1500/900
int servoMidUs = 1630;
int servoMinUs = 630;
int timeDelay = 0;
int servoPos = 1;\
int motorPinSpeed = 21;  //connect to ENA
int motorSpeed = 127; //from 0 to 255


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
  pinMode(trigPin, OUTPUT);  
	pinMode(echoPin, INPUT);

  timeDelay=millis();

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
      while (client) {
        home();
      }
    }
  }
  delay(100);
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


bool beamBreakCheck(void *) {
  if (beam1State == LOW){
    beamCounter += 1;
  }
  if(beam2State == LOW){
    beamCounter -= 1;
  }
  return true;
}

void bridgeOpen(){
  if(!endStopOpenCheck){ //if not open
    if(bridgeState == false){
      ledState = 2;
      trafficLight();
      delay(3000); //change to non blocking delay
      ledState = 3;
      trafficLight();
      moveServoMid(servoPin, servoSpeed);
    }
    //spin forwards --- 1-high 2-low
    digitalWrite(motorPinIn1, HIGH);
    digitalWrite(motorPinIn2, LOW);
    analogWrite(motorPinSpeed, motorSpeed);  //add pwm
    ledState = 3;
    bridgeState = true;
  }
  //motor off 
  digitalWrite(motorPinIn1, LOW);
  digitalWrite(motorPinIn2, LOW);
  analogWrite(motorPinSpeed, 0);
  trafficLight();
}

void bridgeClose(){
  if(endStopOpenCheck){
    ledState = 3; //red
  }
  else{
    ledState = 1; //green
    if(bridgeState == true){
      moveServoMin(servoPin, servoSpeed);
    }
    //spin backwards --- 1-low 2-high
    digitalWrite(motorPinIn1, LOW);
    digitalWrite(motorPinIn2, HIGH);
    analogWrite(motorPinSpeed, motorSpeed);  //add pwm
    bridgeState = false;
  }
  //motor off 
  digitalWrite(motorPinIn1, LOW);
  digitalWrite(motorPinIn2, LOW);
  analogWrite(motorPinSpeed, 0);
  trafficLight();
}

void moveServoMid(int pin, int speed) {
    int speedDelay=millis();
    int i=servoMinUs;

    while(i<=servoMidUs) {
        if(millis()-speedDelay>10) {
            gateServo.writeMicroseconds(pin,i);
            speedDelay=millis();
            i=i+1*servoSpeed;
        }
    }
    gateServo.writeMicroseconds(pin,servoMidUs);
}

void moveServoMin(int pin, int speed) {
    int speedDelay=millis();
    int i=servoMidUs;

    while(i>=servoMinUs) {
        if(millis()-speedDelay>10) {
            gateServo.writeMicroseconds(pin,i);
            speedDelay=millis();
            i=i-1*servoSpeed;
        }
    }
    gateServo.writeMicroseconds(pin,servoMinUs);
}

bool checkBoatUnder(){
  digitalWrite(trigPin, LOW);  
	delayMicroseconds(2);  
	digitalWrite(trigPin, HIGH);  
	delayMicroseconds(10);  
	digitalWrite(trigPin, LOW);

  float duration = pulseIn(echoPin, HIGH);
  float distance = (duration*.0343)/2;
  if(distance < 23.75){ //if return distance is less than the expected 250mm -5%
    return true;
  }
  return false;
}

void home(){
  timer.every(1000, beamBreakCheck);
  while(beamCounter != 0){
    timer.every(1000, beamBreakCheck);
    bridgeOpen();
    timer.tick();
  }
  if(!checkBoatUnder){
    bridgeClose();
  }
  timer.tick();
}