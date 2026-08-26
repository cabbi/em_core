#ifndef __EM_AUTO_BUFFER_H__
#define __EM_AUTO_BUFFER_H__

#include <cstddef>

// A Small Buffer Optimization buffer trying to use stack memory and only allocate heap if
// bigger buffers are requested. 'T' can be of any type, no direct memory copy or set is used!
template<typename T, size_t StackSize>
class EmSboBuffer  {
public:
    EmSboBuffer()
     : m_heapBuf(nullptr), 
       m_maxSize(StackSize) {}

    explicit EmSboBuffer(size_t maxSize)
     : m_heapBuf(nullptr), m_maxSize(StackSize) {
        if (maxSize > StackSize) {
            m_heapBuf = new T[maxSize]; 
            m_maxSize = maxSize;
        }
    }

    ~EmSboBuffer() {
        clear();
    }

    // No copy allowed (move are removed as well)
    EmSboBuffer(const EmSboBuffer &) = delete;
    EmSboBuffer & operator=(const EmSboBuffer &) = delete;
    
    void clear() {
        // Should we delete old heap buffer?
        if (m_heapBuf != nullptr) {
            delete[] m_heapBuf;
            m_heapBuf = nullptr;
        }
        m_maxSize = StackSize;
    }

    // Size is never shrunk, so the max buffer size can only be expanded.
    // This will avoid deleting and re-create heap buffers if not needed.
    //
    // Return the max size of the buffer which might be bigger than the new proposed size.
    size_t setMaxSize(size_t size) {
        if (size > m_maxSize ) {
            // We need a bigger buffer, lets clean old heap if already allocated
            clear();
            // Heap buffer needed?
            if (size > StackSize) {
                m_heapBuf = new T[size]; 
            }
            m_maxSize = size;
        }
        return m_maxSize;
    }

    T* getBuffer() {
        return (m_heapBuf != nullptr) ? m_heapBuf : m_stackBuf;
    }

    size_t getMaxSize() const { return m_maxSize; }

protected:
    T m_stackBuf[StackSize];     
    T* m_heapBuf;
    size_t m_maxSize;
};

#endif //__EM_AUTO_BUFFER_H__