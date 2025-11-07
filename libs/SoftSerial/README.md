# SoftSerial Library

Software serial communication implementation for Arduino with configurable baud rates and parity modes.

## Features

- **Software-Based**: No hardware UART required
- **Configurable Baud Rates**: 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
- **Parity Support**: None, Even, or Odd parity
- **Configurable Stop Bits**: 1-3 stop bits
- **Interrupt-Based**: Efficient interrupt-driven operation
- **Stream Compatible**: Implements Arduino Stream interface
- **Buffer Management**: Separate RX and TX buffers with configurable sizes

## Installation

1. Download or clone this library into your Arduino `libraries` folder
2. Install required dependencies:
   - FastPin
   - CircularBuffers (FastCircularQueue)
   - Utilities (DriverBase)
3. Restart Arduino IDE

## Hardware Setup

Connect your devices as follows:

- **RX Pin**: Connect to TX of the other device
- **TX Pin**: Connect to RX of the other device
- **GND**: Common ground connection

## Usage

### Basic Setup

```cpp
#include <SoftSerial.h>

// Create SoftSerial with RX buffer size 64 and TX buffer size 64
// Parameters: rxPin, txPin
SoftSerial<64, 64> softSerial(2, 3);

// Timer setup callback (required for interrupt-based operation)
void setupTimer(unsigned long period) {
  // Configure your timer interrupt with the given period
  // This example uses Timer1 (adjust for your board)
  // For Arduino Uno/Nano, you might use Timer1 library
  // Timer1.initialize(period);
  // Timer1.attachInterrupt([]() { softSerial.processISR(); });
}

void setup() {
  Serial.begin(9600);
  
  // Initialize SoftSerial
  // Parameters: timerSetupCallback, baudRate, stopBits, parity
  softSerial.begin(setupTimer, BAUD_9600, 1, NONE);
  
  Serial.println("SoftSerial initialized");
}

void loop() {
  // Must call loop() regularly
  softSerial.loop();
  
  // Read available data
  if (softSerial.available()) {
    char c = softSerial.read();
    Serial.print("Received: ");
    Serial.println(c);
  }
  
  // Send data
  if (Serial.available()) {
    char c = Serial.read();
    softSerial.write(c);
  }
}
```

### With Parity and Stop Bits

```cpp
#include <SoftSerial.h>

SoftSerial<64, 64> softSerial(2, 3);

void setupTimer(unsigned long period) {
  // Configure timer interrupt
}

void setup() {
  Serial.begin(9600);
  
  // Initialize with even parity and 2 stop bits
  softSerial.begin(setupTimer, BAUD_9600, 2, EVEN);
}

void loop() {
  softSerial.loop();
  
  // Your communication code here
}
```

### Loopback Test

```cpp
#include <SoftSerial.h>

SoftSerial<64, 64> softSerial(2, 3);

void setupTimer(unsigned long period) {
  // Configure timer interrupt
}

void setup() {
  Serial.begin(9600);
  softSerial.begin(setupTimer, BAUD_9600);
  
  Serial.println("SoftSerial Loopback Test");
  Serial.println("Type characters to send via SoftSerial");
}

void loop() {
  softSerial.loop();
  
  // Echo received data back
  if (softSerial.available()) {
    char c = softSerial.read();
    Serial.print("Echo: ");
    Serial.println(c);
    softSerial.write(c);  // Echo back
  }
  
  // Send data from Serial to SoftSerial
  if (Serial.available()) {
    char c = Serial.read();
    softSerial.write(c);
  }
}
```

## API Reference

### Template Parameters

- `RX_BUFFER_SIZE`: Size of receive buffer (must be power of 2)
- `TX_BUFFER_SIZE`: Size of transmit buffer (must be power of 2)

### Constructor

- `SoftSerial(uint8_t rxPin, uint8_t txPin)` - Create SoftSerial object

### Initialization

- `begin(TimerSetupCallback callback, BaudRate baudRate, uint8_t stopBits = 1, ParityMode parity = NONE)` - Initialize SoftSerial
- `end()` - End SoftSerial communication

### Stream Interface

- `available()` - Get number of bytes available for reading
- `read()` - Read a byte from RX buffer
- `peek()` - Peek at next byte without removing it
- `write(uint8_t data)` - Write a byte to TX buffer
- `flush()` - Flush TX buffer

### Interrupt Handling

- `processISR()` - Process RX/TX bits (call from timer interrupt)

### Main Loop

- `loop()` - Process RX/TX operations (must be called regularly)

## Baud Rate Enumeration

- `BAUD_1200` - 1200 baud
- `BAUD_2400` - 2400 baud
- `BAUD_4800` - 4800 baud
- `BAUD_9600` - 9600 baud (default)
- `BAUD_19200` - 19200 baud
- `BAUD_38400` - 38400 baud
- `BAUD_57600` - 57600 baud
- `BAUD_115200` - 115200 baud

## Parity Mode Enumeration

- `NONE` - No parity (default)
- `EVEN` - Even parity
- `ODD` - Odd parity

## Timer Setup Callback

The timer setup callback receives the bit period in microseconds and should configure a timer interrupt to call `processISR()` at that rate.

Example for Arduino Uno/Nano with Timer1:

```cpp
#include <TimerOne.h>

void setupTimer(unsigned long period) {
  Timer1.initialize(period);
  Timer1.attachInterrupt([]() { softSerial.processISR(); });
}
```

## Notes

- Buffer sizes must be powers of 2 (e.g., 16, 32, 64, 128, 256)
- Maximum buffer size is 256
- Timer interrupt must be configured correctly for reliable communication
- Higher baud rates require more CPU time
- Use hardware UART when possible for better performance

## License

This library is provided as-is for use with the FsmOS framework.

## Author

Aykut ÖZDEMİR

