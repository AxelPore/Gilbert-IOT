#include <Arduino.h>
#include <vector>

#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <vector>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif

#include <PubSubClient.h>

// Wi-Fi credentials
const char* ssid = "Never gonna give you up";
const char* password = "ctji4838";

// MQTT broker details
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;
const char* mqttUser = "Heinrich"; // Optional, if your broker requires authentication
const char* mqttPassword = ""; // Optional
const char* clientId = "VieGehtEsDirMeinFreund"; // Must be unique on the broker

// Topics
const char* publishTopic = "/gilbert/IoT";
const char* subscribeTopic = "/gilbert";

WiFiClient espClient;
PubSubClient client(espClient);

// Function to connect to Wi-Fi
void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Function to connect to MQTT broker
void connectToMQTT() {
  Serial.print("Connecting to MQTT broker");
  while (!client.connected()) {
    if (client.connect(clientId, mqttUser, mqttPassword)) {
      Serial.println("\nConnected to MQTT broker");
    } else {
      Serial.print(".");
      delay(1000);
    }
  }
}

// Callback function
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [" );
  Serial.print(topic);
  Serial.print("]: ");

  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);
}


void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  connectToWiFi();

  // Set MQTT server and callback
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);

  // Connect to MQTT broker
  connectToMQTT();

  // Subscribe to topic
  client.subscribe(subscribeTopic);

  // Publish a test message
  
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  const char* msg = "Hello, MQTT!";

  client.publish(publishTopic, msg);
  delay(1000);
  client.loop();
}
