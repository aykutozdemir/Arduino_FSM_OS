#include <FsmOS.h>

/* ====  ButtonTask(uint8_t button_pin) : Task(4), pin(button_pin) {
    set_period(5); // Poll the button every 5ms
    state = HIGH;
    last_reading = HIGH;
    debounce_counter = 0;========= App Configuration ================== */

// Event types for our application
enum : uint8_t {
  EVT_BUTTON_PRESSED = 1,
  EVT_BUTTON_RELEASED,
  EVT_SET_BLINK_INTERVAL,
};

// Topic for UI-related events
enum : uint8_t {
  TOPIC_UI = 1,
};

/* ================== ButtonTask ================== */
// This task debounces a button and publishes press/release events.

class ButtonTask : public Task {
  const uint8_t pin;
  uint8_t state;
  uint8_t last_reading;
  uint8_t debounce_counter;
  const uint8_t debounce_threshold = 5; // 5 * 5ms = 25ms debounce time

public:
  ButtonTask(uint8_t button_pin) : Task(4, 2), pin(button_pin) {
    set_period(5); // Poll the button every 5ms
    state = HIGH;
    last_reading = HIGH;
    debounce_counter = 0;
  }

  void on_start() override {
    pinMode(pin, INPUT_PULLUP);
  }

  void step() override {
    uint8_t reading = digitalRead(pin);

    if (reading == last_reading) {
      if (debounce_counter < debounce_threshold) {
        debounce_counter++;
      } else if (reading != state) {
        // The button state has changed and is stable
        state = reading;
        if (state == LOW) {
          // Button was pressed (active low)
          publish(TOPIC_UI, EVT_BUTTON_PRESSED);
        } else {
          // Button was released
          publish(TOPIC_UI, EVT_BUTTON_RELEASED);
        }
      }
    } else {
      // Reset counter if reading fluctuates
      debounce_counter = 0;
    }
    last_reading = reading;
  }
};

/* ================== LedTask ================== */
// This task controls an LED. It listens for button events to toggle blinking
// and can also receive direct messages to change its blink rate.

class LedTask : public Task {
  const uint8_t pin;
  Timer blink_timer;
  bool is_blinking = false;
  uint16_t blink_interval = 500;

public:
  LedTask(uint8_t led_pin) : Task(4), pin(led_pin) {
    period_ms = 10; // Run step() every 10ms
  }

  void on_start() override {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    subscribe(TOPIC_UI); // Listen to events on the UI topic
  }

  void on_msg(const Msg& m) override {
    switch (m.type) {
      case EVT_BUTTON_PRESSED:
        is_blinking = !is_blinking;
        if (is_blinking) {
          blink_timer.start(blink_interval);
          digitalWrite(pin, HIGH); // Turn on immediately
        } else {
          digitalWrite(pin, LOW); // Turn off when blinking stops
        }
        break;

      case EVT_SET_BLINK_INTERVAL:
        if (m.arg > 0) {
          blink_interval = m.arg;
        }
        break;
    }
  }

  void step() override {
    if (!is_blinking) return;

    if (blink_timer.expired()) {
      digitalWrite(pin, !digitalRead(pin)); // Toggle LED
      blink_timer.start(blink_interval);    // Restart timer
    }
  }
};

/* ================== App Wiring ================== */

// Create instances of our tasks
ButtonTask button_task(2); // Button on pin 2
LedTask led_task(LED_BUILTIN); // LED on built-in pin

void setup() {
  // Initialize the OS with max 4 tasks and a global queue of 32 messages
  // Using larger global queue since we have message-heavy tasks
  OS.begin(4, 32);

  // Add tasks to the scheduler. The 'add' method assigns an ID to the task object.
  OS.add(&led_task);
  OS.add(&button_task);

  // After adding the task, its 'id' field is populated.
  // A task can 'tell' itself to queue an initial message. This is useful
  // for configuration or triggering an initial state.
  led_task.tell(led_task.id, EVT_SET_BLINK_INTERVAL, 150);
}

void loop() {
  // Run the cooperative scheduler
  OS.loop_once();
}
();
}
