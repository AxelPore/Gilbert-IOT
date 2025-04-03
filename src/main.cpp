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

// Function to play a tone based on the letter
void playToneForLetter(char letter) {
  int frequency1 = 0; // Frequency for the first note
  int frequency2 = 0; // Frequency for the second note
  int duration = 500; // Default duration for all notes
  int volume = 128;   // Default volume for all notes

  // Map each letter to two frequencies (including sharp notes)
  switch (letter) {
    // Octave 3
    case 'A': frequency1 = 262; frequency2 = 277; break; // C3 and C#3
    case 'B': frequency1 = 294; frequency2 = 311; break; // D3 and D#3
    case 'C': frequency1 = 330; frequency2 = 349; break; // E3 and F3
    case 'D': frequency1 = 370; frequency2 = 392; break; // F#3 and G3
    case 'E': frequency1 = 415; frequency2 = 440; break; // G#3 and A3
    case 'F': frequency1 = 466; frequency2 = 494; break; // A#3 and B3
    case 'G': frequency1 = 523; frequency2 = 554; break; // C4 and C#4

    // Octave 4
    case 'H': frequency1 = 587; frequency2 = 622; break; // D4 and D#4
    case 'I': frequency1 = 659; frequency2 = 698; break; // E4 and F4
    case 'J': frequency1 = 740; frequency2 = 784; break; // F#4 and G4
    case 'K': frequency1 = 831; frequency2 = 880; break; // G#4 and A4
    case 'L': frequency1 = 932; frequency2 = 988; break; // A#4 and B4
    case 'M': frequency1 = 1047; frequency2 = 1109; break; // C5 and C#5
    case 'N': frequency1 = 1175; frequency2 = 1245; break; // D5 and D#5

    // Octave 5
    case 'O': frequency1 = 1319; frequency2 = 1397; break; // E5 and F5
    case 'P': frequency1 = 1480; frequency2 = 1568; break; // F#5 and G5
    case 'Q': frequency1 = 1661; frequency2 = 1760; break; // G#5 and A5
    case 'R': frequency1 = 1865; frequency2 = 1976; break; // A#5 and B5
    case 'S': frequency1 = 2093; frequency2 = 2217; break; // C6 and C#6
    case 'T': frequency1 = 2349; frequency2 = 2489; break; // D6 and D#6
    case 'U': frequency1 = 2637; frequency2 = 2794; break; // E6 and F6

    // Octave 6
    case 'V': frequency1 = 2960; frequency2 = 3136; break; // F#6 and G6
    case 'W': frequency1 = 3322; frequency2 = 3520; break; // G#6 and A6
    case 'X': frequency1 = 3729; frequency2 = 3951; break; // A#6 and B6
    case 'Y': frequency1 = 4186; frequency2 = 4435; break; // C7 and C#7
    case 'Z': frequency1 = 4699; frequency2 = 4978; break; // D7 and D#7

    // Silence
    case '-': 
      delay(duration); // Silence for the duration
      return;

    default: return; // Ignore unsupported characters
  }

  // Play the two tones on separate channels
  ledcWriteTone(0, frequency1); // Set frequency for channel 0
  ledcWrite(0, volume);         // Set volume for channel 0
  ledcWriteTone(1, frequency2); // Set frequency for channel 1
  ledcWrite(1, volume);         // Set volume for channel 1

  delay(duration);              // Wait for the duration

  // Stop the tones
  ledcWriteTone(0, 0);          // Stop tone on channel 0
  ledcWriteTone(1, 0);          // Stop tone on channel 1
}

// Callback function
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  // If the payload is empty, do nothing
  if (length == 0) {
    Serial.println("Empty message received. Ignoring.");
    return;
  }

  // Convert payload to a null-terminated string
  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  // Check if the message starts with '0'
  if (message[0] != '0') {
    Serial.println("Message does not start with '0'. Ignoring.");
    return;
  }

  // Handle commands (process the rest of the message after '0')
  for (unsigned int i = 1; i < strlen(message); i++) { // Start from index 1
    if (isalpha(message[i]) || message[i] == '-') {
      playToneForLetter(toupper(message[i])); // Play tone for each letter
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Give some time for serial to initialize

  // Set up buzzer pins
  pinMode(buzzer, OUTPUT);
  pinMode(buzzer2, OUTPUT);
  pinMode(buzzer3, OUTPUT);

  // Configure PWM channels for ESP32
  ledcSetup(0, 5000, 8); // Channel 0, 5 kHz, 8-bit resolution
  ledcAttachPin(buzzer, 0);

  ledcSetup(1, 5000, 8); // Channel 1, 5 kHz, 8-bit resolution
  ledcAttachPin(buzzer2, 1);

  ledcSetup(2, 5000, 8); // Channel 2, 5 kHz, 8-bit resolution
  ledcAttachPin(buzzer3, 2);

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
