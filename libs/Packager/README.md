# Packager Library

Packet-based communication protocol interfaces for Arduino with CRC error detection and reliable transmission.

## Features

- **Abstract Interface**: Base PackageInterface class for custom protocol implementations
- **Default Implementation**: Simple pass-through interface for basic use cases
- **CRC Protocol**: Reliable communication with CRC-16 error detection
- **Automatic Retransmission**: Configurable retry mechanism for failed packets
- **ACK/NACK Support**: Positive and negative acknowledgment handling
- **Connection Management**: Automatic connection state synchronization and reset handling
- **Memory Efficient**: Optimized for embedded systems with minimal overhead

## Installation

1. Download or clone this library into your Arduino `libraries` folder
2. Install required dependencies:
   - PipedStream (from BufferedStreams)
   - SimpleTimer
   - CircularBuffers (FastCircularQueue)
   - Utilities (Traceable)
3. Restart Arduino IDE

## Architecture

The library provides three main classes:

1. **PackageInterface**: Abstract base class defining the interface
2. **DefaultPackageInterface**: Simple pass-through implementation
3. **CRCPackageInterface**: Reliable protocol with CRC-16 validation

## Usage

### Default Package Interface (Simple Pass-Through)

```cpp
#include <DefaultPackageInterface.h>
#include <PipedStream.h>

// Create piped stream pair
PipedStreamPair streams;

// Create default package interface
DefaultPackageInterface packager(streams);

void setup() {
  Serial.begin(9600);
}

void loop() {
  // Process data transfer
  packager.loop();
  
  // Write to plain stream (will be transferred to encoded stream)
  packager.getPlainStream().write("Hello");
  
  // Read from encoded stream
  if (packager.getEncodedStream().available()) {
    char c = packager.getEncodedStream().read();
    Serial.print(c);
  }
}
```

### CRC Package Interface (Reliable Communication)

```cpp
#include <CRCPackageInterface.h>
#include <PipedStream.h>

// Create piped stream pair
PipedStreamPair streams;

// Create CRC package interface
CRCPackageInterface packager(streams);

void setup() {
  Serial.begin(9600);
  
  // Initialize packager
  // (initialization happens in constructor)
}

void loop() {
  // Must call loop() regularly
  packager.loop();
  
  // Write data to plain stream
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  for (int i = 0; i < 4; i++) {
    packager.getPlainStream().write(data[i]);
  }
  
  // Read received data from plain stream
  if (packager.getPlainStream().available()) {
    uint8_t received = packager.getPlainStream().read();
    Serial.print("Received: ");
    Serial.println(received, HEX);
  }
  
  // Send reset packet if needed
  // packager.sendResetPacket();
}
```

## Protocol Details (CRCPackageInterface)

### Packet Structure

Each packet consists of:

- **Header (4 bytes)**:
  - Start byte (0xAA)
  - Packet sequence number (1-255)
  - Packet type (DATA/ACK/NACK/RESET)
  - Payload length (0-8 bytes)

- **Payload (0-8 bytes)**: Data bytes

- **Footer (3 bytes)**:
  - CRC-16 checksum (2 bytes)
  - Stop byte (0x55)

**Total packet size**: 15 bytes (fixed)

### Packet Types

- `DATA_TYPE (0)`: Data packet with payload
- `ACK_TYPE (1)`: Positive acknowledgment
- `NACK_TYPE (2)`: Negative acknowledgment with error code
- `RESET_TYPE (3)`: Connection reset request

### Error Codes (NACK Reasons)

- `NO_ERROR (0x00)`: Packet valid
- `INVALID_CRC (0x01)`: CRC checksum mismatch
- `INVALID_START_STOP (0x02)`: Frame marker error
- `INVALID_TYPE (0x03)`: Unknown packet type
- `INVALID_LENGTH (0x04)`: Invalid payload length
- `UNKNOWN_ERROR (0xFF)`: Unclassified error

### Reliability Features

- **CRC-16-CCITT**: Polynomial 0x1021 for error detection
- **Automatic Retransmission**: Up to 5 retry attempts
- **Sequence Numbers**: Packet ordering and duplicate detection
- **Timeout Handling**: Configurable timeouts for various operations
- **Connection Reset**: Automatic recovery from protocol errors

## API Reference

### PackageInterface (Base Class)

- `loop()` - Process protocol state machine (pure virtual)
- `clear()` - Clear all streams
- `getPlainStream()` - Get plain (decoded) data stream
- `getEncodedStream()` - Get encoded data stream

### DefaultPackageInterface

- `DefaultPackageInterface(PipedStreamPair &streams, uint16_t bufferSize = 8)` - Constructor
- `loop()` - Transfer data between streams

### CRCPackageInterface

- `CRCPackageInterface(PipedStreamPair &streams, uint16_t bufferSize = 15)` - Constructor
- `loop()` - Process protocol state machine
- `sendResetPacket()` - Send connection reset packet

## Protocol Timeouts

- **Outgoing Data Read**: 100ms - Time to collect outgoing data
- **ACK/NACK Wait**: 500ms - Time to wait for acknowledgment
- **Incoming Data Wait**: 500ms - Time to receive complete packet
- **Reset Detection**: 10000ms - Connection timeout threshold

## Example: Bidirectional Communication

```cpp
#include <CRCPackageInterface.h>
#include <PipedStream.h>

PipedStreamPair streams;
CRCPackageInterface packager(streams);

void setup() {
  Serial.begin(9600);
}

void loop() {
  packager.loop();
  
  // Send data periodically
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 1000) {
    lastSend = millis();
    
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    for (int i = 0; i < 3; i++) {
      packager.getPlainStream().write(data[i]);
    }
  }
  
  // Receive data
  while (packager.getPlainStream().available()) {
    uint8_t received = packager.getPlainStream().read();
    Serial.print("RX: 0x");
    Serial.println(received, HEX);
  }
}
```

## Notes

- Maximum payload size is 8 bytes per packet
- Packets are automatically retransmitted on failure
- Sequence numbers wrap around from 255 to 1
- Use `sendResetPacket()` to recover from protocol errors
- The protocol is designed for single-threaded environments

## License

This library is provided as-is for use with the FsmOS framework.

## Author

Aykut ÖZDEMİR

