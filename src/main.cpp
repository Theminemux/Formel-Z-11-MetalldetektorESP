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

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT_PULLUP);

  // WLAN
  WiFi.begin(ssid, password);
  Serial.print("Verbinde mit WLAN");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WLAN verbunden!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // mDNS
  if (MDNS.begin("metalldetektor")) {
    Serial.println("🌐 erreichbar unter: http://metalldetektor.local");
  } else {
    Serial.println("❌ mDNS Fehler");
  }

  // API
  server.on("/reset", handleReset);
  server.begin();

  Serial.println("🚀 Server gestartet (/reset)");
}

// ===== Loop =====
void loop() {
  server.handleClient();

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