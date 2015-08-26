#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "defs.h"
#include "threadarray.h"
#include "optional.h"

struct threadarray {
    const int8_t t;
    size_t m;
    size_t n;
    pthread_mutex_t lock;
    pthread_cond_t notify;
    void_ptr *as;
};

optional tharr_init(size_t m) {
    optional opt;
    opt.e = true;

    threadarray *arr;
    if ((arr = malloc(sizeof(threadarray))) == null) {
        opt.e = false;
        opt.err = malloc_fail;
        return opt;
    }

    if ((arr->as = malloc(m * sizeof(void_ptr))) == null) {
        free(arr);
        opt.e = false;
        opt.err = malloc_fail;
        return opt;
    }

    if ((pthread_cond_init(&arr->notify, null) != 0) ||
            pthread_mutex_init(&arr->lock, null) != 0) {
        free(arr->as);
        free(arr);
        opt.e = false;
        opt.err = init_lock_fail;
        return opt;
    }

    *((int8_t *) arr) = 6;
    memset(arr->as, 0, m * sizeof(void_ptr));
    arr->m = m;
    arr->n = 0;

    opt.val = arr;
    return opt;
}

optional tharr_copy(threadarray *arr, size_t m) {
    optional opt;
    opt.e = true;

    threadarray *new_arr;
    if ((new_arr = malloc(sizeof(threadarray))) == null) {
        opt.e = false;
        opt.err = malloc_fail;
        return opt;
    }

    if ((new_arr->as = malloc(m * sizeof(void_ptr))) == null) {
        free(new_arr);
        opt.e = false;
        opt.err = malloc_fail;
        return opt;
    }

    new_arr->m = m;
    if ((pthread_cond_init(&arr->notify, null) != 0) ||
            pthread_mutex_init(&arr->lock, null) != 0) {
        free(arr->as);
        free(arr);
        opt.e = false;
        opt.err = init_lock_fail;
        return opt;
    }

    if (pthread_mutex_lock(&arr->lock) == 0) {
        new_arr->n = arr->n < m ? arr->n : m;
        memcpy(new_arr->as, arr->as, new_arr->n * sizeof(void_ptr));
        pthread_mutex_unlock(&arr->lock);
    } else {
        free(new_arr->as);
        free(new_arr);
        opt.e = false;
        opt.err = get_lock_fail;
        return opt;
    }

    opt.val = new_arr;
    return opt;
}

int tharr_free(threadarray *arr) {
    if(pthread_mutex_lock(&arr->lock) != 0) {
        return get_lock_fail;
    }

    free(arr->as);

    if (pthread_cond_destroy(&arr->notify) != 0 ||
            pthread_mutex_destroy(&arr->lock) != 0) {
        return init_lock_fail;
    }

    free(arr);
    return no_err;
}

optional tharr_peek(threadarray *arr) {
    optional opt;
    opt.e = true;
    if (pthread_mutex_lock(&arr->lock) == 0) {
        if (arr->n > 0) {
            void_ptr a = arr->as[0];

            pthread_mutex_unlock(&arr->lock);
            opt.val = a;
            return opt;
        } else {
            opt.e = false;
            opt.err = container_empty;
            return opt;
        }
    } else {
        opt.e = false;
        opt.err = get_lock_fail;
        return opt;
    }
}

optional tharr_pop(threadarray *arr) {
    optional opt;
    opt.e = true;
    if (pthread_mutex_lock(&arr->lock) == 0) {
        if (arr->n > 0) {
            void_ptr a = arr->as[0];
            memmove(arr->as, &arr->as[1], (arr->n--) * sizeof(void_ptr));

            pthread_mutex_unlock(&arr->lock);
            pthread_cond_broadcast(&arr->notify);

            opt.val = a;
            return opt;
        } else {
            opt.e = false;
            opt.err = container_empty;
            return opt;
        }
    } else {
        opt.e = false;
        opt.err = get_lock_fail;
        return opt;
    }
}

bool tharr_push(threadarray *arr, void_ptr a) {
    if (pthread_mutex_lock(&arr->lock) == 0) {
        if (arr->n == arr->m) {
            size_t new_size = arr->m * 2;
            void_ptr *new_as;
            if ((new_as = malloc(new_size * sizeof(void_ptr))) == null) {
                pthread_mutex_unlock(&arr->lock);
                return false;
            }
            memcpy(new_as, arr->as, arr->m * sizeof(void_ptr));
            free(arr->as);
            arr->as = new_as;
            arr->m = new_size;
        }

        arr->as[arr->n++] = a;
        pthread_mutex_unlock(&arr->lock);
        pthread_cond_broadcast(&arr->notify);

        return true;
    } else {
        return false;
    }
}

bool tharr_concat(threadarray *dest, threadarray *src) {
    if (pthread_mutex_lock(&dest->lock) == 0 &&
            pthread_mutex_lock(&src->lock) == 0) {
        if (dest->n + src->n >= dest->m) {
            size_t new_size = dest->m + src->m;
            void_ptr *new_as;
            if ((new_as = malloc(new_size * sizeof(void_ptr))) == null) {
                pthread_mutex_unlock(&dest->lock);
                pthread_mutex_unlock(&src->lock);
                return false;
            }
            memcpy(new_as, dest->as, dest->n * sizeof(void_ptr));
            free(dest->as);
            dest->as = new_as;
        }

        memcpy(&dest->as[dest->n], src->as, src->n * sizeof(void_ptr));
        dest->n += src->n;

        free(src->as);
        pthread_mutex_destroy(&src->lock);
        pthread_cond_destroy(&src->notify);
        free(src);

        pthread_mutex_unlock(&dest->lock);
        pthread_cond_broadcast(&dest->notify);
        return true;
    } else {
        pthread_mutex_unlock(&dest->lock);
        pthread_mutex_unlock(&src->lock);
        return false;
    }
}

int tharr_foreach(threadarray *arr, void_ptr (*func)(void_ptr)) {
    if (pthread_mutex_lock(&arr->lock) == 0) {
        int i;
        for (i = 0; i < arr->n; i++) {
            arr->as[i] = func(arr->as[i]);
        }

        pthread_mutex_unlock(&arr->lock);
        pthread_cond_broadcast(&arr->notify);
        return no_err;
    } else {
        return get_lock_fail;
    }
}

int tharr_reduce(threadarray *arr, optional (*func)(void_ptr)) {
    if (pthread_mutex_lock(&arr->lock) == 0) {
        int i, c = 0;
        for (i = 0; i < arr->n; i++) {
            optional opt = func(arr->as[i]);
            if (opt.e) {
                arr->as[c] = opt.val;
                c++;
            }
        }

        if (c < arr->n) {
            arr->n = (size_t)c;
            memset(&arr->as[c], 0, (arr->m - arr->n) * sizeof(void_ptr));
        }

        pthread_mutex_unlock(&arr->lock);
        pthread_cond_broadcast(&arr->notify);
        return c;
    } else {
        return get_lock_fail;
    }
}
