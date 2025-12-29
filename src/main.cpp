#include <Arduino.h>
#include <WiFi.h>
#include <BluetoothSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include "credentials.h"

//////////////////////
// Wi-Fi settings (from credentials.h)
//////////////////////
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

//////////////////////
// Time settings
//////////////////////
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800; // GMT+5:30 for India (5.5 hours * 3600)
const int daylightOffset_sec = 0; // No daylight saving in India

//////////////////////
// Bluetooth
//////////////////////
BluetoothSerial SerialBT;

//////////////////////
// OLED Display
//////////////////////
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String pcBuffer = "";
String phoneBuffer = "";
String lastPhoneMsg = "";
unsigned long lastStatusMsg = 0;
unsigned long lastDisplayUpdate = 0;
bool btConnected = false;
int scrollOffset = 0;
unsigned long scrollStart = 0;
bool scrolling = false;

void updateDisplay()
{
  display.clearDisplay();

  // Time display (top line)
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%I:%M:%S %p", &timeinfo);
    display.print(timeStr);
  }
  else
  {
    display.print("--:--:--");
  }

  // WiFi status (same line, right side)
  display.print(" ");
  if (WiFi.status() == WL_CONNECTED)
  {
    display.println("WiFi:OK");
  }
  else
  {
    display.println("WiFi:X");
  }

  // Date display (second line)
  display.setCursor(0, 10);
  struct tm timeinfo2;
  if (getLocalTime(&timeinfo2))
  {
    char dateStr[25];
    strftime(dateStr, sizeof(dateStr), "%a, %b %d %Y", &timeinfo2);
    display.print(dateStr);
  }
  else
  {
    display.print("Date: N/A");
  }

  // Bluetooth status
  display.setCursor(0, 20);
  display.print("BT: ");
  if (SerialBT.hasClient())
  {
    display.println("CONNECTED");
    btConnected = true;
  }
  else
  {
    display.println("waiting...");
    btConnected = false;
  }

  // Phone message display
  display.setCursor(0, 30);
  if (lastPhoneMsg.length() > 0 && btConnected)
  {
    display.print("NEW MSG: ");

    String msgToShow = lastPhoneMsg;
    int msgLen = msgToShow.length();

    if (msgLen > 18 && scrolling)
    {
      int displayLen = 18;
      String visible = msgToShow.substring(scrollOffset, scrollOffset + displayLen);
      if (scrollOffset + displayLen > msgLen)
      {
        visible += " " + msgToShow.substring(0, scrollOffset + displayLen - msgLen);
      }
      display.println(visible);

      if (millis() - scrollStart > 8000)
      {
        scrollOffset = 0;
        scrolling = false;
      }
    }
    else
    {
      if (msgLen > 18)
      {
        display.println(msgToShow.substring(0, 18) + "...");
      }
      else
      {
        display.println(msgToShow);
      }
    }
  }
  else
  {
    display.println("No messages yet");
  }

  // PC buffer
  display.setCursor(0, 48);
  display.print("PC> ");
  if (pcBuffer.length() > 0)
  {
    display.println(pcBuffer.substring(0, 14));
  }
  else
  {
    display.println("ready");
  }

  // Status
  display.setCursor(0, 56);
  if (millis() - lastStatusMsg < 5000 && btConnected)
  {
    display.println("Status sent!");
  }
  else
  {
    display.println("Ready");
  }

  display.display();
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.print("WiFi SSID: ");
  Serial.println(ssid ? ssid : "NOT SET");
  Serial.print("WiFi PASS: ");
  Serial.println(strlen(password) > 0 ? "***SET***" : "NOT SET");

  // Initialize OLED
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("OLED init FAILED"));
    for (;;)
      ;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ESP32 Starting...");
  display.println("BT: ESP32-WIFI-BT");
  display.display();
  delay(1500);

  // Wi-Fi
  Serial.println("\n=== WiFi Scan Starting ===");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Scan for networks to see if router is visible
  int n = WiFi.scanNetworks();
  Serial.println("Scan complete");
  if (n == 0)
  {
    Serial.println("No networks found");
  }
  else
  {
    Serial.printf("%d networks found:\n", n);
    bool foundOurSSID = false;
    for (int i = 0; i < n; ++i)
    {
      bool isOurNetwork = (WiFi.SSID(i) == String(ssid));
      if (isOurNetwork)
        foundOurSSID = true;

      Serial.printf("%d: %s (%d dBm) Ch:%d %s %s\n",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i),
                    WiFi.channel(i),
                    (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "OPEN" : "ENC",
                    isOurNetwork ? " <- TARGET" : "");
    }
    if (!foundOurSSID)
    {
      Serial.println("WARNING: Target SSID not found in scan!");
    }
  }

  Serial.println("\n=== Connecting to Wi-Fi ===");
  Serial.print("SSID: ");
  Serial.println(ssid);

  display.clearDisplay();
  display.println("WiFi connecting...");
  display.display();

  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30)
  {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
    attempts++;

    // Print WiFi status for debugging
    if (attempts % 5 == 0)
    {
      Serial.print("\nAttempt ");
      Serial.print(attempts);
      Serial.print(" - Status: ");
      switch (WiFi.status())
      {
      case WL_IDLE_STATUS:
        Serial.println("IDLE");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("NO_SSID_AVAIL");
        break;
      case WL_SCAN_COMPLETED:
        Serial.println("SCAN_COMPLETED");
        break;
      case WL_CONNECTED:
        Serial.println("CONNECTED");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("CONNECT_FAILED");
        break;
      case WL_CONNECTION_LOST:
        Serial.println("CONNECTION_LOST");
        break;
      case WL_DISCONNECTED:
        Serial.println("DISCONNECTED");
        break;
      default:
        Serial.printf("UNKNOWN(%d)\n", WiFi.status());
        break;
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\n=== Wi-Fi Connected ===");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Channel: ");
    Serial.println(WiFi.channel());
    Serial.print("Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    // Initialize time from NTP
    Serial.println("\n=== Setting up Time ===");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.print("Syncing with NTP server...");

    struct tm timeinfo;
    int retries = 0;
    while (!getLocalTime(&timeinfo) && retries < 10)
    {
      Serial.print(".");
      delay(500);
      retries++;
    }
    if (getLocalTime(&timeinfo))
    {
      Serial.println("\nTime synchronized!");
      Serial.print("Current time: ");
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    }
    else
    {
      Serial.println("\nFailed to sync time");
    }
  }
  else
  {
    Serial.println("\n=== Wi-Fi FAILED ===");
    Serial.print("Final Status: ");
    switch (WiFi.status())
    {
    case WL_NO_SSID_AVAIL:
      Serial.println("NO_SSID_AVAIL - Router not found or wrong channel");
      Serial.println("FIX: Check router channel (use 1-11), enable SSID broadcast");
      break;
    case WL_CONNECT_FAILED:
      Serial.println("CONNECT_FAILED - Wrong password or auth issue");
      Serial.println("FIX: Verify password, check router security (use WPA2, not WPA3-only)");
      break;
    case WL_DISCONNECTED:
      Serial.println("DISCONNECTED - Connection rejected");
      Serial.println("FIX: Router settings - try channel 1/6/11, 20MHz width, 802.11bgn mode");
      break;
    default:
      Serial.printf("UNKNOWN(%d)\n", WiFi.status());
      break;
    }
  }
  updateDisplay();

  // Bluetooth
  SerialBT.begin("ESP32-WIFI-BT");
  Serial.println("Bluetooth ready - pair with 'ESP32-WIFI-BT'");
  updateDisplay();
}

void loop()
{
  if (millis() - lastDisplayUpdate > 200)
  {
    updateDisplay();
    lastDisplayUpdate = millis();
  }

  if (scrolling && lastPhoneMsg.length() > 18)
  {
    if (millis() - scrollStart > 300)
    {
      scrollOffset++;
      if (scrollOffset > lastPhoneMsg.length())
      {
        scrollOffset = 0;
      }
      scrollStart = millis();
    }
  }

  if (SerialBT.hasClient() && (millis() - lastStatusMsg > 5000))
  {
    SerialBT.println("ESP32 READY - Send messages!");
    Serial.println("Status sent to phone");
    lastStatusMsg = millis();
  }

  // PC Serial -> Phone BT
  while (Serial.available())
  {
    char c = Serial.read();
    if (c == '\r')
      continue;
    if (c == '\n')
    {
      if (pcBuffer.length() > 0)
      {
        SerialBT.println(pcBuffer);
        Serial.println("PC->BT: " + pcBuffer);
        pcBuffer = "";
      }
    }
    else
    {
      pcBuffer += c;
    }
  }

  // Phone BT -> PC Serial + Display Message
  while (SerialBT.available())
  {
    char c = SerialBT.read();
    if (c == '\r')
      continue;
    if (c == '\n')
    {
      if (phoneBuffer.length() > 0)
      {
        lastPhoneMsg = phoneBuffer;
        scrollOffset = 0;
        scrolling = (phoneBuffer.length() > 18);
        scrollStart = millis();

        Serial.println("PHONE MSG: " + phoneBuffer);
        phoneBuffer = "";
      }
    }
    else
    {
      phoneBuffer += c;
    }
  }
}
