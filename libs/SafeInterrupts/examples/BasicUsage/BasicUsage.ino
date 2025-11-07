/**
 * @file BasicUsage.ino
 * @brief Basic example demonstrating SafeInterrupts usage
 * 
 * This example shows how to use SafeInterrupts to safely
 * disable and enable interrupts with nesting support.
 * 
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#include <SafeInterrupts.h>

volatile int sharedCounter = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println(F("SafeInterrupts Basic Usage Example"));
  Serial.println(F("==================================="));
  
  // Example 1: Using macros
  Serial.println(F("\nExample 1: Using cli()/sei() macros"));
  {
    cli();  // Disable interrupts
    
    // Critical section
    sharedCounter++;
    Serial.print(F("Counter in critical section: "));
    Serial.println(sharedCounter);
    
    sei();  // Enable interrupts
  }
  
  // Example 2: Using static methods
  Serial.println(F("\nExample 2: Using static methods"));
  {
    SafeInterrupts::disable();
    
    sharedCounter++;
    Serial.print(F("Counter: "));
    Serial.println(sharedCounter);
    
    SafeInterrupts::enable();
  }
  
  // Example 3: Using RAII ScopedDisable
  Serial.println(F("\nExample 3: Using ScopedDisable"));
  {
    SafeInterrupts::ScopedDisable disable;
    
    sharedCounter++;
    Serial.print(F("Counter: "));
    Serial.println(sharedCounter);
    
    // Interrupts automatically re-enabled when leaving scope
  }
  
  Serial.println(F("\nAll examples completed successfully!"));
}

void loop() {
  // Empty loop
}

