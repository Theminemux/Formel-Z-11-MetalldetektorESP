#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>

// WLAN
const char* ssid = "...";
const char* password = "deineMudda";

// Sensor
const int sensorPin = 14;

// 28BYJ-48 via ULN2003: IN1, IN2, IN3, IN4
const int stepperPins[] = {25, 26, 27, 32};
const int stepSequence[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};
int stepIndex = 0;
unsigned long lastStepMicros = 0;
// Schneller Betrieb für den 28BYJ-48.
// Startet etwas langsamer und beschleunigt, damit der Motor nicht springt.
unsigned long stepIntervalMicros = 1450;
const unsigned long minimumStepIntervalMicros = 700;

// Webserver
WebServer server(80);

// Zähler
int counter = 0;
int lastState = HIGH;

// Ziel-URL
const char* targetUrl = "http://example.com/trigger";

// ===== API =====
void handleReset() {
  counter = 0;
  server.send(200, "text/plain", "Counter reset!");
  Serial.println("🔄 Counter wurde zurückgesetzt");
}

// ===== Request senden =====
void sendRequest() {
  HTTPClient http;
  http.begin(targetUrl);

  int code = http.GET();

  Serial.print("📡 Request gesendet, Code: ");
  Serial.println(code);

  http.end();
}

void stepMotor() {
  unsigned long now = micros();
  if (now - lastStepMicros < stepIntervalMicros) {
    return;
  }

  lastStepMicros = now;
  stepIndex = (stepIndex + 1) % 8;
  for (int pin = 0; pin < 4; pin++) {
    digitalWrite(stepperPins[pin], stepSequence[stepIndex][pin]);
  }

  if (stepIntervalMicros > minimumStepIntervalMicros) {
    stepIntervalMicros -= 1;
  }
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT_PULLUP);

  for (int pin = 0; pin < 4; pin++) {
    pinMode(stepperPins[pin], OUTPUT);
    digitalWrite(stepperPins[pin], LOW);
  }

  // WLAN
  WiFi.begin(ssid, password);

  // API
  server.on("/reset", handleReset);
  server.begin();

  Serial.println("🚀 Server gestartet (/reset), Motor dreht");
}

// ===== Loop =====
void loop() {
  stepMotor();
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) {
    static bool wifiReported = false;
    if (!wifiReported) {
      wifiReported = true;
      Serial.println("✅ WLAN verbunden!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());

      if (MDNS.begin("metalldetektor")) {
        Serial.println("🌐 erreichbar unter: http://metalldetektor.local");
      }
    }
  }

  int state = digitalRead(sensorPin);

  // Flankenerkennung
  if (state == LOW && lastState == HIGH) {
    counter++;

    Serial.print("🔥 erkannt! Count = ");
    Serial.println(counter);

    if (counter >= 25) {
      Serial.println("🚀 25 erreicht! Sende Request...");
      sendRequest();
      counter = 0;
    }
  }

  lastState = state;
}
