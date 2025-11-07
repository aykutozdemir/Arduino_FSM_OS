/**
 * @file BasicUsage.ino
 * @brief Basic example demonstrating FastPin usage
 * 
 * This example shows how to use FastPin for high-performance
 * digital pin operations. It blinks an LED on pin 13.
 * 
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#include <FastPin.h>

// Create a FastPin object for pin 13 as output
FastPin ledPin(13, true);  // true = output

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println(F("FastPin Basic Usage Example"));
  Serial.println(F("==========================="));
  Serial.println(F("LED on pin 13 should blink"));
}

void loop() {
  // Use instance methods for convenience
  ledPin.high();    // Turn on LED (faster than digitalWrite)
  delay(100);
  ledPin.low();     // Turn off LED
  delay(100);
  ledPin.toggle();  // Toggle LED state
  delay(100);
  
  // Or use set() method
  ledPin.set(1);    // Set HIGH
  delay(100);
  ledPin.set(0);    // Set LOW
  delay(100);
}

