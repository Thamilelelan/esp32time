#include <Arduino.h>
// #include <WiFi.h>  // WiFi not needed - Chronos uses BLE and provides time sync
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <Preferences.h>
#include <ChronosESP32.h>
#include "credentials.h"

//////////////////////
// Wi-Fi settings (from credentials.h) - DISABLED (not needed for Chronos BLE)
//////////////////////
// const char *ssid = WIFI_SSID;
// const char *password = WIFI_PASSWORD;

//////////////////////
// Time settings - DISABLED (Chronos provides time via BLE)
//////////////////////
// const char *ntpServer = "pool.ntp.org";
// const long gmtOffset_sec = 19800; // GMT+5:30 for India (5.5 hours * 3600)
// const int daylightOffset_sec = 0;

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

//////////////////////
// ChronosESP32 BLE
//////////////////////
ChronosESP32 Chronos("ESP32-Nav");

unsigned long lastDisplayUpdate = 0;
unsigned long lastValidNavTime = 0; // Track when we last had valid navigation data
bool wasNavigating = false;         // Remember if we were navigating
unsigned long lastTimeSave = 0;

// NVS storage for persistent time
Preferences preferences;

//////////////////////
// Boot Button Configuration
//////////////////////
#define BOOT_BUTTON_PIN 0    // GPIO0 is the BOOT button on ESP32
#define LONG_PRESS_TIME 3000 // 3 seconds to power off

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

// Navigation Arrow Icons (24x24)
const unsigned char arrow_up[] PROGMEM = {
    0x00, 0x18, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x7e, 0x00, 0x00, 0xff, 0x00,
    0x01, 0xff, 0x80, 0x03, 0xff, 0xc0, 0x07, 0xe7, 0xe0, 0x00, 0x18, 0x00,
    0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00,
    0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00,
    0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00,
    0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const unsigned char arrow_left[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0xf0, 0x00, 0x01, 0xf8, 0x00,
    0x03, 0xfc, 0x00, 0x07, 0xfe, 0x00, 0x0f, 0xff, 0x00, 0x1f, 0xff, 0x80,
    0x3f, 0xff, 0xc0, 0x7f, 0xff, 0xe0, 0x3f, 0xff, 0xc0, 0x1f, 0xff, 0x80,
    0x0f, 0xff, 0x00, 0x07, 0xfe, 0x00, 0x03, 0xfc, 0x00, 0x01, 0xf8, 0x00,
    0x00, 0xf0, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const unsigned char arrow_right[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x1f, 0x80,
    0x00, 0x3f, 0xc0, 0x00, 0x7f, 0xe0, 0x00, 0xff, 0xf0, 0x01, 0xff, 0xf8,
    0x03, 0xff, 0xfc, 0x07, 0xff, 0xfe, 0x03, 0xff, 0xfc, 0x01, 0xff, 0xf8,
    0x00, 0xff, 0xf0, 0x00, 0x7f, 0xe0, 0x00, 0x3f, 0xc0, 0x00, 0x1f, 0x80,
    0x00, 0x0f, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Large Navigation Arrows (48x48) for prominent display
const unsigned char arrow_up_large[] PROGMEM = {
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x01, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xC0, 0x00, 0x00,
    0x00, 0x07, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xF0, 0x00, 0x00,
    0x00, 0x1F, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFC, 0x00, 0x00,
    0x00, 0x7F, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x01, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xC0, 0x00,
    0x07, 0xFF, 0xFF, 0xFF, 0xE0, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xF0, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00};

const unsigned char arrow_left_large[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x0F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x0F, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x0F, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x0F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const unsigned char arrow_right_large[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xC0, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFC, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xC0,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFC,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFC,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xC0,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFC, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xC0, 0x00,
    0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void saveCurrentTime()
{
  time_t now;
  time(&now);
  if (now > 0)
  {
    preferences.begin("esp32time", false);
    preferences.putULong64("savedTime", (uint64_t)now);
    preferences.end();
  }
}

void restoreSavedTime()
{
  preferences.begin("esp32time", true);
  uint64_t savedTime = preferences.getULong64("savedTime", 0);
  preferences.end();

  if (savedTime > 0)
  {
    struct timeval tv;
    tv.tv_sec = savedTime;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
  }
}

void updateDisplayLCD()
{
  char line0[17] = "                ";
  char line1[17] = "                ";

  // Line 0: Time and connection status
  String timeStr = Chronos.getHourZ() + ":" + (Chronos.getHourC() < 10 ? "0" : "") + String(Chronos.getHourC());
  snprintf(line1, sizeof(line1), "%s BLE:%s",
           timeStr.c_str(),
           // WiFi.status() == WL_CONNECTED ? "OK" : "X",  // WiFi disabled
           Chronos.isConnected() ? "OK" : "X");

  // Line 1: Navigation or status
  Navigation nav = Chronos.getNavigation();
  if (Chronos.isConnected() && nav.active && nav.directions != "")
  {
    // nav.distance is already a String like "250m" or "1.5km"
    snprintf(line1, sizeof(line1), "%s %.8s", nav.distance.c_str(), nav.directions.c_str());
  }
  else if (Chronos.isConnected())
  {
    snprintf(line1, sizeof(line1), "Connected!");
  }
  else
  {
    snprintf(line1, sizeof(line1), "Wait Chronos...");
  }

  lcd.setCursor(0, 0);
  lcd.print(line0);
  lcd.setCursor(0, 1);
  lcd.print(line1);
}

void updateDisplayOLED()
{
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  // Get navigation data from Chronos
  Navigation nav = Chronos.getNavigation();

  // Check if we have valid navigation data
  bool hasNavData = (nav.active || nav.distance != "" || nav.directions != "" || nav.title != "");

  // Update navigation state tracking
  if (hasNavData)
  {
    lastValidNavTime = millis();
    wasNavigating = true;
  }

  // Show navigation if we have data OR if we were navigating recently (within 10 seconds)
  // This prevents flickering to "Start navigation" during rerouting/road closure alerts
  bool showNavigation = Chronos.isConnected() && (hasNavData || (wasNavigating && (millis() - lastValidNavTime < 10000)));

  if (showNavigation)
  {
    oled.setTextSize(1);

    // ETA (top left, small)
    if (nav.eta != "")
    {
      oled.setCursor(0, 0);
      oled.print(nav.eta.substring(0, min(8, (int)nav.eta.length())));
    }

    // TITLE (top right, large) - Distance to next turn
    oled.setTextSize(2);
    oled.setCursor(70, 0);
    if (nav.title != "")
    {
      oled.print(nav.title.substring(0, min(7, (int)nav.title.length())));
    }

    // DURATION (right side, y=30, small)
    oled.setTextSize(1);
    if (nav.duration != "")
    {
      oled.setCursor(70, 30);
      oled.print(nav.duration.substring(0, min(8, (int)nav.duration.length())));
    }

    // DISTANCE (right side, y=40, small)
    if (nav.distance != "")
    {
      oled.setCursor(70, 40);
      oled.print(nav.distance.substring(0, min(8, (int)nav.distance.length())));
    }

    // ICON (left side, 48x48) - Google Maps navigation arrow
    int iconX = 0;
    int iconY = 8;
    oled.drawBitmap(iconX, iconY, nav.icon, 48, 48, SSD1306_WHITE);

    // DIRECTIONS (bottom line)
    oled.setCursor(0, 56);
    if (nav.directions != "")
    {
      String dirText = nav.directions;
      if (dirText.length() > 21)
      {
        oled.print(dirText.substring(0, 18));
        oled.print("...");
      }
      else
      {
        oled.print(dirText);
      }
    }
  }
  // Show connection status when no navigation
  else if (Chronos.isConnected())
  {
    // Normal display with larger time
    oled.setTextSize(2);
    oled.setCursor(0, 0);
    String timeStr = Chronos.getHourZ() + ":" + (Chronos.getHourC() < 10 ? "0" : "") + String(Chronos.getHourC());
    oled.print(timeStr);

    // WiFi icon - DISABLED (not needed for Chronos BLE)
    // if (WiFi.status() == WL_CONNECTED)
    // {
    //   oled.drawBitmap(80, 0, wifi_icon, 16, 16, SSD1306_WHITE);
    // }
    // else
    // {
    //   oled.drawBitmap(80, 0, wifi_off_icon, 16, 16, SSD1306_WHITE);
    // }

    if (Chronos.isConnected())
    {
      oled.drawBitmap(104, 0, bt_icon, 16, 16, SSD1306_WHITE);
    }
    else
    {
      oled.drawBitmap(104, 0, bt_off_icon, 16, 16, SSD1306_WHITE);
    }

    oled.setTextSize(1);
    oled.setCursor(0, 25);
    oled.println("Chronos Connected!");
    oled.setCursor(0, 40);
    oled.print("App: v");
    oled.println(Chronos.getAppVersion());
    oled.setCursor(0, 52);
    oled.println("Start navigation...");

    // Reset navigation state when on idle screen for more than 10 seconds
    if (millis() - lastValidNavTime > 10000)
    {
      wasNavigating = false;
    }
  }
  else
  {
    oled.setCursor(0, 25);
    oled.println("Waiting for Chronos");
    oled.setCursor(0, 40);
    oled.println("Open Chronos app");
    oled.setCursor(0, 52);
    oled.println("Pair ESP32-Nav");
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

//////////////////////
// Power Management Functions
//////////////////////
void enterDeepSleep()
{
  Serial.println("\n=== Entering Deep Sleep ===");
  Serial.println("Press BOOT button to wake up");

  // Save current time before sleep
  saveCurrentTime();

  // Show sleep message on display
  if (displayType == DISPLAY_LCD)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sleeping...");
    lcd.setCursor(0, 1);
    lcd.print("Press BOOT wake");
    delay(1000);
    lcd.noBacklight();
  }
  else if (displayType == DISPLAY_OLED)
  {
    oled.clearDisplay();
    oled.setCursor(0, 20);
    oled.setTextSize(1);
    oled.println("  Going to sleep");
    oled.setCursor(0, 35);
    oled.println("Press BOOT to wake");
    oled.display();
    delay(1000);
    oled.clearDisplay();
    oled.display();
  }

  delay(500);

  // Configure wake up source (BOOT button on GPIO0)
  // LOW level will wake up the ESP32
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);

  // Enter deep sleep
  esp_deep_sleep_start();
}

bool checkForPowerOff()
{
  static unsigned long buttonPressStart = 0;
  static bool buttonWasPressed = false;

  bool buttonPressed = (digitalRead(BOOT_BUTTON_PIN) == LOW);

  if (buttonPressed && !buttonWasPressed)
  {
    // Button just pressed
    buttonPressStart = millis();
    buttonWasPressed = true;
  }
  else if (!buttonPressed && buttonWasPressed)
  {
    // Button just released - check if it was a short press
    unsigned long pressDuration = millis() - buttonPressStart;
    buttonWasPressed = false;

    // Short press (less than 1 second) - go to sleep
    if (pressDuration < 1000)
    {
      Serial.println("Short press detected - entering sleep mode");
      return true;
    }
  }
  else if (buttonPressed && buttonWasPressed)
  {
    // Button is being held
    unsigned long pressDuration = millis() - buttonPressStart;

    if (pressDuration >= LONG_PRESS_TIME)
    {
      // Long press detected - power off
      Serial.println("Long press detected - entering sleep mode");
      return true;
    }
  }

  return false;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== ESP32 Chronos Navigation ===");

  // Check wake up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
  {
    Serial.println("Woke up from BOOT button press");
  }
  else if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED)
  {
    Serial.println("First boot - checking if BOOT button pressed");

    // Configure boot button as input with pull-up
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    delay(100);

    // Check if BOOT button is NOT pressed on first power-up
    if (digitalRead(BOOT_BUTTON_PIN) == HIGH)
    {
      Serial.println("BOOT button not pressed - entering sleep mode");
      Serial.println("Press and hold BOOT button during power-up to start");

      // Go to sleep immediately - user must press BOOT to wake
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
      delay(100);
      esp_deep_sleep_start();
    }
    else
    {
      Serial.println("BOOT button pressed - starting normally");
    }
  }

  // Configure boot button for long-press shutdown detection
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  // Restore saved time from NVS
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
      oled.println("Chronos Starting...");
      oled.display();
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
      lcd.print("Chronos Start..");
      Serial.println("LCD initialized!");
      delay(2000);
    }
    else
    {
      Serial.println("No display found on I2C bus!");
      displayType = DISPLAY_NONE;
    }
  }

  // Wi-Fi (for NTP time sync) - DISABLED (Chronos provides time via BLE)
  // Serial.println("\n=== Connecting to WiFi ===");
  // if (displayType == DISPLAY_LCD)
  // {
  //   lcd.clear();
  //   lcd.print("WiFi connect...");
  // }
  // else if (displayType == DISPLAY_OLED)
  // {
  //   oled.clearDisplay();
  //   oled.setCursor(0, 0);
  //   oled.println("WiFi connecting...");
  //   oled.display();
  // }

  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);

  // int attempts = 0;
  // while (WiFi.status() != WL_CONNECTED && attempts < 20)
  // {
  //   delay(500);
  //   Serial.print(".");
  //   attempts++;
  // }

  // if (WiFi.status() == WL_CONNECTED)
  // {
  //   Serial.println("\nWiFi connected!");
  //   Serial.print("IP: ");
  //   Serial.println(WiFi.localIP());

  //   // Setup NTP time
  //   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //   delay(2000);
  //   saveCurrentTime();
  // }
  // else
  // {
  //   Serial.println("\nWiFi FAILED (optional - Chronos will sync time)");
  // }

  // Start Chronos BLE
  Serial.println("\n=== Starting Chronos BLE ===");
  Chronos.begin();
  Serial.println("Chronos BLE started!");
  Serial.println("Open Chronos app and pair with 'ESP32-Nav'");

  if (displayType == DISPLAY_LCD)
  {
    lcd.clear();
    lcd.print("Chronos Ready!");
    lcd.setCursor(0, 1);
    lcd.print("Pair ESP32-Nav");
  }
  else if (displayType == DISPLAY_OLED)
  {
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.println("Chronos BLE Ready!");
    oled.setCursor(0, 16);
    oled.println("ESP32-Nav");
    oled.setCursor(0, 32);
    oled.println("Open Chronos app");
    oled.setCursor(0, 44);
    oled.println("and pair device");
    oled.display();
  }
  delay(2000);

  updateDisplay();
}

void loop()
{
  // Check for long press on BOOT button to power off
  if (checkForPowerOff())
  {
    enterDeepSleep();
  }

  // Handle Chronos BLE (CRITICAL - must be called frequently)
  Chronos.loop();

  // Update display every second
  if (millis() - lastDisplayUpdate > 1000)
  {
    // Debug: Print raw navigation data
    Navigation nav = Chronos.getNavigation();
    if (Chronos.isConnected())
    {
      Serial.println("=== Nav Data ===");
      Serial.printf("Active: %d | IsNav: %d\n", nav.active, nav.isNavigation);
      Serial.printf("Title: %s\n", nav.title.c_str());
      Serial.printf("ETA: %s\n", nav.eta.c_str());
      Serial.printf("Duration: %s\n", nav.duration.c_str());
      Serial.printf("Distance: %s\n", nav.distance.c_str());
      Serial.printf("Directions: %s\n", nav.directions.c_str());
      Serial.printf("Speed: %s\n", nav.speed.c_str());
    }

    updateDisplay();
    lastDisplayUpdate = millis();
  }

  // Save time to NVS every 60 seconds
  if (millis() - lastTimeSave > 60000)
  {
    saveCurrentTime();
    lastTimeSave = millis();
  }
}
