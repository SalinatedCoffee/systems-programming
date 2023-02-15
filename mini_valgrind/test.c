/**
 * Mini Valgrind Lab
 * CS 241 - Spring 2019
 */
 
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Your tests here using malloc and free

	write(1, "watermark5\n", 11);

	void *test = realloc(NULL, 7);
	free(test);
/*
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
	void *test = malloc(1);
*/
    return 0;
}
