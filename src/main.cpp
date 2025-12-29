#include <Arduino.h>
#include <WiFi.h>
#include "BluetoothSerial.h"

//////////////////////
// Wi-Fi settings
//////////////////////
const char* ssid     = "OnePlus 12";
const char* password = "a7uccbha";

//////////////////////
// Bluetooth
//////////////////////
BluetoothSerial SerialBT;

String pcBuffer = "";
String phoneBuffer = "";
unsigned long lastStatusMsg = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // --- Wi-Fi ---
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Status: ");
    Serial.println(WiFi.status());
  }

  Serial.println("\nWi-Fi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // --- Bluetooth Serial ---
  SerialBT.begin("ESP32-WIFI-BT");
  Serial.println("Bluetooth started, pair with 'ESP32-WIFI-BT'");
  Serial.println("Waiting for Bluetooth connection...");
}

void loop() {
  // Send status message every 5 seconds if connected
  if (SerialBT.hasClient() && (millis() - lastStatusMsg > 5000)) {
    SerialBT.println("ESP32 READY - Navigation display active!");
    Serial.println("Status sent to phone");
    lastStatusMsg = millis();
  }

  // ----- From PC Serial -> Phone (send on Enter) -----
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (pcBuffer.length() > 0) {
        SerialBT.println(pcBuffer);
        Serial.println("-> " + pcBuffer);
        pcBuffer = "";
      }
    } else {
      pcBuffer += c;
    }
  }

  // ----- From Phone -> PC Serial (send on '\n') -----
  while (SerialBT.available()) {
    char c = SerialBT.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (phoneBuffer.length() > 0) {
        Serial.println("PHONE: " + phoneBuffer);
        phoneBuffer = "";
      }
    } else {
      phoneBuffer += c;
    }
  }
}