// Advanced LCD I2C Test - Tests multiple configurations
// This will help identify the correct pin mapping for your I2C module
#include <Arduino.h>
#include <Wire.h>

#define LCD_ADDRESS 0x27

// Common I2C to LCD pin mappings
// Format: RS, RW, EN, D4, D5, D6, D7, Backlight, Polarity
uint8_t pinConfigs[][9] = {
    {0, 1, 2, 4, 5, 6, 7, 3, POSITIVE}, // Most common
    {4, 5, 6, 0, 1, 2, 3, 7, POSITIVE}, // Alternative 1
    {2, 1, 0, 4, 5, 6, 7, 3, POSITIVE}, // Alternative 2
    {0, 2, 1, 4, 5, 6, 7, 3, POSITIVE}, // Alternative 3
};

void testConfig(int configNum)
{
    Serial.printf("\n=== Testing Config %d ===\n", configNum + 1);

    Wire.begin(21, 22);
    delay(100);

    // Manual LCD initialization using raw I2C
    // This bypasses the library to test direct communication

    Wire.beginTransmission(LCD_ADDRESS);
    Wire.write(0x00); // All low
    Wire.endTransmission();
    delay(50);

    Wire.beginTransmission(LCD_ADDRESS);
    Wire.write(0x08); // Backlight on
    Wire.endTransmission();
    delay(50);

    Serial.println("Backlight command sent");
    Serial.println("Check if backlight is ON");
    delay(2000);

    // Try to send some init commands
    Serial.println("Sending LCD init commands...");

    // This is a simplified init - just to test if LCD responds
    byte commands[] = {0x33, 0x32, 0x28, 0x0C, 0x06, 0x01};

    for (int i = 0; i < 6; i++)
    {
        Wire.beginTransmission(LCD_ADDRESS);
        Wire.write(0x0C); // EN high, backlight on
        Wire.write(commands[i] | 0x08);
        Wire.write(0x08); // EN low, backlight on
        Wire.endTransmission();
        delay(10);
    }

    Serial.println("Init sent. Check LCD for any response.");
    delay(3000);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n=== LCD I2C Module Tester ===");
    Serial.println("This will test if your I2C module is responding");
    Serial.println("");

    Wire.begin(21, 22);

    // Check if device exists
    Wire.beginTransmission(LCD_ADDRESS);
    byte error = Wire.endTransmission();

    if (error == 0)
    {
        Serial.printf("LCD found at 0x%02X\n", LCD_ADDRESS);
    }
    else
    {
        Serial.println("ERROR: No device at 0x27!");
        while (1)
            ;
    }

    Serial.println("\nTrying to turn backlight ON...");

    // Try different backlight commands
    Wire.beginTransmission(LCD_ADDRESS);
    Wire.write(0x08); // Backlight bit
    Wire.endTransmission();

    delay(1000);

    Serial.println("Sent backlight ON command");
    Serial.println("Does the backlight turn ON? (y/n)");
    Serial.println("");
    Serial.println("Now trying raw data writes...");

    delay(2000);

    // Try writing some data
    for (int i = 0; i < 5; i++)
    {
        Wire.beginTransmission(LCD_ADDRESS);
        Wire.write(0xFF); // All bits high
        Wire.endTransmission();
        delay(200);

        Wire.beginTransmission(LCD_ADDRESS);
        Wire.write(0x08); // Just backlight
        Wire.endTransmission();
        delay(200);

        Serial.printf("Pulse %d sent\n", i + 1);
    }

    Serial.println("\n*** IMPORTANT ***");
    Serial.println("Your I2C module might be a DIFFERENT TYPE");
    Serial.println("Common types: PCF8574, PCF8574A, MCP23008");
    Serial.println("");
    Serial.println("What's written on the I2C chip (black IC)?");
    Serial.println("Look for: PCF8574T, PCF8574AT, or similar");
}

void loop()
{
    delay(5000);
}
