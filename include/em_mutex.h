#ifndef __EM_MUTEX__H_
#define __EM_MUTEX__H_

#include "em_defs.h"

// 'EmMutex' and 'EmLockGuard' definitiona are used to abstract thread locking mechanisms
// so you can define them in your code in a platform-independent way (i.e. platforms that do not have <mutex>).

#ifdef EM_MULTITHREAD

#include <mutex>
typedef std::mutex EmMutex;
typedef std::lock_guard<std::mutex> EmLockGuard;

#else

class EmMutex {};

class EmLockGuard {
public:
    EmLockGuard(EmMutex&) {}
};

#endif
#endif