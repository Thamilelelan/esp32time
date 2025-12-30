#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// We'll test with different configurations
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup()
{
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== Simple LCD Test ===");

    Wire.begin(21, 22);

    // Scan I2C
    Serial.println("Scanning I2C...");
    Wire.beginTransmission(0x27);
    if (Wire.endTransmission() == 0)
    {
        Serial.println("Device found at 0x27");
    }
    else
    {
        Serial.println("No device at 0x27!");
        return;
    }

    Serial.println("\nInitializing LCD...");

    // Try init
    lcd.init();
    lcd.backlight();
    Serial.println("Backlight ON - do you see it? (should be YES)");
    delay(2000);

    Serial.println("\nClearing display...");
    lcd.clear();
    delay(500);

    Serial.println("Writing 'HELLO'...");
    lcd.setCursor(0, 0);
    lcd.print("HELLO");

    delay(2000);

    Serial.println("Writing '12345'...");
    lcd.setCursor(0, 1);
    lcd.print("12345");

    Serial.println("\n*** CHECK LCD NOW ***");
    Serial.println("Do you see 'HELLO' on line 1?");
    Serial.println("Do you see '12345' on line 2?");
    Serial.println("\nIf you see BACKLIGHT but NO TEXT:");
    Serial.println("Your I2C module might be incompatible with this library");
    Serial.println("or the LCD itself might be defective.");
}

void loop()
{
    delay(1000);
}
