/**
 * @file NestedCriticalSections.ino
 * @brief Example demonstrating nested critical sections
 * 
 * This example shows how SafeInterrupts handles nested
 * interrupt disable/enable calls correctly.
 * 
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#include <SafeInterrupts.h>

volatile int counter = 0;

// Function that uses critical section
void incrementCounter() {
  SafeInterrupts::ScopedDisable disable;
  counter++;
}

// Function that calls incrementCounter (nested critical section)
void processData() {
  SafeInterrupts::ScopedDisable disable;
  
  // Some processing...
  counter += 10;
  
  // Call another function with critical section
  incrementCounter();  // Nested disable - safe!
  
  // More processing...
  counter += 5;
  
  // Interrupts automatically re-enabled when leaving scope
}

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println(F("SafeInterrupts Nested Critical Sections Example"));
  Serial.println(F("================================================"));
  
  Serial.println(F("Calling processData() which has nested critical sections..."));
  
  processData();
  
  Serial.print(F("Final counter value: "));
  Serial.println(counter);
  Serial.println(F("Interrupts are properly re-enabled after all nested sections complete."));
}

void loop() {
  // Empty loop
}

