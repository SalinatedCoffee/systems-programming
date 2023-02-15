/**
 * Vector Lab
 * CS 241 - Spring 2019
 */
 
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    // Anything you want

	char* str;

	size_t length; //strlen(*str)

	size_t size; //sizeof(*str)

};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
	sstring temp;
	temp.length = strlen(input);
	temp.size = temp.length + 1;
	temp.str = malloc(temp.size); //malloc for string
	strcpy(temp.str, input); //copy input to sstring

	sstring *out = malloc(sizeof(char*) + sizeof(size_t)*2);
	memcpy(out, &temp, sizeof(char*) + sizeof(size_t)*2);

    return out;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
	char* out = malloc(input->size);
	strcpy(out, input->str);
    return out;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
	this->length += addition->length;
	this->size = this->length + 1;
	char* out = malloc(this->size);
	strcpy(out, this->str);
	strcat(out, addition->str);
	free(this->str);
	this->str = out;
    return (int) this->length;
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
	vector *vec = string_vector_create();
	/*char *incr = this->str;
	char *temp = this->str;
	char *str = NULL;

	while(*incr) {
		if(*incr == delimiter) {
			str = malloc(incr - temp + 1);
			strncpy(str, temp, incr - temp);
			str[incr-temp] = '\0';
			vector_push_back(vec, str);
			temp = incr + 1;
			free(str);
		}
		incr++;
	}
	if(*(incr-1) != delimiter) {
		str = malloc(incr - temp + 1);
		strcpy(str, temp);
		vector_push_back(vec, str);
		free(str);
	} */

	char* token = strsep(&this->str, &delimiter);

	while(token) {
		vector_push_back(vec, token);
		token = strsep(&this->str, &delimiter);
	}

    return vec;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
	int count = 0;
	int targ_len = strlen(target);
	int sub_len = strlen(substitution);
	int distance = 0;
	char *out = NULL;
	char *here = this->str + offset;
	char *temp = this->str;
	char *temp2 = this->str;

	while(temp) {
		temp = strstr(here, target);
		if(!temp) { break; }
		here = temp + targ_len;
		count++;
	}

	out = malloc(this->length + (sub_len - targ_len)*count + 1);
	temp = out;

	if(!count) { return -1; }

	temp = strncpy(temp, this->str, offset) + offset;
	this->str += offset;

	while(count != 0) {
		here = strstr(this->str, target);
		distance = here - this->str;
		temp = strncpy(temp, this->str, distance) + distance;
		temp = strcpy(temp, substitution) + sub_len;
		this->str += distance + targ_len;
		count--;
	}
	strcpy(temp, this->str);

	free(temp2);
	this->str = out;
	this->length = strlen(this->str);
	this->size = this->length + 1;

    return 0;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
	size_t size = end - start + 1;

	char* out = malloc(size);
	int i = 0;

	while(start < end) {
		out[i] = this->str[start];
		i++;
		start++;
	}

	out[size-1] = '\0';

    return out;
}

void sstring_destroy(sstring *this) {
    // your code goes here
	assert(this);
	free(this->str);
	free(this);
}
