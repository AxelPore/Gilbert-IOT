#include <Arduino.h>

const int buzzer = 25;
const int buzzer2 = 32;
const int buzzer3 = 33;


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
const char* password = "n€verG0nnAl€tyoUd0wn";

// MQTT broker details
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;
const char* mqttUser = "Heinrich"; // Optional, if your broker requires authentication
const char* mqttPassword = ""; // Optional
const char* clientId = "VieGehtEsDirMeinFreund"; // Must be unique on the broker

// Topics
const char* publishTopic = "/gilbert/IoT";
const char* subscribeTopic = "/gilbert/#";

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
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  // Handle commands
  if (strcmp(message, "play") == 0) {
    client.publish(publishTopic, "Playing melody...");
  } else if (strcmp(message, "status") == 0) {
    char statusMsg[50];
    snprintf(statusMsg, sizeof(statusMsg), "Device is running. IP: %s", WiFi.localIP().toString().c_str());
    client.publish(publishTopic, statusMsg);
  } else if (strcmp(message, "A") == 0) {
    tone(buzzer, 1000, 500); // Play 1kHz tone for 500ms on buzzer
    client.publish(publishTopic, "Tone A played");
  } else if (strcmp(message, "B") == 0) {
    tone(buzzer2, 1500, 500); // Play 1.5kHz tone for 500ms on buzzer2
    client.publish(publishTopic, "Tone B played");
  } else if (strcmp(message, "C") == 0) {
    tone(buzzer3, 2000, 500); // Play 2kHz tone for 500ms on buzzer3
    client.publish(publishTopic, "Tone C played");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Give some time for serial to initialize

  // Set up buzzer pins
  pinMode(buzzer, OUTPUT);
  pinMode(buzzer2, OUTPUT);
  pinMode(buzzer3, OUTPUT);
  digitalWrite(buzzer, LOW);
  digitalWrite(buzzer2, LOW);
  digitalWrite(buzzer3, LOW);

  
  // Connect to Wi-Fi
  connectToWiFi();

  // Set MQTT server and callback
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);

  // Connect to MQTT broker
  connectToMQTT();

  // Subscribe to topic
  client.subscribe(subscribeTopic);

  // Send initial status message
  char startupMsg[50];
  snprintf(startupMsg, sizeof(startupMsg), "Device started. IP: %s", WiFi.localIP().toString().c_str());
  client.publish(publishTopic, startupMsg);
  
}

void loop() {

  client.loop();
  delay(10); // Small delay to prevent watchdog issues

  if (!client.connected()) {
    connectToMQTT();
  }
  
  // Send heartbeat message every 30 seconds
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 30000) {
    client.publish(publishTopic, "Device is alive");
    lastHeartbeat = millis();
  }
  
  // Check for incoming messages
  if (Serial.available() > 0) {
    String message = Serial.readStringUntil('\n');
    message.trim();
    client.publish(publishTopic, message.c_str());
  }
  
  delay(10); // Small delay to prevent bouncing
}
