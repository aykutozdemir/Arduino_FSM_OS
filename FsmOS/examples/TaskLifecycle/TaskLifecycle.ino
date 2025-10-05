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
  const char* blink_type;    // Description of blink rate (for logging)
  Timer16 blink_timer;       // Variable blink timing - 4 bytes
  
public:
  BlinkTask(uint8_t led_pin, uint32_t period_ms, const char* type) 
    : Task(F(type)),  // Use type as task name
      pin(led_pin), 
      state(false),
      blink_type(type)
  {
    set_period(period_ms);
  }

  void on_start() override {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);
    log_info(F("%s blinker started on pin %d"), blink_type, pin);
    log_info(F("Period: %dms"), get_period());
  }

  void step() override {
    // Toggle LED state
    state = !state;
    digitalWrite(pin, state);
    log_debug(F("%s LED %s"), blink_type, state ? "ON" : "OFF");
  }

  void on_suspend() override {
    // Always turn off LED when suspended
    digitalWrite(pin, LOW);
    log_info(F("%s blinker suspended"), blink_type);
  }

  void on_resume() override {
    // Restore LED to previous state
    digitalWrite(pin, state);
    log_info(F("%s blinker resumed"), blink_type);
  }

  void on_terminate() override {
    // Clean up: ensure LED is off
    digitalWrite(pin, LOW);
    log_info(F("%s blinker terminated"), blink_type);
    Task::on_terminate();  // Call base class terminate
  }
};

/**
 * ControlTask: Manages other tasks through their lifecycle states
 * 
 * This task demonstrates:
 * - Task creation and initialization
 * - State transitions between ACTIVE, SUSPENDED, and INACTIVE
 * - Task cleanup and memory management
 */
class ControlTask : public Task {
  BlinkTask* fast_blink;     // Fast blinking LED task
  BlinkTask* slow_blink;     // Slow blinking LED task
  uint8_t phase;             // Current demo phase
  Timer16 phase_timer;       // 5000ms phase duration - 4 bytes
  
  // Phase durations
  static const uint32_t PHASE_DURATION = 5000;  // 5 seconds per phase
  
public:
  ControlTask() : Task(F("Controller")), phase(0) {
    // Create blink tasks but don't add them yet
    fast_blink = new BlinkTask(LED_BUILTIN, 200, "Fast");      // 200ms period
    slow_blink = new BlinkTask(LED_BUILTIN + 1, 1000, "Slow"); // 1000ms period
    set_period(PHASE_DURATION);
  }

  void on_start() override {
    log_info(F("\n=== Task Lifecycle Demo Started ==="));
    log_info(F("Phase duration: %d seconds"), PHASE_DURATION/1000);
    phase_timer = create_timer_typed<Timer16>(PHASE_DURATION);
  }

protected:
  void step() override {
    log_info(F("\n--- Phase %d ---"), phase);
    
    switch(phase) {
      case 0:
        // Start with fast blink
        log_info(F("Starting fast blink task"));
        OS.add(fast_blink);
        break;
        
      case 1:
        // Add slow blink
        log_info(F("Adding slow blink task"));
        OS.add(slow_blink);
        break;
        
      case 2:
        // Suspend fast blink
        log_info(F("Suspending fast blink task"));
        fast_blink->suspend();
        break;
        
      case 3:
        // Resume fast blink
        log_info(F("Resuming fast blink task"));
        fast_blink->activate();
        break;
        
      case 4:
        // Terminate slow blink
        log_info(F("Terminating slow blink task"));
        slow_blink->terminate();
        break;
        
      case 5:
        // Clean up all tasks
        log_info(F("Final cleanup - terminating all tasks"));
        fast_blink->terminate();
        this->terminate();
        log_info(F("\n=== Demo Complete ==="));
        break;
    }
    
    // Advance to next phase
    phase++;
  }
  
  void on_terminate() override {
    log_info(F("Controller task terminated"));
    Task::on_terminate();
  }
};

/* ================== Application Setup ================== */

void setup() {
  // Initialize serial and wait for connection
  Serial.begin(115200);
  while (!Serial) { /* wait for serial port */ }
  
  Serial.println(F("\n=== Task Lifecycle Demo ==="));
  Serial.println(F("Version: 1.2.0"));
  
  // Initialize OS with logging enabled
  OS.begin_with_logger();
  
  // Print demo information
  Serial.println(F("\nThis demo demonstrates:"));
  Serial.println(F("1. Task States:"));
  Serial.println(F("   - ACTIVE: Task runs normally"));
  Serial.println(F("   - SUSPENDED: Task paused but preserved"));
  Serial.println(F("   - INACTIVE: Task marked for deletion"));
  
  Serial.println(F("\n2. Lifecycle Events:"));
  Serial.println(F("   - on_start: Called when task begins"));
  Serial.println(F("   - on_suspend: Called when task paused"));
  Serial.println(F("   - on_resume: Called when task reactivated"));
  Serial.println(F("   - on_terminate: Called before task deleted"));
  
  Serial.println(F("\n3. Memory Management:"));
  Serial.println(F("   - Automatic task cleanup"));
  Serial.println(F("   - Resource handling in callbacks"));
  
  // Create and add the control task
  OS.add(new ControlTask());
  
  Serial.println(F("\nDemo starting...\n"));
}

void loop() {
  OS.loop_once();  // Run the cooperative scheduler
}