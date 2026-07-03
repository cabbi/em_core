#ifndef __EM_TASK_QUEUE_H
#define __EM_TASK_QUEUE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <type_traits>

template <typename T>
class EmTaskQueue {
    // Ensure the type is safe to copy via raw bytes (FreeRTOS requirement)
    static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");

public:
    // Create a queue with a maximum number of items
    explicit EmTaskQueue(UBaseType_t queue_length) {
        m_handle = xQueueCreate(queue_length, sizeof(T));
    }

    ~EmTaskQueue() {
        if (m_handle != nullptr) {
            vQueueDelete(m_handle);
        }
    }

    // Prevent copying to avoid double deletion of the handle
    EmTaskQueue(const EmTaskQueue&) = delete;
    EmTaskQueue& operator=(const EmTaskQueue&) = delete;

    // Allow moving
    EmTaskQueue(EmTaskQueue&& other) noexcept : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }

    EmTaskQueue& operator=(EmTaskQueue&& other) noexcept {
        if (this != &other) {
            if (m_handle != nullptr) {
                vQueueDelete(m_handle);
            }
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    // Send an item to the back of the queue
    bool send(const T& item, TickType_t wait_ticks = portMAX_DELAY) {
        if (m_handle == nullptr) {
            return false;
        }
        return xQueueSend(m_handle, &item, wait_ticks) == pdTRUE;
    }

    // Send an item from an Interrupt Service Routine (ISR)
    bool sendFromISR(const T& item, BaseType_t* higher_priority_task_woken) {
        if (m_handle == nullptr) {
            return false;
        }
        return xQueueSendFromISR(m_handle, &item, higher_priority_task_woken) == pdTRUE;
    }

    // Receive an item from the queue
    bool receive(T& item, TickType_t wait_ticks = portMAX_DELAY) {
        if (m_handle == nullptr) return false;
        return xQueueReceive(m_handle, &item, wait_ticks) == pdTRUE;
    }

    // Get the number of messages waiting in the queue
    UBaseType_t messagesWaiting() const {
        if (m_handle == nullptr) {
            return 0;
        }
        return uxQueueMessagesWaiting(m_handle);
    }

    // Check if the queue handle is valid
    bool isValid() const { return m_handle != nullptr; }

    // --- Status Verification Methods ---

    // Returns true if the queue is empty (for use in Tasks)
    bool isEmpty() const {
        if (m_handle == nullptr) {
            return true;
        }
        return uxQueueMessagesWaiting(m_handle) == 0;
    }

    // Returns true if the queue is NOT empty (for use in Tasks)
    bool isNotEmpty() const {
        return !isEmpty();
    }
    
    // Returns true if the queue is full (for use in Tasks)
    bool isFull() const {
        if (m_handle == nullptr) {
            return true;
        }
        // Returns the number of free slots remaining in the queue
        return uxQueueSpacesAvailable(m_handle) == 0;
    }

    // Returns true if the queue is NOT full (for use in Tasks)
    bool isNotFull() const {
        return !isFull();
    }

    // Returns true if the queue is empty (safe for use in ISRs)
    bool isEmptyFromISR() const {
        if (m_handle == nullptr) {
            return true;
        }
        return xQueueIsQueueEmptyFromISR(m_handle) == pdTRUE;
    }

    // Returns true if the queue is NOT empty (safe for use in ISRs)
    bool isNotEmptyFromISR() const {
        return !isEmptyFromISR();
    }

    // Returns true if the queue is full (safe for use in ISRs)
    bool isFullFromISR() const {
        if (m_handle == nullptr) {
            return true;
        }
        return xQueueIsQueueFullFromISR(m_handle) == pdTRUE;
    }

    // Returns true if the queue is NOT full (safe for use in ISRs)
    bool isNotFullFromISR() const {
        return !isFullFromISR();
    }

private:
    QueueHandle_t m_handle = nullptr;
};

#endif