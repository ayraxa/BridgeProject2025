#include <Servo.h>
#include <ArduinoJson.h>

// wifi setup
const char *ssid = "iPhone";
const char *password = "";

unsigned long hbIntervalMs = 5000; // heartbeat every 5s??
unsigned long nextHeartbeatMs = 0;
uint32_t hbSeq = 0;

// tcp server
const uint16_t PORT = 1234;
WiFiServer server(PORT);
WiFiClient client;

// timing
unsigned long currentMillis = 0;

// logic setup
int bridgeState = 4; // 0 = setup, 1 = opening, 2 = open, 3 = closing, 4 = closed
String stateRead[5] = {"setup", "opening", "open", "closing", "closed"};

// motor setup
int motorPinIn1 = 19;   // connect to in1
int motorPinIn2 = 18;   // connect to in2
int motorPinSpeed = 21; // connect to ENA
int motorSpeed = 127;   // from 0 to 255

// servo setup
Servo gateServo = Servo();
int servoPin = 33;     // pin for servo logic
int servoSpeed = 1;    // servo speed from 1-10 //dont exceed 100
int servoMaxUs = 2100; // specs for SG90 are 2400/1500/600 //specs for FS90 should be 2100/1500/900
int servoMidUs = 1630;
int servoMinUs = 630;
int timeDelay = 0;
int servoPos = 1;      // 1 = min, 2 = mid, 3 = max   //set to 0 for arm calibration
int boomgateState = 0; // 0 = closed, 1 = open

// lights setup
int trafState = 0;     // 0 = off, 1 = green, 2 = yellow/flashing red, 3 = full red
int trafLedGreen = 12; // change pins
int trafLedYellow = 14;
int trafLedRed = 27;
int pedLedRed = 26;
int pedLedGreen = 25;
String lightRead[4] = {"off", "both green", "solid yellow + flasing red", "both red"};

unsigned long redFlash = 0;
unsigned long currentMillis = 0;
unsigned long cycleMillis = 0;

// beam break setup
int beam1Pin = 23;
int beam2Pin = 22;
bool beamCounter = false; // entered = true, exited = false

// endstop setup
int esTopPin = 35;
int esBottomPin = 34;
int esTopState = 0;
int esBottomState = 0;

// ultrasonic setup
int trigPin = 0; // ultra
int echoPin = 0; // ultra

// other setup
int onboardLedPin = 2; // standard for esp32
unsigned long debugMillis = 0;
int spanClosed = 0;

void setup()
{
    Serial.begin(9600);

    // setting up tcp wifi connection
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.print("\nIP: ");
    Serial.println(WiFi.localIP());

    server.begin();
    server.setNoDelay(true);
    Serial.printf("Server listening on %u\n", PORT);

    // motor pin allocations
    pinMode(motorPinIn1, OUTPUT);
    pinMode(motorPinIn2, OUTPUT);
    pinMode(motorPinSpeed, OUTPUT);

    // led pin allocations
    pinMode(trafLedGreen, OUTPUT);
    pinMode(trafLedYellow, OUTPUT);
    pinMode(trafLedRed, OUTPUT);
    pinMode(pedLedRed, OUTPUT);
    pinMode(pedLedGreen, OUTPUT);

    // endstop pin allocations
    pinMode(esTopPin, INPUT_PULLUP);
    pinMode(esBottomPin, INPUT_PULLUP);

    // beam break pin allocations
    pinMode(beam1Pin, INPUT_PULLUP);
    pinMode(beam2Pin, INPUT_PULLUP);

    // ultrasonic pin allocations
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    // other pin allocations
    pinMode(onboardLedPin, OUTPUT); // onboard led allocation

    timeDelay = millis();
}

// handle commands from the UI
void handleIncoming()
{
    if (!client || !client.connected())
        return;

    while (client.available())
    {
        String line = client.readStringUntil('\n');
        line.trim();
        if (line.length() == 0)
            continue;

        // Try JSON first (e.g. {"cmd":"open"})
        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, line);

        String cmd;
        if (!err && doc.containsKey("cmd"))
        {
            cmd = String((const char *)doc["cmd"]);
        }
        else
        {
            // fallback: plain text line like: open / close / get_status
            cmd = line;
        }
        cmd.toLowerCase();
        Serial.println(cmd);
        // TESTS
        if (cmd == "open")
        {
            bridgeState = 2;   // open
            boomgateState = 1; // open
            trafState = 1;     // green
            sendStatus();      // push new state
                               // optional ack:
            Serial.println("Open pressed");
            StaticJsonDocument<96> ack;
            ack["type"] = "ack";
            ack["cmd"] = "open";
            serializeJson(ack, client);
            client.println();
        }
        else if (cmd == "close")
        {
            bridgeState = 4;   // closed
            boomgateState = 0; // closed
            trafState = 3;     // red
            sendStatus();
            Serial.println("close pressed");
            StaticJsonDocument<96> ack;
            ack["type"] = "ack";
            ack["cmd"] = "close";
            serializeJson(ack, client);
            client.println();
        }
        else if (cmd == "shutdown")
        {
            StaticJsonDocument<96> bye;
            bye["type"] = "bye";
            serializeJson(bye, client);
            client.println();
            Serial.println("shutdown pressed");
            client.stop();
        }
    }
}

// send esp32 status
void sendStatus()
{
    if (!client || !client.connected())
        return;

    StaticJsonDocument<256> doc;
    doc["type"] = "status";
    doc["seq"] = hbSeq++; // idx
    doc["uptime_ms"] = millis();
    doc["bridgeState"] = stateRead[bridgeState];
    doc["boomgateState"] = boomgateState == 1 ? "open" : "closed"; // if,else
    doc["trafficLight"] = lightRead[trafState];
    doc["beamCounter"] = beamCounter;
    doc["boatDetected"] = checkBoatUnder();

    serializeJson(doc, client);
    client.println(); // newline => NDJSON
}

// checking to see if its periodically connceted
void sendHeartbeat()
{
    if (!client || !client.connected())
        return;

    StaticJsonDocument<192> doc;
    doc["type"] = "heartbeat";
    doc["seq"] = hbSeq++;
    doc["uptime_ms"] = millis();
    doc["bridgeState"] = stateRead[bridgeState];

    serializeJson(doc, client);
    client.println(); // newline => NDJSON
}

void resetCycleTimer()
{
    currentMillis = millis();
    cycleMillis = currentMillis;
}

void mainMotorOff()
{
    digitalWrite(motorPinIn1, LOW);
    digitalWrite(motorPinIn2, LOW);
    analogWrite(motorPinSpeed, 0);
}

void mainMotorClose()
{
    while (esBottomState == 0)
    {                                    // while endstop not broken, move motor
        digitalWrite(motorPinIn1, HIGH); // forward motor movement
        digitalWrite(motorPinIn2, LOW);
        analogWrite(motorPinSpeed, motorSpeed);
        Serial.println("Motor moving forward");
        if (digitalRead(esBottomPin) == HIGH)
        {
            esBottomState = 1;
        }
    }
    digitalWrite(motorPinIn1, LOW);
    digitalWrite(motorPinIn2, LOW);
    analogWrite(motorPinSpeed, 0);
    esBottomState = 0;
    spanClosed = 1;
    resetCycleTimer();
}

void mainMotorOpen()
{
    while (esTopState == 0)
    {                                   // while endstop not broken, move motor
        digitalWrite(motorPinIn1, LOW); // backward motor movement
        digitalWrite(motorPinIn2, HIGH);
        analogWrite(motorPinSpeed, motorSpeed);
        Serial.println("Motor moving backward");
        if (digitalRead(esTopPin) == HIGH)
        {
            esTopState = 1;
        }
    }
    digitalWrite(motorPinIn1, LOW);
    digitalWrite(motorPinIn2, LOW);
    analogWrite(motorPinSpeed, 0);
    esTopState = 0;
    spanClosed = 0;
}

void moveServoMid(int pin, int speed)
{ // close boomgate
    int speedDelay = millis();
    int i = servoMinUs;

    while (i <= servoMidUs)
    {
        if (millis() - speedDelay > 10)
        {
            gateServo.writeMicroseconds(pin, i);
            speedDelay = millis();
            i = i + 1 * servoSpeed;
        }
    }
    gateServo.writeMicroseconds(pin, servoMidUs);
    boomgateState = 0;
}

void moveServoMin(int pin, int speed)
{ // open boomgate
    int speedDelay = millis();
    int i = servoMidUs;

    while (i >= servoMinUs)
    {
        if (millis() - speedDelay > 10)
        {
            gateServo.writeMicroseconds(pin, i);
            speedDelay = millis();
            i = i - 1 * servoSpeed;
        }
    }
    gateServo.writeMicroseconds(pin, servoMinUs);
    boomgateState = 1;
}

void setTrafficLights(int trafficState)
{
    if (trafficState == 0)
    { // all off
        digitalWrite(trafLedGreen, LOW);
        digitalWrite(trafLedYellow, LOW);
        digitalWrite(trafLedRed, LOW);
        digitalWrite(pedLedGreen, LOW);
        digitalWrite(pedLedRed, LOW);
    }
    else
    {
        if (trafficState == 1)
        { // traffic go
            digitalWrite(trafLedGreen, HIGH);
            digitalWrite(trafLedYellow, LOW);
            digitalWrite(trafLedRed, LOW);
            digitalWrite(pedLedGreen, HIGH);
            digitalWrite(pedLedRed, LOW);
        }
        else
        {
            if (trafficState == 2)
            { // traffic slow
                digitalWrite(trafLedGreen, LOW);
                digitalWrite(trafLedRed, LOW);
                digitalWrite(pedLedGreen, LOW);
                digitalWrite(trafLedYellow, HIGH);

                if (currentMillis - redFlash < 500)
                { // flashing control
                    digitalWrite(pedLedRed, HIGH);
                }
                else
                {
                    digitalWrite(pedLedRed, LOW);
                    if (currentMillis - redFlash > 1000)
                    {
                        redFlash = currentMillis;
                    }
                }
            }
            else
            {
                if (trafficState == 3)
                { // traffic stop
                    digitalWrite(trafLedGreen, LOW);
                    digitalWrite(trafLedYellow, LOW);
                    digitalWrite(trafLedRed, HIGH);
                    digitalWrite(pedLedGreen, LOW);
                    digitalWrite(pedLedRed, HIGH);
                }
            }
        }
    }
}

void beamBreakCheck()
{
    if (digitalRead(beam1Pin) == LOW)
    {
        beamCounter = true;
    }
    if (digitalRead(beam2Pin) == LOW)
    {
        beamCounter = false;
    }
}

bool checkBoatUnder()
{
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    float duration = pulseIn(echoPin, HIGH);
    float distance = (duration * .0343) / 2;
    if (distance < 23.75)
    { // if return distance is less than the expected 250mm -5%
        return true;
    }
    return false;
}

void loop()
{
    // anything needed every loop goes here
    currentMillis = millis();
    beamBreakCheck();
    setTrafficLights(trafState);

    // accept or reuse client
    if (!client || !client.connected())
    {
        WiFiClient newClient = server.available();
        if (newClient)
        {
            client = newClient;
            Serial.println("Client connected");
        }
    }
    // ui commands
    handleIncoming();
    //--------------------
    // Serial monitor output
    //--------------------
    if (currentMillis - debugMillis > 500)
    {
        debugMillis = currentMillis;
        for (int i = 0; i < 20; i++)
        {
            Serial.println(" ");
        }
        Serial.print("Bridge State: ");
        Serial.println(stateRead[bridgeState]);
        Serial.print("Bridge State: ");
        Serial.println(bridgeState);
        Serial.print("Traffic State: ");
        Serial.println(lightRead[trafState]);
    }

    if (bridgeState == 0)
    { // setup state
        //--------------------
        // Traffic lights
        // >Cycle traffic lights every 5s
        //--------------------
        if (currentMillis - cycleMillis > 2000)
        {
            resetCycleTimer();
            trafState++;
            if (trafState > 3)
            {
                trafState = 0;
                sendStatus(); // EXAMPLE
            }
        }

        //--------------------
        // Boomgates
        // >Set boomgates to UP position for alignment
        //--------------------
        gateServo.writeMicroseconds(servoPin, servoMinUs);
    }
    else
    {
        if (bridgeState == 1)
        { // opening state
            //--------------------
            // Traffic Lights
            // >Yellow traffic lights
            // >Flashing red pedestrian lights
            //--------------------
            trafState = 2;

            //--------------------
            // Traffic Lights
            // >Red traffic lights
            // >Red pedestrian lights
            //--------------------
            if (currentMillis - cycleMillis > 6000)
            {
                trafState = 3;
            }

            //--------------------
            // Boomgates
            // >Close boomgates
            //--------------------
            if (currentMillis - cycleMillis > 9000)
            {
                moveServoMid(servoPin, servoSpeed);
                //--------------------
                // Main Motor
                // >Open bridge
                // >Stop at top beam break
                //--------------------
                mainMotorOpen();
                resetCycleTimer();

                bridgeState = 2;
            }

            //--------------------
            // State
            // >Switch to open state
            //--------------------
        }
        else
        {
            if (bridgeState == 2)
            { // open state
                //--------------------
                // Traffic Lights
                // >Red traffic lights
                // >Red pedestrian lights
                //--------------------
                trafState = 3;

                //--------------------
                // Boomgates
                // >Boomgates in UP position
                //--------------------
                gateServo.writeMicroseconds(servoPin, servoMidUs);

                //--------------------
                // Sensors
                // >Wait until boat clears river
                //--------------------
                if (beamCounter == false)
                {
                    if (checkBoatUnder() == false)
                    {
                        resetCycleTimer();
                        bridgeState = 3;
                    }
                }
            }
            else
            {
                if (bridgeState == 3)
                { // closing state
                    //--------------------
                    // Main motor
                    // >Close bridge
                    // >Stop at bottom beam break
                    //--------------------
                    if (spanClosed == 0)
                    {
                        mainMotorClose();
                    }

                    //--------------------
                    // Boomgates
                    // >Open boomgates
                    //--------------------
                    if (currentMillis - cycleMillis > 3000 && boomgateState == 0)
                    {
                        moveServoMin(servoPin, servoSpeed);
                        resetCycleTimer();
                    }

                    //--------------------
                    // Traffic Lights
                    // >Green traffic lights
                    // >Green pedestrian lights
                    //--------------------
                    if (currentMillis - cycleMillis > 6000)
                    {
                        trafState = 1;
                    }

                    //--------------------
                    // State
                    // >Switch state to closed
                    //--------------------
                    if (currentMillis - cycleMillis > 7000)
                    {
                        resetCycleTimer();
                        bridgeState = 4;
                    }
                }
                else
                {
                    if (bridgeState == 4)
                    { // closed state
                        //--------------------
                        // Traffic Lights
                        // >Green traffic lights
                        // >Green pedestrian lights
                        //--------------------
                        trafState = 1;

                        //--------------------
                        // Boomgates
                        // >Boomgates in UP position
                        //--------------------
                        gateServo.writeMicroseconds(servoPin, servoMinUs);

                        //--------------------
                        // Sensors
                        // >Wait until oncoming boat detected
                        //--------------------
                        if (beamCounter == true)
                        {
                            resetCycleTimer();
                            bridgeState = 1;
                        }
                    }
                }
            }
        }
    }
    // periodic heartbeat
    if (currentMillis >= nextHeartbeatMs)
    {
        nextHeartbeatMs = currentMillis + hbIntervalMs;
        sendHeartbeat();
    }
}
