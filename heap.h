#ifndef HEAP
#define HEAP

#include "defs.h"

typedef struct heap heap;

// initialize the heap with the given comparison function
extern heap *h_init(size_t m, bool (*cmp)(void const *, void const *));

// copy the old heap into a new heap of size 's', init a new heap if 'h' is null
extern heap *h_copy(heap *h, size_t m);

// insert a fraction into the heap, return true if successful
extern bool h_push(heap *h, void_ptr a);

// peek the top of the heap
extern void_ptr h_peek(heap *h);

// pop the top of the heap into the given pointer and remove it from the heap
extern void_ptr h_pop(heap *h);

// free the memory used directly by the heap
extern void h_free(heap *h);

// returns whether the tree is empty
extern bool h_empty(heap *h);

// applies the function to each of the elements in the heap
extern void h_foreach(heap *h, void_ptr (*func)(void_ptr));

// remove the items marked as false by the function, returns the new size of the heap
extern size_t h_reduce(heap *h, bool (*func)(void_ptr));

#endif // HEAP
