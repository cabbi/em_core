#ifndef __THREAD_LOCK__H_
#define __THREAD_LOCK__H_

#ifndef MULTI_THREAD

class SemaphoreHandle_t {};

inline SemaphoreHandle_t xSemaphoreCreateBinary() {
    return SemaphoreHandle_t();
}

inline void xSemaphoreGive(SemaphoreHandle_t) {}
inline void xSemaphoreTake(SemaphoreHandle_t) {}


class EmThreadLock {
public:
    EmThreadLock(SemaphoreHandle_t) {}
};

#else

class EmThreadLock {
public:
    EmThreadLock(SemaphoreHandle_t& semaphore)
     : m_semaphore(semaphore)
    { 
        xSemaphoreTake(m_semaphore, 100); 
    }

    virtual ~SyncLock() 
    {  
        xSemaphoreGive(m_semaphore);  
    } 

private:
    SemaphoreHandle_t m_semaphore;
};
#endif
#endif