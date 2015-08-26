#ifndef bool
#define bool _Bool
#define true 1
#define false 0
#endif

#ifndef null
#define null ((void*)0)
#endif

#ifndef void_ptr
#define void_ptr void*
#endif

#ifndef OPTIONAL
typedef struct {
    bool e;
    union {
        void_ptr val;
        int err;
    };
} optional;
#endif  // OPTIONAL

#ifndef CONTAINER_TYPES
typedef enum {
    array_t = 0,
    threadarray_t,
    bst_t,
    threadbst_t,
    bsa_t,
    threadbsa_t,
    heap_t,
    threadheap_t,
    list_t,
    threadlist_t,
    map_t,
    threadmap_t,
} container_t;
#endif  // CONTAINER_TYPES

#ifndef CONTAINER
#define CONTAINER

#include <stddef.h>

// create a container object of the given type with an initial size of 's'
// NOTE: the initial size is always required, even if the container being created does not
extern optional create(int type, size_t s);

// creates a copy of the first 's' elements of the container
extern optional copy(void_ptr src, size_t s);

// destroy the container
// NOTE: this DOES NOT free the contents of the elements within the container
extern int free(void_ptr c);

// push element 'a' to the back of container 'c'
extern bool push(void_ptr c, void_ptr a);

// return the element at the head of the container
extern optional peek(void_ptr c);

// remove the element from the head of the container
extern optional pop(void_ptr c);

// concatenate or union the src and dest containers
extern bool concat(void_ptr dest, void_ptr src);

// apply the function to each element in the container
extern int foreach(void_ptr c, void_ptr (*func)(void_ptr));

// similar to foreach, but can optionally remove elements marked by the supplied function
extern int reduce(void_ptr c, optional (*func)(void_ptr));

#endif  // CONTAINER