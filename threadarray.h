#ifndef THREADARRAY
#define THREADARRAY

#include "defs.h"
#include "optional.h"

typedef struct threadarray threadarray;

typedef enum {
    no_err = 0,
    malloc_fail = -1,
    init_lock_fail = -2,
    get_lock_fail = -3,
    container_empty = -4,
} ta_err_t;

// initialize the item array with max length of m, optionally returns the new array or an error code
extern optional tharr_init(size_t m);

// create and return a new copy of the item array with size 'm', init new item array if 'arr' is null
extern optional tharr_copy(threadarray *arr, size_t m);

// free the memory used by the item array
extern int tharr_free(threadarray *arr);

// insert a item into the item array, return true if successful
extern bool tharr_push(threadarray *arr, void_ptr a);

// concatenate the src item array onto the end of the dest item array
// NOTE: src is consumed in this operation
extern bool tharr_concat(threadarray *dest, threadarray *src);

// peek at the head of the item array
extern optional tharr_peek(threadarray *arr);

// pop the head of the item array
extern optional tharr_pop(threadarray *arr);

// apply the function to each item in the array
extern int tharr_foreach(threadarray *arr, void_ptr (*func)(void_ptr));

// remove the items marked with false by the function, returns the new size of the array
extern int tharr_reduce(threadarray *arr, optional (*func)(void_ptr));

#endif // THREADARRAY
