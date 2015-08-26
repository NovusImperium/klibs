#ifndef BSA
#define BSA

#include "defs.h"
#include <stddef.h>

typedef struct bsa bsa;

// initialize the bsa (binary search array)
extern bsa *bsa_init(int (*cmp)(const void_ptr , const void_ptr));

// insert an item into the bsa
extern bool bsa_push(bsa *b, void_ptr a);

// remove and return the least item out of the tree
extern void_ptr bsa_pop(bsa *b);

// return the least item in the tree
extern void_ptr bsa_peek(bsa *b);

// returns whether the tree has the item
extern bool bsa_has(bsa *b, void_ptr a);

// returns whether the tree is empty
extern bool bsa_empty(bsa *b);

// applies the function to each of the elements in the bsa
extern void bsa_foreach(bsa *b, void_ptr (*func)(void_ptr));

// remove the items marked as false by the function, returns the new size of the bsa
extern size_t bsa_reduce(bsa *b, bool (*func)(void_ptr));

#endif
