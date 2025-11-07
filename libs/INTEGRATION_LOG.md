# FsmOS Framework - Library Integration Summary

## ArduinoMap Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/ArduinoMap  
**Status:** Successfully integrated

### Library Details
- **Name:** ArduinoMap
- **Version:** 1.0.0
- **Author:** Aykut ÖZDEMİR
- **Category:** Data Storage
- **Architectures:** All (*)

### Description
A lightweight, templated map implementation for Arduino providing a dynamic key-value store that can work with any data type.

### Features
- Template-based implementation for type safety
- Dynamic memory allocation
- Iterator support for range-based for loops
- Copy constructor and assignment operator
- Memory efficient linked list implementation
- Thread-safe for single-threaded operations

### Files Integrated
```
libs/ArduinoMap/
├── ArduinoMap.h          # Main header file
├── Pair.h                # Pair template implementation
├── library.properties    # Arduino library metadata
├── README.md             # Library documentation
└── examples/
    └── ArduinoMap_example/
        └── ArduinoMap_example.ino
```

### Usage Example
```cpp
#include <ArduinoMap.h>

ArduinoMap<String, int> map;
map.insert("hello", 42);
int* value = map.get("hello");
```

### Integration Method
Used Git sparse checkout to extract only the ArduinoMap subdirectory from the K810_Security repository.

### Next Steps
1. ✅ Library integrated into `libs/ArduinoMap/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using ArduinoMap (optional)

---

## ArduinoQueue Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/ArduinoQueue  
**Status:** Successfully integrated

### Library Details
- **Name:** ArduinoQueue
- **Version:** 1.2.5
- **Author:** Einar Arnason <einsiarna@gmail.com>
- **Category:** Data Processing
- **Architectures:** All (*)

### Description
A lightweight linked list type queue implementation designed for microcontrollers. Provides standard FIFO operations with minimal memory overhead.

### Features
- Template-based implementation for any data type
- Dynamic memory allocation
- Configurable maximum items and memory limits
- Standard queue operations (enqueue, dequeue)
- Head/tail access without removal
- Memory efficient linked list implementation

### Files Integrated
```
libs/ArduinoQueue/
├── ArduinoQueue.h          # Main header file
├── library.properties      # Arduino library metadata
├── README.md               # Library documentation
├── LICENSE                 # License file
├── CMakeLists.txt          # CMake build configuration
├── examples/
│   ├── intQueueItemsSize/
│   │   └── intQueueItemsSize.ino
│   └── intQueueMemSize/
│       └── intQueueMemSize.ino
└── test/                   # Unit tests
    ├── CMakeLists.txt
    ├── test_IntQueue.cpp
    └── test_performance.cpp
```

### Usage Example
```cpp
#include <ArduinoQueue.h>

ArduinoQueue<int> intQueue(20);  // Queue of 20 items
intQueue.enqueue(1);
intQueue.enqueue(123);
int number = intQueue.dequeue();  // Returns 1
int head = intQueue.getHead();    // Returns 123 without removing
```

### Integration Method
Used Git sparse checkout to extract only the ArduinoQueue subdirectory from the K810_Security repository.

### Next Steps
1. ✅ Library integrated into `libs/ArduinoQueue/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using ArduinoQueue (optional)

---

## BitBool Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/BitBool  
**Status:** Successfully integrated

### Library Details
- **Name:** BitBool
- **Version:** 1.2.0
- **Author:** Christopher Andrews <chris@arduino.land>
- **Category:** Data Processing
- **Architectures:** All (*)

### Description
An efficient bit manipulation library that provides a drop-in replacement for bool arrays. Perfect for embedded systems where memory efficiency is critical. Uses only 1/8th the memory of a standard bool array.

### Features
- Drop-in replacement for bool/boolean arrays
- Memory efficient (8 bits per byte instead of 8 bytes per bool)
- Array subscript notation for bit access
- Iterator support for range-based loops
- Bit/byte order reversal options
- Lookup table optimization option

### Files Integrated
```
libs/BitBool/
├── BitBool.h              # Main header file
├── BitBool.cpp            # Implementation file
├── library.properties     # Arduino library metadata
├── keywords.txt           # Arduino IDE syntax highlighting
├── README.md              # Library documentation
└── examples/
    └── replace_bool_array/
        └── replace_bool_array.ino
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the BitBool subdirectory from the K810_Security repository.

### Next Steps
1. ✅ Library integrated into `libs/BitBool/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using BitBool (optional)

---

## BufferedStreams Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/BufferedStreams  
**Status:** Successfully integrated

### Library Details
- **Name:** BufferedStreams
- **Version:** 1.0.8
- **Author:** Paulo Costa <me+arduino@paulo.costa.nom.br>
- **Category:** Communication
- **Architectures:** All (*)

### Description
Implementation of Arduino's Stream class using internal ring buffers. Provides LoopbackStream for buffering data and PipedStream for bidirectional communication between components. Can be used to add buffering layers to communications or implement Serial-like APIs between components.

### Features
- LoopbackStream: Buffers data internally and loops it back for reading
- PipedStream: Bidirectional pipe streams for component communication
- PipedStreamPair: Creates connected pairs of streams
- Compatible with Arduino Stream API
- Configurable buffer sizes
- Memory efficient ring buffer implementation

### Files Integrated
```
libs/BufferedStreams/
├── src/
│   ├── LoopbackStream.h      # Loopback stream header
│   ├── LoopbackStream.cpp    # Loopback stream implementation
│   ├── PipedStream.h         # Piped stream header
│   └── PipedStream.cpp       # Piped stream implementation
├── library.properties        # Arduino library metadata
├── keywords.txt              # Arduino IDE syntax highlighting
├── LICENSE                   # License file
├── README.md                 # Library documentation
└── examples/
    ├── LoopbackBuffer/
    │   └── LoopbackBuffer.ino
    ├── ESP8266_MirroredConsole/
    │   ├── ESP8266_MirroredConsole.ino
    │   └── StreamPrint.h
    └── PingPongPipedBuffers/
        └── PingPongPipedBuffers.ino
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the BufferedStreams subdirectory from the K810_Security repository.

### Next Steps
1. ✅ Library integrated into `libs/BufferedStreams/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using BufferedStreams (optional)

---

## CircularBuffers Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/CircularBuffers  
**Status:** Successfully integrated

### Library Details
- **Name:** CircularBuffers
- **Version:** Custom implementation
- **Author:** Aykut ÖZDEMİR
- **Category:** Data Processing
- **Architectures:** All (*)

### Description
High-performance circular buffer implementations optimized for embedded systems. Includes FastCircularQueue for efficient FIFO operations and StringBuffer for string manipulation. Uses power-of-2 buffer sizes for optimal performance with bitwise operations.

### Features
- FastCircularQueue: High-performance circular queue with fixed size buffer
- StringBuffer: Efficient string manipulation buffer based on circular queue
- Power-of-2 buffer sizes for optimal performance
- Interrupt-safe operations
- Template-based implementation for any data type
- Memory efficient fixed-size buffers

### Files Integrated
```
libs/CircularBuffers/
├── FastCircularQueue.h    # Fast circular queue template
├── StringBuffer.h         # String buffer header
└── StringBuffer.hpp       # String buffer implementation
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the CircularBuffers subdirectory from the K810_Security repository.

### Next Steps
1. ✅ Library integrated into `libs/CircularBuffers/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using CircularBuffers (optional)

---

## FastPin Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/FastPin  
**Status:** Successfully integrated

### Library Details
- **Name:** FastPin
- **Version:** Custom implementation
- **Author:** Aykut ÖZDEMİR
- **Category:** Hardware Control
- **Architectures:** AVR (Arduino)

### Description
High-performance digital pin manipulation library using direct port register access. Provides faster read/write operations compared to standard Arduino digital I/O functions by bypassing the Arduino abstraction layer.

### Features
- Direct port manipulation for maximum performance
- Faster than standard digitalWrite/digitalRead
- Static methods for direct port access
- Instance methods for object-oriented usage
- Support for input/output modes
- Pull-up resistor configuration

### Files Integrated
```
libs/FastPin/
├── FastPin.h      # FastPin header
└── FastPin.cpp    # FastPin implementation
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the FastPin subdirectory from the K810_Security repository.

### Next Steps
1. ✅ Library integrated into `libs/FastPin/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using FastPin (optional)

---

## SafeInterrupts Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/SafeInterrupts  
**Status:** Successfully integrated

### Library Details
- **Name:** SafeInterrupts
- **Version:** Custom implementation
- **Author:** Aykut ÖZDEMİR
- **Category:** System Control
- **Architectures:** AVR (Arduino)

### Description
Safe interrupt handling utilities for Arduino with nesting support. Provides RAII-based scoped interrupt disabling to prevent issues in critical sections of code. Safely replaces standard cli()/sei() macros with proper nesting support.

### Features
- Nesting support for interrupt disable/enable
- RAII scoped interrupt disabler
- Safe replacement for cli()/sei() macros
- Automatic state restoration
- Thread-safe interrupt management

### Files Integrated
```
libs/SafeInterrupts/
├── SafeInterrupts.h      # SafeInterrupts header
└── SafeInterrupts.cpp    # SafeInterrupts implementation
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the SafeInterrupts subdirectory from the K810_Security repository.

### Next Steps
1. ✅ Library integrated into `libs/SafeInterrupts/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using SafeInterrupts (optional)

---

## SimpleTimer Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/SimpleTimer  
**Status:** Successfully integrated

### Library Details
- **Name:** SimpleTimer
- **Version:** 1.0.0
- **Author:** Alexander Kiryanenko <kiryanenkoav@gmail.com>
- **Category:** Timing
- **Architectures:** All (*)

### Description
A simple, lightweight timer utility for managing timing operations in Arduino applications. Uses millis() for timing calculations and supports customizable intervals. Template-based implementation allows flexible time types.

### Features
- Template-based implementation for flexible time types
- Simple interval-based timing
- Lightweight and memory efficient
- Easy to use API
- Automatic overflow handling

### Files Integrated
```
libs/SimpleTimer/
├── SimpleTimer.h          # SimpleTimer header
├── SimpleTimer.cpp        # SimpleTimer implementation (if needed)
├── library.properties     # Arduino library metadata
├── keywords.txt           # Arduino IDE syntax highlighting
├── LICENSE                # License file
├── README.md              # Library documentation
└── examples/
    └── SimpleTimer/
        └── SimpleTimer.pde
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the SimpleTimer subdirectory from the K810_Security repository.

### Next Steps
1. ✅ Library integrated into `libs/SimpleTimer/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using SimpleTimer (optional)

---

## ezButton Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/ezButton  
**Status:** Successfully integrated

### Library Details
- **Name:** ezButton
- **Version:** 1.0.6
- **Author:** ArduinoGetStarted.com
- **Category:** Signal Input/Output
- **Architectures:** All (*)

### Description
Easy-to-use button handling library with built-in debouncing, pressed/released events, and press counting. Designed for push-buttons, momentary switches, toggle switches, and magnetic contact switches.

### Features
- Built-in debounce to eliminate chattering
- Pressed and released events
- Press counting (FALLING, RISING, BOTH)
- Internal/external pull-up/pull-down support
- Non-blocking functions
- Easy to use with multiple buttons

### Files Integrated
```
libs/ezButton/
├── src/
│   ├── ezButton.h         # ezButton header
│   └── ezButton.cpp       # ezButton implementation
├── library.properties     # Arduino library metadata
├── keywords.txt           # Arduino IDE syntax highlighting
├── README.md              # Library documentation
└── examples/              # Multiple example sketches
    ├── 01.SingleButton/
    ├── 02.SingleButtonEvents/
    ├── 03.SingleButtonDebounce/
    ├── 04.SingleButtonAll/
    ├── 05.MultipleButtonAll/
    ├── 06.ButtonCount/
    └── 07.ButtonArray/
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the ezButton subdirectory from the K810_Security repository.

### Next Steps
1. ✅ Library integrated into `libs/ezButton/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using ezButton (optional)

---

## ezLED Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/ezLED  
**Status:** Successfully integrated

### Library Details
- **Name:** ezLED
- **Version:** 1.0.1
- **Author:** ArduinoGetStarted.com
- **Category:** Signal Input/Output
- **Architectures:** All (*)

### Description
Easy-to-use LED control library supporting on/off, toggle, fade in/out, blink, blink in period, and blink a number of times. All functions are non-blocking.

### Features
- Turn on/off with optional delay
- Toggle between on and off
- Fade in/out with customizable timing
- Blink with configurable timing
- Blink a specific number of times
- Blink in a period of time
- Cancel blinking/fading anytime
- Support for anode and cathode control modes
- Non-blocking functions

### Files Integrated
```
libs/ezLED/
├── src/
│   ├── ezLED.h            # ezLED header
│   └── ezLED.cpp          # ezLED implementation
├── library.properties     # Arduino library metadata
├── keywords.txt           # Arduino IDE syntax highlighting
├── license.txt            # License file
├── README.md              # Library documentation
└── examples/              # Multiple example sketches
    ├── LEDArray/
    ├── LEDBlink/
    ├── LEDBlinkInPeriod/
    ├── LEDBlinkNumberOfTimes/
    ├── LEDFadeInFadeOut/
    ├── LEDOnOff/
    ├── LEDToggle/
    └── MultipleLED/
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the ezLED subdirectory from the K810_Security repository.

### Next Steps
1. ✅ Library integrated into `libs/ezLED/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using ezLED (optional)

---

## ezOutput Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/ezOutput  
**Status:** Successfully integrated

### Library Details
- **Name:** ezOutput
- **Version:** 1.2.0
- **Author:** ArduinoGetStarted.com
- **Category:** Signal Input/Output
- **Architectures:** All (*)

### Description
Easy-to-use output control library for digital pins. Supports HIGH, LOW, TOGGLE, PULSE, and BLINK_WITHOUT_DELAY. Perfect for controlling LEDs, relays, and other digital outputs.

### Features
- HIGH, LOW, TOGGLE, PULSE operations
- Blink without delay
- Get output pin state
- Time offset support for multiple outputs
- Non-blocking functions
- Easy to use with multiple output pins

### Files Integrated
```
libs/ezOutput/
├── src/
│   ├── ezOutput.h         # ezOutput header
│   └── ezOutput.cpp       # ezOutput implementation
├── library.properties     # Arduino library metadata
├── keywords.txt           # Arduino IDE syntax highlighting
├── README.md              # Library documentation
└── examples/              # Multiple example sketches
    ├── 01.OnOff/
    ├── 02.Toggle/
    ├── 03.SingleBlinkWithoutDelay/
    ├── 04.SingleBlinkChangeFrequency/
    ├── 05.MultipleBlinkWithoutDelay/
    ├── 06.MultipleBlinkWithOffset/
    ├── 07.BlinkInPeriod/
    └── 08.Pulse/
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the ezOutput subdirectory from the K810_Security repository.

### Next Steps
1. ✅ Library integrated into `libs/ezOutput/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using ezOutput (optional)

---

## Statistics Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/Statistics  
**Status:** Successfully integrated

### Library Details
- **Name:** Statistics
- **Version:** Custom implementation
- **Author:** Aykut ÖZDEMİR
- **Category:** Performance/Monitoring
- **Architectures:** All (*)

### Description
Simple statistics collection utility for timing measurements. Provides functionality for measuring execution times and tracking minimum, maximum, and average times using exponential moving average. Designed for performance profiling, benchmarking, and optimization.

### Features
- Measure execution times in microseconds
- Track minimum, maximum, and average times
- Exponential moving average for memory efficiency
- Timer wraparound handling
- Convenient macro for measuring code blocks
- Print statistics to any Print object

### Files Integrated
```
libs/Statistics/
├── Statistic.h      # Statistics header
└── Statistic.cpp    # Statistics implementation
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the Statistics subdirectory from the K810_Security repository.

### Next Steps
1. ✅ Library integrated into `libs/Statistics/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using Statistics (optional)

---

## Utilities Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/Utilities  
**Status:** Successfully integrated

### Library Details
- **Name:** Utilities
- **Version:** 1.0.0
- **Author:** K810_Security
- **Category:** Other
- **Architectures:** All (*)
- **Dependencies:** SafeInterrupts

### Description
Common utility functions and base classes to reduce code duplication and flash usage. Provides secure memory operations, serial output formatting, hex string conversion, debug tracing with multiple levels, and a base driver class for standardized implementations.

### Features
- Secure memory operations (cleaning, constant-time comparison)
- Serial output formatting (OK/error messages, hex values)
- Hex string conversion utilities
- Safe interrupt handling wrappers
- Traceable debug logging system with multiple levels
- DriverBase class for standardized driver implementations
- State management helpers with time tracking

### Files Integrated
```
libs/Utilities/
├── Utilities.h          # Utility functions header
├── Utilities.cpp        # Utility functions implementation
├── Traceable.h          # Debug tracing header
├── Traceable.cpp        # Debug tracing implementation
├── DriverBase.h         # Base driver class header
├── TraceHelper.h        # Trace helper utilities
├── library.properties   # Arduino library metadata
├── README.md            # Library documentation
└── examples/
    └── BasicUsage/
        └── BasicUsage.ino
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the Utilities subdirectory from the K810_Security repository.

### Dependencies
- SafeInterrupts (already integrated)

### Next Steps
1. ✅ Library integrated into `libs/Utilities/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using Utilities (optional)

---

## StaticSerialCommands Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/StaticSerialCommands  
**Status:** Successfully integrated

### Library Details
- **Name:** StaticSerialCommands
- **Version:** 1.2.0
- **Author:** naszly
- **Category:** Data Processing
- **Architectures:** All (*)

### Description
An Arduino library for parsing commands received over a serial port. Optimized for low dynamic memory usage by storing commands in program memory. Supports typed arguments with strict validation, subcommands, and friendly error messages.

### Features
- Typed arguments with strict input validation (Int, Float, String)
- Friendly error messages for invalid input
- Customizable delimiter, termination, and quotation characters
- Commands can have subcommands
- Methods to list commands with syntax and description
- Low dynamic memory usage (commands stored in program memory)
- Argument range validation
- Timeout support

### Files Integrated
```
libs/StaticSerialCommands/
├── src/
│   ├── StaticSerialCommands.h    # Main header
│   ├── StaticSerialCommands.cpp   # Main implementation
│   ├── Command.h                  # Command structure
│   ├── CommandBuilder.h           # Command builder utilities
│   ├── Arg.h                      # Argument handling
│   └── Parse.h                    # Parsing utilities
├── library.properties              # Arduino library metadata
├── keywords.txt                   # Arduino IDE syntax highlighting
├── LICENSE                        # License file
├── README.md                      # Library documentation
└── examples/                      # Multiple example sketches
    ├── SimpleCommands/
    ├── CommandsWithArguments/
    └── Subcommands/
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the StaticSerialCommands subdirectory from the K810_Security repository.

### Dependencies
- None (standalone library)

### Notes
- Used by Utilities library for serial command handling
- Commands stored in program memory to reduce RAM usage

### Next Steps
1. ✅ Library integrated into `libs/StaticSerialCommands/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)
4. ⏭️ Create FsmOS-specific examples using StaticSerialCommands (optional)

---

## HC05 Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/HC05  
**Status:** Successfully integrated

### Library Details
- **Name:** HC05
- **Version:** Custom implementation
- **Author:** Aykut ÖZDEMİR
- **Category:** Communication/Bluetooth
- **Architectures:** All (*)

### Description
Bluetooth HC-05 module driver. Provides an interface for communicating with HC-05 Bluetooth modules, supporting both AT command mode for configuration and data mode for communication with paired devices.

### Features
- AT command mode for configuration
- Data mode for communication with paired devices
- Command queue management
- Connection state monitoring
- Reset functionality (normal and permanent)
- Callback support for command responses and received data
- State machine-based operation

### Files Integrated
```
libs/HC05/
├── HC05.h      # HC-05 driver header
└── HC05.cpp    # HC-05 driver implementation
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the HC05 subdirectory from the K810_Security repository.

### Dependencies
- StringBuffer (from CircularBuffers)
- ArduinoQueue
- SimpleTimer
- Utilities
- DriverBase (from Utilities)

### Next Steps
1. ✅ Library integrated into `libs/HC05/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)

---

## I2C-master Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/I2C-master  
**Status:** Successfully integrated

### Library Details
- **Name:** I2C-master
- **Version:** Custom implementation (based on Wayne Truchsess's library)
- **Author:** Wayne Truchsess (modified by Aykut ÖZDEMİR)
- **Category:** Communication/I2C
- **Architectures:** All (*)

### Description
Enhanced I2C/TWI library for Arduino. Provides more functionality than the standard Wire library, including support for repeated start conditions and bus timeout detection.

### Features
- Support for repeated start conditions
- Bus timeout detection and recovery
- Bus scanning functionality
- 8-bit and 16-bit register addressing
- Multiple data type support (uint8_t, uint16_t, uint32_t, uint64_t, strings, buffers)
- Low-level I2C methods for custom protocols
- Configurable bus speed (100kHz/400kHz)
- Pullup resistor control

### Files Integrated
```
libs/I2C-master/
├── I2C.h           # I2C library header
├── I2C.cpp         # I2C library implementation
├── keywords.txt    # Arduino IDE syntax highlighting
├── LICENSE         # License file
├── README.md       # Library documentation
└── examples/       # Example sketches
    ├── HMC5883L/
    └── i2crepl/
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the I2C-master subdirectory from the K810_Security repository.

### Dependencies
- Traceable (from Utilities)
- SimpleTimer

### Next Steps
1. ✅ Library integrated into `libs/I2C-master/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)

---

## MemoryUsage Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/MemoryUsage  
**Status:** Successfully integrated

### Library Details
- **Name:** MemoryUsage
- **Version:** 2.21.1
- **Author:** Thierry PARIS - Locoduino
- **Category:** Development/Debugging
- **Architectures:** AVR only

### Description
Memory usage monitoring utilities for Arduino. Helps monitor and analyze SRAM memory usage on AVR-based Arduino boards.

### Features
- Calculate current free RAM
- Calculate minimum free RAM using stack painting
- Display detailed RAM usage information
- Stack usage monitoring with canary pattern
- Memory layout visualization

### Files Integrated
```
libs/MemoryUsage/
├── src/
│   ├── MemoryUsage.h      # Memory usage header
│   └── MemoryUsage.cpp    # Memory usage implementation
├── library.properties      # Arduino library metadata
├── keywords.txt           # Arduino IDE syntax highlighting
├── LICENSE                # License file
├── README.adoc            # Library documentation
└── examples/              # Example sketches
    ├── FreeRam/
    └── Stack/
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the MemoryUsage subdirectory from the K810_Security repository.

### Dependencies
- None (standalone library)

### Notes
- AVR architecture only (uses AVR-specific memory layout)
- Useful for debugging memory issues

### Next Steps
1. ✅ Library integrated into `libs/MemoryUsage/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)

---

## Packager Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/Packager  
**Status:** Successfully integrated

### Library Details
- **Name:** Packager
- **Version:** Custom implementation
- **Author:** Aykut ÖZDEMİR
- **Category:** Communication/Protocol
- **Architectures:** All (*)

### Description
Abstract interface for packet-based communication protocols. Provides a base interface for implementing various packet-based communication protocols with support for encoding/decoding.

### Features
- Abstract interface for packet protocols
- Plain and encoded stream management
- Piped stream support for bidirectional communication
- Maximum packet size control
- Stream clearing functionality
- CRC-based packet interface implementation

### Files Integrated
```
libs/Packager/
├── PackageInterface.h          # Abstract package interface
├── PackageInterface.cpp        # Interface implementation
├── CRCPackageInterface.h       # CRC-based package interface
├── CRCPackageInterface.cpp     # CRC implementation
├── DefaultPackageInterface.h   # Default package interface
├── DefaultPackageInterface.cpp # Default implementation
├── README_UML.md               # UML diagram documentation
└── CRCPackageInterface.uml     # PlantUML diagram
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the Packager subdirectory from the K810_Security repository.

### Dependencies
- PipedStream (from BufferedStreams)

### Next Steps
1. ✅ Library integrated into `libs/Packager/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)

---

## SoftSerial Library Integration ✅

**Date:** 2025-01-27  
**Source:** https://github.com/aykutozdemir/K810_Security/tree/main/lib/SoftSerial  
**Status:** Successfully integrated

### Library Details
- **Name:** SoftSerial
- **Version:** Custom implementation
- **Author:** Aykut ÖZDEMİR
- **Category:** Communication/Serial
- **Architectures:** All (*)

### Description
Software serial communication implementation for Arduino. Provides software-based asynchronous serial communication without requiring hardware UART.

### Features
- Software-based serial communication
- Configurable baud rates (1200-115200)
- Parity modes (none, even, odd)
- Configurable stop bits
- Efficient bit manipulation
- Interrupt-based operation
- RX and TX buffer queues
- Template-based buffer sizing

### Files Integrated
```
libs/SoftSerial/
├── SoftSerial.h      # SoftSerial header
└── SoftSerial.hpp    # SoftSerial template implementation
```

### Usage Example
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

### Integration Method
Used Git sparse checkout to extract only the SoftSerial subdirectory from the K810_Security repository.

### Dependencies
- FastPin
- FastCircularQueue (from CircularBuffers)
- DriverBase (from Utilities)

### Next Steps
1. ✅ Library integrated into `libs/SoftSerial/`
2. ✅ Documentation updated in `libs/README.md`
3. ⏭️ Test integration with FsmOS examples (optional)

---

## Integration Scripts

Use the provided scripts to integrate additional libraries:

**Bash:**
```bash
./scripts/integrate_library.sh <github_url> [library_name]
```

**Python:**
```bash
python3 scripts/integrate_library.py <github_url> [library_name]
```

For libraries in subdirectories (like this one), manual integration may be required using Git sparse checkout.

