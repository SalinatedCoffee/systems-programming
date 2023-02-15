/**
 * Extreme Edge Cases Lab
 * CS 241 - Spring 2019
 */

 
#include "camelCaser.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

int count_chars(const char*);
const char *next_punct(const char*);
char* construct_sstring(const char*);
int count_puncts(const char*);
void decap(char**);

char **camel_caser(const char *input_str) {

    if(!input_str) { return NULL; }

    int num_sstrings = count_puncts(input_str);


    char **cc_set = malloc((num_sstrings + 1) * sizeof(char*));
    #ifdef DEBUG
    printf("camel_caser: Allocated space for %d pointers at %p.\n", num_sstrings+1, cc_set);
    #endif
    char **output = cc_set;

    while(num_sstrings) {
       *cc_set = construct_sstring(input_str);
       cc_set++;
       num_sstrings--;
       input_str = next_punct(input_str);
    }
    *cc_set = NULL;
    decap(output);
    return output;
}

void decap(char **output) {
    while(*output) {
        **output = tolower(**output);
        output++;
    }
    return;
}

// returns number of characters in sentence
int count_chars(const char *input_str) {

    int count = 0;

    while(!ispunct(*input_str)) {
        if(isalnum(*input_str)) { count++; }
        input_str++;
    }
    return count;
}


const char *next_punct(const char *input_str) {
    #ifdef DEBUG
    printf("next_punct: Called.\n");
    #endif
    while(*input_str) {
        if(ispunct(*input_str)) {
            if(!*(++input_str)) {
                #ifdef DEBUG
                printf("next_punct: Found null character, exited with return value of NULL.\n");
                #endif
                return NULL;
            }
            #ifdef DEBUG
            printf("next_punct: Return value is %c, exiting.\n",*input_str);
            #endif
            return input_str;
        }
        input_str++;
    }
    #ifdef DEBUG
    printf("next_punct: Exited with return value of NULL.\n");
    #endif
    return NULL;
}


char *construct_sstring(const char *input_str) {
    #ifdef DEBUG
    printf("construct_sstring: Called.\n");
    #endif

    int size = count_chars(input_str);
    int index = 0;
    int first_ascii = 1;

    char *output = malloc(size + 1);
    #ifdef DEBUG
    printf("construct_sstring: Allocated space for %d characters at %p.\n",size+1,output);
    #endif
    while(!ispunct(*input_str)) {
        if(isalpha(*input_str) && first_ascii) {
            #ifdef DEBUG
            printf("construct_sstring: Found first letter \'%c\'.\n", *input_str);
            #endif
			strncpy((output+index), input_str, 1);
			output[index] = toupper(output[index]);
            //output[index] = toupper(*input_str);
            first_ascii = 0;
            index++;
        }
        else if(isalpha(*input_str)) {
			strncpy((output+index), input_str, 1);
            output[index] = tolower(output[index]);
            #ifdef DEBUG
            printf("construct_sstring: Found letter \'%c\'.\n", *input_str);
            #endif
            index++;
        }
        else if(!isspace(*input_str) && (isdigit(*input_str) || iscntrl(*input_str))) {
            output[index] = *input_str;
            #ifdef DEBUG
            printf("construct_sstring: Found digit \'%c\'.\n", *input_str);
            #endif
			first_ascii = 0;
            index++;
        }
        else {
			//strncpy((output+index), input_str, 1);
			first_ascii = 1;
			//index++;
		}
        input_str++;
    }
    output[index] = 0;
    return output;
    #ifdef DEBUG
    printf("construct_sstring: Proccessed string into %s, exiting.\n",output);
    #endif
}


int count_puncts(const char *input_str) {

    int count = 0;

    while(*input_str) {
        if(ispunct(*input_str)) {
            count++;
        }
        input_str++;
    }
    return count;
}


void destroy(char **result) {
    char **items = result;
    while(*items) {
        free(*items);
        items++;
    }
    free(result);
    return;
}
