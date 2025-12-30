#include <Arduino.h>
#include <WiFi.h>
#include <BluetoothSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <Preferences.h>
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
// Display (Auto-detect LCD or OLED)
//////////////////////
#define LCD_ADDRESS 0x27
#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum DisplayType
{
  DISPLAY_NONE,
  DISPLAY_LCD,
  DISPLAY_OLED
};
DisplayType displayType = DISPLAY_NONE;

String pcBuffer = "";
String phoneBuffer = "";
String lastPhoneMsg = "";
unsigned long lastStatusMsg = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastTimeSave = 0;
bool btConnected = false;

// NVS storage for persistent time
Preferences preferences;

//////////////////////
// Icons for OLED (16x16 pixels)
//////////////////////
// WiFi Connected Icon
const unsigned char wifi_icon[] PROGMEM = {
    0x00, 0x00, 0x0f, 0xf0, 0x3f, 0xfc, 0x70, 0x0e, 0xe0, 0x07, 0xc7, 0xe3,
    0x0f, 0xf0, 0x1c, 0x38, 0x30, 0x0c, 0x23, 0xc4, 0x07, 0xe0, 0x0c, 0x30,
    0x04, 0x20, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00};

// WiFi Disconnected Icon (X mark)
const unsigned char wifi_off_icon[] PROGMEM = {
    0x00, 0x00, 0xc0, 0x03, 0xe0, 0x07, 0x70, 0x0e, 0x38, 0x1c, 0x1c, 0x38,
    0x0e, 0x70, 0x07, 0xe0, 0x07, 0xe0, 0x0e, 0x70, 0x1c, 0x38, 0x38, 0x1c,
    0x70, 0x0e, 0xe0, 0x07, 0xc0, 0x03, 0x00, 0x00};

// Bluetooth Connected Icon
const unsigned char bt_icon[] PROGMEM = {
    0x00, 0x00, 0x01, 0x00, 0x01, 0x80, 0x11, 0x40, 0x09, 0x20, 0x05, 0x90,
    0x03, 0xc8, 0x01, 0xe4, 0x01, 0xe4, 0x03, 0xc8, 0x05, 0x90, 0x09, 0x20,
    0x11, 0x40, 0x01, 0x80, 0x01, 0x00, 0x00, 0x00};

// Bluetooth Disconnected Icon
const unsigned char bt_off_icon[] PROGMEM = {
    0x00, 0x00, 0x40, 0x02, 0x60, 0x06, 0x30, 0x0c, 0x18, 0x18, 0x0c, 0x30,
    0x06, 0x60, 0x03, 0xc0, 0x03, 0xc0, 0x06, 0x60, 0x0c, 0x30, 0x18, 0x18,
    0x30, 0x0c, 0x60, 0x06, 0x40, 0x02, 0x00, 0x00};

void saveCurrentTime()
{
  time_t now;
  time(&now);
  if (now > 0)
  {
    preferences.begin("esp32time", false);
    preferences.putULong64("savedTime", (uint64_t)now);
    preferences.end();
    Serial.print("Time saved to NVS: ");
    Serial.println(now);
  }
  else
  {
    Serial.println("Cannot save time - time not set");
  }
}

void restoreSavedTime()
{
  preferences.begin("esp32time", true); // read-only
  uint64_t savedTime = preferences.getULong64("savedTime", 0);
  preferences.end();

  if (savedTime > 0)
  {
    struct timeval tv;
    tv.tv_sec = savedTime;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
    Serial.print("Time restored from NVS: ");
    Serial.println((time_t)savedTime);

    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
      char buffer[64];
      strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
      Serial.print("Restored time: ");
      Serial.println(buffer);
    }
  }
  else
  {
    Serial.println("No saved time found in NVS");
  }
}

void updateDisplayLCD()
{
  struct tm timeinfo;
  bool hasTime = getLocalTime(&timeinfo);

  char line0[17] = "                ";
  char line1[17] = "                ";

  // Line 0: Time and WiFi and BT
  if (hasTime)
  {
    char timeStr[9];
    strftime(timeStr, sizeof(timeStr), "%I:%M%p", &timeinfo);
    snprintf(line0, sizeof(line0), "%s W:%s BT:%s",
             timeStr,
             WiFi.status() == WL_CONNECTED ? "OK" : "X",
             SerialBT.hasClient() ? "OK" : "X");
  }
  else
  {
    snprintf(line0, sizeof(line0), "--:-- W:%s BT:%s",
             WiFi.status() == WL_CONNECTED ? "OK" : "X",
             SerialBT.hasClient() ? "OK" : "X");
  }

  // Line 1: Message
  btConnected = SerialBT.hasClient();
  if (lastPhoneMsg.length() > 0 && btConnected)
  {
    snprintf(line1, sizeof(line1), "%.16s", lastPhoneMsg.c_str());
  }
  else
  {
    snprintf(line1, sizeof(line1), "Ready...");
  }

  lcd.setCursor(0, 0);
  lcd.print(line0);
  lcd.setCursor(0, 1);
  lcd.print(line1);
}

void updateDisplayOLED()
{
  struct tm timeinfo;
  bool hasTime = getLocalTime(&timeinfo);

  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);

  // First line: Time + WiFi icon + BT icon
  // Format: "HH:MM [W][B]"
  oled.setCursor(0, 0);
  if (hasTime)
  {
    char timeStr[8];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
    oled.print(timeStr);
  }
  else
  {
    oled.print("--:--");
  }

  // WiFi icon (16x16 at position after time)
  btConnected = SerialBT.hasClient();
  if (WiFi.status() == WL_CONNECTED)
  {
    oled.drawBitmap(80, 0, wifi_icon, 16, 16, SSD1306_WHITE);
  }
  else
  {
    oled.drawBitmap(80, 0, wifi_off_icon, 16, 16, SSD1306_WHITE);
  }

  // Bluetooth icon (16x16 next to WiFi icon)
  if (btConnected)
  {
    oled.drawBitmap(104, 0, bt_icon, 16, 16, SSD1306_WHITE);
  }
  else
  {
    oled.drawBitmap(104, 0, bt_off_icon, 16, 16, SSD1306_WHITE);
  }

  // Rest of screen for additional info
  oled.setTextSize(1);

  // Line 2: WiFi IP if connected
  if (WiFi.status() == WL_CONNECTED)
  {
    oled.setCursor(0, 20);
    oled.print("IP: ");
    oled.println(WiFi.localIP());
  }

  // Line 3+: Messages
  if (lastPhoneMsg.length() > 0 && btConnected)
  {
    oled.setCursor(0, 32);
    oled.println("Message:");
    oled.setCursor(0, 44);
    // Word wrap for long messages
    if (lastPhoneMsg.length() > 21)
    {
      oled.println(lastPhoneMsg.substring(0, 21));
      oled.setCursor(0, 54);
      oled.println(lastPhoneMsg.substring(21, 42));
    }
    else
    {
      oled.println(lastPhoneMsg);
    }
  }
  else if (btConnected)
  {
    oled.setCursor(0, 40);
    oled.println("Ready for messages...");
  }

  oled.display();
}

void updateDisplay()
{
  if (displayType == DISPLAY_LCD)
  {
    updateDisplayLCD();
  }
  else if (displayType == DISPLAY_OLED)
  {
    updateDisplayOLED();
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== ESP32 WiFi+BT Project ===");
  Serial.print("WiFi SSID: ");
  Serial.println(ssid ? ssid : "NOT SET");

  // Restore saved time from NVS (before WiFi)
  Serial.println("\n=== Restoring Time from NVS ===");
  restoreSavedTime();

  // Initialize I2C
  Serial.println("\n=== Detecting Display ===");
  Wire.begin(21, 22); // SDA=GPIO21, SCL=GPIO22

  // Check for OLED at 0x3C
  Wire.beginTransmission(OLED_ADDRESS);
  if (Wire.endTransmission() == 0)
  {
    Serial.println("OLED detected at 0x3C!");
    displayType = DISPLAY_OLED;

    if (oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
    {
      Serial.println("OLED initialized!");
      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setTextColor(SSD1306_WHITE);
      oled.setCursor(0, 0);
      oled.println("ESP32 Starting...");
      oled.setCursor(0, 16);
      oled.println("OLED Mode!");
      oled.display();
      delay(2000);
    }
    else
    {
      Serial.println("OLED init failed!");
      displayType = DISPLAY_NONE;
    }
  }
  // Check for LCD at 0x27
  else
  {
    Wire.beginTransmission(LCD_ADDRESS);
    if (Wire.endTransmission() == 0)
    {
      Serial.println("LCD detected at 0x27!");
      displayType = DISPLAY_LCD;

      lcd.init();
      lcd.backlight();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ESP32 Starting..");
      lcd.setCursor(0, 1);
      lcd.print("LCD I2C Mode!");

      Serial.println("LCD initialized!");
      Serial.println("*** If you see backlight but NO text:");
      Serial.println("1. SOLDER the 4 I2C module pins");
      Serial.println("2. Or use jumper wires directly to I2C module pins");
      Serial.println("3. Adjust contrast pot on I2C module (blue box with grey screw)");
      delay(2000);
    }
    else
    {
      Serial.println("No display found on I2C bus!");
      displayType = DISPLAY_NONE;
    }
  }

  // Wi-Fi
  Serial.println("\n=== Connecting to WiFi ===");
  if (displayType == DISPLAY_LCD)
  {
    lcd.clear();
    lcd.print("WiFi connect...");
  }
  else if (displayType == DISPLAY_OLED)
  {
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.println("WiFi connecting...");
    oled.display();
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    if (displayType == DISPLAY_LCD)
    {
      lcd.print(".");
    }
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    if (displayType == DISPLAY_LCD)
    {
      lcd.clear();
      lcd.print("WiFi OK!");
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP());
    }
    else if (displayType == DISPLAY_OLED)
    {
      oled.clearDisplay();
      oled.setCursor(0, 0);
      oled.println("WiFi Connected!");
      oled.setCursor(0, 16);
      oled.print("IP: ");
      oled.println(WiFi.localIP());
      oled.display();
    }
    delay(2000);

    // Setup time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Time sync started");

    // Wait for time to be set
    delay(2000);
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
      Serial.println("NTP time synced successfully!");
      saveCurrentTime(); // Save to NVS
    }
  }
  else
  {
    Serial.println("\nWiFi FAILED");
    if (displayType == DISPLAY_LCD)
    {
      lcd.clear();
      lcd.print("WiFi FAILED!");
    }
    else if (displayType == DISPLAY_OLED)
    {
      oled.clearDisplay();
      oled.setCursor(0, 0);
      oled.println("WiFi FAILED!");
      oled.display();
    }
    delay(2000);
  }

  // Bluetooth
  SerialBT.begin("ESP32-WIFI-BT");
  Serial.println("Bluetooth ready - pair with 'ESP32-WIFI-BT'");

  if (displayType == DISPLAY_LCD)
  {
    lcd.clear();
    lcd.print("BT: Ready!");
  }
  else if (displayType == DISPLAY_OLED)
  {
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.println("Bluetooth Ready!");
    oled.setCursor(0, 16);
    oled.println("ESP32-WIFI-BT");
    oled.display();
  }
  delay(2000);

  updateDisplay();
}

void loop()
{
  // Update display every second
  if (millis() - lastDisplayUpdate > 1000)
  {
    updateDisplay();
    lastDisplayUpdate = millis();
  }

  // Save time to NVS every 60 seconds
  if (millis() - lastTimeSave > 60000)
  {
    saveCurrentTime();
    lastTimeSave = millis();
  }

  // Send status message every 5 seconds if BT connected
  if (SerialBT.hasClient() && (millis() - lastStatusMsg > 5000))
  {
    SerialBT.println("ESP32 READY");
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
        Serial.println("PHONE MSG: " + phoneBuffer);
        phoneBuffer = "";
        updateDisplay(); // Update immediately when new message arrives
      }
    }
    else
    {
      phoneBuffer += c;
    }
  }
}
