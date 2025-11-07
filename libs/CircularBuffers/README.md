# CircularBuffers Library

High-performance circular buffer implementations for Arduino, optimized for embedded systems with limited memory.

## Features

- **FastCircularQueue**: Template-based circular queue with fixed-size buffer
  - Power-of-2 buffer size for efficient modulo operations
  - Interrupt-safe operations
  - FIFO (First-In-First-Out) data structure
  - Overwrite mode support
  - Peek operations without removal

- **StringBuffer**: Efficient string buffer based on circular queue
  - Memory-efficient string handling
  - Support for C-strings, Flash strings, and Arduino Strings
  - String search and manipulation functions
  - Automatic overflow handling

## Installation

1. Download or clone this library into your Arduino `libraries` folder
2. Restart Arduino IDE

## Usage

### FastCircularQueue

```cpp
#include <FastCircularQueue.h>

// Create a circular queue with buffer size 16 (must be power of 2)
FastCircularQueue<int, 16> queue;

void setup() {
  Serial.begin(9600);
  
  // Push values
  queue.push(10);
  queue.push(20);
  queue.push(30);
  
  // Check if queue is full
  if (queue.isFull()) {
    Serial.println("Queue is full!");
  }
  
  // Pop values
  int value;
  while (!queue.isEmpty()) {
    if (queue.pop(value)) {
      Serial.println(value);
    }
  }
}

void loop() {
  // Your code here
}
```

### StringBuffer

```cpp
#include <StringBuffer.h>

// Create a string buffer with size 32 (must be power of 2)
StringBuffer<32> buffer;

void setup() {
  Serial.begin(9600);
  
  // Append strings
  buffer.append("Hello");
  buffer.append(" ");
  buffer.append(F("World"));  // Flash string
  
  // Convert to Arduino String
  String result = buffer.toString();
  Serial.println(result);
  
  // Search for substring
  int index = buffer.indexOf("World");
  if (index >= 0) {
    Serial.print("Found at index: ");
    Serial.println(index);
  }
  
  // Clear buffer
  buffer.clear();
}

void loop() {
  // Your code here
}
```

## API Reference

### FastCircularQueue

- `push(const T &value)` - Push a value to the queue (returns false if full)
- `pushOverwrite(const T &value)` - Push a value, overwriting oldest if full
- `pop(T &value)` - Pop a value from the queue (returns false if empty)
- `peek(T &value)` - Peek at next value without removing it
- `isEmpty()` - Check if queue is empty
- `isFull()` - Check if queue is full
- `available()` - Get number of elements in queue
- `clear()` - Clear all elements from queue

### StringBuffer

- `append(char c)` - Append a single character
- `append(const char *str)` - Append a C-string
- `append(const __FlashStringHelper *flashStr)` - Append a Flash string
- `append(const String &str)` - Append an Arduino String
- `indexOf(...)` - Find first occurrence of a string/character
- `endsWith(...)` - Check if buffer ends with a string
- `trim()` - Remove leading and trailing spaces
- `substring(int from, int to)` - Extract a substring
- `toCString(char *output, uint8_t maxSize)` - Copy to C-string
- `toString()` - Convert to Arduino String
- `size()` - Get number of characters in buffer
- `clear()` - Clear the buffer

## Requirements

- Buffer size must be a power of 2 (e.g., 2, 4, 8, 16, 32, 64, 128, 256)
- Maximum buffer size is 256 due to uint8_t index type

## License

This library is provided as-is for use with the FsmOS framework.

## Author

Aykut ÖZDEMİR

