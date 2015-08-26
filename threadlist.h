#ifndef THREADLIST
#define THREADLIST

#include "defs.h"

typedef struct threadlist threadlist;

// initialize the item threadlist with max length of m
extern threadlist *thlist_init();

// create and return a new copy of the item threadlist with size 'm', init new item threadlist if 'l' is null
extern threadlist *thlist_copy(threadlist *src, size_t m);

// free the memory used by the item threadlist
extern void thlist_free(threadlist *l);

// insert a item into the item threadlist, return true if successful
extern bool thlist_push(threadlist *l, void_ptr a);

// concatenate the src item threadlist onto the end of the dest item threadlist
extern bool thlist_concat(threadlist *dest, threadlist *src);

// peek at the head of the item threadlist
extern void_ptr thlist_peek(threadlist *l);

// pop the head of the item threadlist
extern void_ptr thlist_pop(threadlist *l);

// apply the function to each item in the threadlist
extern void thlist_foreach(threadlist *l, void_ptr (*func)(void_ptr));

// remove the items marked with false by the function, returns the new size of the threadlist
extern int thlist_reduce(threadlist *l, bool (*func)(void_ptr));

#endif // THREADLIST

