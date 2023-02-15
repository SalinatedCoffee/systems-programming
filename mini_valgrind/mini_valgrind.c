/**
 * Mini Valgrind Lab
 * CS 241 - Spring 2019
 */
 
#include "mini_valgrind.h"
#include <stdio.h>
#include <string.h>

#define md_size (sizeof(meta_data))

meta_data *head = NULL;
size_t total_memory_requested = 0;
size_t total_memory_freed = 0;
size_t invalid_addresses = 0;

// size_t request_size
// const char *filename
// void *instruction
// struct _meta_data *next

void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
	meta_data *this = malloc(md_size + request_size); //metadata size + requested size

	if(!this) { return NULL; } //malloc failed, return NULL

	this->request_size = request_size; //malloc succeeded, insert metadata at start of LL
	this->filename = filename;
	this->instruction = instruction;
	this->next = head;
	head = this;
	total_memory_requested += this->request_size;

    return this + md_size; //return start of empty memory
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
	meta_data *this = calloc(1, (num_elements * element_size) + md_size); //metadata size + elem. * num. size

	if(!this) { return NULL; } //calloc failed, return NULL

	this->request_size = num_elements * element_size; //calloc succeeded, insert metadata at start of LL
	this->filename = filename;
	this->instruction = instruction;
	this->next = head;
	head = this;
	total_memory_requested += (num_elements * element_size);

    return this + md_size; //return start of empty memory
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {

	if(!payload) { //got NULL pointer, malloc
		meta_data *this = mini_malloc(request_size, filename, instruction);
		return this;
	}
	else if(!request_size) { //got size of 0, free
		mini_free(payload);
		return NULL;
	}
	else { // check for valid address match in ll
		meta_data *prev = NULL;
		meta_data *this = head;

		while(this) {
			if(this + md_size == payload) { //hit
				if(this->request_size == request_size) { return this + md_size; }

				this = realloc(this, md_size + request_size);

				if(!this) { //realloc failed, don't touch anything and return NULL
					return NULL;
				}
				else {
					if(this->request_size > request_size) { //requested less
						total_memory_freed += (this->request_size - request_size);
					}
					else if(this->request_size < request_size) { //requested more
						total_memory_requested += (request_size - this->request_size);
					}
					this->request_size = request_size;
					this->filename = filename;
					this->instruction = instruction;

					if(prev) { prev->next = this; }
					else { head = this; }

					return this + md_size;
				}
			}
			else { //move to next node
				prev = this;
 				this = this->next;
	 		}
		}
		invalid_addresses++;
    	return NULL;
	}
}

void mini_free(void *payload) {
    
	meta_data *prev = NULL;  //previous node
	meta_data *this = head;  //current node

	if(!payload) { //got null pointer, do nothing
		return;
	}

	while(this) {
		if(this + md_size == payload) { //hit
			total_memory_freed += this->request_size;
			if(prev) { prev->next = this->next; }
			else { head = this->next; }
			free(this);
			return;
		}
		else { //no hit, move to next node
			prev = this;
			this = this->next;
		}
	}
	invalid_addresses++; //no hit after full traversal, got invalid address
	return;
}
