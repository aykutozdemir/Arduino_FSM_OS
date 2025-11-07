/**
 * @file StringBuffer_Basic.ino
 * @brief Basic example demonstrating StringBuffer usage
 * 
 * This example shows how to use StringBuffer to efficiently handle
 * strings in memory-constrained embedded systems.
 * 
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#include <StringBuffer.h>

// Create a string buffer with size 32 (must be power of 2)
StringBuffer<32> buffer;

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println(F("StringBuffer Basic Example"));
  Serial.println(F("=========================="));
  
  // Append different types of strings
  Serial.println(F("\nAppending strings..."));
  buffer.append("Hello");
  buffer.append(" ");
  buffer.append(F("World"));  // Flash string
  
  // Display buffer contents
  Serial.print(F("Buffer size: "));
  Serial.println(buffer.size());
  
  String result = buffer.toString();
  Serial.print(F("Buffer contents: "));
  Serial.println(result);
  
  // Search for substring
  Serial.println(F("\nSearching for 'World'..."));
  int index = buffer.indexOf("World");
  if (index >= 0) {
    Serial.print(F("Found at index: "));
    Serial.println(index);
  } else {
    Serial.println(F("Not found"));
  }
  
  // Check if buffer ends with a string
  Serial.println(F("\nChecking if buffer ends with 'World'..."));
  if (buffer.endsWith("World")) {
    Serial.println(F("Yes, buffer ends with 'World'"));
  } else {
    Serial.println(F("No, buffer does not end with 'World'"));
  }
  
  // Extract substring
  Serial.println(F("\nExtracting substring (0 to 5)..."));
  StringBuffer<32> sub = buffer.substring(0, 5);
  Serial.print(F("Substring: "));
  Serial.println(sub.toString());
  
  // Clear buffer
  Serial.println(F("\nClearing buffer..."));
  buffer.clear();
  Serial.print(F("Buffer size after clear: "));
  Serial.println(buffer.size());
}

void loop() {
  // Empty loop
}

