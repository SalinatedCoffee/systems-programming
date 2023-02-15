/**
 * Extreme Edge Cases Lab
 * CS 241 - Spring 2019
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

#define psize sizeof(char*)

/*
 * Testing function for various implementations of camelCaser.
 *
 * @param  camelCaser   A pointer to the target camelCaser function.
 * @param  destroy      A pointer to the function that destroys camelCaser
 * output.
 * @return              Correctness of the program (0 for wrong, 1 for correct).
 */
int validate(char*,char**);
void cc_print(char**);

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    int failed = 0;

	printf("1. Testing standard string... ");
    char* check_inp = "This is a standard test with, two sentences, no three.\n";
    char** check_res = malloc(psize*4);
    *check_res = "thisIsAStandardTestWith";
    *(check_res+1) = "twoSentences";
    *(check_res+2) = "noThree";
    *(check_res+3) = NULL;
    failed += validate(check_inp, check_res);
    free(check_res);

	printf("2. Testing missing punctuation...");
    check_inp = "This is another test. With the inclusion of. A hanging sentence";
    check_res = malloc(psize*3);
    *check_res = "thisIsAnotherTest";
    *(check_res+1) = "withTheInclusionOf";
    *(check_res+2) = NULL;
    failed += validate(check_inp, check_res);
    free(check_res);

	printf("3. Testing consecutive punctuation...");
    check_inp = "...";
    check_res = malloc(psize*4);
    *check_res = "";
    *(check_res+1) = "";
    *(check_res+2) = "";
    *(check_res+3) = NULL;
    failed += validate(check_inp, check_res);
    free(check_res);

	printf("4. Testing whitespace...");
	check_inp = ".                                .";
	check_res = malloc(psize*3);
	*check_res = "";
	*(check_res+1) = "";
	*(check_res+2) = NULL;
	failed += validate(check_inp, check_res);
	free(check_res);

	printf("5. Testing 256 byte string...");
	check_inp = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
	check_res = malloc(psize);
	*check_res = NULL;
	failed += validate(check_inp, check_res);
	free(check_res);

	printf("6. Testing null pointer...");
	failed += validate(NULL, NULL);

	printf("7. Testing octal character...");
	check_inp = "\1test.";
	check_res = malloc(psize*2);
	*check_res = "\1test";
	*(check_res+1) = NULL;
	failed += validate(check_inp, check_res);
	free(check_res);

	printf("8. Testing number concatenated string (prefixed)...");
	check_inp = "123test.";
	check_res = malloc(psize*2);
	*check_res = "123test";
	*(check_res+1) = NULL;
	failed += validate(check_inp, check_res);
	free(check_res);

	printf("9. Testing number concatenated string (suffixed)...");
	check_inp = "test321.";
	check_res = malloc(psize*2);
	*check_res = "test321";
	*(check_res+1) = NULL;
	failed += validate(check_inp, check_res);
	free(check_res);

	printf("10. Testing number concatenated string (embedded)...");
	check_inp = "123test321.";
	check_res = malloc(psize*2);
	*check_res = "123test321";
	*(check_res+1) = NULL;
	failed += validate(check_inp, check_res);
	free(check_res);

	printf("11. Testing control characters...");
	check_inp = "\n.\n.\n";
	check_res = malloc(psize*3);
	*check_res = "";
	*(check_res+1) = "";
	*(check_res+2) = NULL;
	failed += validate(check_inp, check_res);
	free(check_res);

	printf("12. Testing empty string...");
	check_res = malloc(psize);
	*check_res = NULL;
	failed += validate("", check_res);
	free(check_res);

	printf("13. Testing digit interleaved string...");
	check_inp = "0i1n2t3e4r5l6e7a8v9e.";
	check_res = malloc(psize*2);
	*check_res = "0i1n2t3e4r5l6e7a8v9e";
	*(check_res+1) = NULL;
	failed += validate(check_inp, check_res);
	free(check_res);

    if(failed) { return 0;}
    return 1;
}

void cc_print(char **array) {
    printf("Printing\n");
    int elem = 1;

	if(!array) {
		printf("Received NULL. Exiting. \n");
		return;
	}

    while(*array) {
        printf("Element %d in array: %s\n",elem,*array);
        elem++;
        array++;
    }
    printf("Done printing\n");
    return;
}


int validate(char* test_str, char** test_res) {
    #ifdef DEBUG
    printf("validate: called.\n");
    #endif
    char** result = camel_caser(test_str);
    char** bookmark = result;
    #ifdef DEBUG
    cc_print(result);
    #endif

	if(result == NULL && test_res == NULL) {
		printf("Passed.\n");
		return 0;
	}

    while(*result && *test_res) {
        #ifdef DEBUG
        printf("Expected %s, got %s.\n", *test_res, *result);
        #endif
        if(strcmp(*result, *test_res) != 0) {
            printf("Failed.\n");;
            return 1;
        }
        result++;
        test_res++;
    }
    if((*result == NULL) && (*test_res == NULL)) { printf("Passed.\n"); }
	else {
		printf("Failed.\n");
		return 1;
	}

    destroy(bookmark);
    #ifdef DEBUG
    printf("validate: called destroy, exiting.\n");
    #endif
        
    return 0;
}
