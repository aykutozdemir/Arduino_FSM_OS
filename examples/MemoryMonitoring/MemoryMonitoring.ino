/*
  MemoryMonitoring - Advanced Memory Management Example

  This example demonstrates FsmOS's memory monitoring capabilities:
  1. System memory tracking
     - RAM usage and fragmentation
     - Heap status and largest free block
     - Stack usage monitoring
     - Flash memory utilization

  2. Task memory analysis
     - Individual task memory consumption
     - Message queue memory usage
     - Subscription storage overhead

  3. Dynamic memory stress testing
     - Controlled memory allocation/deallocation
     - Fragmentation monitoring
     - Memory leak detection

  The example creates two tasks:
  - MemoryMonitorTask: Reports detailed memory statistics
  - MemoryStressTask: Performs controlled memory allocations

  The circuit:
  - No additional hardware required
  - Serial Monitor should be set to 115200 baud

  Created October 2, 2025
  By Aykut Ozdemir

  https://github.com/aykutozdemir/Arduino_FSM_OS
*/

#include <FsmOS.h>

/**
 * Helper Functions
 */
/**
 * Formats and prints memory sizes in human-readable format (bytes, KB, MB)
 * Uses PROGMEM strings to minimize RAM usage
 */
void print_size(const __FlashStringHelper *label, uint32_t size)
{
    // Print the label (stored in flash memory)
    Serial.print(label);

    // Format size based on magnitude
    if (size < 1024)
    {
        // Less than 1KB - print in bytes
        Serial.print(size);
        Serial.println(F(" bytes"));
    }
    else if (size < 1024 * 1024)
    {
        // Less than 1MB - print in KB with one decimal place
        Serial.print(size / 1024);
        Serial.print('.');
        Serial.print((size % 1024) * 10 / 1024);
        Serial.println(F(" KB"));
    }
    else
    {
        // 1MB or larger - print in MB with one decimal place
        Serial.print(size / (1024 * 1024));
        Serial.print('.');
        Serial.print((size % (1024 * 1024)) * 10 / (1024 * 1024));
        Serial.println(F(" MB"));
    }
}

/**
 * MemoryMonitorTask: Monitors and reports system memory usage
 *
 * This task demonstrates:
 * - How to use SystemMemoryInfo to get system-wide memory stats
 * - How to use TaskMemoryInfo to analyze per-task memory usage
 * - Memory monitoring best practices
 */
class MemoryMonitorTask : public Task
{
public:
    MemoryMonitorTask() : Task(F("MemMon"))
    {
        setPeriod(5000);  // Report every 5 seconds
    }

    uint8_t getMaxMessageBudget() const override { return 1; }
    uint16_t getTaskStructSize() const override { return sizeof(*this); }

protected:
    void step() override
    {
        SystemMemoryInfo sys_info;
        OS.getSystemMemoryInfo(sys_info);

        Serial.println(F("\nSystem Memory Status:"));
        Serial.println(F("==================="));

        // RAM Usage
        Serial.println(F("\nRAM:"));
        print_size(F("  Total: "), sys_info.totalRam);
        print_size(F("  Free:  "), sys_info.freeRam);
        print_size(F("  Used:  "), sys_info.totalRam - sys_info.freeRam);

        // Heap Status
        Serial.println(F("\nHeap:"));
        print_size(F("  Size:        "), sys_info.heapSize);
        print_size(F("  Largest Free: "), sys_info.largestBlock);
        Serial.print(F("  Fragments:   "));
        Serial.println(sys_info.heapFragments);
        Serial.print(F("  Fragmentation: "));
        Serial.print(OS.getHeapFragmentation());
        Serial.println(F("%"));

        // Stack Usage
        Serial.println(F("\nStack:"));
        print_size(F("  Size:  "), sys_info.stackSize);
        print_size(F("  Used:  "), sys_info.stackUsed);
        print_size(F("  Free:  "), sys_info.stackFree);

        // Task Memory
        Serial.println(F("\nTasks:"));
        Serial.print(F("  Count:  "));
        Serial.println(sys_info.totalTasks);
        print_size(F("  Memory: "), sys_info.taskMemory);

        // Message System
        Serial.println(F("\nMessages:"));
        Serial.print(F("  Active: "));
        Serial.println(sys_info.activeMessages);
        print_size(F("  Memory: "), sys_info.messageMemory);

        // Flash Usage
        Serial.println(F("\nProgram Memory:"));
        print_size(F("  Used:  "), sys_info.flashUsed);
        print_size(F("  Free:  "), sys_info.flashFree);

        Serial.println(F("\nTask Details:"));
        Serial.println(F("============"));

        // Print info for each task
        for (uint8_t i = 0; i < OS.getTaskCount(); i++)
        {
            Task *task = OS.getTask(i);
            if (task)
            {
                TaskMemoryInfo task_info;
                OS.getTaskMemoryInfo(i, task_info);

                Serial.print(F("\nTask '"));
                Serial.print(F("Task"));
                Serial.println(F("':"));
                print_size(F("  Structure:    "), task_info.task_struct_size);
                print_size(F("  Subscriptions: "), task_info.subscription_size);
                print_size(F("  Queue:        "), task_info.queue_size);
                print_size(F("  Total:        "), task_info.total_allocated);
            }
        }

        Serial.println(F("\n"));
    }
};

/**
 * MemoryStressTask: Performs controlled memory stress testing
 *
 * This task demonstrates:
 * - Dynamic memory allocation patterns
 * - Memory cleanup in destructors
 * - How to safely handle allocation failures
 * - Impact of fragmentation on memory availability
 */
class MemoryStressTask : public Task
{
    uint8_t phase;                    // Current test phase
    uint8_t *buffer;                  // Dynamic buffer for testing
    size_t buffer_size;               // Current buffer size
    const size_t MAX_BUFFER = 300;    // Maximum allocation size
    Timer16 phase_timer;              // Variable phase timing - 4 bytes
    Timer16 delay_timer;              // Delay timer - 4 bytes

public:
    MemoryStressTask()
        : Task(F("MemStress")),
          phase(0),
          buffer(nullptr),
          buffer_size(0)
    {
        setPeriod(10000);  // Change allocation every 10 seconds
    }

    uint8_t getMaxMessageBudget() const override { return 1; }
    uint16_t getTaskStructSize() const override { return sizeof(*this); }

    void on_start() override
    {
        logInfo(F("Memory stress testing started"));
        logInfof(F("Max allocation: %d bytes"), MAX_BUFFER);
        phase_timer = Timer16();
        phase_timer.startTimer(getPeriod());
        delay_timer = Timer16();
        delay_timer.startTimer(getPeriod());
    }

    ~MemoryStressTask()
    {
        cleanup_buffer();  // Ensure memory is freed
    }

protected:
    void step() override
    {
        cleanup_buffer();  // Free previous allocation

        // Determine new buffer size based on phase
        switch (phase)
        {
            case 0:
                buffer_size = 100;  // Small allocation
                break;
            case 1:
                buffer_size = 200;  // Medium allocation
                break;
            case 2:
                buffer_size = 50;   // Reduced allocation
                break;
            case 3:
                buffer_size = 300;  // Large allocation
                break;
            default:
                buffer_size = 0;    // No allocation (rest phase)
                break;
        }

        // Attempt allocation if size is non-zero
        if (buffer_size > 0)
        {
            logInfof(F("Attempting %d byte allocation..."), buffer_size);

            buffer = new uint8_t[buffer_size];
            if (buffer)
            {
                // Fill buffer with test pattern
                for (size_t i = 0; i < buffer_size; i++)
                {
                    buffer[i] = i & 0xFF;
                }
                logInfo(F("Allocation successful"));
            }
            else
            {
                logError(F("Allocation failed!"));
            }
        }
        else
        {
            logInfo(F("Rest phase - no allocation"));
        }

        // Advance to next phase
        phase = (phase + 1) % 5;
        phase_timer = Timer16();
        phase_timer.startTimer(getPeriod());
        delay_timer = Timer16();
        delay_timer.startTimer(getPeriod());
    }

private:
    void cleanup_buffer()
    {
        if (buffer)
        {
            logDebugf(F("Freeing %d byte buffer"), buffer_size);
            delete[] buffer;
            buffer = nullptr;
            buffer_size = 0;
        }
    }
};

/* ================== Application Setup ================== */

void setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial) { /* wait for serial port */ }

    Serial.println(F("\n=== Memory Monitoring Demo ==="));
    Serial.println(F("Version: 1.2.0"));

    // Initialize the OS with logging
    OS.begin(); // With logger);

    // Print demo information
    Serial.println(F("\nThis demo monitors:"));
    Serial.println(F("1. System Memory"));
    Serial.println(F("   - RAM usage and fragmentation"));
    Serial.println(F("   - Stack and heap analysis"));
    Serial.println(F("   - Program memory (flash) usage"));

    Serial.println(F("\n2. Task Memory"));
    Serial.println(F("   - Per-task allocation tracking"));
    Serial.println(F("   - Message queue memory usage"));
    Serial.println(F("   - Memory leak detection"));

    Serial.println(F("\n3. Memory Stress Testing"));
    Serial.println(F("   - Controlled allocation cycles"));
    Serial.println(F("   - Fragmentation monitoring"));
    Serial.println(F("   - Allocation failure handling"));

    // Create and add tasks
    OS.add(new MemoryMonitorTask());
    OS.add(new MemoryStressTask());

    Serial.println(F("\nDemo starting...\n"));
}

void loop()
{
    OS.loopOnce();  // Run the cooperative scheduler
}