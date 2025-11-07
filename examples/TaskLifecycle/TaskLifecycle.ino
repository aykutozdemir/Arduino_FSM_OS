/*
  TaskLifecycle - Task State Management Example
  
  This example demonstrates FsmOS's task lifecycle management features:
  1. Task creation and initialization
  2. Task state transitions (ACTIVE, SUSPENDED, INACTIVE)
  3. Automatic task cleanup
  4. Lifecycle callback methods
  
  The sketch creates two types of tasks:
  - BlinkTask: A simple LED blinker with configurable period
  - ControlTask: Manages other tasks' lifecycles through different phases
  
  The demo progresses through 6 phases:
  Phase 0: Create and start fast blinking task
  Phase 1: Add slow blinking task
  Phase 2: Suspend fast blinking task
  Phase 3: Resume fast blinking task
  Phase 4: Terminate slow blinking task
  Phase 5: Clean up all tasks
  
  The circuit:
  - LED1 connected to built-in LED (usually pin 13)
  - LED2 connected to built-in LED + 1 (usually pin 14)
  
  Created October 2, 2025
  By Aykut Ozdemir
  
  https://github.com/aykutozdemir/Arduino_FSM_OS
*/

#include <FsmOS.h>

/**
 * BlinkTask: Blinks an LED with configurable period
 * 
 * This task demonstrates:
 * - Basic periodic task execution
 * - Lifecycle callback methods
 * - Hardware state management during lifecycle events
 */
class BlinkTask : public Task {
  const uint8_t pin;          // LED pin number
  bool state;                 // Current LED state
  const __FlashStringHelper* blink_type;    // Description of blink rate (for logging)
  Timer16 blink_timer;       // Variable blink timing - 4 bytes
  
public:
  BlinkTask(uint8_t led_pin, uint32_t period_ms, const __FlashStringHelper* type) 
    : Task(type),  // Use type as task name
      pin(led_pin), 
      state(false),
      blink_type(type)
  {
    setPeriod(period_ms);
  }

  void on_start() override {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);
    logInfof(F("%s blinker started on pin %d"), blink_type, pin);
    logInfof(F("Period: %dms"), getPeriod());
  }

  void step() override {
    // Toggle LED state
    state = !state;
    digitalWrite(pin, state);
    logDebugf(F("%s LED %s"), blink_type, state ? "ON" : "OFF");
  }
  
  uint8_t getMaxMessageBudget() const override { return 0; }
  uint16_t getTaskStructSize() const override { return sizeof(*this); }
};

/**
 * ControlTask: Manages other tasks' lifecycles through different phases
 */
class ControlTask : public Task {
  Task* fast_blink;
  Task* slow_blink;
  uint32_t phase;
  
public:
  ControlTask() : Task(F("Controller")), phase(0) {
    // Create tasks with descriptive names
    fast_blink = new BlinkTask(LED_BUILTIN, 200, F("Fast"));     // Fast blink: 200ms
    slow_blink = new BlinkTask(LED_BUILTIN + 1, 1000, F("Slow")); // Slow blink: 1s
    setPeriod(5000);  // Phase changes every 5 seconds
  }

protected:
  void step() override {
    switch(phase) {
      case 0:
        // Start with fast blink
        OS.add(fast_blink);
        Serial.print(F("Added task 'Task'"));
        Serial.println(F("'"));
        break;
        
      case 1:
        // Add slow blink
        OS.add(slow_blink);
        Serial.print(F("Added task 'Task'"));
        Serial.println(F("'"));
        break;
        
      case 2:
        // Suspend fast blink
        fast_blink->suspend();
        break;
        
      case 3:
        // Resume fast blink
        fast_blink->resume();
        break;
        
      case 4:
        // Terminate slow blink
        slow_blink->terminate();
        break;
        
      case 5:
        // Terminate fast blink and self
        fast_blink->terminate();
        this->terminate();
        break;
    }
    
    phase++;
  }
  
  uint8_t getMaxMessageBudget() const override { return 0; }
  uint16_t getTaskStructSize() const override { return sizeof(*this); }
};

ControlTask controller;

void setup() {
  Serial.begin(9600);
  while(!Serial) {
    ; // Wait for serial port to connect
  }
  
  OS.begin();
  OS.add(&controller);
  
  Serial.println(F("Task Lifecycle Demo Started"));
}

void loop() {
  OS.loopOnce();
}
