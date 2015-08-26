#include <stdlib.h>
#include <pthread.h>

#include "threadpool.h"
#include "heap.h"

struct tp_future {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    bool done;
    void *ret;
};

// task to store in the queue
typedef struct {
    int pty;
    void *(*func)(void *);
    void *arg;
    tp_future *fut;
} tp_task;

struct threadpool {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *ts;
    heap *tasks;
    size_t num_ts;
    size_t started;
    bool shutdown;
};

// worker thread for the threadpool
static void *tp_thread(void *arg);

// cleanup and free all threadpool-owned memory
int tp_free(threadpool *pool);

// function to cleanup futures
void *tp_future_free(void *fut);

// comparison function for task priority
bool tp_taskcomp(void const *lhs, void const *rhs);

threadpool *tp_init(size_t num_ts) {
    threadpool *pool;
    if ((pool = (threadpool *) malloc(sizeof(threadpool))) == null) {
        goto err;
    }

    // init the pool
    pool->num_ts = 0;
    pool->shutdown = false;
    pool->started = 0;
    pool->tasks = h_init(32, tp_taskcomp);

    pool->ts = malloc(sizeof(pthread_t) * num_ts);

    // setup mutex and conditional notification
    if (pthread_mutex_init(&pool->lock, null) != 0 ||
            pthread_cond_init(&pool->notify, null) != 0 ||
            pool->ts == null) {
        goto err;
    }

    // spin up worker threads
    size_t i;
    for (i = 0; i < num_ts; i++) {
        if (pthread_create(&pool->ts[i], null, tp_thread, (void *) pool) != 0) {
            tp_dest(pool, 0);
            return null;
        }
        pool->num_ts++;
        pool->started++;
    }

    return pool;

    err:
    // initialization has failed somewhere, cleanup and return null
    if (pool) {
        tp_free(pool);
    }
    return null;
}

int tp_add(threadpool *pool, void *(*func)(void *), void *arg, int priority) {
    int err = 0;

    if (pool == null || func == null) {
        return tp_invalid;
    }

    // wait for lock on the queue, quit if lock fails
    if (pthread_mutex_lock(&pool->lock) != 0) {
        return tp_lockfail;
    }

    // check for shutdown
    if (pool->shutdown) {

        tp_task *task = malloc(sizeof(tp_task));
        task->func = func;
        task->arg = arg;
        task->pty = priority;
        task->fut = null;

        h_push(pool->tasks, task);

        // send notification that the queue is unlocking
        if (pthread_cond_signal(&pool->notify) != 0) {
            err = tp_lockfail;
        }
    } else {
        err = tp_shutdown;
    }

    if (pthread_mutex_unlock(&pool->lock) != 0) {
        err = tp_lockfail;
    }

    return err;
}

tp_future *tp_promise(threadpool *pool, void *func, void *arg, int priority) {
    int err = 0;
    tp_future *fut = null;

    if (pool == null || func == null) {
        return null;
    }

    // wait for lock on the queue, quit if lock fails
    if (pthread_mutex_lock(&pool->lock) != 0) {
        return fut;
    }

    // check for shutdown
    if (!pool->shutdown) {
        tp_task *task = malloc(sizeof(tp_task));
        task->func = func;
        task->arg = arg;
        task->pty = priority;

        // initialize the future
        fut = malloc(sizeof(tp_future));
        pthread_mutex_init(&fut->lock, null);
        pthread_cond_init(&fut->notify, null);
        task->fut = fut;

        // push the task to the queue
        h_push(pool->tasks, task);

        // send notification that the queue is unlocking
        if (pthread_cond_signal(&pool->notify) != 0) {
            err = tp_lockfail;
        }
    } else {
        err = tp_shutdown;
    }


    if (pthread_mutex_unlock(&pool->lock) != 0) {
        err = tp_lockfail;
    }

    // cleanup the future if there has been an error after the future has been created
    if (err != 0 && fut != null) {
        fut = tp_future_free(fut);
    }

    return fut;
}

int tp_dest(threadpool *pool, int flags) {
    int i, err = 0;

    if (pool == null) {
        return tp_invalid;
    }

    if (pthread_mutex_lock(&pool->lock) != 0) {
        return tp_lockfail;
    }

    // check that we're not shutting down already
    if (!pool->shutdown) {
        pool->shutdown = (flags & tpexit_graceful) ?
                tpsdown_soft : tpsdown_now;

        // wakeup worker threads
        if (pthread_cond_broadcast(&pool->notify) != 0 ||
                pthread_mutex_unlock(&pool->lock) != 0) {
            err = tp_lockfail;
        }

        // recall worker threads
        for (i = 0; i < pool->num_ts; i++) {
            if (pthread_join(pool->ts[i], null) != 0) {
                err = tp_threadfail;
            }
        }
    } else {
        err = tp_shutdown;
    }

    // don't free if we hit an error
    if (!err) {
        tp_free(pool);
    }
    return err;
}

int tp_free(threadpool *pool) {
    if (pool == null || pool->started > 0) {
        return -1;
    }

    // release threadpool if it has been created
    if (pool->ts) {
        pthread_mutex_lock(&pool->lock);

        // free the task queue if it has been initialized
        if (pool->tasks != null) {
            h_foreach(pool->tasks, tp_future_free);
            h_free(pool->tasks);
        }

        free(pool->ts);
        pthread_mutex_destroy(&pool->lock);
        pthread_cond_destroy(&pool->notify);
    }
    free(pool);
    return 0;
}

static void *tp_thread(void *arg) {
    threadpool *pool = ((threadpool *) arg);
    tp_task *task;

    while (true) {
        // wait for the queue to be free and lock while getting task
        pthread_mutex_lock(&pool->lock);

        // wait on condition variable, check for spurious wakeups
        // we own the lock when returning from pthread_cond_wait
        while (h_empty(pool->tasks) && !pool->shutdown) {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }

        if ((pool->shutdown == tpsdown_now) ||
                ((pool->shutdown == tpsdown_soft) &&
                        h_empty(pool->tasks))) {
            break;
        }

        // get the task from the queue
        task = (tp_task *) h_pop(pool->tasks);

        // release the lock on the queue
        pthread_mutex_unlock(&pool->lock);

        if (task->fut == null) {
            // run the task without waiting for the future
            task->func(task->arg);
        } else {
            // get the lock on the future so we can run the task
            pthread_mutex_lock(&task->fut->lock);
            task->fut->ret = task->func(task->arg);
            task->fut->done = true;

            // let the calling thread know the
            pthread_cond_broadcast(&task->fut->notify);
            pthread_mutex_unlock(&task->fut->lock);
        }

        // this task is complete, free allocated memory
        free(task);
    }

    pool->started--;

    pthread_mutex_unlock(&pool->lock);
    pthread_exit(null);
}

void *tp_await(threadpool *pool, tp_future *fut) {
    void *ret = null;
    pthread_mutex_lock(&fut->lock);
    while (!fut->done && !pool->shutdown) {
        pthread_cond_wait(&fut->notify, &fut->lock);
    }

    if (fut->done) {
        ret = fut->ret;
    }

    tp_future_free(fut);

    return ret;
}

bool tp_taskcomp(void const *lhs, void const *rhs) {
    return ((tp_task *) lhs)->pty > ((tp_task *) rhs)->pty;
}

void *tp_future_free(void *fut) {
    tp_future *f = (tp_future *) fut;
    pthread_mutex_destroy(&f->lock);
    pthread_cond_destroy(&f->notify);
    free(f);

    return null;
}
