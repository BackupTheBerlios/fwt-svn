#ifndef arraylistH
#define arraylistH
#include "common.h"

typedef struct arraylist_item {
	char *value;
} ARRAYLIST_ITEM;

typedef struct arraylist {
	ARRAYLIST_ITEM *list;
  	unsigned int size;
  	unsigned int capacity;
} ARRAYLIST;

extern ARRAYLIST *arraylist(unsigned int initial_capacity);
extern void arraylist_add(ARRAYLIST *al, char *obj);
extern unsigned arraylist_size(ARRAYLIST *al);
extern char *arraylist_get(ARRAYLIST *al, unsigned index);
extern void arraylist_set(ARRAYLIST *al, unsigned index, char *obj);
#endif
