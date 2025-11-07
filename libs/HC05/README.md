# HC05 Library

Bluetooth HC-05 module driver for Arduino with comprehensive AT command and data mode support.

## Features

- **Dual Mode Operation**: Supports both AT command mode and data mode
- **Automatic State Management**: Handles module initialization and mode switching
- **Command Queue**: Queue multiple AT commands for sequential execution
- **Connection Monitoring**: Monitor connection state via STATE pin
- **Callback Support**: Receive callbacks for command responses and data
- **Reset Capabilities**: Support for both soft and permanent resets
- **Error Handling**: Comprehensive error handling and timeout management

## Installation

1. Download or clone this library into your Arduino `libraries` folder
2. Install required dependencies:
   - StringBuffer (from CircularBuffers)
   - ArduinoQueue
   - SimpleTimer
   - Utilities
3. Restart Arduino IDE

## Hardware Connections

Connect your HC-05 module as follows:

- **VCC** → 5V (or 3.3V for some modules)
- **GND** → GND
- **TXD** → Arduino RX pin (e.g., pin 10)
- **RXD** → Arduino TX pin (e.g., pin 11)
- **KEY/EN** → Arduino digital pin (e.g., pin 9)
- **STATE** → Arduino digital pin (e.g., pin 8) - optional but recommended
- **RESET** → Arduino digital pin (e.g., pin 7) - optional

## Usage

### Basic Setup

```cpp
#include <HC05.h>
#include <SoftwareSerial.h>

// Create software serial for HC-05 communication
SoftwareSerial hc05Serial(10, 11);  // RX, TX

// Create HC05 object
// Parameters: stream, keyPin, statePin, resetPin
HC05 hc05(hc05Serial, 9, 8, 7);

void setup() {
  Serial.begin(9600);
  hc05Serial.begin(9600);
  
  // Initialize HC-05 module
  hc05.begin();
}

void loop() {
  // Must call loop() regularly
  hc05.loop();
  
  // Your code here
}
```

### Sending AT Commands

```cpp
#include <HC05.h>
#include <SoftwareSerial.h>

SoftwareSerial hc05Serial(10, 11);
HC05 hc05(hc05Serial, 9, 8, 7);

// Command callback function
void onCommandResponse(const __FlashStringHelper *command, bool success, const String &response) {
  Serial.print(F("Command: "));
  Serial.print(command);
  Serial.print(success ? F(" - Success") : F(" - Failed"));
  Serial.print(F(" - Response: "));
  Serial.println(response);
}

void setup() {
  Serial.begin(9600);
  hc05Serial.begin(9600);
  hc05.begin();
  
  delay(2000);  // Wait for initialization
  
  // Send AT command
  HC05::Command cmd;
  cmd.commandText = F("AT");
  cmd.callback = onCommandResponse;
  cmd.delayMs = 200;
  
  hc05.sendCommand(cmd);
}

void loop() {
  hc05.loop();
}
```

### Data Mode Communication

```cpp
#include <HC05.h>
#include <SoftwareSerial.h>

SoftwareSerial hc05Serial(10, 11);
HC05 hc05(hc05Serial, 9, 8, 7);

// Data received callback
void onDataReceived(const char data) {
  Serial.print(data);
}

void setup() {
  Serial.begin(9600);
  hc05Serial.begin(9600);
  hc05.begin();
  
  // Set data received callback
  hc05.onDataReceived(onDataReceived);
  
  delay(2000);
  
  // Force into data mode
  hc05.forceDataMode();
}

void loop() {
  hc05.loop();
  
  // Send data to connected device
  if (hc05.isConnected() && hc05.isDataMode()) {
    hc05.sendData("Hello from Arduino!\n");
    delay(1000);
  }
}
```

## API Reference

### Constructor

- `HC05(Stream &stream, uint8_t keyPin, uint8_t statePin, uint8_t resetPin)` - Create HC05 object

### Initialization

- `begin()` - Initialize the HC-05 module

### Command Mode

- `sendCommand(const Command &command)` - Send an AT command
- `clearCommandQueue()` - Clear all pending commands

### Data Mode

- `sendData(const String &data)` - Send string data
- `sendData(const char data)` - Send single character
- `onDataReceived(DataCallback callback)` - Set data received callback

### Control

- `reset(bool permanent = false)` - Reset the module
- `forceDataMode()` - Force module into data mode
- `isResettingPermanently()` - Check if permanent reset is in progress

### Status

- `isConnected()` - Check if module is connected
- `isDataMode()` - Check if module is in data mode

### Main Loop

- `loop()` - Process module state machine (must be called regularly)

## Command Structure

```cpp
struct Command {
  const __FlashStringHelper *commandText;  // Command to send (e.g., F("AT"))
  CommandCallback callback;                // Callback function
  uint16_t delayMs;                        // Delay after command (ms)
};
```

## State Enumeration

The library uses an internal state machine with the following states:

- `IDLE` - Module is idle
- `WAITING_FOR_RESPONSE` - Waiting for command response
- `DATA_MODE` - Module is in data mode
- `RESETTING` - Module is resetting
- `INITIALIZING` - Module is initializing
- And more...

## Notes

- Always call `loop()` regularly in your main loop
- Commands are queued and executed sequentially
- Use Flash strings (F("...")) for command text to save RAM
- STATE pin monitoring requires proper hardware connection
- Some HC-05 modules may require different baud rates

## License

This library is provided as-is for use with the FsmOS framework.

## Author

Aykut ÖZDEMİR

