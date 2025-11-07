/**
 * @file StaticMethods.ino
 * @brief Example demonstrating FastPin static methods for maximum performance
 * 
 * This example shows how to use FastPin static methods for
 * maximum performance in time-critical applications.
 * 
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#include <FastPin.h>

// Pin configuration
const uint8_t LED_PIN = 13;

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  // Configure pin as output
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println(F("FastPin Static Methods Example"));
  Serial.println(F("================================="));
  Serial.println(F("Using static methods for maximum performance"));
}

void loop() {
  // Get port registers for maximum performance
  const uint8_t portNum = digitalPinToPort(LED_PIN);
  volatile uint8_t *port = portOutputRegister(portNum);
  volatile uint8_t *pinReg = portInputRegister(portNum);
  const uint8_t bitMask = digitalPinToBitMask(LED_PIN);
  
  // Use static methods for maximum performance
  FastPin::high(port, bitMask);   // Set pin HIGH
  delay(100);
  FastPin::low(port, bitMask);     // Set pin LOW
  delay(100);
  FastPin::toggle(port, bitMask); // Toggle pin
  delay(100);
  
  // Read pin state
  uint8_t state = FastPin::read(pinReg, bitMask);
  Serial.print(F("Pin state: "));
  Serial.println(state);
  
  delay(700);
}

