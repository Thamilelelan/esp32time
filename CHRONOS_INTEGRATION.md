# ESP32 Chronos Navigation Display

This ESP32 project receives navigation data from the Chronos app via **Bluetooth** and displays it on an OLED/LCD screen in real-time.

## Features

- ✅ **Bluetooth Serial** for navigation data (no WiFi network needed!)
- ✅ WiFi connectivity for NTP time synchronization only
- ✅ JSON-based navigation protocol
- ✅ OLED/LCD auto-detection and display
- ✅ Real-time navigation display with turn arrows
- ✅ Distance and instruction display
- ✅ Connection status icons (WiFi, Bluetooth)
- ✅ Simple pairing - just connect and go!

## Hardware Requirements

- ESP32 development board
- OLED display (128x64, I2C, 0x3C) **OR** LCD display (16x2, I2C, 0x27)
- USB cable for programming
- Power source (USB or battery)

## Wiring

### OLED Display (SSD1306)

```
ESP32 -> OLED
GPIO21 (SDA) -> SDA
GPIO22 (SCL) -> SCL
3.3V -> VCC
GND -> GND
```

### LCD Display (I2C)

```
ESP32 -> LCD I2C Module
GPIO21 (SDA) -> SDA
GPIO22 (SCL) -> SCL
5V -> VCC
GND -> GND
```

## Setup Instructions

### 1. Configure WiFi Credentials

Edit `include/credentials.h`:

```cpp
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASSWORD "YourPassword"
```

### 2. Install Dependencies

The required libraries are automatically installed by PlatformIO:

- ArduinoJson (JSON parsing)
- Adafruit SSD1306 (OLED display)
- Adafruit GFX Library (Graphics)
- LiquidCrystal_I2C (LCD display)

### 3. Upload to ESP32

```bash
pio run --target upload
pio device monitor
```

### 4. Pair with Bluetooth

After upload, the ESP32 will appear as **"ESP32-Chronos-Nav"**:

1. On your phone, go to Bluetooth settings
2. Pair with "ESP32-Chronos-Nav"
3. Open Chronos app and connect
4. Start sending navigation data!

No WiFi network needed - everything works over Bluetooth!

## Chronos App Integration

### Communication Protocol

The ESP32 receives navigation data via **Bluetooth Serial** using JSON format.

### Communication Protocol

The ESP32 receives navigation data via **Bluetooth Serial** using JSON format.

#### Send Navigation Update

Send a JSON object followed by newline (`\n`) via Bluetooth Serial:

**Format:**

```json
{
  "instruction": "Turn left onto Main Street",
  "distance": 250.5,
  "street": "Main Street",
  "angle": -90,
  "active": true,
  "destination": "123 Elm Street"
}\n
```

**Fields:**

- `instruction` (string, required): Turn instruction text
- `distance` (float, required): Distance to next turn in meters
- `street` (string, optional): Current/next street name
- `angle` (int, optional): Turn angle (-180 to 180, negative=left, positive=right)
- `active` (bool, optional): Navigation active status (default: true)
- `destination` (string, optional): Final destination

**Response:**

```json
{ "status": "ok" }
```

#### Stop Navigation

Send a stop command:

```json
{"command": "stop"}\n
```

**Response:**

```json
{ "status": "stopped" }
```

#### Regular Text Messages

You can also send plain text (non-JSON) messages - they'll be displayed as regular messages:

```
Hello ESP32!\n
```

### Python Test Script

Use `chronos_bluetooth_test.py` to test the Bluetooth integration:

1. Pair your computer with ESP32
2. Find the Bluetooth serial port (e.g., COM4 on Windows)
3. Update `BLUETOOTH_PORT` in the script
4. Run:

```bash
pip install pyserial
python chronos_bluetooth_test.py
```

This interactive script lets you:

- Run full navigation simulations
- Send single navigation commands
- Test Bluetooth communication
- Monitor ESP32 responses

## Display Behavior

### OLED Display (128x64)

```
┌────────────────────────────┐
│ 12:45    [WiFi] [BT]      │  <- Time + Status Icons
│ IP: 192.168.1.100         │  <- WiFi IP (when idle)
│                           │
│     250m      ←           │  <- Distance + Arrow
│                           │
│ Turn left onto Main St    │  <- Instruction
└────────────────────────────┘
```

**Arrow Icons:**

- ⬆️ Straight / Continue
- ⬅️ Left turn (angle < -30°)
- ➡️ Right turn (angle > 30°)

### LCD Display (16x2)

```
┌────────────────┐
│12:45 W:OK BT:X │  <- Time + Status
│250m Turn left  │  <- Distance + Instruction
└────────────────┘
```

## Chronos App Implementation Guide

### JavaScript/TypeScript Example (React Native / Expo)

```typescript
import { BluetoothSerial } from "react-native-bluetooth-serial-next";

class ESP32Navigator {
  private deviceId: string | null = null;

  async connect(): Promise<boolean> {
    try {
      // Find ESP32 device
      const devices = await BluetoothSerial.list();
      const esp32 = devices.find((d) => d.name === "ESP32-Chronos-Nav");

      if (!esp32) {
        console.log("ESP32 not found");
        return false;
      }

      // Connect
      await BluetoothSerial.connect(esp32.id);
      this.deviceId = esp32.id;
      console.log("Connected to ESP32");
      return true;
    } catch (error) {
      console.error("Connection failed:", error);
      return false;
    }
  }

  async sendNavigation(data: {
    instruction: string;
    distance: number;
    street?: string;
    angle?: number;
    destination?: string;
  }): Promise<boolean> {
    if (!this.deviceId) return false;

    try {
      const json = JSON.stringify({ ...data, active: true });
      await BluetoothSerial.write(json + "\n");
      console.log("Sent navigation:", data.instruction);
      return true;
    } catch (error) {
      console.error("Send failed:", error);
      return false;
    }
  }

  async stopNavigation(): Promise<boolean> {
    if (!this.deviceId) return false;

    try {
      await BluetoothSerial.write(JSON.stringify({ command: "stop" }) + "\n");
      console.log("Navigation stopped");
      return true;
    } catch (error) {
      console.error("Stop failed:", error);
      return false;
    }
  }

  async disconnect(): Promise<void> {
    if (this.deviceId) {
      await BluetoothSerial.disconnect();
      this.deviceId = null;
    }
  }
}

// Usage in Chronos app
const navigator = new ESP32Navigator();

// Connect
await navigator.connect();

// Send navigation update
await navigator.sendNavigation({
  instruction: "Turn left onto Main Street",
  distance: 250,
  street: "Main Street",
  angle: -90,
  destination: "123 Elm Street",
});

// Stop navigation
await navigator.stopNavigation();

// Disconnect
await navigator.disconnect();
```

### React Native Hook Example

```javascript
import { useEffect, useState } from "react";
import { BluetoothSerial } from "react-native-bluetooth-serial-next";

const useESP32Navigation = () => {
  const [connected, setConnected] = useState(false);
  const [deviceId, setDeviceId] = useState(null);

  const connect = async () => {
    try {
      const devices = await BluetoothSerial.list();
      const esp32 = devices.find((d) => d.name === "ESP32-Chronos-Nav");

      if (esp32) {
        await BluetoothSerial.connect(esp32.id);
        setDeviceId(esp32.id);
        setConnected(true);
        return true;
      }
      return false;
    } catch (error) {
      setConnected(false);
      return false;
    }
  };

  const sendNavigation = async (navData) => {
    if (!deviceId) return false;

    try {
      const json = JSON.stringify(navData);
      await BluetoothSerial.write(json + "\n");
      return true;
    } catch (error) {
      return false;
    }
  };

  useEffect(() => {
    // Auto-connect on mount
    connect();

    return () => {
      if (deviceId) {
        BluetoothSerial.disconnect();
      }
    };
  }, []);

  return { connected, sendNavigation, connect };
};

// Usage in component
function NavigationScreen() {
  const { connected, sendNavigation } = useESP32Navigation();

  const handleTurn = (instruction, distance, angle) => {
    sendNavigation({
      instruction,
      distance,
      angle,
      active: true,
    });
  };

  return (
    <View>
      <Text>ESP32: {connected ? "Connected" : "Disconnected"}</Text>
      {/* Your navigation UI */}
    </View>
  );
}
```

### Android (Java/Kotlin) Example

```kotlin
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothSocket
import java.io.OutputStream
import org.json.JSONObject

class ESP32Navigator {
    private var socket: BluetoothSocket? = null
    private var outputStream: OutputStream? = null

    fun connect(): Boolean {
        val bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
        val device = bluetoothAdapter.bondedDevices
            .find { it.name == "ESP32-Chronos-Nav" } ?: return false

        return try {
            val uuid = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
            socket = device.createRfcommSocketToServiceRecord(uuid)
            socket?.connect()
            outputStream = socket?.outputStream
            true
        } catch (e: Exception) {
            false
        }
    }

    fun sendNavigation(
        instruction: String,
        distance: Float,
        street: String = "",
        angle: Int = 0,
        destination: String = ""
    ): Boolean {
        val json = JSONObject().apply {
            put("instruction", instruction)
            put("distance", distance)
            put("street", street)
            put("angle", angle)
            put("active", true)
            if (destination.isNotEmpty()) put("destination", destination)
        }

        return try {
            outputStream?.write((json.toString() + "\n").toByteArray())
            outputStream?.flush()
            true
        } catch (e: Exception) {
            false
        }
    }

    fun stopNavigation(): Boolean {
        val json = JSONObject().apply {
            put("command", "stop")
        }

        return try {
            outputStream?.write((json.toString() + "\n").toByteArray())
            true
        } catch (e: Exception) {
            false
        }
    }

    fun disconnect() {
        socket?.close()
    }
}
```

## Update Frequency

- The ESP32 checks for stale data every second
- Navigation data expires after **30 seconds** without updates
- Recommended update frequency: **Every 1-3 seconds** during active navigation
- Send updates when:
  - Direction changes
  - Distance significantly changes (>10m)
  - Turn instruction changes

## Troubleshooting

### ESP32 not connecting to WiFi

1. Check credentials in `credentials.h`
2. Ensure WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
3. WiFi is only needed for time sync - navigation works without it!

### Bluetooth pairing issues

1. Ensure ESP32 is powered on and booted
2. Look for "ESP32-Chronos-Nav" in Bluetooth settings
3. Remove old pairings and re-pair
4. Check serial monitor for "Bluetooth Navigation Ready"

### Cannot send navigation data

1. Verify Bluetooth is paired and connected
2. Check that data is valid JSON with newline
3. Monitor serial output to see if data is received
4. Try the Python test script first

### Navigation not displaying

1. Check that navigation data is being received (serial monitor)
2. Verify JSON format matches specification
3. Ensure navigation hasn't timed out (30s limit)
4. Check distance value is > 0

## Serial Monitor Commands

Monitor the ESP32 via USB serial:

```bash
pio device monitor
```

**Output includes:**

- WiFi connection status
- IP address
- HTTP server requests
- Navigation data received
- Display updates
- Bluetooth status

## Power Consumption

- Active (WiFi + Display): ~120-150mA
- Deep sleep mode: Not implemented (always active)

For battery operation, consider:

- Use 3.7V LiPo battery (500-2000mAh)
- Runtime estimate: 3-13 hours depending on battery

## Future Enhancements

- [ ] WebSocket support for real-time updates
- [ ] Multi-step route preview
- [ ] Voice feedback via Bluetooth
- [ ] MQTT support
- [ ] Deep sleep mode when idle
- [ ] Battery level monitoring
- [ ] Route statistics tracking

## License

MIT License - Free to use and modify

## Support

For issues or questions:

1. Check serial monitor output
2. Verify API request format
3. Test with included Python example script
4. Check WiFi and network connectivity
