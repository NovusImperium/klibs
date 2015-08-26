#include <stdlib.h>
#include "pair.h"

struct pair {
    void *a;
    void *b;
};

pair *pair_init(void *key, void *val) {
    pair *t;
    if ((t = malloc(2 * sizeof(void *))) == null) {
        return null;
    }

    t->a = key;
    t->b = val;

    return t;
}

void *key(pair *t) {
    return t->a;
}

void *val(pair *t) {
    return t->b;
}
