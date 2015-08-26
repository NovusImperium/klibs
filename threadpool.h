#ifndef THREADPOOL
#define THREADPOOL

#include "defs.h"

typedef enum {
    tp_invalid = -1,
    tp_lockfail = -2,
    tp_shutdown = -3,
    tp_threadfail = -4
} err_tp;

typedef enum {
    tpexit_graceful = 1
} dflags_tp;

typedef enum {
    tpsdown_now = 1,
    tpsdown_soft = 2
} sflags_tp;

// future struct
typedef struct tp_future tp_future;

// struct to store the threadpool in
typedef struct threadpool threadpool;

// initialize the threadpool with num_ts threads and a queue of initial size q_size; return null on failure
extern threadpool *tp_init(size_t num_ts);

// add a function to the task pool; returns 0 on success and an err_tp on failure
// note: any return value for func is ignored
extern int tp_add(threadpool *pool, void_ptr (*func)(void_ptr), void_ptr arg, int priority);

// add a function to the task pool; return a tp_future on success and null on failure
extern tp_future *tp_promise(threadpool *pool, void_ptr (*func), void_ptr arg, int priority);

// destroy the given threadpool in the manner determined by the flag (sflags_tp)
extern int tp_dest(threadpool *pool, int flags);

// wait for the queue to empty and return
extern void_ptr tp_await(threadpool *pool, tp_future *fut);

#endif
