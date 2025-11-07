# Integrated Libraries

This directory contains external libraries integrated with the FsmOS framework.

## Currently Integrated Libraries

### ArduinoMap (v1.0.0)
**Source:** [K810_Security/lib/ArduinoMap](https://github.com/aykutozdemir/K810_Security/tree/main/lib/ArduinoMap)

A lightweight, templated map implementation for Arduino providing a dynamic key-value store.

**Features:**
- Template-based implementation for type safety
- Dynamic memory allocation
- Iterator support for range-based for loops
- Copy constructor and assignment operator
- Memory efficient linked list implementation

**Usage:**
```cpp
#include <ArduinoMap.h>

ArduinoMap<String, int> map;
map.insert("hello", 42);
int* value = map.get("hello");
```

**Location:** `libs/ArduinoMap/`

---

### ArduinoQueue (v1.2.5)
**Source:** [K810_Security/lib/ArduinoQueue](https://github.com/aykutozdemir/K810_Security/tree/main/lib/ArduinoQueue)

A lightweight linked list type queue implementation designed for microcontrollers. Provides standard FIFO operations with minimal memory overhead.

**Features:**
- Template-based implementation for any data type
- Dynamic memory allocation
- Configurable maximum items and memory limits
- Standard queue operations (enqueue, dequeue)
- Head/tail access without removal
- Memory efficient linked list implementation

**Usage:**
```cpp
#include <ArduinoQueue.h>

ArduinoQueue<int> intQueue(20);  // Queue of 20 items
intQueue.enqueue(1);
intQueue.enqueue(123);
int number = intQueue.dequeue();  // Returns 1
int head = intQueue.getHead();    // Returns 123 without removing
```

**Location:** `libs/ArduinoQueue/`

---

### BitBool (v1.2.0)
**Source:** [K810_Security/lib/BitBool](https://github.com/aykutozdemir/K810_Security/tree/main/lib/BitBool)

An efficient bit manipulation library that provides a drop-in replacement for bool arrays. Perfect for embedded systems where memory efficiency is critical.

**Features:**
- Drop-in replacement for bool/boolean arrays
- Memory efficient (8 bits per byte instead of 8 bytes per bool)
- Array subscript notation for bit access
- Iterator support for range-based loops
- Bit/byte order reversal options
- Lookup table optimization option

**Usage:**
```cpp
#include <BitBool.h>

// Create a BitBool array of 100 bits (uses only 13 bytes instead of 100)
BitBool<100> flags;

// Use like a normal bool array
flags[0] = true;
flags[1] = false;
bool value = flags[5];

// Iterate through bits
for (auto& bit : flags) {
    bit = !bit;  // Invert each bit
}
```

**Location:** `libs/BitBool/`

---

### BufferedStreams (v1.0.8)
**Source:** [K810_Security/lib/BufferedStreams](https://github.com/aykutozdemir/K810_Security/tree/main/lib/BufferedStreams)

Implementation of Arduino's Stream class using internal ring buffers. Provides LoopbackStream for buffering and PipedStream for bidirectional communication between components.

**Features:**
- LoopbackStream: Buffers data internally and loops it back for reading
- PipedStream: Bidirectional pipe streams for component communication
- PipedStreamPair: Creates connected pairs of streams
- Compatible with Arduino Stream API
- Configurable buffer sizes
- Memory efficient ring buffer implementation

**Usage:**
```cpp
#include <LoopbackStream.h>
#include <PipedStream.h>

// Loopback stream - data written can be read back
LoopbackStream loopback(64);
loopback.write("Hello");
char c = loopback.read();  // Returns 'H'

// Piped streams - bidirectional communication
PipedStreamPair pipes(128);
pipes.first.write("Data");
int data = pipes.second.read();  // Reads "Data"
```

**Location:** `libs/BufferedStreams/`

---

### CircularBuffers
**Source:** [K810_Security/lib/CircularBuffers](https://github.com/aykutozdemir/K810_Security/tree/main/lib/CircularBuffers)

High-performance circular buffer implementations optimized for embedded systems. Includes FastCircularQueue for efficient FIFO operations and StringBuffer for string manipulation.

**Features:**
- FastCircularQueue: High-performance circular queue with fixed size buffer
- StringBuffer: Efficient string manipulation buffer based on circular queue
- Power-of-2 buffer sizes for optimal performance
- Interrupt-safe operations
- Template-based implementation for any data type
- Memory efficient fixed-size buffers

**Usage:**
```cpp
#include <FastCircularQueue.h>
#include <StringBuffer.h>

// Fast circular queue
FastCircularQueue<int, 64> queue;  // Buffer size must be power of 2
queue.push(42);
int value;
queue.pop(value);

// String buffer
StringBuffer<128> buffer;
buffer.append("Hello");
buffer.append(" World");
String str = buffer.toString();
```

**Location:** `libs/CircularBuffers/`

---

### FastPin
**Source:** [K810_Security/lib/FastPin](https://github.com/aykutozdemir/K810_Security/tree/main/lib/FastPin)

High-performance digital pin manipulation library using direct port register access. Provides faster read/write operations compared to standard Arduino digital I/O functions.

**Features:**
- Direct port manipulation for maximum performance
- Faster than standard digitalWrite/digitalRead
- Static methods for direct port access
- Instance methods for object-oriented usage
- Support for input/output modes
- Pull-up resistor configuration

**Usage:**
```cpp
#include <FastPin.h>

// Create a FastPin instance
FastPin ledPin(13, true);  // Pin 13, output mode
ledPin.high();
ledPin.low();
ledPin.toggle();

// Or use static methods for direct port access
FastPin::high(port, bitMask);
uint8_t value = FastPin::read(pinReg, bitMask);
```

**Location:** `libs/FastPin/`

---

### SafeInterrupts
**Source:** [K810_Security/lib/SafeInterrupts](https://github.com/aykutozdemir/K810_Security/tree/main/lib/SafeInterrupts)

Safe interrupt handling utilities for Arduino with nesting support. Provides RAII-based scoped interrupt disabling to prevent issues in critical sections of code.

**Features:**
- Nesting support for interrupt disable/enable
- RAII scoped interrupt disabler
- Safe replacement for cli()/sei() macros
- Automatic state restoration
- Thread-safe interrupt management

**Usage:**
```cpp
#include <SafeInterrupts.h>

// Manual control with nesting support
SafeInterrupts::disable();
// Critical section code
SafeInterrupts::enable();

// RAII scoped disable (automatic)
{
    SafeInterrupts::ScopedDisable disable;
    // Critical section code
    // Interrupts automatically re-enabled when scope exits
}

// Or use macros (replaces standard cli/sei)
cli();  // Safe disable with nesting
// Critical code
sei();  // Safe enable with nesting
```

**Location:** `libs/SafeInterrupts/`

---

### SimpleTimer (v1.0.0)
**Source:** [K810_Security/lib/SimpleTimer](https://github.com/aykutozdemir/K810_Security/tree/main/lib/SimpleTimer)

A simple, lightweight timer utility for managing timing operations in Arduino applications. Uses millis() for timing calculations and supports customizable intervals.

**Features:**
- Template-based implementation for flexible time types
- Simple interval-based timing
- Lightweight and memory efficient
- Easy to use API
- Automatic overflow handling

**Usage:**
```cpp
#include <SimpleTimer.h>

// Create a timer with 5 second interval
SimpleTimer timer(5000);

// Or create and set interval later
SimpleTimer timer2;
timer2.setInterval(3000);

void loop() {
    if (timer.isReady()) {
        // Do something every 5 seconds
        timer.reset();  // Reset for next interval
    }
}
```

**Location:** `libs/SimpleTimer/`

---

### ezButton (v1.0.6)
**Source:** [K810_Security/lib/ezButton](https://github.com/aykutozdemir/K810_Security/tree/main/lib/ezButton)

Easy-to-use button handling library with built-in debouncing, pressed/released events, and press counting. Designed for push-buttons, momentary switches, toggle switches, and magnetic contact switches.

**Features:**
- Built-in debounce to eliminate chattering
- Pressed and released events
- Press counting (FALLING, RISING, BOTH)
- Internal/external pull-up/pull-down support
- Non-blocking functions
- Easy to use with multiple buttons

**Usage:**
```cpp
#include <ezButton.h>

ezButton button(2);  // Pin 2 with internal pull-up

void setup() {
    button.setDebounceTime(50);
}

void loop() {
    button.loop();
    
    if (button.isPressed()) {
        // Button was pressed
    }
    
    if (button.isReleased()) {
        // Button was released
    }
}
```

**Location:** `libs/ezButton/`

---

### ezLED (v1.0.1)
**Source:** [K810_Security/lib/ezLED](https://github.com/aykutozdemir/K810_Security/tree/main/lib/ezLED)

Easy-to-use LED control library supporting on/off, toggle, fade in/out, blink, blink in period, and blink a number of times. All functions are non-blocking.

**Features:**
- Turn on/off with optional delay
- Toggle between on and off
- Fade in/out with customizable timing
- Blink with configurable timing
- Blink a specific number of times
- Blink in a period of time
- Cancel blinking/fading anytime
- Support for anode and cathode control modes
- Non-blocking functions

**Usage:**
```cpp
#include <ezLED.h>

ezLED led(13);  // Pin 13

void setup() {
    led.turnON();
}

void loop() {
    led.loop();
    
    // Blink every 500ms
    led.blink(250, 250);
    
    // Fade in/out
    led.fade(0, 255, 1000);
}
```

**Location:** `libs/ezLED/`

---

### ezOutput (v1.2.0)
**Source:** [K810_Security/lib/ezOutput](https://github.com/aykutozdemir/K810_Security/tree/main/lib/ezOutput)

Easy-to-use output control library for digital pins. Supports HIGH, LOW, TOGGLE, PULSE, and BLINK_WITHOUT_DELAY. Perfect for controlling LEDs, relays, and other digital outputs.

**Features:**
- HIGH, LOW, TOGGLE, PULSE operations
- Blink without delay
- Get output pin state
- Time offset support for multiple outputs
- Non-blocking functions
- Easy to use with multiple output pins

**Usage:**
```cpp
#include <ezOutput.h>

ezOutput output(13);  // Pin 13

void setup() {
    output.high();
}

void loop() {
    output.loop();
    
    // Toggle every second
    output.toggle(1000);
    
    // Blink without delay
    output.blink(500, 500);
    
    // Pulse for 100ms
    output.pulse(100);
}
```

**Location:** `libs/ezOutput/`

---

### Statistics
**Source:** [K810_Security/lib/Statistics](https://github.com/aykutozdemir/K810_Security/tree/main/lib/Statistics)

Simple statistics collection utility for timing measurements. Provides functionality for measuring execution times and tracking minimum, maximum, and average times using exponential moving average.

**Features:**
- Measure execution times in microseconds
- Track minimum, maximum, and average times
- Exponential moving average for memory efficiency
- Timer wraparound handling
- Convenient macro for measuring code blocks
- Print statistics to any Print object

**Usage:**
```cpp
#include <Statistic.h>

Statistic stats;
stats.setName(F("TaskExecution"));

// Manual measurement
stats.start();
// ... code to measure ...
stats.end();

// Or use macro for automatic measurement
MEASURE_TIME(stats) {
    // Code to measure
}

// Print statistics
stats.print(Serial);  // Output: TaskExecution:100/150/200 us
```

**Location:** `libs/Statistics/`

---

### Utilities (v1.0.0)
**Source:** [K810_Security/lib/Utilities](https://github.com/aykutozdemir/K810_Security/tree/main/lib/Utilities)

Common utility functions and base classes to reduce code duplication and flash usage. Provides secure memory operations, serial output formatting, hex string conversion, debug tracing, and a base driver class.

**Features:**
- Secure memory operations (cleaning, constant-time comparison)
- Serial output formatting (OK/error messages, hex values)
- Hex string conversion utilities
- Safe interrupt handling wrappers
- Traceable debug logging system with multiple levels
- DriverBase class for standardized driver implementations
- State management helpers

**Usage:**
```cpp
#include <Utilities.h>
#include <Traceable.h>
#include <DriverBase.h>

// Utility functions
byte data[32];
Utilities::secureClean(data, sizeof(data));
bool isEqual = Utilities::constantTimeCompare(data1, data2, 32);
Utilities::printOK(Serial);
Utilities::printHexArray(Serial, byteArray, 16, ':');

// Traceable debug logging
Traceable trace(F("MyFunction"), Traceable::Level::DEBUG);
trace.printDebug(__LINE__) << F("Debug message") << endl;

// DriverBase for controllers
class MyDriver : public DriverBase {
    DriverBase::StateManager<State> stateManager;
public:
    MyDriver() : DriverBase(F("MyDriver"), Level::INFO), 
                 stateManager(IDLE) {}
    void loop() override {
        if (stateManager.isStateTimeElapsed(1000)) {
            stateManager.setState(RUNNING);
        }
    }
};
```

**Dependencies:** SafeInterrupts

**Location:** `libs/Utilities/`

---

### StaticSerialCommands (v1.2.0)
**Source:** [K810_Security/lib/StaticSerialCommands](https://github.com/aykutozdemir/K810_Security/tree/main/lib/StaticSerialCommands)

An Arduino library for parsing commands received over a serial port. Optimized for low dynamic memory usage by storing commands in program memory. Supports typed arguments, subcommands, and friendly error messages.

**Features:**
- Typed arguments with strict input validation (Int, Float, String)
- Friendly error messages for invalid input
- Customizable delimiter, termination, and quotation characters
- Commands can have subcommands
- Methods to list commands with syntax and description
- Low dynamic memory usage (commands stored in program memory)
- Argument range validation
- Timeout support

**Usage:**
```cpp
#include <StaticSerialCommands.h>

void cmd_help(SerialCommands& sender, Args& args) {
    sender.listCommands();
}

void cmd_multiply(SerialCommands& sender, Args& args) {
    auto n1 = args[0].getInt();
    auto n2 = args[1].getInt();
    sender.getSerial().println(n1 * n2);
}

Command commands[] {
    COMMAND(cmd_help, "help"),
    COMMAND(cmd_multiply, "mul", ArgType::Int, ArgType::Int, nullptr, "multiply two numbers"),
};

SerialCommands serialCommands(Serial, commands, sizeof(commands) / sizeof(Command));

void loop() {
    serialCommands.readSerial();
}
```

**Location:** `libs/StaticSerialCommands/`

---

### HC05
**Source:** [K810_Security/lib/HC05](https://github.com/aykutozdemir/K810_Security/tree/main/lib/HC05)

Bluetooth HC-05 module driver. Provides an interface for communicating with HC-05 Bluetooth modules, supporting both AT command mode for configuration and data mode for communication with paired devices.

**Features:**
- AT command mode for configuration
- Data mode for communication with paired devices
- Command queue management
- Connection state monitoring
- Reset functionality (normal and permanent)
- Callback support for command responses and received data
- State machine-based operation

**Usage:**
```cpp
#include <HC05.h>

HC05 hc05(Serial1, KEY_PIN, STATE_PIN, RESET_PIN);

void onDataReceived(const char data) {
    Serial.print(data);
}

void setup() {
    hc05.begin();
    hc05.onDataReceived(onDataReceived);
}

void loop() {
    hc05.loop();
    hc05.sendData("Hello");
}
```

**Dependencies:** StringBuffer, ArduinoQueue, SimpleTimer, Utilities, DriverBase

**Location:** `libs/HC05/`

---

### I2C-master
**Source:** [K810_Security/lib/I2C-master](https://github.com/aykutozdemir/K810_Security/tree/main/lib/I2C-master)

Enhanced I2C/TWI library for Arduino. Provides more functionality than the standard Wire library, including support for repeated start conditions and bus timeout detection.

**Features:**
- Support for repeated start conditions
- Bus timeout detection and recovery
- Bus scanning functionality
- 8-bit and 16-bit register addressing
- Multiple data type support (uint8_t, uint16_t, uint32_t, uint64_t, strings, buffers)
- Low-level I2C methods for custom protocols
- Configurable bus speed (100kHz/400kHz)
- Pullup resistor control

**Usage:**
```cpp
#include <I2C.h>

I2c.begin();
I2c.setSpeed(1);  // 400kHz

// Write to device
I2c.write(0x50, 0x00, 0x42);

// Read from device
uint8_t data[4];
I2c.read(0x50, 0x00, 4, data);

// Scan bus
I2c.scan();
```

**Dependencies:** Traceable, SimpleTimer

**Location:** `libs/I2C-master/`

---

### MemoryUsage (v2.21.1)
**Source:** [K810_Security/lib/MemoryUsage](https://github.com/aykutozdemir/K810_Security/tree/main/lib/MemoryUsage)

Memory usage monitoring utilities for Arduino. Helps monitor and analyze SRAM memory usage on AVR-based Arduino boards.

**Features:**
- Calculate current free RAM
- Calculate minimum free RAM using stack painting
- Display detailed RAM usage information
- Stack usage monitoring with canary pattern
- Memory layout visualization

**Usage:**
```cpp
#include <MemoryUsage.h>

void setup() {
    MemoryUsage::stackPaint();  // Paint stack with canary pattern
}

void loop() {
    int free = MemoryUsage::freeRam();
    int minFree = MemoryUsage::minimumFreeRam();
    MemoryUsage::ramDisplay(Serial);
}
```

**Architectures:** AVR only

**Location:** `libs/MemoryUsage/`

---

### Packager
**Source:** [K810_Security/lib/Packager](https://github.com/aykutozdemir/K810_Security/tree/main/lib/Packager)

Abstract interface for packet-based communication protocols. Provides a base interface for implementing various packet-based communication protocols with support for encoding/decoding.

**Features:**
- Abstract interface for packet protocols
- Plain and encoded stream management
- Piped stream support for bidirectional communication
- Maximum packet size control
- Stream clearing functionality

**Usage:**
```cpp
#include <PackageInterface.h>
#include <CRCPackageInterface.h>

PipedStreamPair streams(128);
CRCPackageInterface packager(streams, 64);

void loop() {
    packager.loop();
    // Use packager.getPlainStream() and packager.getEncodedStream()
}
```

**Dependencies:** PipedStream (from BufferedStreams)

**Location:** `libs/Packager/`

---

### SoftSerial
**Source:** [K810_Security/lib/SoftSerial](https://github.com/aykutozdemir/K810_Security/tree/main/lib/SoftSerial)

Software serial communication implementation for Arduino. Provides software-based asynchronous serial communication without requiring hardware UART.

**Features:**
- Software-based serial communication
- Configurable baud rates (1200-115200)
- Parity modes (none, even, odd)
- Configurable stop bits
- Efficient bit manipulation
- Interrupt-based operation
- RX and TX buffer queues

**Usage:**
```cpp
#include <SoftSerial.h>

SoftSerial<64, 64> softSerial(2, 3);  // RX pin 2, TX pin 3

void setup() {
    softSerial.begin(setupTimer, BAUD_9600, 1, NONE);
}

void loop() {
    softSerial.loop();
    if (softSerial.available()) {
        char c = softSerial.read();
    }
    softSerial.write('A');
}
```

**Dependencies:** FastPin, FastCircularQueue, DriverBase

**Location:** `libs/SoftSerial/`

---

## Adding a New Library

1. Clone or copy the library repository into a subdirectory here
2. Update this README to document the new library
3. Ensure the library is compatible with FsmOS architecture
4. Test integration with FsmOS examples

## Library Structure

Each library should follow Arduino library conventions:
- Header files in root or `src/` directory
- Implementation files in root or `src/` directory
- `library.properties` file for Arduino IDE integration
- `keywords.txt` for syntax highlighting (optional)
- `README.md` with documentation (optional)
