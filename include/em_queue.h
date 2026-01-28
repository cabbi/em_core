#ifndef __EM_QUEUE_H__
#define __EM_QUEUE_H__

#include <stdint.h>

// Defines the queue type
enum class EmQueueType : uint8_t {
    Fifo = 0,  // First In First Out
    Lifo = 1   // Last In First Out
};

// A circular/sized FIFO or LIFO queue.
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
            pItem = last();
            if (pItem != nullptr) {
                item = *pItem;
                removeLast();
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

#endif // __EM_QUEUE_H__