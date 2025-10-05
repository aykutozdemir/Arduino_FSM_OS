/*
 * Memory-Optimized Timers Example
 * 
 * This example demonstrates how to use memory-optimized timers
 * to save RAM by choosing the appropriate timer type based on
 * your duration requirements.
 * 
 * Timer Types:
 * - Timer8:  0-255ms, 2 bytes total (1 byte start + 1 byte duration)
 * - Timer16: 0-65535ms, 4 bytes total (2 bytes start + 2 bytes duration)  
 * - Timer32: 0-4294967295ms, 8 bytes total (4 bytes start + 4 bytes duration)
 * 
 * Usage:
 * - Use create_timer_typed<Timer8>(duration) for short durations (0-255ms)
 * - Use create_timer_typed<Timer16>(duration) for medium durations (256-65535ms)
 * - Use create_timer_typed<Timer32>(duration) for long durations (65536ms+)
 */

#include <FsmOS.h>

// Example task showing different timer types
class TimerDemoTask : public Task {
public:
  TimerDemoTask() : Task("TimerDemo") {}

  virtual void on_start() override {
    // Create different types of timers based on duration needs
    
    // Short duration (10ms) - use Timer8 (2 bytes)
    shortTimer = create_timer_typed<Timer8>(10);
    
    // Medium duration (1500ms) - use Timer16 (4 bytes) 
    mediumTimer = create_timer_typed<Timer16>(1500);
    
    // Long duration (30000ms) - use Timer32 (8 bytes)
    longTimer = create_timer_typed<Timer32>(30000);
    
    // Default timer (8 bytes) - same as Timer32
    defaultTimer = create_timer_typed<Timer32>(5000);
    
    log_info(F("TimerDemo started with memory-optimized timers"));
    log_info(F("Timer8 (10ms): 2 bytes"));
    log_info(F("Timer16 (1500ms): 4 bytes"));
    log_info(F("Timer32 (30000ms): 8 bytes"));
    log_info(F("Timer32 (5000ms): 8 bytes"));
    log_info(F("Total memory used: 22 bytes vs 32 bytes (31% savings)"));
  }

  virtual void step() override {
    // Check short timer (10ms)
    if (shortTimer.expired()) {
      shortTimer.start(10);
      log_debug(F("Short timer fired (10ms)"));
    }
    
    // Check medium timer (1500ms)
    if (mediumTimer.expired()) {
      mediumTimer.start(1500);
      log_info(F("Medium timer fired (1500ms)"));
    }
    
    // Check long timer (30000ms)
    if (longTimer.expired()) {
      longTimer.start(30000);
      log_info(F("Long timer fired (30000ms)"));
    }
    
    // Check default timer (5000ms)
    if (defaultTimer.expired()) {
      defaultTimer.start(5000);
      log_info(F("Default timer fired (5000ms)"));
    }
  }

private:
  Timer8 shortTimer;     // 2 bytes - for durations 0-255ms
  Timer16 mediumTimer;   // 4 bytes - for durations 0-65535ms  
  Timer32 longTimer;     // 8 bytes - for durations 0-4294967295ms
  Timer32 defaultTimer;  // 8 bytes - explicit 32-bit timer
};

// Create the demo task
TimerDemoTask timerDemo;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println(F("=== Memory-Optimized Timers Example ==="));
  Serial.println(F("This example shows how to save RAM by choosing"));
  Serial.println(F("the right timer type for your duration needs."));
  Serial.println();
  
  // Start the OS
  OS.begin();
  
  // Add the demo task
  OS.addTask(timerDemo);
  
  Serial.println(F("System started. Check serial output for timer events."));
}

void loop() {
  OS.step();
}
