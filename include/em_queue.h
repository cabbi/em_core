#ifndef __EM_QUEUE_H__
#define __EM_QUEUE_H__

#include <stdint.h>

// Defines the queue type
enum class EmQueueType : uint8_t {
    Fifo = 0,  // First In First Out
    Lifo = 1   // Last In First Out
};

// A sized FIFO or LIFO queue.
template<class T>
class EmQueue : protected EmList<T> {
public:
    EmQueue(EmQueueType type, uint16_t maxSize) :
        m_type(type), m_maxSize(maxSize) {}

    ~EmQueue() {
        clear();
    }

    // Push item to the queue.
    //
    // If the queue size exceeds maxSize, the oldest item is removed.
    void push(T& item, bool takeOwnership) {
        if (m_type == EmQueueType::Fifo) {
            append(item, takeOwnership);
        } else {
            insertFirst(item, takeOwnership);
        }
        // Remove oldest items if exceeding max size.
        while (count() > m_maxSize) {
            if (m_type == EmQueueType::Fifo) {
                removeFirst();
            } else {
                removeLast();
            }
        }
    }   

    // Pop item from the queue.
    bool pop(T& item) {
        T* pItem = nullptr;
        if (m_type == EmQueueType::Fifo) {
            pItem = first();
            if (pItem != nullptr) {
                item = *pItem;
                removeFirst();
                return true;
            }
        } else {
            pItem = first(); // Corrected for LIFO: Last In is at the front of the list
            if (pItem != nullptr) {
                item = *pItem;
                removeFirst(); // Corrected for LIFO: Remove from the front
                return true;
            }
        }
        return false;
    }   
    
    // Returns the number of items in the queue.
    uint16_t count() const {        
        return EmList<T>::count();
    }

    // Returns true if the queue is empty.
    bool isEmpty() const { 
        return EmList<T>::isEmpty();
    }
    
    // Returns true if the queue is not empty.
    bool isNotEmpty() const { 
        return !isEmpty(); 
    }

    // Clears the queue.
    void clear() {
        EmList<T>::clear();
    }
    
protected:
    // Member vars
    EmQueueType m_type;
    uint16_t m_maxSize;
};


// A sized circular FIFO or LIFO queue.
template<class T, uint16_t Size>
class EmSizedQueue {
public:
    EmSizedQueue(EmQueueType type = EmQueueType::Fifo) : 
        m_type(type), m_head(0), m_count(0) {}

    ~EmSizedQueue() {
        clear();
    }

    void clear() {
        m_head = 0;
        m_count = 0;
    }

    uint16_t count() const { return m_count; }
    bool isFull() const { return count() >= Size; }
    bool isEmpty() const { return count() == 0; }

    bool push(const T& item, bool failIfFull = true) {
        if (isFull()) {
            if (failIfFull) {
                return false;
            } else {
                // Remove oldest item to make space for new item.
                T dummy; // Item is discarded, so no need to store it
                _popFirst(dummy); // Always remove the oldest item (at m_head)
            }
        }
        _push(item);
        return true;
    }

    bool pop(T& item) {
        if (isEmpty()) {
            return false;
        }
        // Remove and return item.
        if (m_type == EmQueueType::Fifo) {
            // Remove first item.
            _popFirst(item);
        } else {
            // Remove last item.
            _popLast(item);
        }
        return true;
    }

protected:
    void _push(const T& item) {
        uint16_t tail = (static_cast<uint32_t>(m_head) + m_count) % Size;
        m_items[tail] = item;
        m_count++;
    }

    void _popFirst(T& item) {
        item = m_items[m_head];
        m_head = (m_head + 1) % Size;
        m_count--;
    }

    void _popLast(T& item) {
        uint16_t last = (static_cast<uint32_t>(m_head) + m_count - 1) % Size;
        item = m_items[last];
        m_count--;
    }

private:
    // Member vars
    EmQueueType m_type;
    T m_items[Size];
    uint16_t m_head;  // Index of the head item  
    uint16_t m_count; // Number of items in the queue
};

#endif // __EM_QUEUE_H__