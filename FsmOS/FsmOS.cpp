/**
 * @file FsmOS.cpp
 * @brief Implementation of the FsmOS cooperative task scheduler
 * @author Aykut Özdemir <aykutozdemirgyte@gmail.com>
 * @date 2025-10-02
 *
 * This file contains the implementation of the FsmOS scheduler and task
 * management system. It provides:
 *
 * - Task scheduling and execution
 * - Message passing and event handling
 * - Memory management and monitoring
 * - System diagnostics and profiling
 * - Hardware watchdog integration
 *
 * @version 1.3.0 - Major refactoring and code organization
 */

#include "FsmOS.h"
#include <stdarg.h>
#include "../../src/BuildMemoryInfo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(__AVR__)
// Stream stdio directly to Serial to allow vfprintf_P without intermediate buffers
static int serial_putc(char c, FILE *)
{
    Serial.write(c);
    return 0;
}
static FILE serial_stdout;
static void init_stdio_to_serial()
{
    static bool inited = false;
    if (inited)
    {
        return;
    }
    inited = true;
    fdev_setup_stream(&serial_stdout, serial_putc, nullptr, _FDEV_SETUP_WRITE);
    stdout = &serial_stdout;
}
#endif

/* ================== Global Variables ================== */
Scheduler OS;

MemoryStats fsmos_memory_stats = {0, 0, 0, 0};

/* ================== TimerT Implementation ================== */
template <typename T>
void TimerT<T>::startTimer(T d)
{
    startMs = static_cast<T>(OS.now());
    durationMs = d;
}

template <typename T>
bool TimerT<T>::isExpired() const
{
    if (durationMs == 0)
    {
        return true;
    }

    T current_time = static_cast<T>(OS.now());

    // Handle timer overflow correctly using safer arithmetic
    if (current_time >= startMs)
    {
        // No overflow case - simple subtraction
        return (current_time - startMs) >= durationMs;
    }
    else
    {
        // Timer overflow occurred - calculate elapsed time correctly
        T max_value = static_cast<T>(~static_cast<T>(0));

        // Calculate elapsed time: time before overflow + time after overflow
        T elapsed_before_overflow = max_value - startMs + 1;
        T elapsed_after_overflow = current_time;
        T total_elapsed = elapsed_before_overflow + elapsed_after_overflow;

        // Check for overflow in the addition
        if (total_elapsed < elapsed_before_overflow)
        {
            // Addition overflowed, timer definitely expired
            return true;
        }

        return total_elapsed >= durationMs;
    }
}

// Explicit template instantiations for common types
template struct TimerT<uint8_t>;
template struct TimerT<uint16_t>;
template struct TimerT<uint32_t>;

/* ================== SharedMsg Implementation ================== */
SharedMsg::SharedMsg() : msgData(nullptr) {}

SharedMsg::SharedMsg(MsgData *msg) : msgData(msg)
{
    if (msgData)
    {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { msgData->refCount++; }
    }
}

SharedMsg::SharedMsg(const SharedMsg &other) : msgData(other.msgData)
{
    if (msgData)
    {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { msgData->refCount++; }
    }
}

SharedMsg &SharedMsg::operator=(const SharedMsg &other)
{
    if (this != &other)
    {
        // Avoid nested atomic blocks by doing the work outside atomic block
        MsgData *oldData = msgData;
        MsgData *newData = other.msgData;
        MsgData *toDeallocate = nullptr;

        // Single atomic block for the entire operation
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            if (newData)
            {
                newData->refCount++;
            }
            msgData = newData;

            if (oldData)
            {
                oldData->refCount--;
                if (oldData->refCount == 0)
                {
                    toDeallocate = oldData;
                }
            }
        }

        // Deallocate outside atomic block if needed
        if (toDeallocate)
        {
            OS.msgPool.deallocate(toDeallocate);
        }
    }
    return *this;
}

SharedMsg::~SharedMsg()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { release(); }
}

void SharedMsg::release()
{
    if (msgData)
    {
        msgData->refCount--;
        if (msgData->refCount == 0)
        {
            OS.msgPool.deallocate(msgData);
        }
        msgData = nullptr;
    }
}

MsgData *SharedMsg::getData() const { return msgData; }

MsgData *SharedMsg::operator->() const { return msgData; }

bool SharedMsg::isValid() const { return msgData != nullptr; }

/* ================== MsgDataPool Implementation ================== */
MsgDataPool::MsgDataPool()
    : pool(nullptr), poolSize(0), poolLimit(MAX_MESSAGE_POOL_SIZE), currentInUse(0), nextFree(0)
{
    // Don't allocate memory during static initialization
    // Memory will be allocated lazily when first needed
}

MsgDataPool::~MsgDataPool()
{
    if (pool)
    {
        delete[] pool;
        pool = nullptr;
    }
}

MsgData *MsgDataPool::allocate()
{
    // Lazy initialization - allocate pool if not already done
    if (!pool && !initialize())
    {
        fsmos_memory_stats.total_allocated += sizeof(MsgData) * poolLimit;
        return nullptr;
    }

    if (currentInUse >= poolSize)
    {
        return nullptr;
    }

    MsgData *msg = &pool[nextFree];

    // Reset message to initial state
    msg->refCount = 0;
    msg->type = 0;
    msg->topic = 0;
    msg->arg = 0;

    currentInUse++;
    nextFree = (nextFree + 1) % poolSize;

    fsmos_memory_stats.current_usage += sizeof(MsgData);
    if (fsmos_memory_stats.current_usage > fsmos_memory_stats.peak_usage)
    {
        fsmos_memory_stats.peak_usage = fsmos_memory_stats.current_usage;
    }

    // Update adaptive limit based on usage
    updateAdaptiveLimit();

    return msg;
}

void MsgDataPool::deallocate(MsgData *msg)
{
    if (!msg || !pool)
    {
        return;
    }

    // Reset message to initial state
    msg->refCount = 0;
    msg->type = 0;
    msg->topic = 0;
    msg->arg = 0;

    currentInUse--;

    fsmos_memory_stats.current_usage -= sizeof(MsgData);
    fsmos_memory_stats.total_freed += sizeof(MsgData);

    // Update adaptive limit based on usage
    updateAdaptiveLimit();
}

void MsgDataPool::updateAdaptiveLimit()
{
    // Simple adaptive algorithm: increase limit if we're using most of it
    if (currentInUse > (poolSize * 3) / 4 && poolSize < poolLimit)
    {
        poolSize++;
    }
    // Decrease limit if we're using very little
    else if (currentInUse < poolSize / 4 && poolSize > 4)
    {
        poolSize--;
    }
}

uint8_t MsgDataPool::getPoolSize() const { return poolSize; }

uint8_t MsgDataPool::getPoolLimit() const { return poolLimit; }

uint8_t MsgDataPool::getCurrentInUse() const { return currentInUse; }

bool MsgDataPool::initialize()
{
    // If already initialized, return true
    if (pool != nullptr)
    {
        return true;
    }

    // Try to allocate memory
    pool = new MsgData[poolLimit];
    if (pool)
    {
        poolSize = poolLimit;
        // Initialize all messages as free
        for (uint8_t i = 0; i < poolSize; i++)
        {
            pool[i].refCount = 0;
            pool[i].type = 0;
            pool[i].topic = 0;
            pool[i].arg = 0;
        }
        fsmos_memory_stats.total_allocated += sizeof(MsgData) * poolLimit;
        return true;
    }

    // Log error if memory allocation fails
    OS.logMessage(nullptr, Scheduler::LOG_ERROR, F("Msg pool alloc failed"));
    return false;
}

/* ================== LinkedQueue Implementation ================== */
template <typename T>
LinkedQueue<T>::LinkedQueue() : head(nullptr), tail(nullptr), count(0) {}

template <typename T>
LinkedQueue<T>::~LinkedQueue()
{
    while (head)
    {
        Node *temp = head;
        head = head->next;
        delete temp;
    }
}

template <typename T>
LinkedQueue<T>::LinkedQueue(LinkedQueue &&other) noexcept : head(other.head), tail(other.tail), count(other.count)
{
    other.head = nullptr;
    other.tail = nullptr;
    other.count = 0;
}

template <typename T>
void LinkedQueue<T>::push(const T &item)
{
    Node *new_node = new Node;
    if (!new_node)
    {
        return;    // Memory allocation failed
    }

    new_node->data = item;
    new_node->next = nullptr;

    if (tail)
    {
        tail->next = new_node;
    }
    else
    {
        head = new_node;
    }
    tail = new_node;
    count++;
}

template <typename T>
bool LinkedQueue<T>::pop(T &item)
{
    if (!head)
    {
        return false;
    }

    Node *temp = head;
    item = temp->data;  // This properly handles SharedMsg reference counting
    head = head->next;

    if (!head)
    {
        tail = nullptr;
    }

    delete temp;
    count--;
    return true;
}

template <typename T>
bool LinkedQueue<T>::isEmpty() const
{
    return head == nullptr;
}

template <typename T>
uint8_t LinkedQueue<T>::getSize() const
{
    return count;
}

// Explicit template instantiations for common types
template class LinkedQueue<SharedMsg>;

/* ================== Mutex Implementation ================== */
Mutex::Mutex() : locked(false), owner_id(0) {}

bool Mutex::tryLock(uint8_t task_id)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (!locked)
        {
            locked = true;
            owner_id = task_id;
            return true;
        }
    }
    return false;
}

void Mutex::unlock(uint8_t task_id)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (locked && owner_id == task_id)
        {
            locked = false;
            owner_id = 0;
        }
    }
}

bool Mutex::isLocked() const { return locked; }

uint8_t Mutex::getOwner() const { return owner_id; }

/* ================== Semaphore Implementation ================== */
Semaphore::Semaphore(uint8_t initial_count, uint8_t max_count) : count(initial_count), max_count(max_count) {}

bool Semaphore::wait(uint8_t task_id)
{
    (void)task_id;  // Unused parameter
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (count > 0)
        {
            count--;
            return true;
        }
    }
    return false;
}

void Semaphore::signal()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (count < max_count)
        {
            count++;
        }
    }
}

uint8_t Semaphore::getCount() const { return count; }

uint8_t Semaphore::getMaxCount() const { return max_count; }

/* ================== Task Implementation ================== */
// Initialize static counter
uint16_t Task::createdInstanceCount = 0;
Task::Task(const __FlashStringHelper *task_name)
    : remainingTime(0),
      periodMs(DEFAULT_TASK_PERIOD),
      taskId(0),
      stateAndPriority(PRIORITY_NORMAL << 4), // INACTIVE state, PRIORITY_NORMAL priority
      name(task_name)
{
    // Bitfield is already initialized to 0
    createdInstanceCount++;
}

Task::~Task()
{
    if (createdInstanceCount > 0)
    {
        createdInstanceCount--;
    }
}

void Task::start()
{
    if (getState() == INACTIVE)
    {
        setState(ACTIVE);
        remainingTime = periodMs;
        on_start();
    }
}

void Task::stop()
{
    if (getState() == ACTIVE || getState() == SUSPENDED)
    {
        setState(INACTIVE);
        on_stop();
    }
}

void Task::suspend()
{
    if (getState() == ACTIVE)
    {
        setState(SUSPENDED);
    }
}

void Task::resume()
{
    if (getState() == SUSPENDED)
    {
        setState(ACTIVE);
        remainingTime = periodMs;
    }
}

void Task::terminate() { setState(TERMINATED); }

void Task::setPeriod(uint16_t period)
{
    periodMs = (period < MIN_TASK_PERIOD) ? MIN_TASK_PERIOD : ((period > MAX_TASK_PERIOD) ? MAX_TASK_PERIOD : period);
}

uint16_t Task::getPeriod() const { return periodMs; }

void Task::setPriority(Priority priority) { stateAndPriority = (stateAndPriority & 0x0F) | ((static_cast<uint8_t>(priority) & 0x0F) << 4); }

void Task::setPriority(uint8_t prio) { stateAndPriority = (stateAndPriority & 0x0F) | ((prio & 0x0F) << 4); }

uint8_t Task::getPriority() const { return (stateAndPriority >> 4) & 0x0F; }

void Task::setMaxMessageBudget(uint8_t budget)
{
    maxMessageBudget = budget;
}


Task::State Task::getState() const { return static_cast<State>(stateAndPriority & 0x0F); }

void Task::setState(State newState) { stateAndPriority = (stateAndPriority & 0xF0) | static_cast<uint8_t>(newState); }

bool Task::checkState(State expected) const { return getState() == expected; }

bool Task::isActive() const { return getState() == ACTIVE; }

bool Task::isInactive() const { return getState() == INACTIVE; }

uint8_t Task::getId() const { return taskId; }

const __FlashStringHelper *Task::getName() const { return name; }

void Task::setName(const __FlashStringHelper *task_name) { name = task_name; }

// Subscribe/unsubscribe methods are now inline in header file

void Task::publish(uint8_t topic, uint8_t type, uint16_t arg)
{
    OS.publishMessage(topic, type, arg);
}

void Task::tell(uint8_t target_task_id, uint8_t type, uint16_t arg)
{
    OS.sendMessage(target_task_id, type, arg);
}

void Task::log(const __FlashStringHelper *msg) { OS.logMessage(this, Scheduler::LOG_INFO, msg); }

void Task::logDebug(const __FlashStringHelper *msg) { OS.logMessage(this, Scheduler::LOG_DEBUG, msg); }

void Task::logInfo(const __FlashStringHelper *msg) { OS.logMessage(this, Scheduler::LOG_INFO, msg); }

void Task::logWarn(const __FlashStringHelper *msg) { OS.logMessage(this, Scheduler::LOG_WARN, msg); }

void Task::logError(const __FlashStringHelper *msg) { OS.logMessage(this, Scheduler::LOG_ERROR, msg); }

template <typename T>
T Task::createTimerTyped(uint32_t duration_ms) const
{
    T timer;
    // Extract the underlying type from TimerT<T>
    using UnderlyingType = decltype(timer.durationMs);
    UnderlyingType max_duration = static_cast<UnderlyingType>(~static_cast<UnderlyingType>(0));
    UnderlyingType safe_duration =
        (duration_ms > max_duration) ? max_duration : static_cast<UnderlyingType>(duration_ms);
    timer.startTimer(safe_duration);
    return timer;
}

// Explicit template instantiations for Task::createTimerTyped
template Timer8 Task::createTimerTyped<Timer8>(uint32_t) const;
template Timer16 Task::createTimerTyped<Timer16>(uint32_t) const;
template Timer32 Task::createTimerTyped<Timer32>(uint32_t) const;

void Task::processMessages()
{
    // Message processing removed for RAM optimization
    // Messages are now handled directly by the scheduler
}

// Task timing monitoring methods
uint16_t Task::getDelayCount() const
{
    return delayCount;
}

uint16_t Task::getMaxDelay() const
{
    return maxDelayMs;
}

uint32_t Task::getScheduledTime() const
{
    return scheduledTime;
}

uint32_t Task::getActualStartTime() const
{
    return actualStartTime;
}

/* ================== Scheduler Implementation ================== */
Scheduler::Scheduler()
    : taskCount(0),
      nextTaskId(1),
      systemTime(0),
      running(false),
      currentLogLevel(LOG_INFO)
{
    // Initialize linked list pointers
    taskHead = nullptr;
    taskTail = nullptr;
}
// ========== Stack Canary (AVR only) ==========
#if defined(__AVR__)
// Canary byte used to mark free RAM for stack usage measurement
static const uint8_t STACK_CANARY_BYTE = 0xCD;

// Pointers delimiting the canary region. We avoid touching the current stack.
static uint8_t *g_canary_start = nullptr;
static uint8_t *g_canary_end   = nullptr;

/**
 * Initialize the stack canary region.
 * We fill from heap_top up to (SP - FSMOS_STACK_CANARY_MARGIN).
 * This covers the entire free RAM area between heap and stack.
 */
static void init_stack_canary()
{
    extern char __bss_end;
    extern char *__brkval;
    // Current stack pointer approximation via address of a local
    uint8_t sp_probe;
    uint8_t *sp = (uint8_t *)&sp_probe;

    // Compute heap top
    uint8_t *heap_top = (uint8_t *)(__brkval ? __brkval : &__bss_end);

    // Safety margin to keep above the canary (below SP)
#ifndef FSMOS_STACK_CANARY_MARGIN
#define FSMOS_STACK_CANARY_MARGIN 32
#endif

    // Region end just below current SP
    uint8_t *region_end = sp - FSMOS_STACK_CANARY_MARGIN;

    // Start from heap top - cover entire free RAM area
    uint8_t *region_start = heap_top;

    if (region_start >= region_end)
    {
        // Nothing to initialize; leave pointers null
        g_canary_start = g_canary_end = nullptr;
        return;
    }

    g_canary_start = region_start;
    g_canary_end   = region_end;

    // Disable interrupts around the fill to keep ISR stacks from interfering
    uint8_t sreg = SREG;
    cli();
    for (uint8_t *p = g_canary_start; p <= g_canary_end; ++p)
    {
        *p = STACK_CANARY_BYTE;
    }
    SREG = sreg; // restore interrupt state
}

static uint16_t measure_stack_used()
{
    if (!g_canary_start || !g_canary_end || g_canary_start > g_canary_end)
    {
        return 0;
    }
    uint8_t *p = g_canary_start;
    while (p <= g_canary_end && *p == STACK_CANARY_BYTE)
    {
        ++p;
    }
    if (p > g_canary_end)
    {
        return 0;
    }
    // Bytes from first non-canary up to end are considered used
    return (uint16_t)(g_canary_end - p + 1);
}
#endif

Scheduler::~Scheduler() { removeAll(); }

// Missing method implementations
TaskNode *Scheduler::acquireTaskNode(Task *task)
{
    return allocateTaskNode(task);
}

void Scheduler::releaseTaskNode(TaskNode *node)
{
    deallocateTaskNode(node);
}

bool Scheduler::dequeueQueuedMessageNode(MsgNode *&outNode)
{
    // This method should work with the main message queue, not a separate queued message queue
    if (!msgHead)
    {
        outNode = nullptr;
        return false;
    }

    outNode = msgHead;
    msgHead = msgHead->next;

    if (!msgHead)
    {
        msgTail = nullptr;
    }

    outNode->next = nullptr;
    msgCount--;
    return true;
}

bool Scheduler::initializeTaskNodePool()
{
    if (taskNodePoolInitialized)
    {
        return true;
    }
    // Preallocate TaskNode objects as a singly-linked free-list.
    // Start with the number of created tasks (at least 1).
    TaskNode *prev = nullptr;
    uint16_t initial = Task::getCreatedInstanceCount();
    if (initial == 0)
    {
        initial = 1;
    }
    for (uint8_t i = 0; i < initial; ++i)
    {
        TaskNode *node = (TaskNode *)malloc(sizeof(TaskNode));
        if (!node)
        {
            // Allocation failure; cleanup any already allocated nodes
            while (prev)
            {
                TaskNode *temp = prev;
                prev = prev->next;
                free(temp);
            }
            taskNodePoolCapacity = 0;
            return false;
        }
        // Initialize fields manually; building free-list in reverse
        node->task = nullptr;
        node->next = prev;
        prev = node;
        taskNodePoolCapacity++;
    }
    freeTaskNodeHead = prev;
    taskNodePoolInitialized = true;
    return true;
}


bool Scheduler::add(Task *task)
{
    if (!task)
    {
        return false;
    }

    // Check task limit based on TOPIC_BITFIELD_SIZE
    if (taskCount >= MAX_TOPICS)
    {
        logSystemEvent(LOG_ERROR, F("Task limit reached"));
        logInfof(F("Max tasks: %u, Current: %u"), MAX_TOPICS, taskCount);
        return false;
    }

    // Acquire node from pool using helper method
    TaskNode *newNode = allocateTaskNode(task);
    if (!newNode)
    {
        return false;  // Pool allocation failed
    }

    // Assign task ID
    task->taskId = nextTaskId++;
    if (nextTaskId == 0)
    {
        nextTaskId = 1;    // Avoid zero task ID
    }

    // Add to linked list (singly-linked)
    if (taskHead == nullptr)
    {
        // First task
        taskHead = taskTail = newNode;
    }
    else
    {
        // Append to tail
        taskTail->next = newNode;
        taskTail = newNode;
    }

    taskCount++;
    return true;
}

bool Scheduler::remove(Task *task)
{
    if (!task)
    {
        return false;
    }

    // Find the node containing this task, tracking previous (singly-linked)
    TaskNode *current = taskHead;
    TaskNode *previous = nullptr;
    while (current != nullptr)
    {
        if (current->task == task)
        {
            // Remove from linked list
            if (previous)
            {
                previous->next = current->next;
            }
            else
            {
                // Removing head
                taskHead = current->next;
            }

            if (current == taskTail)
            {
                // Update tail if needed
                taskTail = previous;
            }

            deallocateTaskNode(current);
            taskCount--;
            return true;
        }
        previous = current;
        current = current->next;
    }

    return false;
}

void Scheduler::removeAll()
{
    TaskNode *current = taskHead;
    while (current != nullptr)
    {
        TaskNode *next = current->next;
        current->task->stop();
        deallocateTaskNode(current);
        current = next;
    }
    taskHead = taskTail = nullptr;
    taskCount = 0;
}

Task *Scheduler::getTask(uint8_t task_id)
{
    return findTask([task_id](Task * task) { return task->getId() == task_id; });
}

// getTaskCount is now inline in header file

uint16_t Scheduler::getMaxTasks() const { return taskNodePoolCapacity; }

void Scheduler::begin()
{
    // Initialize stack canary before tasks start executing
#if defined(__AVR__)
    init_stack_canary();
#endif
    // Set log level to INFO for better debugging
    setLogLevel(LOG_INFO);

    // Log startup message
    logSystemEvent(LOG_INFO, F("FsmOS starting"));

    running = true;
    systemTime = millis();

#if defined(__AVR__)
    init_stdio_to_serial();
#endif

    // Start all tasks
    forEachTask([this](Task * task)
    {
        task->start();
        feedWatchdog();
    });

    // Log task count
    logSystemEvent(LOG_INFO, F("Scheduler ready"));
}

void Scheduler::loopOnce()
{
    if (!running)
    {
        return;
    }

    updateSystemTime();

    // Decrease remaining time for all active tasks
    forEachTask([](Task * task)
    {
        if (task->isActive() && task->remainingTime > 0)
        {
            task->remainingTime--;
        }
    });

    // Feed watchdog timer
    feedWatchdog();

    // Process a limited number of queued messages per tick
    processMessages();

    Task *next_task = findNextTask();
    if (next_task)
    {
        executeTask(next_task);
    }
}

void Scheduler::loop()
{
    while (running)
    {
        loopOnce();
    }
}

void Scheduler::stop() { running = false; }

void Scheduler::publishMessage(uint8_t topic, uint8_t type, uint16_t arg)
{
    // Enqueue for all subscribed tasks
    forEachTask([this, topic, type, arg](Task * task)
    {
        if (task->isActive() && task->isSubscribedToTopic(topic))
        {
            enqueueQueuedMessage(task->getId(), topic, type, arg);
        }
    });
}

void Scheduler::sendMessage(uint8_t task_id, uint8_t type, uint16_t arg)
{
    Task *target_task = getTask(task_id);
    if (!target_task || !target_task->isActive())
    {
        return;
    }

    enqueueQueuedMessage(task_id, 0, type, arg);
}

uint32_t Scheduler::now() const { return systemTime; }

uint16_t Scheduler::getFreeMemory() const
{
    extern char __heap_start;      // start of heap (in .bss)
    extern char *__brkval;         // current heap break (nullptr if none)
    char v;                        // stack variable to get current stack address
    // Difference between current stack pointer and heap end/start
    return (uint16_t)(&v - (__brkval == 0 ? &__heap_start : __brkval));
}

void Scheduler::setLogLevel(LogLevel level) { currentLogLevel = level; }

void Scheduler::logMessage(Task *task, LogLevel level, const char *msg)
{
    if (level < currentLogLevel)
    {
        return;
    }

    const char *level_str = "DEBUG";
    switch (level)
    {
        case LOG_INFO:
            level_str = "INFO";
            break;
        case LOG_WARN:
            level_str = "WARN";
            break;
        case LOG_ERROR:
            level_str = "ERROR";
            break;
        default:
            break;
    }

    Serial.print(F("["));
    Serial.print(level_str);
    Serial.print(F("] "));

    if (task)
    {
        Serial.print(F("T"));
        Serial.print(task->getId());
        Serial.print(F(": "));
    }

    Serial.println(msg);
}

void Scheduler::logMessage(Task *task, LogLevel level, const __FlashStringHelper *msg)
{
    if (level < currentLogLevel)
    {
        return;
    }

    const char *level_str = "DEBUG";
    switch (level)
    {
        case LOG_INFO:
            level_str = "INFO";
            break;
        case LOG_WARN:
            level_str = "WARN";
            break;
        case LOG_ERROR:
            level_str = "ERROR";
            break;
        default:
            break;
    }

    Serial.print(F("["));
    Serial.print(level_str);
    Serial.print(F("] "));

    if (task)
    {
        Serial.print(F("T"));
        Serial.print(task->getId());
        Serial.print(F(": "));
    }

    Serial.println(msg);
}

void Scheduler::onTick() { systemTime++; }

void Scheduler::processMessages()
{
    while (true)
    {
        // Dequeue node to ensure we recycle only after handler finishes
        if (msgCount == 0)
        {
            break;
        }
        MsgNode *node = msgHead;
        msgHead = node->next;
        if (!msgHead)
        {
            msgTail = nullptr;
        }
        msgCount--;

        QueuedMessage &qm = node->payload;
        Task *target = getTask(qm.targetTaskId);
        if (target && target->isActive())
        {
            target->on_msg(qm.msg);
        }

        // Recycle node after handler returns; keep buffer for reuse
        node->next = freeHead;
        freeHead = node;
    }
}

void Scheduler::updateSystemTime() { systemTime = millis(); }

Task *Scheduler::findNextTask()
{
    Task *nextTask = nullptr;
    uint16_t shortestTime = UINT16_MAX;

    TaskNode *current = taskHead;
    while (current != nullptr)
    {
        if (current->task && current->task->isActive() && current->task->remainingTime == 0)
        {
            // If task declares a message production budget, ensure enough free queue slots
            uint8_t budget = current->task->getMaxMessageBudget();
            // Only use default if task hasn't explicitly set a budget (inherited default)
            if (budget == DEFAULT_TASK_MESSAGE_BUDGET)
            {
                // Task is using the default, which is fine
            }
            // If budget is 0, task explicitly wants no message production
            // If budget > 0, task has explicit budget
            // In both cases, respect the task's decision
            if (getFreeQueueSlots() < budget)
            {
                // Skip this task for now; not enough capacity to accept its messages
            }
            else if (nextTask == nullptr)
            {
                nextTask = current->task;
            }
            else if (current->task->getPriority() > nextTask->getPriority())
            {
                // Higher enum value wins (PRIORITY_SYSTEM=7 > PRIORITY_LOWEST=0)
                nextTask = current->task;
            }
            else if (current->task->getPriority() == nextTask->getPriority())
            {
                // Same priority, smaller task ID wins
                if (current->task->getId() < nextTask->getId())
                {
                    nextTask = current->task;
                }
            }
        }
        else if (current->task && current->task->isActive() && current->task->remainingTime < shortestTime)
        {
            shortestTime = current->task->remainingTime;
        }
        current = current->next;
    }

    return nextTask;
}

void Scheduler::executeTask(Task *task)
{
    if (!task || !task->isActive())
    {
        return;
    }

    uint32_t execStart = micros();
    uint32_t currentTime = systemTime;

    // Handle task timing monitoring
    handleTaskTiming(task, currentTime);

    // Execute the actual task step
    executeTaskStep(task);

    // Update task execution statistics
    updateTaskStatistics(task, execStart);

    // Update timing monitoring variables
    updateTimingVariables(task);

    // Check for terminated tasks
    checkForTerminatedTask(task);
}

// Refactored helper methods for executeTask
void Scheduler::handleTaskTiming(Task *task, uint32_t currentTime)
{
    // Set scheduled time (when task should have started)
    task->scheduledTime = currentTime - task->getPeriod();
    task->actualStartTime = currentTime;

    // Check for delay
    if (task->actualStartTime > task->scheduledTime)
    {
        uint32_t delay = task->actualStartTime - task->scheduledTime;
        uint16_t delayMs = (delay > 65535) ? 65535 : static_cast<uint16_t>(delay);

        // Update delay statistics
        task->delayCount++;
        if (delayMs > task->maxDelayMs)
        {
            task->maxDelayMs = delayMs;
        }

        // Log delay with attribution
        logTaskDelay(task, delayMs, lastExecutedTaskId);
    }
}

void Scheduler::executeTaskStep(Task *task)
{
    // Reset remaining time for next execution
    task->remainingTime = task->getPeriod();

    // Execute task step
    task->step();
}

void Scheduler::updateTaskStatistics(Task *task, uint32_t execStart)
{
    // Calculate execution time
    uint32_t execTime = micros() - execStart;
    uint16_t execTime16 = (execTime > 65535) ? 65535 : static_cast<uint16_t>(execTime);

    // Update run count
    if (task->runCount < 65535)
    {
        task->runCount++;
    }

    // Update max execution time
    if (execTime16 > task->maxExecTimeUs)
    {
        task->maxExecTimeUs = execTime16;
    }

    // Update average execution time
    if (task->runCount == 1)
    {
        task->avgExecTimeUs = execTime16;
    }
    else if (task->runCount == 65535)
    {
        // Exponential moving average to avoid overflow
        uint32_t diff = execTime16 - task->avgExecTimeUs;
        int32_t adjustment = diff / 1000;  // Slow adaptation
        int32_t newAvg = task->avgExecTimeUs + adjustment;

        // Clamp to valid range
        if (newAvg < 0)
        {
            newAvg = 0;
        }
        if (newAvg > 65535)
        {
            newAvg = 65535;
        }

        task->avgExecTimeUs = static_cast<uint16_t>(newAvg);
    }
    else
    {
        // Simple moving average
        uint32_t newAvg = ((uint32_t)task->avgExecTimeUs * (task->runCount - 1) + execTime16) / task->runCount;
        task->avgExecTimeUs = (newAvg > 65535) ? 65535 : static_cast<uint16_t>(newAvg);
    }
}

void Scheduler::updateTimingVariables(Task *task)
{
    lastExecutedTaskId = task->getId();
    lastTaskEndTime = systemTime;
}

void Scheduler::checkForTerminatedTask(Task *task)
{
    if (task->getState() == Task::TERMINATED)
    {
        remove(task);
    }
}

void Scheduler::logTaskDelay(Task *task, uint16_t delayMs, uint8_t causingTaskId)
{
    // Task delay logging disabled to save ROM
    (void)task;
    (void)delayMs;
    (void)causingTaskId;
}

// Task iteration template methods
template<typename Func>
void Scheduler::forEachTask(Func func)
{
    TaskNode *current = taskHead;
    while (current != nullptr)
    {
        if (current->task)
        {
            func(current->task);
        }
        current = current->next;
    }
}

template<typename Func>
Task *Scheduler::findTask(Func predicate)
{
    TaskNode *current = taskHead;
    while (current != nullptr)
    {
        if (current->task && predicate(current->task))
        {
            return current->task;
        }
        current = current->next;
    }
    return nullptr;
}

// Explicit template instantiations for common use cases
template void Scheduler::forEachTask<void(*)(Task *)>(void(*)(Task *));
template Task *Scheduler::findTask<bool(*)(Task *)>(bool(*)(Task *));

// Memory management helpers
TaskNode *Scheduler::allocateTaskNode(Task *task)
{
    if (!taskNodePoolInitialized && !initializeTaskNodePool())
    {
        return nullptr;
    }
    if (!freeTaskNodeHead)
    {
        // Try to expand by one
        TaskNode *node = (TaskNode *)malloc(sizeof(TaskNode));
        if (!node)
        {
            return nullptr;
        }
        node->task = nullptr;
        node->next = nullptr;
        freeTaskNodeHead = node;
        taskNodePoolCapacity++;
    }
    TaskNode *node = freeTaskNodeHead;
    freeTaskNodeHead = freeTaskNodeHead->next;
    node->task = task;
    node->next = nullptr;
    return node;
}

void Scheduler::deallocateTaskNode(TaskNode *node)
{
    if (!node)
    {
        return;
    }
    node->task = nullptr;
    node->next = freeTaskNodeHead;
    freeTaskNodeHead = node;
}

Scheduler::MsgNode *Scheduler::allocateMsgNode()
{
    if (!freeHead)
    {
        if (!allocateMsgNodesChunk())
        {
            return nullptr;
        }
    }

    MsgNode *node = freeHead;
    freeHead = freeHead->next;
    node->next = nullptr;
    return node;
}

void Scheduler::deallocateMsgNode(Scheduler::MsgNode *node)
{
    if (!node)
    {
        return;
    }

    // Free the buffer if it exists
    if (node->payload.buffer)
    {
        free(node->payload.buffer);
        node->payload.buffer = nullptr;
        node->payload.capacity = 0;
    }

    // Reset message data
    node->payload.targetTaskId = 0;
    node->payload.msg.type = 0;
    node->payload.msg.topic = 0;
    node->payload.msg.arg = 0;
    node->payload.msg.refCount = 0;

    // Return to free list
    node->next = freeHead;
    freeHead = node;
}

// Logging system helpers
void Scheduler::logSystemEvent(LogLevel level, const __FlashStringHelper *msg)
{
    logMessage(nullptr, level, msg);
}

void Scheduler::logTaskExecution(Task *task, uint32_t execTime)
{
    // Execution time logging disabled to save ROM
    (void)task;
    (void)execTime;
}

/* ================== Additional Scheduler Methods ================== */
bool Scheduler::getResetInfo(ResetInfo &info)
{
    // Basic reset info
    info.resetReason = 0;
    info.resetTime = systemTime;
    info.watchdogTimeout = 0;
    info.lastTaskId = 0;

    // Get reset cause information
    info.optibootResetFlags = getResetCauseFlags();
    info.optibootResetCause = getResetCause();

    return true;
}

ResetCause Scheduler::getResetCause()
{
    uint8_t flags = getResetCauseFlags();

    if (flags == 0)
    {
        return RESET_UNKNOWN;
    }

    // Check for multiple causes
    uint8_t count = 0;
    if (flags & RESET_CAUSE_POWER_ON)
    {
        count++;
    }
    if (flags & RESET_CAUSE_EXTERNAL)
    {
        count++;
    }
    if (flags & RESET_CAUSE_BROWN_OUT)
    {
        count++;
    }
    if (flags & RESET_CAUSE_WATCHDOG)
    {
        count++;
    }

    if (count > 1)
    {
        return RESET_MULTIPLE;
    }

    // Single cause
    if (flags & RESET_CAUSE_POWER_ON)
    {
        return RESET_POWER_ON;
    }
    if (flags & RESET_CAUSE_EXTERNAL)
    {
        return RESET_EXTERNAL;
    }
    if (flags & RESET_CAUSE_BROWN_OUT)
    {
        return RESET_BROWN_OUT;
    }
    if (flags & RESET_CAUSE_WATCHDOG)
    {
        return RESET_WATCHDOG;
    }

    return RESET_UNKNOWN;
}

uint8_t Scheduler::getResetCauseFlags()
{
#if defined(__AVR__)
    return GPIOR0;
#else
    return 0;  // Not available on non-AVR platforms
#endif
}

bool Scheduler::wasResetCause(ResetCause cause) { return getResetCause() == cause; }

bool Scheduler::getTaskStats(uint8_t task_id, TaskStats &stats)
{
    Task *task = getTask(task_id);
    if (!task)
    {
        return false;
    }

    stats.taskId = task->getId();
    stats.name = task->getName();
    stats.state = static_cast<uint8_t>(task->getState());
    stats.periodMs = task->getPeriod();
    stats.priority = task->getPriority();
    stats.runCount = task->runCount;
    stats.maxExecTimeUs = task->maxExecTimeUs;      // 16-bit max execution time
    stats.totalExecTimeUs = task->runCount * task->avgExecTimeUs;    // Calculate total from avg * count
    stats.stackUsage = 0;         // Still placeholder - requires stack monitoring
    stats.delayCount = task->delayCount;           // Task delay count
    stats.maxDelayMs = task->maxDelayMs;           // Maximum delay experienced
    return true;
}

bool Scheduler::getSystemMemoryInfo(SystemMemoryInfo &info)
{
    info.freeRam = getFreeMemory();
    info.totalRam = 2048;    // AVR typical
    // Heap size (bytes) from end of .bss to current break value (if any)
#if defined(__AVR__)
    extern char __bss_end;
    extern char *__brkval;
    if (__brkval)
    {
        info.heapSize = (uint16_t)((uint16_t)__brkval - (uint16_t)&__bss_end);
    }
    else
    {
        info.heapSize = 0;
    }
#else
    info.heapSize = 0;
#endif
    info.largestBlock = 0;   // Not tracked
    info.heapFragments = 0;  // Not tracked
#if defined(__AVR__)
    // Approximate stack usage based on canary region
    extern char __bss_end;
    extern char *__brkval;
    uint16_t lower = (uint16_t)(__brkval ? __brkval : &__bss_end);
    uint16_t upper = (uint16_t)RAMEND;
    uint16_t approxUsed = measure_stack_used();
    extern uint8_t *g_canary_start; extern uint8_t *g_canary_end;
    uint16_t windowSize = 0;
    if (g_canary_start && g_canary_end && g_canary_end >= g_canary_start)
    {
        windowSize = (uint16_t)(g_canary_end - g_canary_start + 1);
    }
    else
    {
        windowSize = (upper >= lower) ? (upper - lower + 1) : 0;
    }
    info.stackSize = windowSize;
    info.stackUsed = (approxUsed > windowSize) ? windowSize : approxUsed;
    info.stackFree = (info.stackUsed >= info.stackSize) ? 0 : (info.stackSize - info.stackUsed);
#else
    info.stackSize = 0;
    info.stackUsed = 0;
    info.stackFree = 0;
#endif
    info.totalTasks = taskCount;
    // Sum estimated task memory (struct size + subscription bitfield)
    uint16_t taskMem = 0;
    TaskNode *tn = taskHead;
    while (tn)
    {
        if (tn->task)
        {
            taskMem += tn->task->getTaskStructSize();
            taskMem += sizeof(tn->task->subscribedTopics);
        }
        tn = tn->next;
    }
    info.taskMemory = taskMem;
    info.activeMessages = msgCount;  // Provide actual active message count
    // Approximate message memory: nodes + buffers across queue and free lists
    uint16_t msgMem = 0;
    MsgNode *mn = msgHead;
    while (mn)
    {
        msgMem += sizeof(MsgNode);
        msgMem += mn->payload.capacity;
        mn = mn->next;
    }
    mn = freeHead;
    while (mn)
    {
        msgMem += sizeof(MsgNode);
        msgMem += mn->payload.capacity;
        mn = mn->next;
    }
    info.messageMemory = msgMem;

    // Flash usage from build-time constants
#ifdef BUILD_FLASH_USED
    // Use real build-time values injected by build script
    info.flashUsed = BUILD_FLASH_USED;
    info.flashFree = BUILD_FLASH_FREE;
#else
    info.flashUsed = 0;       // Not available at runtime
    info.flashFree = 0;       // Not available at runtime
#endif

    // EEPROM usage (approximate based on known usage)
#ifdef FSMOS_EEPROM_SIZE
    // Estimate EEPROM usage based on known variables stored
    // LightTask saves dim level (1 byte), other tasks may use EEPROM
    uint16_t eepromUsed = 1;  // Conservative estimate
    info.eepromUsed = eepromUsed;
    info.eepromFree = FSMOS_EEPROM_SIZE - eepromUsed;
#else
    info.eepromUsed = 0;      // Not available at runtime
    info.eepromFree = 0;      // Not available at runtime
#endif
    return true;
}

bool Scheduler::getTaskMemoryInfo(uint8_t task_id, TaskMemoryInfo &info)
{
    Task *task = getTask(task_id);
    if (!task)
    {
        return false;
    }

    info.task_id = task->getId();
    // Derive struct size via virtual API; if not implemented by a task, it won't link.
    info.task_struct_size = task->getTaskStructSize();
    info.subscription_size = sizeof(task->subscribedTopics);
    info.queue_size = 0;         // No per-task queue in current design
    info.total_allocated = info.task_struct_size + info.subscription_size;
    return true;
}

uint8_t Scheduler::getHeapFragmentation()
{
    return 0;  // Placeholder
}

bool Scheduler::getMemoryLeakStats(MemoryStats &stats)
{
    stats = fsmos_memory_stats;
    return true;
}

/* ================== Additional Logging Functions ================== */
// Internal helpers to reuse formatted logging logic
static void printLogHeader(Scheduler::LogLevel level)
{
#if defined(__AVR__)
    const char *level_str = "DEBUG";
    switch (level)
    {
        case Scheduler::LOG_INFO:
            level_str = "INFO";
            break;
        case Scheduler::LOG_WARN:
            level_str = "WARN";
            break;
        case Scheduler::LOG_ERROR:
            level_str = "ERROR";
            break;
        default:
            break;
    }
    Serial.print(F("["));
    Serial.print(level_str);
    Serial.print(F("] "));
#else
    (void)level;
#endif
}

static void logFormattedV(Scheduler::LogLevel level, const __FlashStringHelper *format, va_list args)
{
#if defined(__AVR__)
    printLogHeader(level);
    vfprintf_P(stdout, (PGM_P)format, args);
    Serial.println();
#else
    char formatted[128];
    vsnprintf(formatted, sizeof(formatted), (const char *)format, args);
    OS.logMessage(nullptr, level, formatted);
#endif
}
void logDebugf(const __FlashStringHelper *format, ...)
{
    va_list args;
    va_start(args, format);
    logFormattedV(Scheduler::LOG_DEBUG, format, args);
    va_end(args);
}

void logInfof(const __FlashStringHelper *format, ...)
{
    va_list args;
    va_start(args, format);
    logFormattedV(Scheduler::LOG_INFO, format, args);
    va_end(args);
}

void logWarnf(const __FlashStringHelper *format, ...)
{
    va_list args;
    va_start(args, format);
    logFormattedV(Scheduler::LOG_WARN, format, args);
    va_end(args);
}

void logErrorf(const __FlashStringHelper *format, ...)
{
    va_list args;
    va_start(args, format);
    logFormattedV(Scheduler::LOG_ERROR, format, args);
    va_end(args);
}

/* ================== Additional Scheduler System Methods ================== */

void Scheduler::enableWatchdog(uint8_t timeout)
{
#if defined(__AVR__)
    wdt_enable(timeout);
#endif
}

void Scheduler::feedWatchdog()
{
#if defined(__AVR__)
    wdt_reset();
#endif
}

void Scheduler::logFormatted(Task *task, LogLevel level, const __FlashStringHelper *format, ...)
{
    // Simplified implementation - just log the format string
    logMessage(task, level, format);
}

uint8_t Scheduler::getFreeQueueSlots() const
{
    return static_cast<uint8_t>(MAX_MESSAGE_POOL_SIZE - msgCount);
}

uint8_t Scheduler::getMostDelayingTask() const
{
    uint8_t mostDelayingTaskId = 0;
    uint16_t maxDelayCount = 0;

    TaskNode *current = taskHead;
    while (current != nullptr)
    {
        if (current->task && current->task->getDelayCount() > maxDelayCount)
        {
            maxDelayCount = current->task->getDelayCount();
            mostDelayingTaskId = current->task->getId();
        }
        current = current->next;
    }

    return mostDelayingTaskId;
}

bool Scheduler::allocateMsgNodesChunk()
{
    if (totalNodes >= MAX_MESSAGE_POOL_SIZE)
    {
        return false;
    }

    // Allocate up to 4 nodes, but don’t exceed hard cap
    uint8_t canAdd = static_cast<uint8_t>(MAX_MESSAGE_POOL_SIZE - totalNodes);
    uint8_t toAdd = (canAdd >= 4) ? 4 : canAdd;
    for (uint8_t i = 0; i < toAdd; i++)
    {
        MsgNode *n = (MsgNode *)malloc(sizeof(MsgNode));
        if (!n)
        {
            // Allocation failed; stop early
            break;
        }
        // Initialize node payload to safe defaults
        n->next = freeHead;
        n->payload.targetTaskId = 0;
        n->payload.msg.type = 0;
        n->payload.msg.topic = 0;
        n->payload.msg.arg = 0;
        n->payload.msg.refCount = 0;
        n->payload.buffer = nullptr;
        n->payload.capacity = 0;
        freeHead = n;
        totalNodes++;
    }
    return true;
}

bool Scheduler::enqueueQueuedMessage(uint8_t targetTaskId, uint8_t topic, uint8_t type, uint16_t arg)
{
    if (msgCount >= MAX_MESSAGE_POOL_SIZE)
    {
        return false;
    }

    if (!freeHead)
    {
        allocateMsgNodesChunk();
        if (!freeHead)
        {
            return false;
        }
    }

    MsgNode *node = freeHead;
    freeHead = freeHead->next;

    QueuedMessage &slot = node->payload;
    slot.targetTaskId = targetTaskId;
    slot.msg.type = type;
    slot.msg.topic = topic;
    slot.msg.arg = arg;
    slot.msg.refCount = 0;

    node->next = nullptr;
    if (msgTail)
    {
        msgTail->next = node;
        msgTail = node;
    }
    else
    {
        msgHead = msgTail = node;
    }
    msgCount++;
    return true;
}

bool Scheduler::dequeueQueuedMessage(QueuedMessage &out)
{
    if (msgCount == 0)
    {
        return false;
    }
    MsgNode *node = msgHead;
    msgHead = node->next;
    if (!msgHead)
    {
        msgTail = nullptr;
    }
    out = node->payload;
    // Node is not recycled here to avoid in-flight overwrite; caller should recycle
    msgCount--;
    return true;
}