/*
  Logger - Advanced Logging and Task Monitoring Example

  This example demonstrates FsmOS's built-in logging system:
  1. Different log levels (DEBUG, INFO, WARNING, ERROR)
  2. Task context in log messages
  3. Automatic timestamps and formatting
  4. Memory-efficient logging with PROGMEM strings
  5. System event logging

  The example creates two tasks:
  - WorkerTask: Generates various types of log messages
  - ControlTask: Manages worker lifecycle and logs state changes

  The circuit:
  - No additional hardware required
  - Serial Monitor should be set to 115200 baud

  Log Format:
  [time][level][task] message

  Where:
  - time: milliseconds since start
  - level: D=Debug, I=Info, W=Warning, E=Error
  - task: name of the task that generated the message

  Created October 2, 2025
  By Aykut Ozdemir

  https://github.com/aykutozdemir/Arduino_FSM_OS
*/

#include <FsmOS.h>

/**
 * WorkerTask: Demonstrates various logging features
 *
 * This task shows:
 * - Different log levels and when to use them
 * - Lifecycle event logging
 * - Condition monitoring and alerts
 * - Performance tracking
 */
class WorkerTask : public Task
{
    uint8_t counter;           // Used to cycle through message types
    Timer16 delay_timer;       // 10000ms timeout - 4 bytes
    Timer8 perf_timer;         // 0ms performance - 2 bytes
    const uint32_t OPERATION_TIMEOUT = 10000;  // 10 second timeout
    uint32_t total_operations;  // Track number of operations
    uint32_t failed_operations; // Track failures

public:
    WorkerTask()
        : Task(F("Worker")),
          counter(0),
          total_operations(0),
          failed_operations(0)
    {
        setPeriod(2000);  // Run every 2 seconds
    }

    uint8_t getMaxMessageBudget() const override { return 1; }
    uint16_t getTaskStructSize() const override { return sizeof(*this); }

    void on_start() override
    {
        logInfo(F("Worker task initialized"));
        logInfof(F("Operation timeout: %dms"), OPERATION_TIMEOUT);
        delay_timer = Timer16(); delay_timer.startTimer(OPERATION_TIMEOUT);
    }

protected:
    void step() override
    {
        counter++;
        total_operations++;

        // Start timing this operation
        perf_timer = Timer8(); perf_timer.startTimer(0);

        // Simulate different scenarios with logging
        switch (counter % 4)
        {
            case 0:
                // Debug: Detailed operation information
                logDebugf(F("Operation #%d starting..."), total_operations);
                logDebugf(F("Memory available: %d bytes"), OS.getFreeMemory());
                break;

            case 1:
                // Info: Normal operation completion
                logInfof(F("Operation #%d complete"), total_operations);
                logInfof(F("Processing time: %dms"), OS.now() - perf_timer.startMs);
                break;

            case 2:
                // Warning: Potential issues
                if (OS.getFreeMemory() < 1024)
                {
                    logWarnf(F("Low memory warning: %d bytes free"), OS.getFreeMemory());
                }
                if (perf_timer.isExpired())
                {
                    logWarn(F("Operation taking longer than usual"));
                }
                break;

            case 3:
                // Error: Operation failures
                failed_operations++;
                logErrorf(F("Operation #%d failed"), total_operations);
                logErrorf(F("Failure rate: %.1f%%"),
                          (failed_operations * 100.0f) / total_operations);
                break;
        }

        // Monitor long-running operations
        if (delay_timer.isExpired())
        {
            delay_timer.startTimer(OPERATION_TIMEOUT);
            logWarnf(F("Operation timeout - possible deadlock"));
            logWarnf(F("Total ops: %d, Failed: %d"),
                     total_operations, failed_operations);
        }
    }



};

/**
 * ControlTask: Manages worker lifecycle and demonstrates system logging
 *
 * This task shows:
 * - System event logging
 * - Task state transition logging
 * - Error handling and recovery
 * - Clean shutdown procedures
 */
class ControlTask : public Task
{
    WorkerTask *worker;     // The worker task we're managing
    uint8_t phase;         // Current demo phase
    Timer16 phase_timer;   // Variable phase timing - 4 bytes

public:
    ControlTask()
        : Task(F("Control")),
          phase(0)
    {
        worker = new WorkerTask();
        setPeriod(8000);  // Change phase every 8 seconds
    }

    uint8_t getMaxMessageBudget() const override { return 1; }
    uint16_t getTaskStructSize() const override { return sizeof(*this); }

    void on_start() override
    {
        logInfo(F("Control task starting - Demo Sequence:"));
        logInfo(F("Phase 0: Start worker"));
        logInfo(F("Phase 1: Suspend worker"));
        logInfo(F("Phase 2: Resume worker"));
        logInfo(F("Phase 3: Clean shutdown"));
        phase_timer = Timer16();
        phase_timer.startTimer(getPeriod());
    }

protected:
    void step() override
    {
        logInfof(F("\n=== Phase %d Starting ==="), phase);

        switch (phase)
        {
            case 0:
                // Initialize worker
                logInfo(F("Initializing worker task..."));
                if (!OS.add(worker))
                {
                    logError(F("Failed to add worker task!"));
                    this->terminate();
                    return;
                }
                break;

            case 1:
                // Suspend worker
                logInfo(F("Suspending worker task..."));
                worker->suspend();
                logInfo(F("Worker suspended"));
                break;

            case 2:
                // Resume worker
                logInfo(F("Resuming worker task..."));
                worker->resume();
                logInfo(F("Worker resumed"));
                break;

            case 3:
                // Clean shutdown
                logInfo(F("Initiating shutdown sequence..."));

                // Stop worker first
                logDebug(F("Terminating worker task..."));
                worker->terminate();

                // Stop self
                logInfo(F("All tasks terminated"));
                logInfo(F("Demo complete - check log output above"));
                this->terminate();
                break;
        }

        phase++;
        phase_timer = Timer16();
        phase_timer.startTimer(getPeriod());
    }

};

/* ================== Application Setup ================== */

void setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial) { /* wait for serial port */ }

    Serial.println(F("\n=== FsmOS Logger Demo ==="));
    Serial.println(F("Version: 1.2.0"));

    // Print demo information
    Serial.println(F("\nThis demo shows:"));
    Serial.println(F("1. Log Levels"));
    Serial.println(F("   DEBUG   - Detailed debugging information"));
    Serial.println(F("   INFO    - General operational messages"));
    Serial.println(F("   WARNING - Potential issues or concerns"));
    Serial.println(F("   ERROR   - Critical problems and failures"));

    Serial.println(F("\n2. Log Format"));
    Serial.println(F("   [time][level][task] message"));
    Serial.println(F("   Example: [1234][I][Worker] Task started"));

    Serial.println(F("\n3. Features"));
    Serial.println(F("   - Automatic timestamps"));
    Serial.println(F("   - Task context tracking"));
    Serial.println(F("   - Memory-efficient logging"));
    Serial.println(F("   - System event capture"));

    // Initialize the OS with logging enabled
    OS.begin(); // With logger);

    // Create and add the control task
    OS.add(new ControlTask());

    Serial.println(F("\nDemo starting...\n"));
}

void loop()
{
    OS.loopOnce();  // Run the cooperative scheduler
}