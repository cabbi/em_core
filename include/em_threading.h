#ifndef __EM_THREADING__H_
#define __EM_THREADING__H_

#include "em_defs.h"

// 'EmMutex' and 'EmLockGuard' definitions are used to abstract thread locking mechanisms
// so you can define them in your code in a platform-independent way (i.e. platforms that do not have <mutex>).

#ifdef EM_MULTITHREAD

#include <mutex>
#include <atomic>

using EmMutex = std::mutex;
using EmMutexLock = std::lock_guard<std::mutex>;

// Thread safe atomic basic types
using ts_bool = std::atomic<bool>;
using ts_int8 = std::atomic<int8_t>;
using ts_uint8 = std::atomic<uint8_t>;
using ts_int16 = std::atomic<int16_t>;
using ts_uint16 = std::atomic<uint16_t>;
using ts_int32 = std::atomic<int32_t>;
using ts_uint32 = std::atomic<uint32_t>;
using ts_int64 = std::atomic<int64_t>;
using ts_uint64 = std::atomic<uint64_t>;

#else

class EmMutex {};

class EmMutexLock {
public:
    EmMutexLock(EmMutex&) {}
};

// Thread safe atomic basic types
using ts_bool = bool;
using ts_int8 = int8_t;
using ts_uint8 = uint8_t;
using ts_int16 = int16_t;
using ts_uint16 = uint16_t;
using ts_int32 = int32_t;
using ts_uint32 = uint32_t;
using ts_int64 = int64_t;
using ts_uint64 = uint64_t;

#endif
#endif