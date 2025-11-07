/**
 * @file MeasureTimeMacro.ino
 * @brief Example demonstrating MEASURE_TIME macro usage
 * 
 * This example shows how to use the MEASURE_TIME macro for
 * convenient code block timing measurements.
 * 
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#include <Statistic.h>

Statistic loopTime;
Statistic functionTime;

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println(F("Statistics MEASURE_TIME Macro Example"));
  Serial.println(F("====================================="));
  
  loopTime.setName(F("Loop"));
  functionTime.setName(F("Function"));
}

void loop() {
  // Measure the entire loop execution time
  MEASURE_TIME(loopTime) {
    // Simulate some work
    delayMicroseconds(100);
    
    // Measure a function call
    MEASURE_TIME(functionTime) {
      simulateFunction();
    }
    
    delayMicroseconds(50);
  }
  
  // Print statistics every 100 iterations
  static int count = 0;
  if (++count >= 100) {
    count = 0;
    Serial.println(F("\nStatistics:"));
    loopTime.print(Serial);
    functionTime.print(Serial);
    Serial.println();
  }
}

void simulateFunction() {
  // Simulate some function work
  delayMicroseconds(50);
}

