/**
 * Critical Concurrency Lab
 * CS 241 - Spring 2019
 */
 
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node;

struct queue {
    /* queue_node pointers to the head and tail of the queue */
    queue_node *head, *tail;

    /* The number of elements in the queue */
    ssize_t size;

    /**
     * The maximum number of elements the queue can hold.
     * max_size is non-positive if the queue does not have a max size.
     */
    ssize_t max_size;

    /* Mutex and Condition Variable for thread-safety */
    pthread_cond_t cv;
    pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
    /* Your code here */
	queue *new = malloc(sizeof(queue));
	new->max_size = max_size;
	new->head = NULL;
	new->tail = NULL;
	new->size = 0;
	pthread_mutex_init(&new->m, NULL);
	pthread_cond_init(&new->cv, NULL);
    return new;
}

void queue_destroy(queue *this) {
    /* Your code here */
	queue_node *now = this->head;
	queue_node *temp;
	while(now) {
		temp = now->next;
		free(now);
		now = temp;
	}

	pthread_mutex_destroy(&this->m);
	pthread_cond_destroy(&this->cv);
	free(this);
}

void queue_push(queue *this, void *data) {
    /* Your code here */
	pthread_mutex_lock(&this->m);
	while((this->max_size > 0) && (this->size == this->max_size)) { //block only if max_size is positive
		pthread_cond_wait(&this->cv, &this->m);
	}
	this->size++;
	queue_node *new = malloc(sizeof(queue_node));
	new->data = data;
	new->next = NULL;
	if(!this->head && !this->tail) { // no node in queue
		this->head = new;
		this->tail = new;
	}
	else {
		this->tail->next = new;
		this->tail = new;
	}
	if(this->max_size > 0) { pthread_cond_broadcast(&this->cv); } //broadcast only if max_size is positive
	pthread_mutex_unlock(&this->m);
}

void *queue_pull(queue *this) {
    /* Your code here */
	pthread_mutex_lock(&this->m);
	while(this->size == 0) {
		pthread_cond_wait(&this->cv, &this->m);
	}
	this->size--;
	queue_node *temp = this->head;
	this->head = temp->next;
	void *ret = temp->data;
	free(temp);
	if(!this->head) { // pulled last node in queue
		this->tail = NULL;
	}
	pthread_cond_broadcast(&this->cv);
	pthread_mutex_unlock(&this->m);

    return ret;
}
