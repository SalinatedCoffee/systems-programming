/**
 * Vector Lab
 * CS 241 - Spring 2019
 */
 
#include "vector.h"
#include <stdio.h>

void print_items_int(vector*);
void vec_caps(vector*);

int main(int argc, char *argv[]) {
    // Write your test cases here

	vector *vect = int_vector_create();
	vec_caps(vect);
	vector_resize(vect, 12);
	vec_caps(vect);

	int i;

	printf("Setting items equal to their position \n");

	for(i = 0; i < (int)vector_size(vect); i++) {
		vector_set(vect, i, &i);
	}

	printf("Checking if items were properly set \n");

	print_items_int(vect);

	printf("Resizing to 9\n");
	vector_resize(vect, 9);
	vec_caps(vect);
	print_items_int(vect);

	i = 42;
	printf("Checking push_backs\n");
	vector_push_back(vect, &i);
	vector_push_back(vect, &i);
	vector_push_back(vect, &i);
	vec_caps(vect);
	
	printf("Item pushed is %d \n", *((int*)vector_get(vect, vector_size(vect)-1)));

	printf("Checking pop_backs\n");
	vector_pop_back(vect);
	vector_pop_back(vect);
	vector_pop_back(vect);
	vector_pop_back(vect);
	vec_caps(vect);
	print_items_int(vect);

	i = 123;
	printf("Checking insert at position 5 \n");
	vector_insert(vect, 5, &i);

	print_items_int(vect);
	vec_caps(vect);

	printf("Checking erase at position 0 twice\n");
	vector_erase(vect, 0);
	vec_caps(vect);
	print_items_int(vect);
	vector_erase(vect, 0);
	vec_caps(vect);
	print_items_int(vect);

	void **ott = vector_at(vect, 3);
	int *tt = (int*) *ott;
	printf("%d \n", *tt);

	ott = vector_back(vect);
	tt = (int*) *ott;
	printf("%d \n", *tt);
	
	printf("Checking vector_clear\n");
	vector_clear(vect);
	vec_caps(vect);

	printf("Inserting 500 elements\n");
	int k;
	int *q = malloc(sizeof(int));
	*q = 321;
	for(k = 0; k < 200; k++) {
		vector_push_back(vect, q);
	}
	
	vector_resize(vect, 512);
	vec_caps(vect);
	printf("Destroying vector \n");
	vector_destroy(vect);


    return 0;
}

void vec_caps(vector* vec) {
	printf("Vector has size %lu and capacity %lu.\n", vector_size(vec), vector_capacity(vec));
}

void print_items_int(vector* vec) {
	int i;

	for(i = 0; i < (int) vector_size(vec); i++) {
		printf("Item at pos %d, has value %d \n", i, *((int*)vector_get(vec, i)));
	}
}
