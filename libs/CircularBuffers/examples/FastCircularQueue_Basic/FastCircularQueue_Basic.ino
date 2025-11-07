/**
 * @file FastCircularQueue_Basic.ino
 * @brief Basic example demonstrating FastCircularQueue usage
 * 
 * This example shows how to use FastCircularQueue to store and retrieve
 * integer values in a FIFO (First-In-First-Out) manner.
 * 
 * @author Aykut ÖZDEMİR
 * @date 2025
 */

#include <FastCircularQueue.h>

// Create a circular queue with buffer size 16 (must be power of 2)
FastCircularQueue<int, 16> queue;

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println(F("FastCircularQueue Basic Example"));
  Serial.println(F("=============================="));
  
  // Push some values
  Serial.println(F("\nPushing values: 10, 20, 30, 40, 50"));
  queue.push(10);
  queue.push(20);
  queue.push(30);
  queue.push(40);
  queue.push(50);
  
  // Display queue status
  Serial.print(F("Queue available: "));
  Serial.println(queue.available());
  Serial.print(F("Queue is full: "));
  Serial.println(queue.isFull() ? F("Yes") : F("No"));
  Serial.print(F("Queue is empty: "));
  Serial.println(queue.isEmpty() ? F("Yes") : F("No"));
  
  // Peek at the first value without removing it
  int peekValue;
  if (queue.peek(peekValue)) {
    Serial.print(F("Peeked value (not removed): "));
    Serial.println(peekValue);
  }
  
  // Pop all values
  Serial.println(F("\nPopping all values:"));
  int value;
  while (!queue.isEmpty()) {
    if (queue.pop(value)) {
      Serial.print(F("Popped: "));
      Serial.println(value);
    }
  }
  
  Serial.print(F("\nQueue is now empty: "));
  Serial.println(queue.isEmpty() ? F("Yes") : F("No"));
}

void loop() {
  // Empty loop
}

