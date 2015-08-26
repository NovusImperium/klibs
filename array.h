#ifndef ARRAY
#define ARRAY

#include "defs.h"

typedef struct array array;

// initialize the item array with max length of m
extern array *arr_init(size_t m);

// create and return a new copy of the item array with size 'm', init new item array if 'arr' is null
extern array *arr_copy(array *arr, size_t m);

// free the memory used by the item array
extern void arr_free(array *arr);

// insert a item into the item array, return true if successful
extern bool arr_push(array *arr, void_ptr a);

// concatenate the src item array onto the end of the dest item array
extern bool arr_concat(array *dest, array *src);

// peek at the head of the item array
extern void_ptr arr_peek(array *arr);

// pop the head of the item array
extern void_ptr arr_pop(array *arr);

// apply the function to each item in the array
extern void arr_foreach(array *arr, void_ptr (*func)(void_ptr));

// remove the items marked with false by the function, returns the new size of the array
extern size_t arr_reduce(array *arr, bool (*func)(void_ptr));

#endif // ARRAY