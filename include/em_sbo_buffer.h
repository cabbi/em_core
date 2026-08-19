#ifndef __EM_AUTO_BUFFER_H__
#define __EM_AUTO_BUFFER_H__

#include <cstddef>

template<typename T, size_t StackSize>
class EmSboBuffer  {
public:
    explicit EmSboBuffer (size_t size)
     : m_heapBuf(nullptr), m_size(size) {
        if (size > StackSize) {
            m_heapBuf = new T[size]; 
        }
    }

    ~EmSboBuffer () {
        if (m_heapBuf != nullptr) {
            delete[] m_heapBuf;
        }
    }

    // No copy allowed
    EmSboBuffer (const EmSboBuffer &) = delete;
    EmSboBuffer & operator=(const EmSboBuffer &) = delete;

    // Simple Move constructor 
    EmSboBuffer (EmSboBuffer && other) noexcept : m_heapBuf(other.m_heapBuf), m_size(other.m_size) {
        other.m_heapBuf = nullptr;
        other.m_size = 0;
    }

    T* getBuffer() {
        return (m_heapBuf != nullptr) ? m_heapBuf : m_stackBuf;
    }

    size_t getSize() const { return m_size; }

private:
    T m_stackBuf[StackSize];     
    T* m_heapBuf;
    size_t m_size;
};

#endif //__EM_AUTO_BUFFER_H__