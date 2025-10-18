/**
 * @file TaskTimingMonitoring.ino
 * @brief Example demonstrating task timing monitoring and delay detection
 * @author Aykut Ozdemir
 * @date 2025-01-27
 * 
 * This example shows how to use FsmOS task timing monitoring features
 * to detect when tasks are delayed and identify which task is causing delays.
 * 
 * Features demonstrated:
 * - Task timing monitoring (always active)
 * - Delay detection and logging
 * - Delay attribution to specific tasks
 * - Task statistics including delay information
 */

#include "FsmOS.h"

// Task classes for demonstration
class FastTask : public Task
{
public:
    FastTask() : Task(F("FastTask")) 
    {
        setPeriod(10);  // Run every 10ms
        setPriority(PRIORITY_HIGH);
    }
    
    void step() override
    {
        // Simulate some work
        delay(2);  // 2ms work
        
        // No normal logging - only delay logs will appear
    }
    
    uint8_t getMaxMessageBudget() const override { return 0; }
    uint16_t getTaskStructSize() const override { return sizeof(FastTask); }
};

class SlowTask : public Task
{
public:
    SlowTask() : Task(F("SlowTask")) 
    {
        setPeriod(20);  // Run every 20ms
        setPriority(PRIORITY_NORMAL);
    }
    
    void step() override
    {
        // Simulate heavy work that causes delays
        delay(15);  // 15ms work - this will cause delays!
        
        // No normal logging - only delay logs will appear
    }
    
    uint8_t getMaxMessageBudget() const override { return 0; }
    uint16_t getTaskStructSize() const override { return sizeof(SlowTask); }
};

class MonitoringTask : public Task
{
public:
    MonitoringTask() : Task(F("MonitoringTask")) 
    {
        setPeriod(1000);  // Run every 1 second
        setPriority(PRIORITY_LOW);
    }
    
    void step() override
    {
        // Print timing statistics
        Serial.println(F("\n=== Task Timing Statistics ==="));
        
        TaskNode *current = nullptr;
        // We need to access the task list, but it's private
        // For this example, we'll use a different approach
        
        // Get stats for known tasks (this is a simplified approach)
        TaskStats stats;
        
        // Check FastTask stats
        if (OS.getTaskStats(1, stats))  // Assuming FastTask has ID 1
        {
            Serial.print(F("FastTask - Delays: "));
            Serial.print(stats.delayCount);
            Serial.print(F(", Max Delay: "));
            Serial.print(stats.maxDelayMs);
            Serial.println(F("ms"));
        }
        
        // Check SlowTask stats
        if (OS.getTaskStats(2, stats))  // Assuming SlowTask has ID 2
        {
            Serial.print(F("SlowTask - Delays: "));
            Serial.print(stats.delayCount);
            Serial.print(F(", Max Delay: "));
            Serial.print(stats.maxDelayMs);
            Serial.println(F("ms"));
        }
        
        // Get most delaying task
        uint8_t mostDelayingTaskId = OS.getMostDelayingTask();
        if (mostDelayingTaskId != 0)
        {
            Serial.print(F("Most delaying task ID: "));
            Serial.println(mostDelayingTaskId);
        }
        
        Serial.println(F("===============================\n"));
    }
    
    uint8_t getMaxMessageBudget() const override { return 0; }
    uint16_t getTaskStructSize() const override { return sizeof(MonitoringTask); }
};

// Task instances
FastTask fastTask;
SlowTask slowTask;
MonitoringTask monitoringTask;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    
    Serial.println(F("FsmOS Task Timing Monitoring Example"));
    Serial.println(F("====================================="));
    
    // Task timing monitoring is always active - no need to enable it
    
    // Add tasks to scheduler
    OS.add(&fastTask);
    OS.add(&slowTask);
    OS.add(&monitoringTask);
    
    // Start scheduler
    OS.begin();
    
    Serial.println(F("Scheduler started with timing monitoring always active"));
    Serial.println(F("Only delay warnings will be logged - watch for them!"));
}

void loop()
{
    OS.loopOnce();
}
