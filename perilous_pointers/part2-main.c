/**
 * Perilous Pointers Lab
 * CS 241 - Spring 2019
 */
 
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // your code here
    first_step(81);

    int *ptr = NULL;
    int n = 132;
    ptr = &n;
    second_step(ptr);

    n = 8942;
    int **ptr2 = NULL;
    ptr2 = &ptr;
    double_step(ptr2);

    char* str = malloc(20);
	int i = 0;
	while(i<10) {
		str[i] = 0;
		i++;
	}
	str[5] = (char) 15;
    strange_step(str);
    free(str);

    str = "   \0";
    empty_step(str);

    char* str2 = "uwuuuuuuuuu";
    void* ptr3 = str2;
    two_step(ptr3, str2);

    char* first = "";
    char* second = first + 2;
    char* third = second + 2;
    three_step(first, second, third);

    char* one = " a";
    char* two = "  i";
    char* tre = "   q";
    step_step_step(one, two, tre);

    it_may_be_odd(one, 32);

    char* pstr = malloc(strlen("CS233,CS241")+1);
    strcpy(pstr,"CS233,CS241");
    tok_step(pstr);
    free(pstr);

    char* nstr1 = malloc(5);
    nstr1[0] = 1;
    nstr1[1] = 0;
    nstr1[2] = 0;
    nstr1[3] = 2;
    char* nstr2 = nstr1;
    the_end(nstr1, nstr2);
    free(nstr1);

    return 0;
}
