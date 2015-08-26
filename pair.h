#include "defs.h"

typedef struct pair pair;

// initialize the pair struct with values 'a' and 'b'
extern pair *pair_init(void_ptr key, void_ptr val);

// return the key from the pair
extern void_ptr key(pair *t);

// return the value from the pair
extern void_ptr val(pair *t);