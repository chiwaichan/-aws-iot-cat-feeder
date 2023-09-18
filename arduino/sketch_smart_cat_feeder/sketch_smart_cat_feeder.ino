#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "cat-feeder/states"
#define AWS_IOT_SUBSCRIBE_TOPIC "cat-feeder/action"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

int motor1pin1 = D3;
int motor1pin2 = D4;
int motor2pin1 = D5;
int motor2pin2 = D6;
int ledGreen = D7;
int ledRed = D8;
int ledBlue = D9;

// Global variable to store MQTT message
String receivedMessage;

void blinkLed(int led, int number_of_blinks, int blink_delay = 200) {
  for (int i = 0; i < number_of_blinks; i++) {
    digitalWrite(led, HIGH);
    delay(blink_delay);
    digitalWrite(led, LOW);
    delay(blink_delay);
  }
}

void confirmWifi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Wifi Connect Attempt => SSID: \"");
    Serial.print(WIFI_SSID);
    Serial.print("\" Passowrd: \"");
    // Serial.print(WIFI_PASSWORD);
    Serial.print("*****");
    Serial.print("\"\n");

    blinkLed(ledGreen, 3);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    delay(2000);
  }

  digitalWrite(ledGreen, HIGH);
}

void connectAWS()
{
  Serial.print("Connecting to AWS IOT using \"");
  Serial.print(AWS_IOT_ENDPOINT);
  Serial.print("\" endpoint as \"");
  Serial.print(THINGNAME);
  Serial.print("\"\n");

  while (!client.connect(THINGNAME)) {
    confirmWifi();

    Serial.print("AWS IoT Connect Attempt\n");
    if (client.lastError() != 0 ) {
      Serial.print("Last Error Code: ");
      Serial.print(client.lastError());
      Serial.print("\n");
    }  

    blinkLed(ledGreen, 5);
  }

  // if (!client.connected()) {
  //   Serial.println("AWS IoT Timeout!");
  //   digitalWrite(ledGreen, LOW);
  //   digitalWrite(ledRed, HIGH);
  //   return;
  // }

  Serial.print("About to subscribe to \"");
  Serial.print(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.print("\"\n");

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Fully Connected!");
  digitalWrite(ledGreen, HIGH);
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["state_1"] = millis();
  doc["state_2"] = 2 * millis();
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);

  Serial.println("publishMessage states to AWS IoT" );
}

void messageHandler(String &topic, String &payload) {
  Serial.println("Incoming Message - topic: \"" + topic + "\" payload: \"" + payload + "\"");

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* event = doc["event"];

  receivedMessage = event;  
}

void setup() {
  Serial.print("Starting Setup!!!");

  Serial.begin(9600);
  pinMode(motor1pin1, OUTPUT);
  pinMode(motor1pin2, OUTPUT);
  pinMode(motor2pin1, OUTPUT);
  pinMode(motor2pin2, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledBlue, OUTPUT);
  pinMode(ledRed, OUTPUT);

  // blinkLed(ledRed);
  // blinkLed(ledGreen);

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);
  
  connectAWS();

  Serial.print("Finished Setup!!!");
}

void feedMe(String event) {
  Serial.println("feedMe Event: " + event);
  
  bool feedLeft = false;
  bool feedRight = false;

  if (event == "SINGLE") {
    feedLeft = true;
  }
  if (event == "DOUBLE") {
    feedRight = true;
  }
  if (event == "LONG") {
    feedLeft = true;
    feedRight = true;
  }

  if (feedLeft) {
    Serial.println("Feeding Left Side");
    digitalWrite(motor2pin1, HIGH);
    digitalWrite(motor2pin2, LOW);
    digitalWrite(ledBlue, HIGH);
  }

  if (feedRight) {
    Serial.println("Feeding Right Side");
    digitalWrite(motor1pin1, HIGH);
    digitalWrite(motor1pin2, LOW);
    digitalWrite(ledRed, HIGH);
  }

  delay(10000);
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, LOW);
  digitalWrite(motor2pin1, LOW);
  digitalWrite(motor2pin2, LOW);
  digitalWrite(ledBlue, LOW);
  digitalWrite(ledRed, LOW);
  delay(2000);

  Serial.println("Feed Complete");
}

void loop() {
  client.loop();
  delay(10); // <- fixes some issues with WiFi stability
  
  if (!client.connected()) {
    digitalWrite(ledGreen, LOW);
    connectAWS();
  }
  
  if (!receivedMessage.isEmpty()) {
    Serial.print("Received message: ");
    Serial.println(receivedMessage);

    feedMe(receivedMessage);

    receivedMessage = "";
  }
}
