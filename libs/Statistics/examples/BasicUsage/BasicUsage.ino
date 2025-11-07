/**
 * @file BasicUsage.ino
 * @brief Basic example demonstrating Statistics library usage
 * 
 * This example shows how to use the Statistics library to measure
 * and track execution times of code blocks.
 * 
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#include <Statistic.h>

Statistic myStat;

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println(F("Statistics Basic Usage Example"));
  Serial.println(F("==============================="));
  
  // Set a name for this statistic
  myStat.setName(F("DelayTest"));
  
  // Perform several measurements
  Serial.println(F("\nPerforming 10 measurements..."));
  for (int i = 0; i < 10; i++) {
    myStat.start();
    
    // Simulate some work
    delayMicroseconds(100 + random(50));
    
    myStat.end();
  }
  
  // Print statistics
  Serial.println(F("\nStatistics:"));
  myStat.print(Serial);
  
  // Reset and measure again
  Serial.println(F("\nResetting and measuring again..."));
  myStat.reset();
  
  for (int i = 0; i < 10; i++) {
    myStat.start();
    delayMicroseconds(200 + random(50));
    myStat.end();
  }
  
  Serial.println(F("\nNew statistics:"));
  myStat.print(Serial);
}

void loop() {
  // Empty loop
}

