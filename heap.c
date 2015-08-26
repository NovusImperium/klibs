#include <stdlib.h>
#include <string.h>
#include "heap.h"

struct heap {
    const int8_t t;
    size_t m;
    size_t n;
    void_ptr *as;

    bool (*cmp)(void const *, void const *);
};

heap *h_init(size_t m, bool (*cmp)(void const *, void const *)) {
    heap *h = malloc(sizeof(heap));

    *((int8_t *) h) = 3;
    h->m = m;
    h->n = 0;
    h->cmp = cmp;
    h->as = malloc(h->m * sizeof(void_ptr));
    h->as[0] = null;

    return h;
}

heap *h_copy(heap *h, size_t m) {
    heap *new_h;
    if (h == null || (new_h = malloc(sizeof(heap))) == null) {
        return null;
    }

    new_h->m = m;
    new_h->as = malloc(m * sizeof(void_ptr));

    new_h->n = h->n > m ? m : h->n;
    new_h->cmp = h->cmp;
    memcpy(new_h->as, h->as, new_h->n * sizeof(void_ptr));

    return new_h;
}

void h_free(heap *h) {
    free(h->as);
    free(h);
}

bool h_push(heap *h, void_ptr a) {
    h->n++;
    if (h->n == h->m) {
        size_t new_size = h->m << 2;
        void_ptr *new_fs;
        if ((new_fs = malloc(new_size * sizeof(void_ptr))) == null) {
            return false;
        }
        memmove(new_fs, h->as, h->m * sizeof(void_ptr));
        free(h->as);
        h->as = new_fs;
        h->m = new_size;
    }

    h->as[h->n] = a;

    size_t curr = h->n;
    size_t half = h->n >> 1;
    while (half > 0 && h->cmp(h->as[curr], h->as[half])) {
        void_ptr tmp = h->as[half];
        h->as[half] = h->as[curr];
        h->as[curr] = tmp;

        curr = half;
        half = half >> 1;
    }

    return true;
}

void_ptr h_peek(heap *h) {
    return h->n == 0 ? null : h->as[1];
}

void_ptr h_pop(heap *h) {
    if (h->n <= 1) {
        size_t i = h->n;
        h->n = 0;
        return h->as[i];
    }

    void_ptr f = h->as[1];
    h->as[1] = h->as[h->n];
    h->as[h->n] = null;

    size_t curr = 1;
    while (true) {
        size_t l = curr << 1;
        size_t r = l + 1;
        if (r < h->n && h->cmp(h->as[r], h->as[l]) && h->cmp(h->as[r], h->as[curr])) {
            void_ptr t = h->as[curr];
            h->as[curr] = h->as[r];
            h->as[r] = t;
            curr = r;
        } else if (l < h->n && h->cmp(h->as[l], h->as[curr])) {
            void_ptr t = h->as[curr];
            h->as[curr] = h->as[l];
            h->as[l] = t;
            curr = l;
        } else {
            break;
        }
    }

    h->n--;
    return f;
}

bool h_empty(heap *h) {
    return h->n == 0;
}

void h_foreach(heap *h, void_ptr (*func)(void_ptr)) {
    size_t i;
    for (i = 1; i < h->n; i++) {
        h->as[i] = (func)(h->as[i]);
    }
}

size_t h_reduce(heap *h, bool (*func)(void_ptr)) {
    size_t i, c = 1;
    for (i = 1; i <= h->n && (func)(h->as[i]); i++, c++) {
    }

    if (c <= h->n) {
        for (i++; i < h->n; i++) {
            if ((func)(h->as[i])) {
                h->as[c++] = h->as[i];
            }
        }

        h->n = c;
    }

    return h->n;
}
