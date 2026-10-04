#ifndef __EM_TASK_STATIC_QUEUE_H
#define __EM_TASK_STATIC_QUEUE_H

#include "em_defs.h"
#include "em_timeout.h"
#include "em_task_queue.h"

// The queue operation result
enum class EmQueueResult : uint8_t {
    OK = 0,
    ERROR_INVALID_QUEUE,    
    ERROR_FULL,             
    ERROR_EMPTY,            
    ERROR_SEQUENCE_VIOLATION 
};

inline const char* EmQueueResultToText(EmQueueResult result) {
    switch (result) {
        case EmQueueResult::OK:                       return "OK";
        case EmQueueResult::ERROR_INVALID_QUEUE:      return "Invalid queue";
        case EmQueueResult::ERROR_FULL:               return "Queue full";
        case EmQueueResult::ERROR_EMPTY:              return "Queue empty";
        case EmQueueResult::ERROR_SEQUENCE_VIOLATION: return "Sequence violation";
        default :                                     return "Unknown result";
    }
}
// A static 'Single Producer' queue that uses a fixed-size array to store items and an 
// EmTaskQueue to manage the indices of the items in the array.
// To avoid element copying, the user should first call getNextItemToPush() to 
// get a pointer to the next available item in the buffer pool, fill it with data, 
// and then call pushCommit() to push the index of that item to the queue. 
// This design avoids unnecessary copying of large objects and allows for efficient memory management.
// NOTES: 
//  - This class is designed to have only one producer that gets and commits push elements! 
//  - 'T' type should have default constructor defined. 
template<typename T, size_t QUEUE_SZ>
class EmTaskStaticQueue {
public:
    EmTaskStaticQueue()
     : m_queue(QUEUE_SZ), 
       m_headIndex(0), 
       m_freeSlots(xSemaphoreCreateCounting(QUEUE_SZ, QUEUE_SZ)),
       m_isPushPending(false) {}

    virtual ~EmTaskStaticQueue() {
        if (m_freeSlots != nullptr) {
            vSemaphoreDelete(m_freeSlots);
        }
    }

     EmQueueResult getNextItemToPush(T*& item, uint32_t waitIfFullMillis = 0) {
        if (isNotValid()) {
            return EmQueueResult::ERROR_INVALID_QUEUE;
        }

        if (m_isPushPending) {
            return EmQueueResult::ERROR_SEQUENCE_VIOLATION;
        }

        TickType_t ticksToWait = (waitIfFullMillis == portMAX_DELAY) ? 
                                 portMAX_DELAY : pdMS_TO_TICKS(waitIfFullMillis);
        if (xSemaphoreTake(m_freeSlots, ticksToWait) == pdTRUE) {
            item = &m_items[m_headIndex];
            m_isPushPending = true; 
            return EmQueueResult::OK;
        }

        return EmQueueResult::ERROR_FULL; 
    }

    // Commit the push of the element that was obtained from getNextItemToPush() to the queue.
    EmQueueResult commitPush() {
        if (isNotValid()) {
            return EmQueueResult::ERROR_INVALID_QUEUE;
        }
        
        if (!m_isPushPending) {
            return EmQueueResult::ERROR_SEQUENCE_VIOLATION;
        }

        // Push the index of the item to the queue with 0 timeout (non-blocking). 
        // The item itself is already in the buffer pool, so we only need to push the index.
        if (m_queue.push(m_headIndex, 0)) {
            m_headIndex = (m_headIndex + 1) % QUEUE_SZ;
            m_isPushPending = false;
            return EmQueueResult::OK;
        }
        // Push unexpectedly failed, release the semaphore to indicate that the slot is still available.
        xSemaphoreGive(m_freeSlots); 
        m_isPushPending = false;
        return EmQueueResult::ERROR_FULL;
    }

    // Pull an item from the queue
    EmQueueResult pull(T*& item, uint32_t waitIfEmptyMillis = 0) {
        if (isNotValid()) {
            return EmQueueResult::ERROR_INVALID_QUEUE;
        }
        size_t index;
        if (!m_queue.pull(index, waitIfEmptyMillis)) {
            return EmQueueResult::ERROR_EMPTY;
        }
        item = &m_items[index];
        xSemaphoreGive(m_freeSlots);
        return EmQueueResult::OK;
    }

    bool isValid() const { return m_queue.isValid() && m_freeSlots != nullptr; }
    bool isNotValid() const { return !isValid(); }
    bool isEmpty() const { return m_queue.isEmpty(); }
    bool isNotEmpty() const { return m_queue.isNotEmpty(); }
    bool isFull() const { return m_queue.isFull(); }
    bool isNotFull() const { return m_queue.isNotFull(); }

protected:
    EmTaskQueue<size_t> m_queue;
    size_t m_headIndex;
    SemaphoreHandle_t m_freeSlots;
    bool m_isPushPending;
    T m_items[QUEUE_SZ];
};

#endif // __EM_TASK_STATIC_QUEUE_H