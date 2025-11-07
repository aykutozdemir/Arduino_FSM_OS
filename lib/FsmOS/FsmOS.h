/**
 * @file FsmOS.h
 * @brief A lightweight cooperative task scheduler for Arduino
 * @author Aykut Özdemir <aykutozdemirgyte@gmail.com>
 * @date 2025-10-02
 *
 * FsmOS provides a simple, memory-efficient task scheduler for Arduino,
 * supporting cooperative multitasking, message passing, and system monitoring.
 *
 * Key features:
 * - Cooperative task scheduling with configurable periods
 * - Inter-task communication through messages and events
 * - Dynamic task creation and deletion
 * - Memory management and monitoring
 * - System diagnostics and profiling
 * - Logging system with multiple levels
 *
 * @note This library is designed for AVR-based Arduino boards but includes
 * partial support for other architectures.
 *
 * @version 1.3.0 - Major refactoring and code organization
 * @copyright 2025 Aykut Özdemir <aykutozdemirgyte@gmail.com>
 */
/**
 * @defgroup fsmos FsmOS
 * @brief Lightweight cooperative scheduler and message-passing library for Arduino.
 * @details Provides tasks, scheduler, message passing, synchronization primitives, and diagnostics
 *          with a focus on small RAM/flash usage and clear APIs.
 */


#ifndef FSMOS_H
#define FSMOS_H

/* ================== Core Dependencies ================== */
#include <Arduino.h>
#include <stdarg.h>
#include <stdint.h>
#include <util/atomic.h>  // For ATOMIC_BLOCK

#include <new>  // For placement new operator

#if defined(__AVR__)
#include <avr/io.h>   // For GPIOR0 and MCUSR
#include <avr/wdt.h>  // For watchdog timer
#else
// Fallback for non-AVR platforms
#define ATOMIC_BLOCK(type) for (uint8_t _ab_once = 1; _ab_once; _ab_once = 0)
#define ATOMIC_RESTORESTATE
#endif

// ================== Stack Canary Configuration ==================
/**
 * @brief Safety margin (bytes) to keep between stack canary and current stack pointer
 * @note Prevents corruption of active stack frames and ISR stacks
 */
#ifndef FSMOS_STACK_CANARY_MARGIN
#define FSMOS_STACK_CANARY_MARGIN 32
#endif

/**
 * @brief Topic bitfield configuration
 * @ingroup fsmos
 * @details Choose the bitfield size based on your topic count:
 * - TOPIC_BITFIELD_8: 8 topics max (1 byte)
 * - TOPIC_BITFIELD_16: 16 topics max (2 bytes) - DEFAULT
 * - TOPIC_BITFIELD_32: 32 topics max (4 bytes)
 */
#ifndef TOPIC_BITFIELD_SIZE
#define TOPIC_BITFIELD_SIZE 16  // Default: 16-bit bitfield
#endif

// Bitfield type selection based on define
#if TOPIC_BITFIELD_SIZE <= 8
typedef uint8_t TopicBitfield;
#define MAX_TOPICS 8
#elif TOPIC_BITFIELD_SIZE <= 16
typedef uint16_t TopicBitfield;
#define MAX_TOPICS 16
#elif TOPIC_BITFIELD_SIZE <= 32
typedef uint32_t TopicBitfield;
#define MAX_TOPICS 32
#else
#error "TOPIC_BITFIELD_SIZE must be 8, 16, or 32"
#endif

/* ================== Forward Declarations ================== */
class Task;       ///< Forward declaration for Task class
class Scheduler;  ///< Forward declaration for Scheduler class
// ================== Message Queue Constants (must appear before use) ==================
#ifndef MAX_MESSAGE_POOL_SIZE
#define MAX_MESSAGE_POOL_SIZE 32
#endif

/* ================== Task Node Structure ================== */
/**
 * @brief Node structure for Task linked list
 * @ingroup fsmos
 */
struct TaskNode
{
    Task *task;           ///< Pointer to the task
    TaskNode *next;       ///< Pointer to next node

    TaskNode(Task *t) : task(t), next(nullptr) {}
};

/* ================== Additional Types and Structures ================== */
/**
 * @brief Memory tracking statistics
 * @details Used for memory leak detection and monitoring
 * @ingroup fsmos
 */
struct MemoryStats
{
    uint32_t total_allocated;  ///< Total bytes allocated
    uint32_t total_freed;      ///< Total bytes freed
    uint32_t peak_usage;       ///< Peak memory usage
    uint32_t current_usage;    ///< Current memory usage
};

/**
 * @brief Task execution statistics
 * @details Used for performance monitoring and debugging
 * @ingroup fsmos
 */
struct TaskStats
{
    uint8_t taskId;              ///< Task identifier
    const __FlashStringHelper *name;             ///< Task name
    uint8_t state;                ///< Current task state (Task::State enum value)
    uint16_t periodMs;           ///< Task period in milliseconds
    uint8_t priority;             ///< Task priority
    uint32_t runCount;           ///< Number of times task has run
    uint32_t maxExecTimeUs;    ///< Maximum execution time in microseconds
    uint32_t totalExecTimeUs;  ///< Total execution time in microseconds
    uint16_t stackUsage;         ///< Stack usage in bytes
    uint16_t delayCount;         ///< Number of times task was delayed
    uint16_t maxDelayMs;         ///< Maximum delay experienced in milliseconds
};

/**
 * @brief System reset information
 * @details Used for debugging system resets and crashes
 * @ingroup fsmos
 */
struct ResetInfo
{
    uint8_t resetReason;       ///< Reason for reset
    uint32_t resetTime;        ///< Time of reset
    uint16_t watchdogTimeout;  ///< Watchdog timeout value
    uint8_t lastTaskId;       ///< ID of last running task

    // Optiboot reset flag information
    uint8_t optibootResetFlags;  ///< Raw reset flags from Optiboot GPIOR0
    uint8_t optibootResetCause;  ///< Processed reset cause (ResetCause enum)
};

/**
 * @brief Reset cause enumeration for Optiboot reset flags
 * @details Used to identify the cause of system reset
 * @ingroup fsmos
 */
enum ResetCause
{
    RESET_UNKNOWN = 0,  ///< Unknown reset cause
    RESET_POWER_ON,     ///< Power-on reset
    RESET_EXTERNAL,     ///< External reset
    RESET_BROWN_OUT,    ///< Brown-out reset
    RESET_WATCHDOG,     ///< Watchdog reset
    RESET_MULTIPLE      ///< Multiple reset causes detected
};

/**
 * @brief Reset cause flag constants from MCUSR register
 * @details These flags are stored by Optiboot in GPIOR0
 * @ingroup fsmos
 */
#if defined(__AVR__)
#define RESET_CAUSE_EXTERNAL (1 << EXTRF)  ///< External Reset flag
#define RESET_CAUSE_BROWN_OUT (1 << BORF)  ///< Brown-out Reset flag
#define RESET_CAUSE_POWER_ON (1 << PORF)   ///< Power-on Reset flag
#define RESET_CAUSE_WATCHDOG (1 << WDRF)   ///< Watchdog Reset flag
#else
#define RESET_CAUSE_EXTERNAL 0x01   ///< External Reset flag (fallback)
#define RESET_CAUSE_BROWN_OUT 0x02  ///< Brown-out Reset flag (fallback)
#define RESET_CAUSE_POWER_ON 0x04   ///< Power-on Reset flag (fallback)
#define RESET_CAUSE_WATCHDOG 0x08   ///< Watchdog Reset flag (fallback)
#endif

/**
 * @brief System memory information
 * @details Comprehensive memory usage statistics
 * @ingroup fsmos
 */
struct SystemMemoryInfo
{
    uint16_t freeRam;        ///< Free RAM in bytes
    uint16_t totalRam;       ///< Total RAM in bytes
    uint16_t heapSize;       ///< Heap size in bytes
    uint16_t largestBlock;   ///< Largest free block in bytes
    uint8_t heapFragments;   ///< Number of heap fragments
    uint16_t stackSize;      ///< Stack size in bytes
    uint16_t stackUsed;      ///< Stack used in bytes
    uint16_t stackFree;      ///< Stack free in bytes
    uint8_t totalTasks;      ///< Total number of tasks
    uint16_t taskMemory;     ///< Memory used by tasks
    uint8_t activeMessages;  ///< Number of active messages
    uint16_t messageMemory;  ///< Memory used by messages
    uint16_t flashUsed;      ///< Flash memory used
    uint16_t flashFree;      ///< Flash memory free
    uint16_t eepromUsed;     ///< EEPROM used in bytes
    uint16_t eepromFree;     ///< EEPROM free in bytes
};

/**
 * @brief Task memory information
 * @details Memory usage statistics for individual tasks
 * @ingroup fsmos
 */
struct TaskMemoryInfo
{
    uint8_t task_id;             ///< Task identifier
    uint16_t task_struct_size;   ///< Size of task structure
    uint16_t subscription_size;  ///< Size of subscription data
    uint16_t queue_size;         ///< Size of message queue
    uint16_t total_allocated;    ///< Total memory allocated for task
};

/* ================== Timer System ================== */
/**
 * @brief Memory-optimized template-based timer for specific duration ranges
 * @ingroup fsmos
 *
 * @tparam T The integer type to use for timing (uint8_t, uint16_t, uint32_t)
 *           Choose based on your maximum duration needs:
 *           - uint8_t: 0-255ms (2 bytes total)
 *           - uint16_t: 0-65535ms (4 bytes total)
 *           - uint32_t: 0-4294967295ms (8 bytes total)
 *
 * @note This template allows memory optimization by using smaller data types
 * for shorter timer durations, reducing RAM usage in memory-constrained systems.
 */
template <typename T>
struct __attribute__((packed)) TimerT
{
    T startMs = 0;     ///< Timer start timestamp in milliseconds
    T durationMs = 0;  ///< Timer duration in milliseconds

    /**
     * @brief Start the timer with specified duration
     * @param d Duration in milliseconds
     * @note Timer will be marked as expired if duration is 0
     */
    void startTimer(T d);

    /**
     * @brief Check if timer has expired
     * @return true if timer duration has elapsed, false otherwise
     * @note Handles type-specific overflow correctly
     */
    [[nodiscard]] bool isExpired() const;
};

/**
 * @brief 8-bit timer for short durations (0-255ms)
 * @details Uses 2 bytes total memory, ideal for debouncing and short delays
 */
using Timer8 = TimerT<uint8_t>;

/**
 * @brief 16-bit timer for medium durations (0-65535ms)
 * @details Uses 4 bytes total memory, ideal for most timing needs
 */
using Timer16 = TimerT<uint16_t>;

/**
 * @brief 32-bit timer for long durations (0-4294967295ms)
 * @details Uses 8 bytes total memory, for very long timing requirements
 */
using Timer32 = TimerT<uint32_t>;

/* ================== Message System ================== */
/**
 * @brief Message data structure for inter-task communication
 *
 * This structure holds the actual message data and is managed by
 * the message pool system for efficient memory usage.
 *
 * @note Messages are reference-counted and automatically returned
 * to the pool when no longer needed.
 * @ingroup fsmos
 */
struct __attribute__((packed)) MsgData
{
    uint8_t type;       ///< Message type identifier
    uint8_t topic;      ///< Topic/channel for message routing
    uint16_t arg;       ///< Additional argument data
    uint8_t refCount;   ///< Reference count for memory management
};

/**
 * @brief Smart pointer-like wrapper for MsgData with reference counting
 *
 * SharedMsg provides automatic memory management for messages through
 * reference counting. When the last reference is destroyed, the message
 * is automatically returned to the pool.
 *
 * @note This class is thread-safe for single-threaded cooperative multitasking.
 * @warning Do not use across different scheduler instances.
 * @ingroup fsmos
 */
class SharedMsg
{
public:
    /**
     * @brief Default constructor
     * @details Creates an empty SharedMsg with no associated data
     */
    SharedMsg();

    /**
     * @brief Constructor from MsgData pointer
     * @param data Pointer to MsgData to wrap
     * @note Increments reference count of the message
     */
    explicit SharedMsg(MsgData *data);

    /**
     * @brief Copy constructor
     * @param other SharedMsg to copy from
     * @note Increments reference count of the shared message
     */
    SharedMsg(const SharedMsg &other);

    /**
     * @brief Assignment operator
     * @param other SharedMsg to assign from
     * @return Reference to this SharedMsg
     * @note Properly manages reference counting
     */
    SharedMsg &operator=(const SharedMsg &other);

    /**
     * @brief Destructor
     * @note Decrements reference count and frees message if count reaches 0
     */
    ~SharedMsg();

    /**
     * @brief Get raw MsgData pointer
     * @return Pointer to MsgData, or nullptr if invalid
     */
    MsgData *getData() const;

    /**
     * @brief Arrow operator for direct access to MsgData
     * @return Pointer to MsgData for member access
     */
    MsgData *operator->() const;

    /**
     * @brief Check if SharedMsg contains valid data
     * @return true if valid, false if empty or invalid
     */
    bool isValid() const;

private:
    MsgData *msgData;  ///< Pointer to the wrapped MsgData
    void release();     ///< Internal method to release reference
};

/**
 * @brief Memory pool for efficient MsgData allocation
 *
 * MsgDataPool manages a pool of MsgData objects to avoid frequent
 * memory allocation/deallocation. It uses an adaptive limit system
 * to balance memory usage and performance.
 *
 * @note The pool automatically adjusts its size based on usage patterns.
 * @ingroup fsmos
 */
class MsgDataPool
{
public:
    /**
     * @brief Constructor
     * @details Initializes the message pool with default settings
     */
    MsgDataPool();

    /**
     * @brief Destructor
     * @details Frees all allocated memory
     */
    ~MsgDataPool();

    /**
     * @brief Allocate a new MsgData from the pool
     * @return Pointer to allocated MsgData, or nullptr if pool is full
     * @note Returns nullptr if no memory available
     */
    MsgData *allocate();

    /**
     * @brief Return a MsgData to the pool
     * @param msg Pointer to MsgData to deallocate
     * @note Frees any associated data and returns MsgData to pool
     */
    void deallocate(MsgData *msg);

    /**
     * @brief Update adaptive pool limit based on usage
     * @details Automatically adjusts pool size for optimal performance
     */
    void updateAdaptiveLimit();

    /**
     * @brief Get current pool size
     * @return Current number of MsgData objects in pool
     */
    uint8_t getPoolSize() const;

    /**
     * @brief Get maximum pool limit
     * @return Maximum number of MsgData objects allowed
     */
    uint8_t getPoolLimit() const;

    /**
     * @brief Get number of currently allocated messages
     * @return Number of messages currently in use
     */
    uint8_t getCurrentInUse() const;

    /**
     * @brief Initialize the message pool if not already initialized
     * @return true if pool was initialized successfully, false if already initialized or failed
     * @note This method performs lazy initialization to avoid static allocation issues
     */
    bool initialize();

private:
    MsgData *pool;           ///< Array of MsgData objects
    uint8_t poolSize;        ///< Current pool size
    uint8_t poolLimit;       ///< Maximum pool size
    uint8_t currentInUse;    ///< Number of messages currently in use
    uint8_t nextFree;        ///< Index of next free message
};

/* ================== Synchronization Primitives ================== */
/**
 * @brief Mutex for cooperative task synchronization
 *
 * Mutex provides mutual exclusion for shared resources in a
 * cooperative multitasking environment. Only one task can
 * hold the mutex at a time.
 *
 * @note This mutex is designed for cooperative multitasking only.
 * It does not provide blocking behavior - tasks must check
 * try_lock() and yield if the mutex is not available.
 * @ingroup fsmos
 */
class Mutex
{
public:
    /**
     * @brief Default constructor
     * @details Creates an unlocked mutex
     */
    Mutex();

    /**
     * @brief Try to acquire the mutex
     * @param task_id ID of the task attempting to acquire the mutex
     * @return true if mutex was acquired, false if already locked
     */
    [[nodiscard]] bool tryLock(uint8_t task_id);

    /**
     * @brief Release the mutex
     * @param task_id ID of the task releasing the mutex
     * @note Only the task that acquired the mutex can release it
     */
    void unlock(uint8_t task_id);

    /**
     * @brief Check if mutex is currently locked
     * @return true if locked, false if available
     */
    [[nodiscard]] bool isLocked() const;

    /**
     * @brief Get ID of task that owns the mutex
     * @return Task ID of owner, or 0 if unlocked
     */
    uint8_t getOwner() const;

private:
    volatile bool locked;       ///< Lock state
    volatile uint8_t owner_id;  ///< ID of owning task
};

/**
 * @brief Semaphore for resource counting and synchronization
 *
 * Semaphore allows a limited number of tasks to access a resource
 * simultaneously. It maintains a count of available resources.
 *
 * @note This semaphore is designed for cooperative multitasking only.
 * Tasks must check wait() and yield if no resources are available.
 * @ingroup fsmos
 */
class Semaphore
{
public:
    /**
     * @brief Constructor
     * @param initial_count Initial number of available resources
     * @param max_count Maximum number of resources
     */
    Semaphore(uint8_t initial_count, uint8_t max_count);

    /**
     * @brief Try to acquire a resource
     * @param task_id ID of the task attempting to acquire resource
     * @return true if resource was acquired, false if none available
     */
    [[nodiscard]] bool wait(uint8_t task_id);

    /**
     * @brief Release a resource
     * @note Increments the available resource count
     */
    void signal();

    /**
     * @brief Get current resource count
     * @return Number of available resources
     */
    uint8_t getCount() const;

    /**
     * @brief Get maximum resource count
     * @return Maximum number of resources
     */
    uint8_t getMaxCount() const;

private:
    volatile uint8_t count;  ///< Current resource count
    uint8_t max_count;       ///< Maximum resource count
};

/* ================== Task System ================== */
/**
 * @brief Base class for all tasks in FsmOS
 * @ingroup fsmos
 *
 * Task provides the foundation for cooperative multitasking.
 * Each task runs in its own context and can communicate with
 * other tasks through messages and events.
 *
 * @note Tasks must implement the step() method to define their behavior.
 * The scheduler calls step() periodically based on the task's period.
 */
/**
 * @brief Default message budget for tasks
 * @details If a task does not explicitly declare a budget via
 *          Task::setMaxMessageBudget, the scheduler applies this
 *          default to ensure capacity checks are enforced.
 */
const uint8_t DEFAULT_TASK_MESSAGE_BUDGET = 1;

/**
 * @brief Base class for all tasks in FsmOS
 */
class Task
{
public:
    /**
     * @brief Constructor
     * @param name Optional name for the task (for debugging)
     * @details Creates a new task in INACTIVE state
     */
    explicit Task(const __FlashStringHelper *name = nullptr);

    /**
     * @brief Virtual destructor
     * @details Ensures proper cleanup of derived classes
     */
    virtual ~Task();

    /**
     * @brief Get total number of Task instances ever created
     */
    static uint16_t getCreatedInstanceCount() { return createdInstanceCount; }

    // Task lifecycle
    /**
     * @brief Called when task is started
     * @details Override this method to perform initialization
     * @note Called once when task transitions to ACTIVE state
     */
    virtual void on_start() {}

    /**
     * @brief Main task execution method
     * @details This method is called periodically by the scheduler
     * @note Must be implemented by derived classes
     */
    virtual void step() = 0;

    /**
     * @brief Called when task is stopped
     * @details Override this method to perform cleanup
     * @note Called when task transitions to INACTIVE state
     */
    virtual void on_stop() {}

    /**
     * @brief Handle incoming messages
     * @param msg The received message
     * @details Override this method to handle specific message types
     * @note Called automatically when messages are received
     */
    virtual void on_msg(const MsgData &msg) {}

    // Task control
    /**
     * @brief Start the task
     * @details Transitions task to ACTIVE state and calls on_start()
     */
    void start();

    /**
     * @brief Stop the task
     * @details Transitions task to INACTIVE state and calls on_stop()
     */
    void stop();

    /**
     * @brief Suspend the task
     * @details Task remains in memory but is not scheduled
     */
    void suspend();

    /**
     * @brief Resume the task
     * @details Task returns to scheduling queue
     */
    void resume();

    /**
     * @brief Terminate the task
     * @details Marks task for removal from scheduler
     */
    void terminate();

    // Task configuration
    /**
     * @brief Set task execution period
     * @param period_ms Period in milliseconds
     * @note Minimum period is 1ms, maximum is 65535ms
     */
    void setPeriod(uint16_t period_ms);

    /**
     * @brief Get task execution period
     * @return Period in milliseconds
     */
    uint16_t getPeriod() const;

    // Task priority
    /**
     * @brief Task priority levels
     * @details Priority levels for task scheduling
     */
    enum Priority
    {
        PRIORITY_LOWEST = 0,    ///< Lowest priority (0)
        PRIORITY_LOW = 1,       ///< Low priority (1)
        PRIORITY_NORMAL = 2,    ///< Normal priority (2)
        PRIORITY_HIGH = 3,      ///< High priority (3)
        PRIORITY_HIGHEST = 4,   ///< Highest priority (4)
        PRIORITY_CRITICAL = 5,  ///< Critical priority (5)
        PRIORITY_REALTIME = 6,  ///< Real-time priority (6)
        PRIORITY_SYSTEM = 7,    ///< System priority (7)
        PRIORITY_MAX = 15       ///< Maximum priority (15)
    };

    /**
     * @brief Set task priority
     * @param priority Priority level
     * @details Sets the task priority for scheduling
     */
    void setPriority(Priority priority);

    /**
     * @brief Set task priority (legacy)
     * @param priority Priority level (0-15)
     * @details Sets the task priority for scheduling
     */
    void setPriority(uint8_t priority);

    /**
     * @brief Get task priority
     * @return Priority level
     */
    uint8_t getPriority() const;

    /**
     * @brief Declare the maximum number of messages this task may produce in one step()
     * @details Used by the scheduler to avoid running producers when the global
     *          message queue has fewer free slots than the declared budget. 0 disables gating.
     */
    void setMaxMessageBudget(uint8_t budget);

    /**
     * @brief Get the maximum number of messages this task may produce in one step()
     * @ingroup fsmos
     * @details Scheduler uses this to ensure there are at least this many
     *          free message slots before running the task.
     * @return Planned message production budget for the upcoming step
     */
    virtual uint8_t getMaxMessageBudget() const { return DEFAULT_TASK_MESSAGE_BUDGET; }

    /**
     * @brief Get the size in bytes of the concrete task object
     * @details Implement in each derived Task as: return sizeof(DerivedClass);
     */
    virtual uint16_t getTaskStructSize() const { return sizeof(*this); }

protected:
    /**
     * @brief Access the configured budget set via setMaxMessageBudget()
     * @return The configured budget value
     */
    uint8_t getConfiguredMessageBudget() const { return maxMessageBudget; }

    // Task state
    /**
     * @brief Task state enumeration
     */
    enum State
    {
        INACTIVE,   ///< Task is not running
        ACTIVE,     ///< Task is running and scheduled
        SUSPENDED,  ///< Task is paused
        TERMINATED  ///< Task is marked for removal
    };

    /**
     * @brief Get current task state
     * @return Current state of the task
     */
    State getState() const;

    /**
     * @brief Set task state
     * @param newState New state to set
     */
    void setState(State newState);

    /**
     * @brief Check if task is in expected state
     * @param expected Expected state to check against
     * @return true if task is in expected state
     */
    bool checkState(State expected) const;

    /**
     * @brief Check if task is active
     * @return true if task is in ACTIVE state
     */
    bool isActive() const;

    /**
     * @brief Check if task is inactive
     * @return true if task is in INACTIVE state
     */
    bool isInactive() const;

    // Task identification
    /**
     * @brief Get unique task ID
     * @return Task ID assigned by scheduler
     */
    uint8_t getId() const;

    /**
     * @brief Get task name
     * @return Task name string
     */
    const __FlashStringHelper *getName() const;
public:
    /**
     * @brief Public helper to read another task's name safely from diagnostics
     */
    static const __FlashStringHelper *readTaskName(const Task *t) { return t ? t->getName() : nullptr; }
protected:

    /**
     * @brief Set task name
     * @param name New name for the task
     */
    void setName(const __FlashStringHelper *name);

    // Message handling
    /**
     * @brief Subscribe to a message topic
     * @param topic Topic ID to subscribe to
     * @note Task will receive messages published to this topic
     */
    void subscribe(uint8_t topic)
    {
        if (topic < MAX_TOPICS)
        {
            subscribedTopics |= (static_cast<TopicBitfield>(1) << topic);
        }
    }

    /**
     * @brief Unsubscribe from a message topic
     * @param topic Topic ID to unsubscribe from
     */
    void unsubscribe(uint8_t topic)
    {
        if (topic < MAX_TOPICS)
        {
            subscribedTopics &= ~(static_cast<TopicBitfield>(1) << topic);
        }
    }

    /**
     * @brief Check if task is subscribed to a topic
     * @param topic Topic ID to check
     * @return true if subscribed, false otherwise
     */
    bool isSubscribedToTopic(uint8_t topic) const
    {
        if (topic >= MAX_TOPICS)
        {
            return false;
        }
        return (subscribedTopics & (static_cast<TopicBitfield>(1) << topic)) != 0;
    }

    /**
     * @brief Get the number of subscribed topics
     * @return Number of subscribed topics
     */
    uint8_t getTopicCount() const
    {
#if TOPIC_BITFIELD_SIZE <= 8
        return __builtin_popcount(static_cast<uint8_t>(subscribedTopics));
#elif TOPIC_BITFIELD_SIZE <= 16
        return __builtin_popcount(static_cast<uint16_t>(subscribedTopics));
#else
        return __builtin_popcountl(static_cast<unsigned long>(subscribedTopics));
#endif
    }

    /**
     * @brief Publish a message to a topic
     * @param topic Topic ID to publish to
     * @param type Message type
     * @param arg Additional argument data
     * @note All subscribed tasks will receive this message
     */
    void publish(uint8_t topic, uint8_t type, uint16_t arg = 0);

    /**
     * @brief Send a direct message to a specific task
     * @param task_id ID of target task
     * @param type Message type
     * @param arg Additional argument data
     */
    void tell(uint8_t task_id, uint8_t type, uint16_t arg = 0);

    // Logging
    /**
     * @brief Log an info message
     * @param msg Message to log
     */
    void log(const __FlashStringHelper *msg);

    /**
     * @brief Log a debug message
     * @param msg Message to log
     */
    void logDebug(const __FlashStringHelper *msg);

    /**
     * @brief Log an info message
     * @param msg Message to log
     */
    void logInfo(const __FlashStringHelper *msg);

    /**
     * @brief Log a warning message
     * @param msg Message to log
     */
    void logWarn(const __FlashStringHelper *msg);

    /**
     * @brief Log an error message
     * @param msg Message to log
     */
    void logError(const __FlashStringHelper *msg);

    // Timer utility methods
    /**
     * @brief Create a memory-optimized timer
     * @tparam T Timer type (Timer8, Timer16, Timer32)
     * @param duration_ms Duration in milliseconds
     * @return Timer object ready to use
     * @note Choose timer type based on expected duration for memory optimization
     */
    template <typename T>
    T createTimerTyped(uint32_t duration_ms) const;

    /**
     * @brief Process pending messages for this task
     * @note Called automatically by scheduler, rarely needs direct use
     */
    void processMessages();

    // Task timing monitoring methods
    /**
     * @brief Get number of times this task was delayed
     * @return Number of delay occurrences
     */
    uint16_t getDelayCount() const;

    /**
     * @brief Get maximum delay experienced by this task
     * @return Maximum delay in milliseconds
     */
    uint16_t getMaxDelay() const;

    /**
     * @brief Get scheduled execution time
     * @return Scheduled time in milliseconds
     */
    uint32_t getScheduledTime() const;

    /**
     * @brief Get actual start time of last execution
     * @return Actual start time in milliseconds
     */
    uint32_t getActualStartTime() const;

private:
    friend class Scheduler;

    uint16_t remainingTime = 0;  ///< Remaining time until next execution (in ms)
    uint16_t periodMs = 1;       ///< Task execution period in milliseconds
    uint8_t taskId = 0;     ///< Unique task identifier
    uint8_t stateAndPriority = 0; ///< Combined state and priority (4 bits each)
    const __FlashStringHelper *name;       ///< Task name for debugging

    // Minimalist task statistics (RAM optimized)
    uint16_t runCount = 0;           ///< Number of times task has run (16-bit for space)
    uint16_t maxExecTimeUs = 0;      ///< Maximum execution time in microseconds (16-bit)
    uint16_t avgExecTimeUs = 0;      ///< Average execution time in microseconds (16-bit)

    // Task timing monitoring
    uint32_t scheduledTime = 0;      ///< When this task was scheduled to run
    uint32_t actualStartTime = 0;    ///< When this task actually started running
    uint16_t delayCount = 0;         ///< Number of times this task was delayed
    uint16_t maxDelayMs = 0;         ///< Maximum delay experienced in milliseconds

    TopicBitfield subscribedTopics = 0;   ///< Bitfield for subscribed topics

    // Scheduler gating: maximum messages this task may produce in a single step()
    uint8_t maxMessageBudget = 0;

    // Global count of created Task instances (for diagnostics/pool sizing hints)
    static uint16_t createdInstanceCount;
};

/* ================== Scheduler System ================== */
/**
 * @brief Core scheduler and task manager for FsmOS
 * @ingroup fsmos
 *
 * Scheduler manages the execution of tasks, message routing,
 * and system resources. It provides the main interface for
 * task management and system control.
 *
 * @note Only one scheduler instance should exist per application.
 * The global OS instance is provided for convenience.
 */
/**
 * @brief Core scheduler and task manager for FsmOS
 */
class Scheduler
{
public:
    /**
     * @brief Constructor
     * @details Initializes scheduler with default settings
     */
    Scheduler();

    /**
     * @brief Destructor
     * @details Removes all tasks and cleans up resources
     */
    ~Scheduler();

    // Task management
    /**
     * @brief Add a task to the scheduler
     * @param task Pointer to task to add
     * @return true if task was added successfully, false if scheduler is full
     * @note Task starts in INACTIVE state
     */
    bool add(Task *task);

    /**
     * @brief Remove a task from the scheduler
     * @param task Pointer to task to remove
     * @return true if task was removed, false if not found
     */
    bool remove(Task *task);

    /**
     * @brief Remove all tasks from the scheduler
     * @details Stops and removes all tasks
     */
    void removeAll();

    /**
     * @brief Get task by ID
     * @param task_id ID of task to find
     * @return Pointer to task, or nullptr if not found
     */
    Task *getTask(uint8_t task_id);

    /**
     * @brief Get number of active tasks
     * @return Number of tasks currently in scheduler
     */
    uint8_t getTaskCount() const { return taskCount; }

    /**
     * @brief Get maximum number of tasks
     * @return Maximum number of tasks supported
     */
    uint16_t getMaxTasks() const;

    // System control
    /**
     * @brief Start the scheduler
     * @details Starts all tasks and begins scheduling
     */
    void begin();

    /**
     * @brief Execute one scheduling step
     * @details Processes messages and executes one ready task
     */
    void loopOnce();

    /**
     * @brief Run scheduler continuously
     * @details Runs scheduler until stop() is called
     */
    void loop();

    /**
     * @brief Stop the scheduler
     * @details Stops all tasks and halts scheduling
     */
    void stop();

    // Message system
    /**
     * @brief Publish a message to a topic
     * @param topic Topic ID to publish to
     * @param type Message type
     * @param arg Additional argument data
     * @note All tasks subscribed to the topic will receive this message
     */
    void publishMessage(uint8_t topic, uint8_t type, uint16_t arg = 0);

    /**
     * @brief Send a direct message to a specific task
     * @param task_id ID of target task
     * @param type Message type
     * @param arg Additional argument data
     */
    void sendMessage(uint8_t task_id, uint8_t type, uint16_t arg = 0);

    /**
     * @brief Get number of free slots in the global message queue
     */
    uint8_t getFreeQueueSlots() const;

    // System monitoring
    /**
     * @brief Get current system time
     * @return Current time in milliseconds
     */
    uint32_t now() const;

    /**
     * @brief Get amount of free memory
     * @return Free memory in bytes
     * @note AVR-specific implementation
     */
    uint16_t getFreeMemory() const;

    // Logging system
    /**
     * @brief Log level enumeration
     */
    enum LogLevel
    {
        LOG_DEBUG = 0,  ///< Debug level messages
        LOG_INFO = 1,   ///< Info level messages
        LOG_WARN = 2,   ///< Warning level messages
        LOG_ERROR = 3   ///< Error level messages
    };

    /**
     * @brief Set minimum log level
     * @param level Minimum level to display
     * @note Messages below this level will be filtered out
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief Log a message
     * @param task Task that generated the message (can be nullptr)
     * @param level Log level
     * @param msg Message to log
     */
    void logMessage(Task *task, LogLevel level, const char *msg);
    void logMessage(Task *task, LogLevel level, const __FlashStringHelper *msg);

    // System callbacks
    /**
     * @brief Handle system tick
     * @details Called by system timer interrupt
     * @note Updates internal system time
     */
    void onTick();

    // Additional diagnostic methods
    /**
     * @brief Get system reset information
     * @param info Reference to store reset information
     * @return true if reset info was retrieved successfully
     */
    bool getResetInfo(ResetInfo &info);

    /**
     * @brief Get reset cause from system
     * @return ResetCause enumeration value
     * @note Combines Optiboot and other reset sources
     */
    ResetCause getResetCause();

    /**
     * @brief Get raw reset flags from system
     * @return Raw reset flags byte
     * @note Internal method for reset cause processing
     */
    uint8_t getResetCauseFlags();

    /**
     * @brief Check if a specific reset cause occurred
     * @param cause Reset cause to check for
     * @return true if the specified cause occurred
     */
    bool wasResetCause(ResetCause cause);

    /**
     * @brief Get task statistics
     * @param task_id ID of task to get stats for
     * @param stats Reference to store task statistics
     * @return true if stats were retrieved successfully
     */
    bool getTaskStats(uint8_t task_id, TaskStats &stats);

    /**
     * @brief Get system memory information
     * @param info Reference to store memory information
     * @return true if memory info was retrieved successfully
     */
    bool getSystemMemoryInfo(SystemMemoryInfo &info);

    /**
     * @brief Get task memory information
     * @param task_id ID of task to get memory info for
     * @param info Reference to store task memory information
     * @return true if memory info was retrieved successfully
     */
    bool getTaskMemoryInfo(uint8_t task_id, TaskMemoryInfo &info);

    /**
     * @brief Get heap fragmentation percentage
     * @return Heap fragmentation as percentage (0-100)
     */
    uint8_t getHeapFragmentation();

    /**
     * @brief Get memory leak detection statistics
     * @param stats Reference to store memory leak statistics
     * @return true if stats were retrieved successfully
     */
    bool getMemoryLeakStats(MemoryStats &stats);

    // Additional system methods

    /**
     * @brief Enable hardware watchdog timer
     * @param timeout Watchdog timeout value
     * @note AVR-specific feature
     */
    void enableWatchdog(uint8_t timeout);
    void feedWatchdog();

    /**
     * @brief Log a formatted message
     * @param task Task that generated the message (can be nullptr)
     * @param level Log level
     * @param format Format string (FlashStringHelper)
     * @param ... Variable arguments for formatting
     * @note Simplified implementation - just logs the format string
     */
    void logFormatted(Task *task, LogLevel level, const __FlashStringHelper *format, ...);

    // Task timing monitoring
    /**
     * @brief Get task that caused the most delays
     * @return Task ID of the task causing most delays, or 0 if none
     */
    uint8_t getMostDelayingTask() const;

private:
    TaskNode *taskHead = nullptr;          ///< Head of task linked list
    TaskNode *taskTail = nullptr;          ///< Tail of task linked list
    // Preallocated node pool (initialized on first add)
    TaskNode *freeTaskNodeHead = nullptr;  ///< Head of free-list for TaskNode pool
    bool taskNodePoolInitialized = false;  ///< Whether pool has been initialized
    uint16_t taskNodePoolCapacity = 0;     ///< Total nodes currently allocated to pool/list
    uint8_t taskCount = 0;                 ///< Current number of tasks
    uint8_t nextTaskId = 1;                ///< Next available task ID

    MsgDataPool msgPool;  ///< Message pool for efficient allocation
    uint32_t systemTime;  ///< Current system time
    bool running;         ///< Scheduler running state

    LogLevel currentLogLevel;     ///< Current minimum log level

    // Task timing monitoring (always active)
    uint8_t lastExecutedTaskId = 0;         ///< ID of last executed task (for delay attribution)
    uint32_t lastTaskEndTime = 0;           ///< When the last task finished execution

    friend class SharedMsg;  ///< Allow SharedMsg to access msgPool

    /**
     * @brief Process pending messages for all tasks
     * @details Internal method called by step()
     */
    void processMessages();

    /**
     * @brief Update system time
     * @details Internal method to update system time from millis()
     */
    void updateSystemTime();

    /**
     * @brief Find next task to execute
     * @return Pointer to next task to execute, or nullptr if none ready
     */
    Task *findNextTask();

    /**
     * @brief Execute a task
     * @param task Task to execute
     * @details Updates task timing and calls task->step()
     */
    void executeTask(Task *task);

    // Refactored helper methods for executeTask
    /**
     * @brief Handle task timing monitoring
     * @param task Task to monitor
     * @param currentTime Current system time
     */
    void handleTaskTiming(Task *task, uint32_t currentTime);

    /**
     * @brief Execute the actual task step
     * @param task Task to execute
     */
    void executeTaskStep(Task *task);

    /**
     * @brief Update task execution statistics
     * @param task Task to update
     * @param execStart Execution start time in microseconds
     */
    void updateTaskStatistics(Task *task, uint32_t execStart);

    /**
     * @brief Update timing monitoring variables
     * @param task Task that was executed
     */
    void updateTimingVariables(Task *task);

    /**
     * @brief Check if task should be terminated and remove if needed
     * @param task Task to check
     */
    void checkForTerminatedTask(Task *task);

    /**
     * @brief Log task delay with attribution
     * @param task Delayed task
     * @param delayMs Delay amount in milliseconds
     * @param causingTaskId ID of task that caused the delay
     */
    void logTaskDelay(Task *task, uint16_t delayMs, uint8_t causingTaskId);

    // Task iteration helpers
    /**
     * @brief Iterate through all tasks with a function
     * @tparam Func Function type that takes Task* parameter
     * @param func Function to call for each task
     */
    template<typename Func>
    void forEachTask(Func func);

    /**
     * @brief Find a task using a predicate function
     * @tparam Func Function type that takes Task* and returns bool
     * @param predicate Function that returns true for the desired task
     * @return Pointer to found task, or nullptr if not found
     */
    template<typename Func>
    Task *findTask(Func predicate);

    // Memory management helpers
    /**
     * @brief Allocate a TaskNode from the pool
     * @param task Task to wrap in the node
     * @return Pointer to allocated TaskNode, or nullptr if failed
     */
    TaskNode *allocateTaskNode(Task *task);

    /**
     * @brief Deallocate a TaskNode back to the pool
     * @param node TaskNode to deallocate
     */
    void deallocateTaskNode(TaskNode *node);

    // Logging system helpers
    /**
     * @brief Log a system event message
     * @param level Log level
     * @param msg Message to log
     */
    void logSystemEvent(LogLevel level, const __FlashStringHelper *msg);

    /**
     * @brief Log a task execution event
     * @param task Task that was executed
     * @param execTime Execution time in microseconds
     */
    void logTaskExecution(Task *task, uint32_t execTime);

    // ================== Message Queue (bounded, no dynamic allocation) ==================
    struct QueuedMessage
    {
        uint8_t targetTaskId;
        MsgData msg;               // msg.data points to buffer when dataSize > 0
        uint8_t *buffer;           // retained across free-list reuse
        uint8_t capacity;          // allocated size of buffer
    };

    // Linked-list node for queued messages
    struct MsgNode
    {
        MsgNode *next;
        QueuedMessage payload;
    };

    // Queue (linked list) and free-node pool
    MsgNode *msgHead = nullptr;
    MsgNode *msgTail = nullptr;
    MsgNode *freeHead = nullptr;
    uint8_t msgCount = 0;        // number of enqueued messages
    uint8_t totalNodes = 0;      // total nodes allocated into pool

    // Grow the pool by a chunk (4 nodes) up to MAX_MESSAGE_POOL_SIZE
    bool allocateMsgNodesChunk();

    // Enqueue a message destined for a specific task
    bool enqueueQueuedMessage(uint8_t targetTaskId, uint8_t topic, uint8_t type, uint16_t arg);

    // Dequeue next message into out; returns false if empty
    bool dequeueQueuedMessage(QueuedMessage &out);

    // Dequeue and return node itself; caller is responsible for recycling node
    bool dequeueQueuedMessageNode(MsgNode *&outNode);

    // Allocate TaskNode pool on first use
    bool initializeTaskNodePool();
    // Acquire a node from pool
    TaskNode *acquireTaskNode(Task *task);
    // Return a node to pool
    void releaseTaskNode(TaskNode *node);

    // MsgNode allocation methods
    MsgNode *allocateMsgNode();
    void deallocateMsgNode(MsgNode *node);
};

/* ================== Global Scheduler Instance ================== */
/**
 * @brief Global scheduler instance
 * @details Convenient global instance for easy access
 * @note This is the main scheduler instance used by most applications
 * @ingroup fsmos
 */
extern Scheduler OS;

/* ================== System Constants ================== */
/**
 * @brief Default task period in milliseconds
 * @ingroup fsmos
 */
const uint16_t DEFAULT_TASK_PERIOD = 100;

/**
 * @brief Minimum allowed task period in milliseconds
 * @ingroup fsmos
 */
const uint16_t MIN_TASK_PERIOD = 1;

/**
 * @brief Maximum allowed task period in milliseconds
 * @ingroup fsmos
 */
const uint16_t MAX_TASK_PERIOD = 65535;

/**
 * @brief Default per-task message production budget
 * @ingroup fsmos
 * @details If a task does not explicitly declare a budget via
 *          Task::setMaxMessageBudget, the scheduler applies this
 *          default to ensure capacity checks are enforced.
 */

// Backward compatibility direct constants removed; use Scheduler::LogLevel

/* ================== Additional Scheduler Methods ================== */
/**
 * @brief Log a debug message with formatting
 * @param format Format string (FlashStringHelper)
 * @param ... Variable arguments for formatting
 * @ingroup fsmos
 */
void logDebugf(const __FlashStringHelper *format, ...);

/**
 * @brief Log an info message with formatting
 * @param format Format string (FlashStringHelper)
 * @param ... Variable arguments for formatting
 * @ingroup fsmos
 */
void logInfof(const __FlashStringHelper *format, ...);

/**
 * @brief Log a warning message with formatting
 * @param format Format string (FlashStringHelper)
 * @param ... Variable arguments for formatting
 * @ingroup fsmos
 */
void logWarnf(const __FlashStringHelper *format, ...);

/**
 * @brief Log an error message with formatting
 * @param format Format string (FlashStringHelper)
 * @param ... Variable arguments for formatting
 * @ingroup fsmos
 */
void logErrorf(const __FlashStringHelper *format, ...);

#endif  // FSMOS_H