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
        handle_ = xQueueCreate(queue_length, sizeof(T));
    }

    ~EmTaskQueue() {
        if (handle_ != nullptr) {
            vQueueDelete(handle_);
        }
    }

    // Prevent copying to avoid double deletion of the handle
    EmTaskQueue(const Queue&) = delete;
    EmTaskQueue& operator=(const EmTaskQueue&) = delete;

    // Allow moving
    EmTaskQueue(EmTaskQueue&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    EmTaskQueue& operator=(EmTaskQueue&& other) noexcept {
        if (this != &other) {
            if (handle_ != nullptr) vQueueDelete(handle_);
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    // Send an item to the back of the queue
    bool send(const T& item, TickType_t wait_ticks = portMAX_DELAY) {
        if (handle_ == nullptr) return false;
        return xQueueSend(handle_, &item, wait_ticks) == pdTRUE;
    }

    // Send an item from an Interrupt Service Routine (ISR)
    bool sendFromISR(const T& item, BaseType_t* higher_priority_task_woken) {
        if (handle_ == nullptr) return false;
        return xQueueSendFromISR(handle_, &item, higher_priority_task_woken) == pdTRUE;
    }

    // Receive an item from the queue
    bool receive(T& item, TickType_t wait_ticks = portMAX_DELAY) {
        if (handle_ == nullptr) return false;
        return xQueueReceive(handle_, &item, wait_ticks) == pdTRUE;
    }

    // Get the number of messages waiting in the queue
    UBaseType_t messagesWaiting() const {
        if (handle_ == nullptr) return 0;
        return uxQueueMessagesWaiting(handle_);
    }

    // Check if the queue handle is valid
    bool isValid() const { return handle_ != nullptr; }

    // --- Status Verification Methods ---

    // Returns true if the queue is empty (for use in Tasks)
    bool isEmpty() const {
        if (handle_ == nullptr) return true;
        return uxQueueMessagesWaiting(handle_) == 0;
    }

    // Returns true if the queue is NOT empty (for use in Tasks)
    bool isNotEmpty() const {
        return !isEmpty();
    }
    
    // Returns true if the queue is full (for use in Tasks)
    bool isFull() const {
        if (handle_ == nullptr) return true;
        // Returns the number of free slots remaining in the queue
        return uxQueueSpacesAvailable(handle_) == 0;
    }

    // Returns true if the queue is NOT full (for use in Tasks)
    bool isNotFull() const {
        return !isFull();
    }

    // Returns true if the queue is empty (safe for use in ISRs)
    bool isEmptyFromISR() const {
        if (handle_ == nullptr) return true;
        return xQueueIsQueueEmptyFromISR(handle_) == pdTRUE;
    }

    // Returns true if the queue is NOT empty (safe for use in ISRs)
    bool isNotEmptyFromISR() const {
        return !isEmptyFromISR();
    }

    // Returns true if the queue is full (safe for use in ISRs)
    bool isFullFromISR() const {
        if (handle_ == nullptr) return true;
        return xQueueIsQueueFullFromISR(handle_) == pdTRUE;
    }

    // Returns true if the queue is NOT full (safe for use in ISRs)
    bool isNotFullFromISR() const {
        return !isFullFromISR();
    }

private:
    QueueHandle_t handle_ = nullptr;
};

#endif