/**
 * @file CRCPackageInterface.ino
 * @brief Example demonstrating CRCPackageInterface usage
 * 
 * This example shows how to use the CRCPackageInterface
 * for reliable packet-based communication with CRC error detection.
 * 
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#include <CRCPackageInterface.h>
#include <PipedStream.h>

// Create piped stream pair
PipedStreamPair streams;

// Create CRC package interface
CRCPackageInterface packager(streams);

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println(F("CRCPackageInterface Example"));
  Serial.println(F("==========================="));
  Serial.println(F("Reliable packet communication with CRC"));
}

void loop() {
  // Must call loop() regularly to process protocol
  packager.loop();
  
  // Send data periodically
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 2000) {
    lastSend = millis();
    
    Serial.println(F("Sending packet..."));
    
    // Write data to plain stream (will be packetized automatically)
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    for (int i = 0; i < 4; i++) {
      packager.getPlainStream().write(data[i]);
    }
  }
  
  // Read received data from plain stream
  while (packager.getPlainStream().available()) {
    uint8_t received = packager.getPlainStream().read();
    Serial.print(F("Received packet data: 0x"));
    Serial.println(received, HEX);
  }
  
  // Send reset packet if needed (uncomment to test)
  // static bool resetSent = false;
  // if (!resetSent && millis() > 5000) {
  //   resetSent = true;
  //   Serial.println(F("Sending reset packet..."));
  //   packager.sendResetPacket();
  // }
}

