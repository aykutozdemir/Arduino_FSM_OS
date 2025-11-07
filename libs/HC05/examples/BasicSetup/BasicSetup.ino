/**
 * @file BasicSetup.ino
 * @brief Basic example demonstrating HC05 initialization and basic usage
 * 
 * This example shows how to initialize the HC-05 module and
 * monitor its connection state.
 * 
 * Hardware Connections:
 * - HC-05 TXD → Arduino pin 10 (RX)
 * - HC-05 RXD → Arduino pin 11 (TX)
 * - HC-05 KEY/EN → Arduino pin 9
 * - HC-05 STATE → Arduino pin 8
 * - HC-05 RESET → Arduino pin 7
 * 
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#include <HC05.h>
#include <FsmOS.h>
#include <SoftwareSerial.h>

// Create software serial for HC-05 communication
SoftwareSerial hc05Serial(10, 11);  // RX, TX

// Create HC05 object (now a Task)
// Parameters: stream, keyPin, statePin, resetPin
HC05 hc05(hc05Serial, 9, 8, 7);

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println(F("HC05 Basic Setup Example"));
  Serial.println(F("========================"));
  
  // Initialize serial communication with HC-05
  hc05Serial.begin(9600);
  
  // Initialize FsmOS and add HC05 as a task
  OS.begin();
  OS.add(&hc05);
  hc05.setPeriod(10); // Run every 10ms
  hc05.start(); // This will call on_start() which calls begin()
  
  Serial.println(F("HC-05 module initialized"));
  Serial.println(F("Waiting for connection..."));
}

void loop() {
  // Run OS scheduler (this will call hc05.step() periodically)
  OS.loopOnce();
  
  // Check connection status
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 1000) {
    lastCheck = millis();
    
    if (hc05.isConnected()) {
      Serial.println(F("HC-05 is connected"));
    } else {
      Serial.println(F("HC-05 is not connected"));
    }
    
    if (hc05.isDataMode()) {
      Serial.println(F("HC-05 is in data mode"));
    } else {
      Serial.println(F("HC-05 is in command mode"));
    }
  }
}

