/**
 * @file BasicUsage.ino
 * @brief Basic example demonstrating SoftSerial usage
 * 
 * This example shows how to use SoftSerial for software-based
 * serial communication. Note that you need to configure a timer
 * interrupt to call processISR() for this to work properly.
 * 
 * Hardware Connections:
 * - Connect RX pin (pin 2) to TX of other device
 * - Connect TX pin (pin 3) to RX of other device
 * - Connect GND to common ground
 * 
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#include <SoftSerial.h>

// Create SoftSerial with RX buffer size 64 and TX buffer size 64
// Parameters: rxPin, txPin
SoftSerial<64, 64> softSerial(2, 3);

// Timer setup callback
// NOTE: You need to implement this based on your timer library
// This is a placeholder - actual implementation depends on your hardware
void setupTimer(unsigned long period) {
  // Example for Timer1 library (if available):
  // Timer1.initialize(period);
  // Timer1.attachInterrupt([]() { softSerial.processISR(); });
  
  // For this example, we'll just print the period
  // In a real implementation, configure your timer interrupt here
  Serial.print(F("Timer period (us): "));
  Serial.println(period);
}

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println(F("SoftSerial Basic Usage Example"));
  Serial.println(F("=============================="));
  
  // Initialize SoftSerial
  // Parameters: timerSetupCallback, baudRate, stopBits, parity
  softSerial.begin(setupTimer, BAUD_9600, 1, NONE);
  
  Serial.println(F("SoftSerial initialized at 9600 baud"));
  Serial.println(F("NOTE: Timer interrupt must be configured for this to work!"));
}

void loop() {
  // Must call loop() regularly
  softSerial.loop();
  
  // Read available data from SoftSerial
  if (softSerial.available()) {
    char c = softSerial.read();
    Serial.print(F("Received: "));
    Serial.println(c);
  }
  
  // Send data from Serial to SoftSerial
  if (Serial.available()) {
    char c = Serial.read();
    softSerial.write(c);
    Serial.print(F("Sent: "));
    Serial.println(c);
  }
}

