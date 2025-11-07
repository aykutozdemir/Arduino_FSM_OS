# MemoryUsage Library

Memory usage monitoring utilities for Arduino AVR boards.

## Features

- **Free RAM Calculation**: Calculate current free memory between heap and stack
- **Stack Usage Monitoring**: Track maximum stack usage with canary pattern
- **Memory Analysis**: Analyze SRAM memory layout and usage patterns
- **Stack Painting**: Fill unused memory with canary values for stack overflow detection
- **Minimum Free RAM Tracking**: Track the minimum free RAM observed during execution

## Installation

1. Download or clone this library into your Arduino `libraries` folder
2. Restart Arduino IDE

**Note**: This library is designed for AVR-based Arduino boards (Uno, Nano, Mega, etc.)

## Usage

### Basic Free RAM Check

```cpp
#include <MemoryUsage.h>

void setup() {
  Serial.begin(9600);
  
  // Calculate free RAM
  int free = MemoryUsage::freeRam();
  Serial.print("Free RAM: ");
  Serial.println(free);
}

void loop() {
  // Your code here
}
```

### Stack Usage Monitoring

```cpp
#include <MemoryUsage.h>

void setup() {
  Serial.begin(9600);
  
  // Paint the stack with canary pattern
  MemoryUsage::stackPaint();
  
  // Your initialization code here
  
  // Check minimum free RAM (maximum stack usage)
  int minFree = MemoryUsage::minimumFreeRam();
  Serial.print("Minimum free RAM: ");
  Serial.println(minFree);
}

void loop() {
  // Your code here
  
  // Periodically check current free RAM
  int free = MemoryUsage::freeRam();
  Serial.print("Current free RAM: ");
  Serial.println(free);
}
```

### Complete Memory Analysis

```cpp
#include <MemoryUsage.h>

void setup() {
  Serial.begin(9600);
  
  // Paint stack before starting
  MemoryUsage::stackPaint();
  
  // Your code that uses memory...
  
  // Analyze memory usage
  Serial.println("Memory Analysis:");
  Serial.print("Free RAM: ");
  Serial.println(MemoryUsage::freeRam());
  Serial.print("Minimum Free RAM: ");
  Serial.println(MemoryUsage::minimumFreeRam());
}

void loop() {
  // Your code here
}
```

## API Reference

### Static Methods

- `MemoryUsage::stackPaint()` - Fill unused memory with canary pattern (0xC5)
- `MemoryUsage::freeRam()` - Calculate current free RAM in bytes
- `MemoryUsage::minimumFreeRam()` - Get minimum free RAM observed since stackPaint()

## Memory Layout

The library understands AVR memory layout:

```
+---------------+------------------+---------------------------------------------+-----------------+
|               |                  |                                             |                 |
|    static     |       heap       |                   free ram                  |      stack      |
|     data      |                  |                                             |                 |
+---------------+------------------+---------------------------------------------+-----------------+
     _end or __heap_start     __brkval                                         SP             RAMEND
```

## How It Works

1. **Stack Painting**: Fills unused memory between heap and stack with a canary value (0xC5)
2. **Free RAM Calculation**: Calculates the difference between heap top (`__brkval`) and stack pointer (`SP`)
3. **Minimum Tracking**: Scans the painted area to find where the stack has grown, indicating maximum stack usage

## Notes

- Call `stackPaint()` early in `setup()` before significant memory allocation
- The canary pattern helps detect stack overflow
- Free RAM calculation is approximate and may vary slightly
- This library is AVR-specific and uses AVR memory layout symbols

## License

Copyright (c) 2015-2021 Locoduino.org. All right reserved.
Copyright (c) 2015-2021 Thierry Paris. All right reserved.

This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.

## Documentation

For complete documentation, see the `extras/Doc/index.html` file in your browser.

## Author

Original by Locoduino.org and Thierry Paris
Modified by Aykut ÖZDEMİR

