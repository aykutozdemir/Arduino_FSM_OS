# Statistics Library

Simple statistics collection utility for timing measurements in Arduino applications.

## Features

- **Timing Measurements**: Measure execution time of code blocks
- **Statistics Tracking**: Track minimum, maximum, and average execution times
- **Exponential Moving Average**: Efficient average calculation with minimal memory
- **Memory Efficient**: Uses 16-bit values and exponential moving average
- **Convenient Macro**: MEASURE_TIME macro for easy code block timing
- **Flash String Support**: Names stored in program memory to save RAM

## Installation

1. Download or clone this library into your Arduino `libraries` folder
2. Restart Arduino IDE

## Usage

### Basic Timing Measurement

```cpp
#include <Statistic.h>

Statistic myStat;

void setup() {
  Serial.begin(9600);
  
  // Set a name for this statistic
  myStat.setName(F("MyFunction"));
  
  // Measure execution time
  myStat.start();
  
  // Your code to measure here
  delay(10);
  
  myStat.end();
  
  // Print statistics
  myStat.print(Serial);
}
```

### Using MEASURE_TIME Macro

```cpp
#include <Statistic.h>

Statistic loopTime;

void setup() {
  Serial.begin(9600);
  loopTime.setName(F("Loop"));
}

void loop() {
  // Measure execution time of this block
  MEASURE_TIME(loopTime) {
    // Code to measure
    delay(50);
    // More code...
  }
  
  // Print statistics every 100 iterations
  static int count = 0;
  if (++count >= 100) {
    count = 0;
    loopTime.print(Serial);
  }
}
```

### Multiple Statistics

```cpp
#include <Statistic.h>

Statistic functionA;
Statistic functionB;

void setup() {
  Serial.begin(9600);
  
  functionA.setName(F("FunctionA"));
  functionB.setName(F("FunctionB"));
}

void loop() {
  // Measure function A
  functionA.start();
  doSomethingA();
  functionA.end();
  
  // Measure function B
  functionB.start();
  doSomethingB();
  functionB.end();
  
  // Print all statistics
  functionA.print(Serial);
  functionB.print(Serial);
  
  delay(1000);
}
```

### Resetting Statistics

```cpp
#include <Statistic.h>

Statistic myStat;

void setup() {
  Serial.begin(9600);
  myStat.setName(F("Test"));
}

void loop() {
  // Collect some measurements
  for (int i = 0; i < 10; i++) {
    myStat.start();
    delayMicroseconds(100);
    myStat.end();
  }
  
  // Print statistics
  myStat.print(Serial);
  
  // Reset and start fresh
  myStat.reset();
  
  delay(1000);
}
```

## API Reference

### Constructor

- `Statistic()` - Create a new Statistic object

### Configuration

- `setName(const __FlashStringHelper *name)` - Set name for this statistic (stored in PROGMEM)

### Measurement

- `start()` - Mark the start of a timing measurement
- `end()` - Mark the end of a timing measurement and update statistics

### Statistics

- `reset()` - Reset all collected statistics
- `print(Print &output)` - Print statistics to Serial or other Print object

### Macro

- `MEASURE_TIME(statistic)` - Macro for measuring code block execution time

## Statistics Output Format

The `print()` method outputs statistics in the following format:

```
Name:min/average/max us
```

Example:
```
MyFunction:45/52/78 us
```

Where:
- **min**: Minimum measured time in microseconds
- **average**: Exponential moving average in microseconds
- **max**: Maximum measured time in microseconds

## How It Works

1. **Start Measurement**: Call `start()` to record the start time
2. **End Measurement**: Call `end()` to calculate elapsed time and update statistics
3. **Statistics Update**:
   - Minimum: Updated if new measurement is smaller
   - Maximum: Updated if new measurement is larger
   - Average: Calculated using exponential moving average (EMA)

## Exponential Moving Average

The library uses an exponential moving average with alpha = 4 (weight = 1/16) for efficient calculation:

```
average = average - (average >> 4) + (elapsed >> 4)
```

This provides a smooth average while using minimal memory and computation.

## Notes

- Times are measured in microseconds
- Uses 16-bit values, so maximum measurable time is ~65ms
- Timer wraparound is handled automatically
- Names should be stored in Flash memory using F() macro
- The MEASURE_TIME macro creates a scope, so variables declared inside are local

## Example: Performance Profiling

```cpp
#include <Statistic.h>

Statistic loopTime;
Statistic sensorRead;
Statistic dataProcess;

void setup() {
  Serial.begin(9600);
  
  loopTime.setName(F("Loop"));
  sensorRead.setName(F("SensorRead"));
  dataProcess.setName(F("DataProcess"));
}

void loop() {
  loopTime.start();
  
  sensorRead.start();
  int value = analogRead(A0);
  sensorRead.end();
  
  dataProcess.start();
  processData(value);
  dataProcess.end();
  
  loopTime.end();
  
  // Print statistics every second
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
    loopTime.print(Serial);
    sensorRead.print(Serial);
    dataProcess.print(Serial);
  }
}
```

## License

This library is provided as-is for use with the FsmOS framework.

## Author

Aykut ÖZDEMİR

