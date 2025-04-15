#include <Arduino.h>

#define MAX_QUEUE_SIZE 128
#define MAX_STACK_SIZE 128

const int buzzer = 25;
const int buzzer2 = 32;
const int buzzer3 = 33;

#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <vector>
#include <cstring>
#include <queue>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif

#include <PubSubClient.h>
#include <WebServer.h>

WebServer server(8000);  // Create web server on port 8000

// Forward declaration
const char* generateSilentNotes(const char* pattern);

// Helper function to generate silent notes matching the pattern of active notes
const char* generateSilentNotes(const char* pattern) {
  static char silentNotes[100]; // Static buffer for the generated string
  int j = 0;
  
  for (unsigned int i = 0; i < strlen(pattern); i += 2) {
    if (i + 1 < strlen(pattern)) {
      silentNotes[j++] = '#'; // Silent note
      silentNotes[j++] = pattern[i+1]; // Keep original duration
    }
  }
  silentNotes[j] = '\0';
  return silentNotes;
}

enum class BuzzerState {
  OFF,
  ON
};

enum Tones {
  REST = 0,  // Silent note/rest
  C0 = 16,
  C3 = 262,
  C3s = 277,
  D3 = 294,
  D3s = 311,
  E3 = 330,
  E3s = 349,
  F3 = 370,
  F3s = 392,
  G3 = 415,
  G3s = 440,
  A3_TONE = 466,
  A3s = 494,
  B3 = 523,
  C4 = 554,
  C4s = 587,
  D4 = 622,
  D4s = 659,
  E4 = 698,
  E4s = 740,
  F4 = 784,
  F4s = 830,
  G4 = 880,
  G4s = 932,
  A4_TONE = 987,
  A4s = 1047,
  B4 = 1175,
  C5 = 1255,
  C5s = 1320,
  D5 = 1397,
  D5s = 1479,
  E5 = 1568,
  E5s = 1661,
  F5 = 1760,
  F5s = 1865,
  G5 = 1976,
  G5s = 2093,
  A5_TONE = 2217,
  A5s = 2349,
  B5 = 2489
};

enum durations {
  ZERO = 100,
  QUARTER = 250,
  HALF = 500,
  WHOLE = 1000,
  WHOLE_DOTTED = 1250,
  WHOLE_HALF = 1500,
  WHOLE_HALF_DOTTED = 1750,
  TWO_WHOLE = 2000
};

Tones getToneByLetter(char letter) {
  switch (letter) {
    case 'A': return C3;
    case 'B': return C3s;
    case 'C': return D3;
    case 'D': return D3s;
    case 'E': return E3;
    case 'F': return E3s;
    case 'G': return F3;
    case 'H': return F3s;
    case 'I': return G3;
    case 'J': return G3s;
    case 'K': return A3_TONE;
    case 'L': return A3s;
    case 'M': return B3;

    case 'N': return C4;
    case 'O': return C4s;
    case 'P': return D4;
    case 'Q': return D4s;
    case 'R': return E4;
    case 'S': return E4s;
    case 'T': return F4;
    case 'U': return F4s;
    case 'V': return G4;
    case 'W': return G4s;
    case 'X': return A4_TONE;
    case 'Y': return A4s;
    case 'Z': return B4;
    case '#': return REST;  // Space character represents silent note

    default: return C0; // Default to C0 for unrecognized letters
  }
}

durations getDurationByNumber(char number) {
  switch (number) {
    case '0': return ZERO;
    case '1': return QUARTER;
    case '2': return HALF;
    case '3': return WHOLE;
    case '4': return WHOLE_DOTTED;
    case '5': return WHOLE_HALF;
    case '6': return WHOLE_HALF_DOTTED;
    case '7': return TWO_WHOLE;
    default: return ZERO;
  }
}

const char* ssid = "Never gonna give you up";
const char* password = "n€verG0nnAl€tyoUd0wn";

// MQTT broker details
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;

const char* clientId = "VieGehtEsDirMeinFreund"; // Must be unique on the broker

// Topics
const char* publishTopic = "/gilbert/alive";
const char* subscribeTopic = "/gilbert/#";

WiFiClient espClient;
PubSubClient client(espClient);

int difficulty = 1;  // Global variable to store difficulty

void handleSetDifficulty() {
  if (server.hasArg("difficulty")) {
    difficulty = server.arg("difficulty").toInt();
    server.send(200, "text/plain", "Difficulty set to " + String(difficulty));
  } else {
    server.send(400, "text/plain", "Missing difficulty parameter");
  }
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Modifier la difficulte du jeu Simon</h1>";
  html += "<form action=\"/setDifficulty\" method=\"POST\">";
  html += "<label for=\"difficulty\">Difficulte : </label>";
  html += "<input type=\"number\" id=\"difficulty\" name=\"difficulty\" min=\"1\" max=\"10\" value=\"";
  html += String(difficulty);
  html += "\">";
  html += "<input type=\"submit\" value=\"Changer la difficulte\">";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Define a structure to hold tone and duration
struct ToneData {
  int frequency;
  durations duration;
};

// Circular stack structure
struct CircularStack {
  ToneData stack[MAX_STACK_SIZE];
  int head = 0; // Points to the next position to write
  int tail = 0; // Points to the next position to read
  int size = 0; // Tracks the number of elements in the stack

  // Push a new tone onto the stack
  void push(ToneData tone) {
    if (size < MAX_STACK_SIZE) {
      stack[head] = tone;
      head = (head + 1) % MAX_STACK_SIZE;
      size++;
    } else {
      Serial.println("Stack full, overwriting oldest tone.");
      stack[head] = tone;
      head = (head + 1) % MAX_STACK_SIZE;
      tail = (tail + 1) % MAX_STACK_SIZE; // Move tail forward to maintain circularity
    }
  }

  // Pop a tone from the stack
  ToneData pop() {
    if (size > 0) {
      ToneData tone = stack[tail];
      tail = (tail + 1) % MAX_STACK_SIZE;
      size--;
      return tone;
    } else {
      Serial.println("Stack empty, returning default tone.");
      return {C0, ZERO}; // Return a default tone if the stack is empty
    }
  }

  // Check if the stack is empty
  bool empty() {
    return size == 0;
  }
};

// Circular stacks for each buzzer
CircularStack buzzerStacks[3];

// Function to connect to Wi-Fi
#include <HTTPClient.h>

void connectToWiFi() {
    Serial.print("Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // After connecting to Wi-Fi, attempt to pair device with server
    pairDeviceWithServer();
}

// Function to get device MAC address as device ID
String getDeviceID() {
    return WiFi.macAddress();
}

// Function to pair device with Flask server
void pairDeviceWithServer() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        // Replace "YOUR_FLASK_SERVER_IP" with the actual IP address or domain of your Flask server
        String serverUrl = "http://YOUR_FLASK_SERVER_IP:5000/pair_device";
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");

        // Prepare JSON payload with device_id and user credentials
        // For simplicity, using username and password here; in production use token-based auth
        String username = "your_username"; // Replace with actual username or get dynamically
        String password = "your_password"; // Replace with actual password or get dynamically
        String deviceID = getDeviceID();

        String payload = "{\"device_id\":\"" + deviceID + "\"}";

        // Add basic auth header (optional, depending on server auth)
        // String authHeader = "Basic " + base64::encode(username + ":" + password);
        // http.addHeader("Authorization", authHeader);

        int httpResponseCode = http.POST(payload);

        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.print("Pairing response: ");
            Serial.println(response);
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    } else {
        Serial.println("WiFi not connected, cannot pair device");
    }
}

// Function to connect to MQTT broker
void connectToMQTT() {
  client.setServer(mqttServer, mqttPort);
  Serial.print("Connecting to MQTT broker");
  while (!client.connected()) {
    if (client.connect(clientId)) {
      Serial.println("\nConnected to MQTT broker");
    } else {
      Serial.print(".");
    }
  }
}

// Function to process the circular stack for a specific buzzer
void processBuzzerStack(int channel) {
  static unsigned long lastToneStart[3] = {0, 0, 0}; // When current tone started
  static ToneData currentTone[3]; // Current tone being played
  static bool isPlaying[3] = {false, false, false}; // Track if tone is playing
  static unsigned long lastEmptyMessageTime[3] = {0, 0, 0};

    if (isPlaying[channel]) {
      // Check if current tone duration has elapsed
      if (millis() - lastToneStart[channel] >= currentTone[channel].duration) {
        ledcWriteTone(channel, 0); // Stop tone
        isPlaying[channel] = false;
        Serial.println("Tone finished");
      } else if (currentTone[channel].frequency == REST) {
        // For silent notes, just wait the duration without playing
        if (millis() - lastToneStart[channel] >= currentTone[channel].duration) {
          isPlaying[channel] = false;
          Serial.println("Rest finished");
        }
      }
  } else if (!buzzerStacks[channel].empty()) {
    // Get new tone to play
    currentTone[channel] = buzzerStacks[channel].pop();
    Serial.print("Processing buzzer ");
    Serial.print(channel);
    Serial.print(": Frequency = ");
    Serial.print(currentTone[channel].frequency);
    Serial.print(", Duration = ");
    Serial.println(currentTone[channel].duration);

    ledcWriteTone(channel, currentTone[channel].frequency);
    ledcWrite(channel, 160); // Set volume
    lastToneStart[channel] = millis();
    isPlaying[channel] = true;
    Serial.println("Playing tone...");
  } else {
    ledcWriteTone(channel, 0);

    // Check if 10 seconds have passed since the last empty message
    if (millis() - lastEmptyMessageTime[channel] >= 10000) {
        Serial.print("Buzzer ");
        Serial.print(channel);
        Serial.println(" stack is empty.");
        lastEmptyMessageTime[channel] = millis(); // Update the timestamp
    }
  }
}

// Updated MQTT callback function
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  if (length == 0) {
    Serial.println("Empty message received. Ignoring.");
    return;
  }

  // Check for "Device is alive" message and ignore it
  if (strncmp((const char*)payload, "Device is alive", length) == 0) {
    Serial.println("Heartbeat message received. Ignoring.");
    return;
  }

  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0'; // Null-terminate the message
  Serial.println(message);

  // Split the message by underscores
  char* tokens[3] = {nullptr, nullptr, nullptr};
  int numTokens = 0;
  
  // First count how many parts we have
  char* token = strtok(message, "_");
  while (token != nullptr && numTokens < 3) {
    tokens[numTokens++] = token;
    token = strtok(nullptr, "_");
  }

  // Process each token
  for (int buzzerIndex = 0; buzzerIndex < 3; buzzerIndex++) {
    Serial.print("Processing token for buzzer ");
    Serial.println(buzzerIndex);

    // If we have fewer tokens than buzzers, generate silent notes for remaining buzzers
    const char* toProcess = (buzzerIndex < numTokens) ? 
                            tokens[buzzerIndex] : 
                            generateSilentNotes(tokens[0]); // Generate silent notes matching first part

    // Process the token in chunks of two characters
    for (unsigned int i = 0; i < strlen(toProcess); i += 2) {
        if (i + 1 < strlen(toProcess)) { // Ensure there are at least two characters left
          char letter = toProcess[i];
          char number = toProcess[i + 1];

        int frequency = getToneByLetter(toupper(letter));
        durations duration = getDurationByNumber(number);

        if (frequency != C0) { // Validate the token
          Serial.print("Adding to buzzer ");
          Serial.print(buzzerIndex);
          Serial.print(": Frequency = ");
          Serial.print(frequency);
          Serial.print(", Duration = ");
          Serial.println(duration);

          // Push the tone onto the circular stack
          buzzerStacks[buzzerIndex].push({frequency, duration});
        } else {
          Serial.print("Invalid token: ");
          Serial.print(letter);
          Serial.print(number);
          Serial.println(". Skipping.");
        }
      } else {
        Serial.print("Incomplete token at the end of the message: ");
        Serial.println(token[i]);
      }
    }

    // Move to the next buzzer
    buzzerIndex++;
    token = strtok(nullptr, "_");
  }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(buzzer, OUTPUT);
    pinMode(buzzer2, OUTPUT);
    pinMode(buzzer3, OUTPUT);

    Serial.println("Configuring PWM channels...");
    ledcSetup(0, 5000, 8); // Channel 0, 5 kHz, 8-bit resolution
    ledcAttachPin(buzzer, 0);

    ledcSetup(1, 5000, 8); // Channel 1, 5 kHz, 8-bit resolution
    ledcAttachPin(buzzer2, 1);

    ledcSetup(2, 5000, 8); // Channel 2, 5 kHz, 8-bit resolution
    ledcAttachPin(buzzer3, 2);
    Serial.println("PWM channels configured.");

    unsigned long startTime = millis();
    Serial.println("Testing buzzer...");
    ledcWriteTone(0, 440); // Play A4 (440 Hz)
    while (millis() - startTime < 1000) {
        // Non-blocking wait
    }
    ledcWriteTone(0, 0);   // Stop tone
    Serial.println("Buzzer test complete.");

    connectToWiFi();
    connectToMQTT();
    client.publish("test/topic", "Hello, MQTT!");

    server.on("/", HTTP_GET, handleRoot);
    server.on("/setDifficulty", HTTP_POST, handleSetDifficulty);
  
    server.begin();
    Serial.println("Serveur démarré");
    Serial.println(WiFi.localIP());

    client.setCallback(mqttCallback);
    if (client.subscribe(subscribeTopic)) {
        Serial.println("Subscribed to topic: " + String(subscribeTopic));
    } else {
        Serial.println("Failed to subscribe to topic: " + String(subscribeTopic));
    }
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  // Process one buzzer's stack per iteration
  static int currentBuzzer = 0; // Track which buzzer to process
  processBuzzerStack(currentBuzzer);
  currentBuzzer = (currentBuzzer + 1) % 3; // Move to the next buzzer

  // Send heartbeat message every 30 seconds
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 30000) {
    client.publish(publishTopic, "Device is alive");
    lastHeartbeat = millis();
  }
}
