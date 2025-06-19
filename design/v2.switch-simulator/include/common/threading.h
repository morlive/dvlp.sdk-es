/**
 * @file threading.h
 * @brief Threading type definitions and synchronization primitives for switch simulator
 */

#ifndef SWITCH_SIM_THREADING_H
#define SWITCH_SIM_THREADING_H

/**
 * @brief Spinlock type for thread synchronization
 */
typedef struct {
    volatile int lock;  /**< Lock variable for atomic operations */
} spinlock_t;

/**
 * @brief Initialize a spinlock
 *
 * @param lock Pointer to spinlock to initialize
 */
static inline void spinlock_init(spinlock_t *lock) {
    lock->lock = 0;
}

/**
 * @brief Acquire a spinlock
 *
 * @param lock Pointer to spinlock to acquire
 */
static inline void spinlock_acquire(spinlock_t *lock) {
    while (__sync_lock_test_and_set(&lock->lock, 1)) {
        // Spin until acquired
        // In a real implementation, we might want to add a short delay
        // or CPU yield instruction here

        // Варианты реализации задержки:
        
        // 1. CPU yield - для х86/х86_64
        #if defined(__x86_64__) || defined(__i386__)
            __asm__ volatile("pause" ::: "memory");
        #elif defined(__arm__) || defined(__aarch64__)
            // Для ARM архитектуры
            __asm__ volatile("yield" ::: "memory");
        #endif

        // 2. Короткий цикл для создания задержки
        for (volatile int i = 0; i < 10; i++) { }

        // 3. Счетчик попыток с экспоненциальной задержкой
        // static int backoff = 1;
        // if (backoff < 1024) backoff *= 2;
        // for (volatile int i = 0; i < backoff; i++) { }

    }

    // Сброс счетчика задержки при успешном получении блокировки
    // backoff = 1;
}

/**
 * @brief Release a spinlock
 *
 * @param lock Pointer to spinlock to release
 */
static inline void spinlock_release(spinlock_t *lock) {
    __sync_lock_release(&lock->lock);
}

/**
 * @brief Try to acquire a spinlock without blocking
 *
 * @param lock Pointer to spinlock to acquire
 * @return int 0 if lock was acquired, non-zero if it was already locked
 */
static inline int spinlock_try_acquire(spinlock_t *lock) {
    return __sync_lock_test_and_set(&lock->lock, 1);
}

#endif /* SWITCH_SIM_THREADING_H */
