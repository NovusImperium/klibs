#ifndef OPTIONAL
#include "defs.h"

typedef struct {
    bool e;
    union {
        void_ptr val;
        int err;
    };
} optional;
#endif  // OPTIONAL