#ifndef _RWLOCK_H_
#define _RWLOCK_H_

#include <sys/select.h>
#include <stdint.h>
extern uint16_t g_proc_index;

#define MY_PROC_SET_LOCK_FLAG       (1ul << g_proc_index)
#define MY_PROC_CLEAR_LOCK_FLAG     (~MY_PROC_SET_LOCK_FLAG)

typedef struct rwlock {
    volatile int write;
    volatile int read;
    volatile uint64_t wrflag;
    volatile uint64_t rdflag;
} rwlock_t;

inline static void rwlock_init(rwlock_t *lock) {
    lock->write = 0;
    lock->read = 0;

    __sync_and_and_fetch(&lock->wrflag, 0ul);
    __sync_and_and_fetch(&lock->rdflag, 0ul);
}

inline static void rwlock_rdlock(rwlock_t *lock) {
    struct timeval wait_time;

    wait_time.tv_sec = 0;

    for (;;) {
        while (lock->write) {
            __sync_synchronize();
            wait_time.tv_usec = 1000;
            select(0, NULL, NULL, NULL, &wait_time);
        }
        __sync_add_and_fetch(&lock->read, 1);
        if (lock->write) {
            __sync_sub_and_fetch(&lock->read, 1);
        } else {
            __sync_or_and_fetch(&lock->rdflag, MY_PROC_SET_LOCK_FLAG);
            break;
        }
    }
}

/**
 * @function: rwlock_rdlock_ms
 * @brief: get read lock, but max wait ms milliseconds
 * @param: lock: rwlock
 *         ms: millisecond
 * @return: return 1 if got this lock, otherwise return 0 (timeout)
 */
inline static int rwlock_rdlock_ms(rwlock_t *lock, int ms) {
    struct timeval wait_time;
    int t = ms * 1000;

    wait_time.tv_sec = 0;

    for (;;) {
        while (lock->write) {
            __sync_synchronize();
            wait_time.tv_usec = 1000;
            t -= 1000;
            if (t < 0) {
                wait_time.tv_usec = (t + 1000);
            }

            select(0, NULL, NULL, NULL, &wait_time);

            if (t < 0) {
                return 0;
            }
        }
        __sync_add_and_fetch(&lock->read, 1);
        if (lock->write) {
            __sync_sub_and_fetch(&lock->read, 1);
            return 0;
        } else {
            __sync_or_and_fetch(&lock->rdflag, MY_PROC_SET_LOCK_FLAG);
            break;
        }
    }

    return 1;
}

inline static void rwlock_wrlock(rwlock_t *lock) {
    struct timeval wait_time;

    wait_time.tv_sec = 0;

    while (lock->read) {
        __sync_synchronize();
        wait_time.tv_usec = 1000;
        select(0, NULL, NULL, NULL, &wait_time);
    }

    while (__sync_lock_test_and_set(&lock->write, 1)) {
        wait_time.tv_usec = 1000;
        select(0, NULL, NULL, NULL, &wait_time);
    }

    while (lock->read) {
        __sync_synchronize();
        wait_time.tv_usec = 1000;
        select(0, NULL, NULL, NULL, &wait_time);
    }

    __sync_or_and_fetch(&lock->wrflag, MY_PROC_SET_LOCK_FLAG);
}

/**
 * @function: rwlock_wrlock_ms
 * @brief: get write lock, but max wait ms milliseconds
 * @param: lock: rwlock
 *         ms: millisecond
 * @return: return 1 if got this lock, otherwise return 0 (timeout)
 */
inline static int rwlock_wrlock_ms(rwlock_t *lock, int ms) {
    struct timeval wait_time;
    int t = ms * 1000;

    wait_time.tv_sec = 0;

    while (__sync_lock_test_and_set(&lock->write, 1)) {
        wait_time.tv_usec = 1000;
        t -= 1000;
        if (t < 0) {
            wait_time.tv_usec = (t + 1000);
        }

        select(0, NULL, NULL, NULL, &wait_time);

        if (t < 0) {
            return 0;
        }
    }

    while (lock->read) {
        __sync_synchronize();
        wait_time.tv_usec = 1000;
        t -= 1000;
        if (t < 0) {
            wait_time.tv_usec = (t + 1000);
        }

        select(0, NULL, NULL, NULL, &wait_time);

        if (t < 0) {
            __sync_sub_and_fetch(&lock->write, 1);
            return 0;
        }
    }

    __sync_or_and_fetch(&lock->wrflag, MY_PROC_SET_LOCK_FLAG);
    
    return 1;
}

inline static void rwlock_wrunlock(rwlock_t *lock) {
    __sync_lock_release(&lock->write);
    __sync_and_and_fetch(&lock->wrflag, MY_PROC_CLEAR_LOCK_FLAG);
}

inline static void rwlock_rdunlock(rwlock_t *lock) {
    __sync_sub_and_fetch(&lock->read, 1);
    __sync_and_and_fetch(&lock->rdflag, MY_PROC_CLEAR_LOCK_FLAG);
}

#endif

