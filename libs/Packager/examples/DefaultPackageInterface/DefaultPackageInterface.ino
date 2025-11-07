/**
 * @file DefaultPackageInterface.ino
 * @brief Example demonstrating DefaultPackageInterface usage
 * 
 * This example shows how to use the DefaultPackageInterface
 * for simple pass-through data transfer.
 * 
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#include <DefaultPackageInterface.h>
#include <PipedStream.h>

// Create piped stream pair
PipedStreamPair streams;

// Create default package interface
DefaultPackageInterface packager(streams);

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println(F("DefaultPackageInterface Example"));
  Serial.println(F("==============================="));
  Serial.println(F("Simple pass-through data transfer"));
}

void loop() {
  // Process data transfer
  packager.loop();
  
  // Send data to plain stream
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 1000) {
    lastSend = millis();
    
    Serial.println(F("Sending data..."));
    packager.getPlainStream().print("Hello World!");
  }
  
  // Read data from encoded stream
  while (packager.getEncodedStream().available()) {
    char c = packager.getEncodedStream().read();
    Serial.print(F("Received: "));
    Serial.println(c);
  }
}

