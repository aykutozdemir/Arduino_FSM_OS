#include <FsmOS.h>

// A simple task that blinks the built-in LED.
class BlinkTask : public Task {
public:
  BlinkTask() {
    // Set the task to run every 500 milliseconds.
    set_period(500);
  }

  // on_start() is called once when the task is added to the scheduler.
  void on_start() override {
    pinMode(LED_BUILTIN, OUTPUT);
  }

  // step() is called periodically by the scheduler.
  void step() override {
    // Toggle the LED state.
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
};

// Create an instance of our task.
BlinkTask blinker;

// Standard Arduino setup function.
void setup() {
  // Initialize the OS. This sets up the 1ms system tick.
  OS.begin();

  // Add our task to the scheduler.
  OS.add(&blinker);
}

// Standard Arduino loop function.
void loop() {
  // This is the heart of the OS. It must be called continuously.
  // It runs all scheduled tasks and delivers messages.
  OS.loop_once();
}
