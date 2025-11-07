/*
  ButtonLed - Inter-Task Communication Example

  This example demonstrates how tasks can communicate using both direct messaging
  and publish/subscribe patterns in FsmOS. It creates two tasks:
  1. ButtonTask: Monitors a button and publishes events when pressed/released
  2. LedTask: Controls an LED based on button events and direct messages

  The example shows:
  - How to set up publish/subscribe communication
  - How to implement button debouncing
  - How to use the Timer utility for timing
  - How to handle different types of messages
  - How to configure tasks via direct messaging

  The circuit:
  - Push button connected between pin 2 and ground
  - Built-in LED on pin 13
  - 10K pull-up resistor for button (or use INPUT_PULLUP)

  Created October 2, 2025
  By Aykut Ozdemir

  https://github.com/aykutozdemir/Arduino_FSM_OS
*/

#include <FsmOS.h>

/* ================== Application Configuration ================== */

// Event types for our application
enum : uint8_t
{
    EVT_BUTTON_PRESSED = 1,   // Button press event
    EVT_BUTTON_RELEASED,      // Button release event
    EVT_SET_BLINK_INTERVAL,   // Configuration event for LED blink rate
};

// Communication topics
enum : uint8_t
{
    TOPIC_UI = 1,  // Topic for UI-related events (button presses, etc.)
};

/* ================== ButtonTask ================== */

/**
 * ButtonTask: Handles button input with debouncing
 *
 * This task demonstrates:
 * - Hardware input debouncing
 * - Publishing events to a topic
 * - Periodic task execution for polling
 */
class ButtonTask : public Task
{
    const uint8_t pin;               // Button input pin
    uint8_t state;                   // Current debounced state
    uint8_t last_reading;            // Last raw pin reading
    uint8_t debounce_counter;        // Counter for debounce stability
    const uint8_t DEBOUNCE_THRESHOLD = 5;  // 5 * 5ms = 25ms debounce time

public:
    ButtonTask(uint8_t button_pin)
        : Task(F("Button")),  // Task name for logging
          pin(button_pin)
    {
        setPeriod(5);  // Poll every 5ms for responsive button handling
        state = HIGH;
        last_reading = HIGH;
        debounce_counter = 0;
    }

    uint8_t getMaxMessageBudget() const override { return 2; }
    uint16_t getTaskStructSize() const override { return sizeof(*this); }

    void on_start() override
    {
        pinMode(pin, INPUT_PULLUP);
        logInfof(F("Button task started on pin %d"), pin);
    }

    void step() override
    {
        uint8_t reading = digitalRead(pin);

        if (reading == last_reading)
        {
            // Reading is stable
            if (debounce_counter < DEBOUNCE_THRESHOLD)
            {
                debounce_counter++;
            }
            else if (reading != state)
            {
                // Button state has changed and is stable
                state = reading;
                if (state == LOW)
                {
                    // Button pressed (active low)
                    publish(TOPIC_UI, EVT_BUTTON_PRESSED);
                    logDebug(F("Button pressed"));
                }
                else
                {
                    // Button released
                    publish(TOPIC_UI, EVT_BUTTON_RELEASED);
                    logDebug(F("Button released"));
                }
            }
        }
        else
        {
            // Reading changed - reset debounce counter
            debounce_counter = 0;
        }
        last_reading = reading;
    }
};

/* ================== LedTask ================== */

/**
 * LedTask: Controls LED behavior based on button events
 *
 * This task demonstrates:
 * - Subscribing to topics
 * - Handling different message types
 * - Using the Timer utility
 * - Combining periodic updates with event handling
 */
class LedTask : public Task
{
    const uint8_t pin;            // LED output pin
    Timer16 blink_timer;          // 500ms blink interval - 4 bytes
    bool is_blinking = false;     // Current blink state
    uint16_t blink_interval = 500; // Blink interval in ms

public:
    LedTask(uint8_t led_pin)
        : Task(F("LED")),  // Task name for logging
          pin(led_pin)
    {
        setPeriod(10);  // Check timer every 10ms for smooth blinking
    }

    uint8_t getMaxMessageBudget() const override { return 1; }
    uint16_t getTaskStructSize() const override { return sizeof(*this); }

    void on_start() override
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        subscribe(TOPIC_UI);  // Listen to UI events (button presses)
        blink_interval = 150;  // Set default blink interval
        logInfof(F("LED task started on pin %d"), pin);
    }

    void on_msg(const MsgData &m) override
    {
        switch (m.type)
        {
            case EVT_BUTTON_PRESSED:
                // Toggle blinking state
                is_blinking = !is_blinking;
                if (is_blinking)
                {
                    blink_timer = Timer16();
                    blink_timer.startTimer(blink_interval);
                    digitalWrite(pin, HIGH);
                    logDebug(F("Blinking started"));
                }
                else
                {
                    digitalWrite(pin, LOW);
                    logDebug(F("Blinking stopped"));
                }
                break;

            case EVT_SET_BLINK_INTERVAL:
                if (m.arg > 0)
                {
                    blink_interval = m.arg;
                    logDebugf(F("Blink interval set to %d ms"), blink_interval);
                }
                break;
        }
    }

    void step() override
    {
        if (!is_blinking)
        {
            return;
        }

        if (blink_timer.isExpired())
        {
            blink_timer.startTimer(blink_interval);
            digitalWrite(pin, !digitalRead(pin));  // Toggle LED
        }
    }
};

/* ================== Application Setup ================== */

// Create task instances
ButtonTask button_task(2);           // Button on pin 2
LedTask led_task(LED_BUILTIN);      // LED on built-in pin

void setup()
{
    Serial.begin(9600);  // Initialize serial for logging

    // Add tasks to the scheduler
    OS.add(&led_task);
    OS.add(&button_task);

    // Start the scheduler
    OS.begin();
}

void loop()
{
    OS.loopOnce();  // Run the cooperative scheduler
}
