#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <Adafruit_TMP117.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

// I2C pins for the TMP117 sensor (these can be changed to match your board configuration)
#define SDA_PIN 21  // GPIO 21 for SDA
#define SCL_PIN 22  // GPIO 22 for SCL

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);
Adafruit_TMP117 tmp117;

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

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

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void publishMessage()
{
  sensors_event_t temp_event;
  tmp117.getEvent(&temp_event);  // Get temperature data from TMP117 sensor
  float temperatureC = temp_event.temperature;  // Temperature in Celsius

  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["temperature"] = temperatureC;  // Use the temperature value read from the sensor
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.println("Message published: ");
  serializeJsonPretty(doc, Serial); // Print the message to Serial Monitor for debugging
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
}

void setup() {
   Serial.begin(115200);
  Serial.println("Starting setup...");
  // Initialize I2C communication (Set custom SDA and SCL pins)
  Wire.begin(21, 20);

  // Initialize TMP117 sensor
  if (!tmp117.begin(0x48)) {  // Use the I2C address for the TMP117 sensor
    Serial.println("Couldn't find TMP117 sensor!");
    while (1);
  }
  Serial.println("TMP117 sensor initialized.");
  
  connectAWS();
}

void loop() {
  publishMessage();
  client.loop();
  delay(1000);
}
