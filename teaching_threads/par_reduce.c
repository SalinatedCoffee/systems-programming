/**
 * Teaching Threads Lab
 * CS 241 - Spring 2019
 */
 
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */
typedef struct params {
	size_t size;
	int base_case;
	int res;
	int *list;
	reducer reduce_func;	
} params;

/* You should create a start routine for your threads. */
void* fold(void *manifest) {
	params *package = (params*) manifest;

	package->res = reduce(package->list, package->size, package->reduce_func, package->base_case);

	return NULL;
}

int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */
	pthread_t tids[num_threads];
	int res = 0;
	params book[num_threads];

	size_t num_items_typ_thread = list_len / num_threads;
	size_t num_items_last_thread = (list_len / num_threads) + (list_len % num_threads);

	size_t i;
	// split list into num_thread segments and use pointer arithmetic to pass in appropriate starting location
	for(i = 0; i < num_threads - 1; i++) {
		book[i].size = num_items_typ_thread;
		book[i].base_case = base_case;
		book[i].list = list+(i*num_items_typ_thread);
		book[i].reduce_func = reduce_func;
		pthread_create(&tids[i], NULL, &fold, book+i);
	}

	// start last thread
	book[i].size = num_items_last_thread;
	book[i].base_case = base_case;
	book[i].list = list+(i*num_items_typ_thread);
	book[i].reduce_func = reduce_func;
	pthread_create(&tids[i], NULL, &fold, book+i);

	// poll for finished threads
	for(i = 0; i < num_threads; i++) {
		pthread_join(tids[i], NULL);
	}

	int retvarr[num_threads];
	for(i = 0; i < num_threads; i++) {
		retvarr[i] = book[i].res;
	}

	res = reduce(retvarr, num_threads, reduce_func, base_case);

	return res;
}
