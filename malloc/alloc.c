/**
 * Malloc Lab
 * CS 241 - Spring 2019
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define KiB (1024)
#define MiB (1024 * 1024)

// Boundary tags + explicit free block linked list
// Aligned to 16 bytes, but alloc'd blocks will be at least 32 bits in size due to boundary tag overhead

// | Tag |           Payload           | Tag | for occupied blocks
// | Tag | Prv | Nxt |   Empty space   | Tag | for free blocks
// LSB of tag will be 1 if free, 0 if occupied

// Struct for free block linked list node
typedef struct _node {
	size_t size;
	struct _node *prev;
	struct _node *next;
} node;

// Beginning of linked list
static node *head_free = NULL;

// End of linked list
static node *tail_free = NULL;

// Starting address of heap
static void *bottom = NULL;

// Ending address of heap
static void *top = NULL;

void insert(node*);
void delete(node*);
void *split(node*, size_t);
node *coalesce_below(node*);
node *coalesce_above(node*);
void set_tag(void*, size_t);

void *calloc(size_t num, size_t size) {
	if(!num || !size) { return NULL; }
	void *ret = malloc(num*size);
	if(!ret) { return NULL; }
	return memset(ret, 0, num*size);
}

void *malloc(size_t size) {
	if(!size) { return NULL; }

	size_t num_blocks = (2 * sizeof(size_t) + size + 15) / 16;
	size_t aligned_size = num_blocks * 16;
	//fprintf(stderr, " MALLOC: Calculated aligned size of %lu \n", aligned_size);

	if(!bottom) {	// initial malloc call
		void *ptr = sbrk(aligned_size + KiB); // arbitrary initial sbrk request of size+1KiB
		if(ptr == (void*) -1) { return NULL; } // sbrk failure
		bottom = ptr;
		top = (char*) ptr + aligned_size + KiB;
		set_tag(ptr, aligned_size);
		ptr = (char*) ptr + aligned_size;
		node *new = ptr;
		set_tag(ptr, KiB | 1);
		insert(new);
		return (char*) bottom + sizeof(size_t);
	}
	else {
		node *this = head_free;
		while(this) { // check LL for available free block
			if((this->size & ~1) > aligned_size) {
				return (char*) split(this, aligned_size) + sizeof(size_t);
			}
			if((this->size & ~1) == aligned_size) {
				//fprintf(stderr, " MALLOC: Exact fit for size %lu \n", aligned_size);
				delete(this);
				void *ret_alloc = this;
				set_tag((void*)this, aligned_size);
				return (char*) ret_alloc + sizeof(size_t);
			}
			this = this->next;
		}
		//fprintf(stderr, " MALLOC: Calling sbrk for %lu \n", aligned_size + KiB);
		void *new_alloc = sbrk(aligned_size + KiB);
		if(new_alloc == (void*) -1) { return NULL; } // sbrk failure
		top = (char*) new_alloc + aligned_size + KiB;
		set_tag(new_alloc, aligned_size);
		node *new_free = (node*) ((char*) new_alloc + aligned_size);
		set_tag((void*) new_free, KiB | 1);
		new_free->prev = tail_free;
		if(!head_free) { head_free = new_free; }
		else {
			tail_free->next = new_free;
		}
		tail_free = new_free;
		new_free->next = NULL;
		return (char*) new_alloc + sizeof(size_t);
	}
	return NULL;
}

void free(void *ptr) {
	if(!ptr) { return; }
	node *freed = (node*) ((char*) ptr - sizeof(size_t));
	set_tag((void*)freed, freed->size | 1);
	insert(freed);
	node *check = coalesce_below(freed);
	if(check) { freed = check; }
	coalesce_above(freed);
}

void *realloc(void *ptr, size_t size) {
	if(!ptr) { return malloc(size); }
	if(!size) {
		free(ptr);
		return NULL;
	}

	size_t num_blocks = (2 * sizeof(size_t) + size + 15) / 16;
	size_t aligned_size = num_blocks * 16;
	size_t *orig_size = (size_t*) ((char*) ptr - sizeof(size_t));
	size_t payload_size = *orig_size - sizeof(size_t)*2;
	//fprintf(stderr, " REALLOC: Called with size %lu(%lu) on original size %lu(%p, %d) || Payload: %lu \n", size, aligned_size, *orig_size, orig_size, *(int*)ptr, payload_size);
	
	if(*orig_size == aligned_size) { // same size after alignment scaling
		return ptr;
	}
	else if(*orig_size < aligned_size) { //requested size is larger
		//fprintf(stderr, " REALLOC: Larger size requested, ");
		size_t *tag = (size_t*) ((char*) orig_size + *orig_size);
		if(*tag & 1) { //neighboring free block, attempt inline
			//fprintf(stderr, "found free neighboring block(%lu), ", *tag&~1);
			size_t additional_size = aligned_size - *orig_size;
			if(additional_size <= (*tag & ~1)) { // inline available
				//fprintf(stderr, "resizing inline. \n");
				if((*tag & ~1) > 16) { // block in LL, update node
					split((node*)tag, additional_size);
					set_tag((char*) ptr - sizeof(size_t), aligned_size);
				}
				else if((*tag & ~1) == 16) {
					set_tag((char*) ptr - sizeof(size_t), aligned_size);
				}
				return ptr;
			}
			//fprintf(stderr, "inline unavailable, ");
		}
		//fprintf(stderr, "relocating \n");
		void *new_alloc = malloc(size);
		memmove(new_alloc, ptr, payload_size);
		free(ptr);
		return new_alloc;
	}
	else { //requested size is smaller
		//fprintf(stderr, " REALLOC: Smaller size requested, ");
		size_t size_to_free = *orig_size - aligned_size;
		if(size_to_free > 16) { //node-able size, insert new block in LL
			//fprintf(stderr, "freeing block with size %lu \n", size_to_free);
			set_tag((void*) orig_size, aligned_size);
			node *new_free = (node*) ((char*) orig_size + aligned_size);
			set_tag((void*) new_free, size_to_free | 1);
			insert(new_free);
			coalesce_above(new_free);
			return ptr;
		}
		else { //not node-able
			//fprintf(stderr, "freed block is not node-able \n");
			set_tag((void*) orig_size, aligned_size);
			orig_size = (size_t*) ((char*) orig_size + aligned_size);
			set_tag((void*) orig_size, size_to_free | 1);
			return ptr;
		}
	}
	return NULL;
}

void set_tag(void *this, size_t size) {
	size_t actual_size = size & ~1;
	*(size_t*) this = size;
	this = (char*) this + actual_size - sizeof(size_t);
	*(size_t*) this = size;
}

void delete(node *this) {
	if(!this) { return; }
	if(this == head_free && this == tail_free) { // only node in LL
		head_free = NULL;
		tail_free = NULL;
		return;
	}
	else if(this == head_free) { // first node in LL
		head_free = this->next;
		head_free->prev = NULL;
		return;
	}
	else if(this == tail_free) { // last node in LL
		tail_free = this->prev;
		tail_free->next = NULL;
		return;
	}

	node *current = head_free;
	while(current) { // check validity of given node
		if(current == this) { break; }
		current = current->next;
	}

	if(!current) { return; } // given node does not exist in LL

	node *prev = this->prev;
	node *next = this->next;
	prev->next = next;
	next->prev = prev;
	return;
}

void insert(node *this) {
	if(!this) { return; } // got NULL
	if(!head_free) { // no node exists
		head_free = this;
		tail_free = this;
		this->prev = NULL;
		this->next = NULL;
		return;
	}
	node *current = head_free;
	while(current && current < this) { // traverse LL until we get node at higher address than node to be inserted
		if(current == this) { return; }
		current = current->next;
	}

	if(!current) { // node to insert is at higher address than tail of LL
		this->prev = tail_free;
		this->next = NULL;
		tail_free->next = this;
		tail_free = this;
		return;
	}

	this->next = current; // insert node
	this->prev = current->prev;
	if(head_free == current) { // if reached node is head of LL
		head_free = this;
	}
	else {
		current->prev->next = this;
	}
	current->prev = this;
	return;
}

// Takes pointer to free node, splits it into two blocks.
// One alloc'd block with size aligned towards the bottom,
// one free block with size of the remaining bytes (may not be inserted into LL if size < 32)
// Returns address of new alloc'd block
void *split(node *this, size_t size) {
	if((this->size & ~1) == size) {
		delete(this);
		set_tag((void*)this, size);
		return this;
	}
	void *ret = (void*) this;
	node *prev = this->prev;
	node *next = this->next;
	size_t prev_size = this->size & ~1;
	set_tag((void*) this, size);
	this = (node*) ((char*) this + size);
	size_t resized_free = (prev_size - size) | 1; // new size of resized free block
	if((resized_free & ~1) > 16) { // split free block is larger than 16 bytes
		node *new = this; // init new node
		set_tag((void*)this, resized_free);
		new->prev = prev;
		new->next = next;
		if(next && prev) { // prev and next nodes exist
			next->prev = new;
			prev->next = new;
		}
		else if(!prev && !next) { // was only node
			head_free = new;
			tail_free = new;
		}
		else if(!prev) { // was head node
			head_free = new;
			next->prev = new;
		}
		else if(!next) { // was tail node
			tail_free = new;
			prev->next = new;
		}
		return ret;
	}
	else { // split free block is 16 bytes
		*(size_t*) this = prev_size;
		this = (node*) ((char*) this + sizeof(size_t));
		*(size_t*) this = prev_size;
		if(next && prev) { // prev and next nodes exist
			next->prev = prev;
			prev->next = next;
		}
		else if(!prev && !next) { // was only node
			head_free = NULL;
			tail_free = NULL;
		}
		else if(!prev) { // was head node
			head_free = next;
			next->prev = NULL;
		}
		else if(!next) { // was tail node
			tail_free = prev;
			prev->next = NULL;
		}
		return ret;
	}
}

// Takes pointer to free node, coalesces with immediate neighbor below
// Returns address of new free block, NULL otherwise
node *coalesce_below(node *this) {
	if(!this) {	return NULL; }
	if(this == head_free) { return NULL; }

	node *prev = this->prev;
	node *next = this->next;

	size_t orig_size = this->size & ~1;
	void *ptr = (char*) this - sizeof(size_t);
	if(ptr > bottom) {
		size_t prev_block_size = *(size_t*) ptr;
		if((prev_block_size & 1) && ((prev_block_size & ~1) > 16)) {
			node *prev_block = (node*) ((char*) this - (prev_block_size & ~1));
			set_tag((void*)prev_block, orig_size + prev_block_size);
			prev_block->next = next;
			if(prev_block->prev) {
				prev_block->prev->next = prev_block;
			}
			if(prev_block->next) {
				prev_block->next->prev = prev_block;
			}
			else {
				tail_free = this;
			}
			return prev_block;
		}
		else if((prev_block_size & ~1) == 16) {
			node *merged = (node*) ((char*) this - 16);
			set_tag((void*) merged, orig_size + prev_block_size);
			merged->prev = prev;
			merged->prev->next = merged;
			merged->next = next;
			merged->next->prev = merged;
			return merged;
		}
		else { return NULL; }
	}
	else { return NULL; }
}

// Takes pointer to free node, coalesces with immidiate neighbor above
// Returns address of new free block, NULL otherwise
node *coalesce_above(node *this) {
	if(!this) { return NULL; }
	if(this == tail_free) { return NULL; }

	size_t orig_size = this->size & ~1;
	node *next_block = (node*) ((char*) this + orig_size);
	size_t next_block_size = next_block->size;

	if((next_block_size & 1) && ((next_block_size & ~1) > 16)) {
		set_tag((void*) this, orig_size + next_block_size);
		this->next = next_block->next;
		if(next_block == tail_free) {
			tail_free = this;
		}
		else {
			next_block->next->prev = this;
		}
	}
	else if((next_block_size & ~1) == 16) {
		set_tag((void*) this, orig_size + next_block_size);
	}
	else { return NULL; }

	return this;
}
