#ifndef __EM_MUTEX__H_
#define __EM_MUTEX__H_

#include "em_defs.h"

// 'EmMutex' and 'EmLockGuard' definitions are used to abstract thread locking mechanisms
// so you can define them in your code in a platform-independent way (i.e. platforms that do not have <mutex>).

#ifdef EM_MULTITHREAD

#include <mutex>
#include <atomic>

typedef std::mutex EmMutex;
typedef std::lock_guard<std::mutex> EmLockGuard;

// Thread safe atomic basic types
typedef std::atomic<bool> ts_bool;
typedef std::atomic<int8_t> ts_int8;
typedef std::atomic<uint8_t> ts_uint8;
typedef std::atomic<int16_t> ts_int16;
typedef std::atomic<uint16_t> ts_uint16;
typedef std::atomic<int32_t> ts_int32;
typedef std::atomic<uint32_t> ts_uint32;
typedef std::atomic<int64_t> ts_int64;
typedef std::atomic<uint64_t> ts_uint64;

#else

class EmMutex {};

class EmLockGuard {
public:
    EmLockGuard(EmMutex&) {}
};

typedef bool ts_bool;
typedef int8_t ts_int8;
typedef uint8_t ts_uint8;
typedef int16_t ts_int16;
typedef uint16_t ts_uint16;
typedef int32_t ts_int32;
typedef uint32_t ts_uint32;
typedef int64_t ts_int64;
typedef uint64_t ts_uint64;

#endif
#endif