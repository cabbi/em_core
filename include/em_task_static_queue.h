#ifndef __EM_TASK_STATIC_QUEUE_H
#define __EM_TASK_STATIC_QUEUE_H

#include <type_traits>
#include <cstdint>
#include <cstddef>

#include "em_defs.h"
#include "em_timeout.h"
#include "em_task_queue.h"

// The queue operation result
enum class EmQueueResult : uint8_t {
    OK = 0,
    ERROR_INVALID_QUEUE,    
    ERROR_FULL,             
    ERROR_EMPTY
};

inline const char* EmQueueResultToText(EmQueueResult result) {
    switch (result) {
        case EmQueueResult::OK:                       return "OK";
        case EmQueueResult::ERROR_INVALID_QUEUE:      return "Invalid queue";
        case EmQueueResult::ERROR_FULL:               return "Queue full";
        case EmQueueResult::ERROR_EMPTY:              return "Queue empty";
        default :                                     return "Unknown result";
    }
}

// A static queue that uses a fixed-size array to store items and and
// EmTaskQueue to manage the indices of the items in the array.
// NOTE: 
//  - 'T' type should have default constructor and assignment operator defined. 
//  - We intentionally avoid using 'std::function' for callbacks here because:
//    1. HEAP ALLOCATION RISK: If a lambda captures variables, 'std::function' can
//       silently trigger a hidden malloc() to store the context. On an MCU handling 
//       high-frequency queue events, this causes severe heap fragmentation and crashes.
//    2. PERFORMANCE OVERHEAD: It introduces virtual table call indirection and runtime 
//       type-checking, which slows down execution compared to direct function pointers.
//    Instead, we use raw function pointers paired with a 'void* context' argument. 
//    This guarantees 100% compile-time, zero-allocation memory safety on the MCU.
template<typename T, size_t QueueSize>
class EmTaskStaticQueue {
public:
    // Callback types for push and pull operations
    typedef void (*PushCallback)(T& internalSlot, void* context);
    typedef void (*PullCallback)(const T& internalSlot, void* context);

    EmTaskStaticQueue()
     : m_queue(QueueSize), 
       m_headIndex(0), 
       m_freeSlots(xSemaphoreCreateCounting(QueueSize, QueueSize)) {}
    virtual ~EmTaskStaticQueue() { 
        if (m_freeSlots != nullptr) vSemaphoreDelete(m_freeSlots); 
    }

    // Push an item into the queue if there is space, invoking the callback to fill the item.
    // If the queue is full, it can optionally free the oldest item to make space.
    EmQueueResult push(PushCallback cb, 
                       void* context, 
                       bool freeOldestItemIfFull, 
                       uint32_t waitIfFullMillis = 0) {
        if (!isValid() || cb == nullptr) {
            return EmQueueResult::ERROR_INVALID_QUEUE;
        }
        
        if (isFull()) {
            if (!freeOldestItemIfFull) {
                return EmQueueResult::ERROR_FULL; 
            }
            EmQueueResult res = freeOldest();
            if (res != EmQueueResult::OK) {
                 return res; 
            }
        }
        
        TickType_t ticksToWait = (waitIfFullMillis == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(waitIfFullMillis);
        EmMutexLock lock(m_pushMutex);       
        if (xSemaphoreTake(m_freeSlots, ticksToWait) == pdTRUE) {
            cb(m_items[m_headIndex], context);
            
            if (m_queue.push(m_headIndex, 0)){
                m_headIndex = (m_headIndex + 1) % QueueSize;
                return EmQueueResult::OK;
            }
            xSemaphoreGive(m_freeSlots); 
        }
        return EmQueueResult::ERROR_FULL; 
    }

    // Pull an item from the queue if any and invoke the callback with the item.
    EmQueueResult pull(PullCallback cb, void* context, uint32_t waitIfEmptyMillis = 0) {
        if (!isValid()) return EmQueueResult::ERROR_INVALID_QUEUE;
        
        size_t index;
        if (!m_queue.pull(index, waitIfEmptyMillis)) return EmQueueResult::ERROR_EMPTY;
        
        if (cb != nullptr) {
            cb(m_items[index], context);
        }
        
        xSemaphoreGive(m_freeSlots);
        return EmQueueResult::OK;
    }

    // Free the oldest item in the queue, if any.
    EmQueueResult freeOldest() { 
        return pull([](const T&, void*) {}, nullptr, 0); 
    }

    bool isValid() const { return m_queue.isValid() && m_freeSlots != nullptr; }
    bool isNotValid() const { return !isValid(); }
    bool isEmpty() const { return m_queue.isEmpty(); }
    bool isNotEmpty() const { return m_queue.isNotEmpty(); }
    bool isFull() const { return m_queue.isFull(); }
    bool isNotFull() const { return m_queue.isNotFull(); }    

protected:
    // Member vars
    EmTaskQueue<size_t> m_queue;
    size_t m_headIndex;
    SemaphoreHandle_t m_freeSlots;
    EmMutex m_pushMutex;
    T m_items[QueueSize];
};


template<typename T, size_t QueueSize>
class EmTaskStaticQueueValue : public EmTaskStaticQueue<T, QueueSize> {
public:
    static_assert(std::is_trivially_copyable<T>::value, 
        "ERROR: Type T must be trivially copyable to use EmTaskStaticQueueValue! Use the callback Base class instead.");

    using typename EmTaskStaticQueue<T, QueueSize>::PushCallback;
    using typename EmTaskStaticQueue<T, QueueSize>::PullCallback;
    using EmTaskStaticQueue<T, QueueSize>::push;
    using EmTaskStaticQueue<T, QueueSize>::pull;

    EmQueueResult push(const T& item, 
                       bool freeOldestItemIfFull, 
                       uint32_t waitIfFullMillis = 0) {
        return push([](T& slot, void* ctx) {
            slot = *static_cast<const T*>(ctx); 
        }, const_cast<T*>(&item), freeOldestItemIfFull, waitIfFullMillis);
    }

    EmQueueResult pull(T& item, 
                       uint32_t waitIfEmptyMillis = 0) {
        return pull([](const T& slot, void* ctx) {
            *static_cast<T*>(ctx) = slot;
        }, &item, waitIfEmptyMillis);
    }
};

#endif // __EM_TASK_STATIC_QUEUE_H