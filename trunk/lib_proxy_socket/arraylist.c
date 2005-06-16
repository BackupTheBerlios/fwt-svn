#include "arraylist.h"

ARRAYLIST *arraylist(unsigned int initial_capacity){
	ARRAYLIST *res;
	if(initial_capacity == 0){
    	initial_capacity = 10;
    }
	res = (ARRAYLIST *)malloc(sizeof(ARRAYLIST));
    res->size = 0;
    res->capacity = initial_capacity;
	res->list = (ARRAYLIST_ITEM *)malloc(sizeof(ARRAYLIST_ITEM) * res->capacity);
	memset(res->list, 0, sizeof(ARRAYLIST_ITEM) * res->capacity);
  	return res;
}

void arraylist_add(ARRAYLIST *al, char *obj){
	if(al->size == al->capacity){
		al->list = (ARRAYLIST_ITEM *)realloc(al->list, al->capacity * sizeof(ARRAYLIST_ITEM) * 2);
  		memset(al->list, al->capacity * sizeof(ARRAYLIST_ITEM), al->capacity * sizeof(ARRAYLIST_ITEM));
        al->capacity *= 2;
    }
  	al->list[al->size].value = obj;
  	al->size++;
}

unsigned arraylist_size(ARRAYLIST *al){
	return al->size;
}

char *arraylist_get(ARRAYLIST *al, unsigned index){
	return al->list[index].value;
}

void arraylist_set(ARRAYLIST *al, unsigned index, char *obj){
	al->list[index].value = obj;
}

