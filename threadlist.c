#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include "threadlist.h"

typedef struct node node;
struct node {
    void_ptr a;
    node *n;
};

struct threadlist {
    const int8_t t;
    pthread_cond_t notify;
    pthread_mutex_t lock;
    size_t n;
    node *head;
    node *tail;
};

threadlist *thlist_init() {
    threadlist *l;
    if ((l = malloc(sizeof(threadlist))) == null) {
        return null;
    }

    *((int8_t*)l) = 5;

    if (pthread_cond_init(&l->notify, null) != 0 ||
            pthread_mutex_init(&l->lock, null) != 0) {
        free(l);
        return null;
    }

    l->n = 0;
    l->head = l->tail = null;

    return l;
}

threadlist *thlist_copy(threadlist *src, size_t m) {
    threadlist *l;
    if ((l = thlist_init()) == null) {
        return null;
    }

    pthread_mutex_lock(&src->lock);
    node *n = src->head;
    for (n; n != null; n = n->n) {
        thlist_push(l, n->a);
    }
    pthread_mutex_unlock(&src->lock);

    return l;
}

void thlist_free(threadlist *l) {
    pthread_mutex_lock(&l->lock);

    while (l->head != null) {
        node *tmp = l->head;
        l->head = l->head->n;
        free(tmp);
    }

    pthread_cond_destroy(&l->notify);
    pthread_mutex_destroy(&l->lock);

    free(l);
}

bool thlist_push(threadlist *l, void_ptr a) {
    if (pthread_mutex_lock(&l->lock) != 0) {
        return false;
    }
    if (l->n == 0 && (l->tail = l->head = malloc(sizeof(node))) == null) {
        pthread_mutex_unlock(&l->lock);
        return false;
    } else if (l->n > 0) {
        l->tail->n = malloc(sizeof(node));
        l->tail = l->tail->n;
    }

    l->tail->n = null;
    l->tail->a = a;
    l->n++;

    pthread_cond_broadcast(&l->notify);
    pthread_mutex_unlock(&l->lock);
    return true;
}

bool thlist_concat(threadlist *dest, threadlist *src) {
    if (pthread_mutex_lock(&dest->lock) != 0) {
        return false;
    } else if (pthread_mutex_lock(&src->lock) != 0) {
        pthread_mutex_unlock(&dest->lock);
        return false;
    }

    if (dest->n == 0) {
        dest->head = src->head;
        dest->tail = src->tail;
        dest->n = src->n;
    }
    else if (src->n != 0) {
        dest->tail->n = src->head;
        dest->tail = src->tail;
        dest->n += src->n;
    }

    pthread_cond_destroy(&src->notify);
    pthread_mutex_destroy(&src->lock);

    free(src);

    pthread_mutex_unlock(&dest->lock);

    return true;
}

void_ptr thlist_peek(threadlist *l) {
    void_ptr a = null;

    if (pthread_mutex_lock(&l->lock)) {
        a = l->n != 0 ? l->head->a : a;
        pthread_mutex_unlock(&l->lock);
    }

    return a;
}

void_ptr thlist_pop(threadlist *l) {
    void_ptr a = null;

    if (pthread_mutex_lock(&l->lock) == 0) {
        if (l->n != 0) {
            a = l->head->a;
            node *tmp = l->head;
            l->head = l->head->n;
            if (--l->n == 0) {
                l->head = l->tail = null;
            }
            free(tmp);
        }
        pthread_mutex_unlock(&l->lock);
    }

    return a;
}

void thlist_foreach(threadlist *l, void_ptr (*func)(void_ptr)) {
    if (pthread_mutex_lock(&l->lock) == 0) {
        node *n;
        for (n = l->head; n != null; n = n->n) {
            n->a = func(n->a);
        }
        pthread_cond_broadcast(&l->notify);
        pthread_mutex_unlock(&l->lock);
    }
}

int thlist_reduce(threadlist *l, bool (*func)(void_ptr)) {
    int i = -1;
    if (pthread_mutex_lock(&l->lock) == 0) {
        while (l->head != null && !func(l->head)) {
            node *tmp = l->head->n;
            free(l->head);
            l->head = tmp;
            l->n--;
        }
        node *n = l->head == null ? null : l->head->n;
        node *p = l->head;
        while (n != null) {
            if (!func(n->a)) {
                p->n = n->n;
                node *tmp = n;
                n = n->n;
                free(tmp);
                l->n--;
            } else {
                p = n;
                n = n->n;
            }
        }

        i = (int)l->n;
        pthread_mutex_unlock(&l->lock);
    }
    return i;
}
