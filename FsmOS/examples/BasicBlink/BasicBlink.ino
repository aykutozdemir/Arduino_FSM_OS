/*
  BasicBlink - Simple LED Blink Example
  
  This example demonstrates the most basic usage of FsmOS by creating a task
  that blinks the built-in LED. It shows how to:
  1. Create a task by inheriting from the Task base class
  2. Set up periodic task execution
  3. Initialize hardware in on_start()
  4. Perform the task's work in step()
  
  The circuit:
  - Uses the built-in LED on pin 13 (most Arduino boards)
  - No additional components needed

  Created October 2, 2025
  By Aykut Ozdemir
  
  https://github.com/aykutozdemir/Arduino_FSM_OS
*/

#include <FsmOS.h>

/**
 * BlinkTask: A simple task that blinks the built-in LED.
 * 
 * This task demonstrates the basic structure of an FsmOS task:
 * - Constructor: Sets up task parameters
 * - on_start(): Initializes hardware (runs once)
 * - step(): Contains the main task logic (runs periodically)
 */
class BlinkTask : public Task {
public:
  BlinkTask() : Task(F("Blinker")) { // Give our task a name for logging
    set_period(500);  // Set the task to run every 500 milliseconds
  }

  void on_start() override {
    pinMode(LED_BUILTIN, OUTPUT);
    log_info(F("Blink task started"));  // Log task initialization
  }

  void step() override {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle LED
  }
};

// Create an instance of our task
BlinkTask blinker;

void setup() {
  Serial.begin(9600);  // Initialize serial for logging
  
  // Initialize the OS with logging enabled
  OS.begin_with_logger();

  // Add our task to the scheduler
  OS.add(&blinker);
}

void loop() {
  // Run the cooperative scheduler - must be called continuously
  OS.loop_once();
}
