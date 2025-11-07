# FastPin Library

High-performance digital pin operations for Arduino using direct port manipulation.

## Features

- **Direct Port Access**: Uses port registers for faster operations
- **Significant Performance Gain**: Up to 10x faster than standard `digitalWrite()`/`digitalRead()`
- **Static Methods**: Can be used without instantiating objects
- **Instance Methods**: Object-oriented interface for pin management
- **Pull-up Support**: Built-in pull-up resistor configuration

## Installation

1. Download or clone this library into your Arduino `libraries` folder
2. Restart Arduino IDE

## Usage

### Instance-based Usage

```cpp
#include <FastPin.h>

// Create a FastPin object for pin 13 as output
FastPin ledPin(13, true);  // true = output

void setup() {
  // Pin is automatically configured as output
}

void loop() {
  ledPin.high();    // Turn on LED (faster than digitalWrite)
  delay(100);
  ledPin.low();     // Turn off LED
  delay(100);
  ledPin.toggle();  // Toggle LED state
  delay(100);
}
```

### Static Method Usage (Maximum Performance)

```cpp
#include <FastPin.h>

// Get port registers for pin 13
const uint8_t pin = 13;
const uint8_t portNum = digitalPinToPort(pin);
volatile uint8_t *port = portOutputRegister(portNum);
volatile uint8_t *pinReg = portInputRegister(portNum);
const uint8_t bitMask = digitalPinToBitMask(pin);

void setup() {
  pinMode(pin, OUTPUT);
}

void loop() {
  FastPin::high(port, bitMask);   // Set pin HIGH
  delay(100);
  FastPin::low(port, bitMask);    // Set pin LOW
  delay(100);
  FastPin::toggle(port, bitMask); // Toggle pin
  delay(100);
  
  // Read pin state
  uint8_t state = FastPin::read(pinReg, bitMask);
}
```

### Input Pin with Pull-up

```cpp
#include <FastPin.h>

// Create input pin with pull-up resistor
FastPin buttonPin(2, false, true);  // false = input, true = pullup

void setup() {
  Serial.begin(9600);
}

void loop() {
  uint8_t state = buttonPin.read();
  if (state == 1) {
    Serial.println("Button pressed");
  }
  delay(10);
}
```

## API Reference

### Constructor

- `FastPin(uint8_t pin, bool isOutput, bool pullup = false)` - Create a FastPin object

### Instance Methods

- `high()` - Set pin to HIGH state
- `low()` - Set pin to LOW state
- `toggle()` - Toggle pin state
- `set(uint8_t value)` - Set pin to specific state (0=LOW, non-zero=HIGH)
- `read()` - Read pin state (returns 0 or 1)
- `setMode(bool isOutput, bool pullup = false)` - Change pin mode

### Static Methods

- `FastPin::high(volatile uint8_t *port, uint8_t bitMask)` - Set pin HIGH
- `FastPin::low(volatile uint8_t *port, uint8_t bitMask)` - Set pin LOW
- `FastPin::toggle(volatile uint8_t *port, uint8_t bitMask)` - Toggle pin
- `FastPin::set(volatile uint8_t *port, uint8_t bitMask, uint8_t value)` - Set pin state
- `FastPin::read(volatile const uint8_t *pinReg, uint8_t bitMask)` - Read pin state

## Performance Notes

- Static methods provide maximum performance (no object overhead)
- Instance methods are more convenient but slightly slower
- Direct port manipulation is significantly faster than Arduino's `digitalWrite()`/`digitalRead()`
- Use static methods in time-critical ISRs or tight loops

## Platform Support

This library is optimized for AVR-based Arduino boards (Uno, Nano, Mega, etc.).
For other platforms, it falls back to standard Arduino functions.

## License

This library is provided as-is for use with the FsmOS framework.

## Author

Aykut ÖZDEMİR

