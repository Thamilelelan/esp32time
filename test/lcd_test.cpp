// Simple LCD Test - Upload this to test LCD only
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Try 0x27 first

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== LCD Test Program ===");

    // I2C Scanner
    Wire.begin(21, 22);
    Serial.println("Scanning I2C...");

    for (byte addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            Serial.printf("Device found at 0x%02X\n", addr);
        }
    }

    Serial.println("\nInitializing LCD...");
    lcd.init();
    lcd.backlight();
    Serial.println("Backlight ON");

    delay(500);
    lcd.clear();
    delay(500);

    Serial.println("Writing test text...");
    lcd.setCursor(0, 0);
    lcd.print("** LCD TEST **");
    lcd.setCursor(0, 1);
    lcd.print("0123456789ABCDEF");

    Serial.println("\n*** CHECK LCD NOW ***");
    Serial.println("If you see BACKLIGHT but NO TEXT:");
    Serial.println("1. Find the BLUE POTENTIOMETER (small screw)");
    Serial.println("2. It might be on the LCD itself OR on the I2C module");
    Serial.println("3. Use a small screwdriver");
    Serial.println("4. Turn it SLOWLY - you'll see text appear!");
    Serial.println("\nIf still nothing, try:");
    Serial.println("- Move VCC from 5V to 3.3V (or vice versa)");
    Serial.println("- Check if address is 0x3F instead of 0x27");
}

void loop()
{
    // Flash text to prove it's running
    static unsigned long lastUpdate = 0;
    static bool toggle = false;

    if (millis() - lastUpdate > 1000)
    {
        toggle = !toggle;
        lcd.setCursor(15, 0);
        lcd.print(toggle ? "*" : " ");
        lastUpdate = millis();

        Serial.println(toggle ? "BLINK ON" : "BLINK OFF");
    }
}
