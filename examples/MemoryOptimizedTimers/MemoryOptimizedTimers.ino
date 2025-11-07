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
 * - Use Timer8(); Timer8(); TimerTimer8 timer_var; timer_var.startTimer(duration); timer_var.startTimer(duration).startTimer(duration) for short durations (0-255ms)
 * - Use Timer16(); Timer16(); Timer16(); Timer16 timer_var; timer_var.startTimer(duration); timer_var.startTimer(duration).startTimer(duration).startTimer(duration) for medium durations (256-65535ms)
 * - Use Timer32(); Timer32(); TimerTimer32 timer_var; timer_var.startTimer(duration); timer_var.startTimer(duration).startTimer(duration) for long durations (65536ms+)
 */

#include <FsmOS.h>

// Example task showing different timer types
class TimerDemoTask : public Task {
public:
  TimerDemoTask() : Task(F("TimerDemo")) {}

  virtual void on_start() override {
    // Create different types of timers based on duration needs
    
    // Short duration (10ms) - use Timer8 (2 bytes)
    shortTimer = Timer8();
    shortTimer.startTimer(10);
    
    // Medium duration (1500ms) - use Timer16 (4 bytes) 
    mediumTimer = Timer16();
    mediumTimer.startTimer(1500);
    
    // Long duration (30000ms) - use Timer32 (8 bytes)
    longTimer = Timer32();
    longTimer.startTimer(30000);
    
    // Default timer (8 bytes) - same as Timer32
    defaultTimer = Timer32();
    defaultTimer.startTimer(5000);
    
    logInfo(F("TimerDemo started with memory-optimized timers"));
    logInfo(F("Timer8 (10ms): 2 bytes"));
    logInfo(F("Timer16 (1500ms): 4 bytes"));
    logInfo(F("Timer32 (30000ms): 8 bytes"));
    logInfo(F("Timer32 (5000ms): 8 bytes"));
    logInfo(F("Total memory used: 22 bytes vs 32 bytes (31% savings)"));
  }

  virtual void step() override {
    // Check short timer (10ms)
    if (shortTimer.isExpired()) {
      shortTimer.startTimer(10);
      logDebug(F("Short timer fired (10ms)"));
    }
    
    // Check medium timer (1500ms)
    if (mediumTimer.isExpired()) {
      mediumTimer.startTimer(1500);
      logInfo(F("Medium timer fired (1500ms)"));
    }
    
    // Check long timer (30000ms)
    if (longTimer.isExpired()) {
      longTimer.startTimer(30000);
      logInfo(F("Long timer fired (30000ms)"));
    }
    
    // Check default timer (5000ms)
    if (defaultTimer.isExpired()) {
      defaultTimer.startTimer(5000);
      logInfo(F("Default timer fired (5000ms)"));
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
  OS.add(&timerDemo);
  
  Serial.println(F("System started. Check serial output for timer events."));
}

void loop() {
  OS.loopOnce();
}
