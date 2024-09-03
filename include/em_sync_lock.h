#ifndef __SYNCLOCK__H_
#define __SYNCLOCK__H_


#ifndef MULTI_THREAD

class SemaphoreHandle_t {};

inline SemaphoreHandle_t xSemaphoreCreateBinary() {
    return SemaphoreHandle_t();
}

inline void xSemaphoreGive(SemaphoreHandle_t) {}
inline void xSemaphoreTake(SemaphoreHandle_t) {}


class EmSyncLock {
public:
    EmSyncLock(SemaphoreHandle_t) {}
};

#elif 

class EmSyncLock {
public:
    EmSyncLock(SemaphoreHandle_t& semaphore)
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