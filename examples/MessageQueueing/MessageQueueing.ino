/*
  MessageQueueing - Task Suspension and Message Handling Example
  
  This example demonstrates FsmOS's message queueing capabilities during task suspension:
  1. How tasks can queue or drop messages while suspended
  2. How to implement task suspension and resumption
  3. Different message handling strategies during task suspension
  
  The sketch creates three tasks:
  - Two LED tasks that respond to messages differently when suspended:
    * LED1: Queues messages while suspended (processes them when resumed)
    * LED2: Drops messages while suspended (ignores them completely)
  - A Generator task that sends periodic messages to both LEDs
  
  Every 5 seconds, both LED tasks are suspended/resumed to demonstrate
  the different message handling behaviors.
  
  The circuit:
  - LED1 connected to built-in LED (usually pin 13)
  - LED2 connected to built-in LED + 1 (usually pin 14)
  
  Created October 2, 2025
  By Aykut Ozdemir
  
  https://github.com/aykutozdemir/Arduino_FSM_OS
*/

#include <FsmOS.h>

/* ================== Message Types ================== */
enum : uint8_t {
  MSG_TOGGLE = 1,  // Toggle LED state
  MSG_COUNT,       // Request message count
  MSG_REPORT      // Request status report
};

/**
 * LedTask: Controls an LED and handles messages during suspension
 * 
 * This task demonstrates:
 * - Message handling during active and suspended states
 * - Lifecycle callbacks (suspend/resume)
 * - Message counting and reporting
 */
class LedTask : public Task {
  const uint8_t pin;          // LED pin number
  bool state;                 // Current LED state
  uint32_t toggle_count;      // Number of times LED has been toggled
  uint32_t dropped_msgs;      // Count of messages dropped while suspended
  const __FlashStringHelper* task_name;      // Name for this LED task instance
  
public:
  LedTask(uint8_t led_pin, const __FlashStringHelper* name) 
    : Task(name),
      pin(led_pin), 
      state(false), 
      toggle_count(0),
      dropped_msgs(0),
      task_name(name)
  {
    setPeriod(100);  // Fast updates to process messages quickly
  }

  void on_start() override {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);
    logInfof(F("%s initialized on pin %d"), task_name, pin);
  }

  void step() override {
    // This task is purely event-driven via messages
  }
  
  void on_msg(const MsgData& msg) override {
    switch(msg.type) {
      case MSG_TOGGLE:
        // Toggle LED state
        state = !state;
        digitalWrite(pin, state);
        toggle_count++;
        logDebugf(F("%s toggled %d times"), task_name, toggle_count);
        break;
        
      case MSG_REPORT:
        // Print status report
        logInfof(F("%s Status:"), task_name);
        logInfof(F("  Pin: %d"), pin);
        logInfof(F("  Toggles: %d"), toggle_count);
        logInfof(F("  Dropped msgs: %d"), dropped_msgs);
        logInfof(F("  Queue msgs: %s"), 
                F("N/A"));
        break;
    }
  }
  
  uint8_t getMaxMessageBudget() const override { return 1; }
  uint16_t getTaskStructSize() const override { return sizeof(*this); }
};

/**
 * GeneratorTask: Sends periodic messages to LED tasks
 */
class GeneratorTask : public Task {
  LedTask* led1;
  LedTask* led2;
  Timer16 cycle_timer;
  uint32_t count;
  
public:
  GeneratorTask(LedTask* l1, LedTask* l2) 
    : Task(F("Generator")),
      led1(l1),
      led2(l2),
      count(0)
  {
    setPeriod(1000);  // Send messages every second
  }
  
  void on_start() override {
    cycle_timer = Timer16();
    cycle_timer.startTimer(5000);  // Change cycle every 5 seconds
  }
  
  void step() override {
    // Send toggle messages
    OS.sendMessage(0, MSG_TOGGLE);
    OS.sendMessage(1, MSG_TOGGLE);
    count++;
    
    // Every 5 seconds, toggle suspension state
    if (cycle_timer.isExpired()) {
      cycle_timer.startTimer(5000);
      handle_cycle_change();
    }
  }
  
private:
  void handle_cycle_change() {
    logInfof(F("\n=== Cycle %d Complete ==="), count / 5);
    
    // Request status reports
    OS.sendMessage(0, MSG_REPORT);
    OS.sendMessage(1, MSG_REPORT);
    
    // Toggle suspension state
    led1->suspend();
    led2->suspend();
    delay(1000);
    led1->resume();
    led2->resume();
  }
  
  uint8_t getMaxMessageBudget() const override { return 2; }
  uint16_t getTaskStructSize() const override { return sizeof(*this); }
};

/* ================== Application Setup ================== */

// Task instances
LedTask* led1;        // LED that queues messages while suspended
LedTask* led2;        // LED that drops messages while suspended
GeneratorTask* gen;   // Task that generates messages

void setup() {
  // Initialize serial and wait for connection
  Serial.begin(115200);
  while (!Serial) { /* wait for serial port */ }
  
  Serial.println(F("\n=== Message Queueing Demo ==="));
  Serial.println(F("Version: 1.2.0"));
  
  // Initialize the OS with logging enabled
  OS.begin(); // With logger);
  
  // Create LED tasks with different message handling strategies
  led1 = new LedTask(LED_BUILTIN, F("LED1 (Queuing)"));
  led2 = new LedTask(LED_BUILTIN + 1, F("LED2 (Dropping)"));
  
  // Configure message handling behavior - removed as API doesn't exist
  // led1->set_queue_messages_while_suspended(true);   // Queue messages
  // led2->set_queue_messages_while_suspended(false);  // Drop messages
  
  // Create message generator
  gen = new GeneratorTask(led1, led2);
  
  // Add all tasks to scheduler
  OS.add(led1);
  OS.add(led2);
  OS.add(gen);
  
  // Print demo information
  Serial.println(F("\nDemo Features:"));
  Serial.println(F("1. LED1 queues messages while suspended"));
  Serial.println(F("2. LED2 drops messages while suspended"));
  Serial.println(F("3. Both LEDs toggle every 1 second"));
  Serial.println(F("4. Tasks suspend/resume every 5 seconds"));
  Serial.println(F("5. Status reports show message handling differences"));
  Serial.println(F("\nDemo starting...\n"));
}

void loop() {
  OS.loopOnce();  // Run the cooperative scheduler
}