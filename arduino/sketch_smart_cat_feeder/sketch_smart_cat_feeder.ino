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

int motor1pin1 = 32;
int motor1pin2 = 33;
int motor2pin1 = 16;
int motor2pin2 = 17;

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");
  Serial.println(AWS_IOT_ENDPOINT);


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.println("Connecting to AWS IOT");
  Serial.println(THINGNAME);

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  Serial.println("About to subscribe");
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
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
  Serial.println("incoming: " + topic + " - " + payload);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* event = doc["event"];

  Serial.println(event);

  feedMe(event);  
}

void setup() {
  Serial.begin(9600);
  connectAWS();

  pinMode(motor1pin1, OUTPUT);
  pinMode(motor1pin2, OUTPUT);
  pinMode(motor2pin1, OUTPUT);
  pinMode(motor2pin2, OUTPUT);
}

void feedMe(String event) {
  Serial.println(event);
  
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
    Serial.println("run left");
    digitalWrite(motor1pin1, HIGH);
    digitalWrite(motor1pin2, LOW);
  }

  if (feedRight) {
    Serial.println("run right");
    digitalWrite(motor2pin1, HIGH);
    digitalWrite(motor2pin2, LOW);
  }

  delay(10000);
  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor1pin2, LOW);
  digitalWrite(motor2pin1, LOW);
  digitalWrite(motor2pin2, LOW);
  delay(2000);

  Serial.println("fed");
}

void loop() {
  publishMessage();
  client.loop();
  delay(3000);
}
