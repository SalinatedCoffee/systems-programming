/**
 * Deadlock Demolition Lab
 * CS 241 - Spring 2019
 */
 
#include "libdrm.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void *try_lock(void*);

drm_t *drm1;
drm_t *drm2;

int main() {
	pthread_t tids[2];
	void *ret[2];	
	drm1 = drm_init();
	drm2 = drm_init();

	pthread_create(tids, NULL, &try_lock, NULL);
	sleep(1);
	pthread_create(tids+1, NULL, &try_lock, tids);


    pthread_join(tids[0], ret);
	pthread_join(tids[1], ret+1);
	drm_destroy(drm1);
	drm_destroy(drm2);
}

void *try_lock(void *param) {
	pthread_t mytid = pthread_self();
	if(!param) {
		int test = drm_wait(drm1, &mytid);
		printf("Locked drm1, 1st thread waiting...\n");
		sleep(2);
		printf("Thread 1 attempting to lock drm2 \n");
		test = drm_wait(drm2, &mytid);
		if(test == 0) {
			printf("Thread 1 failed to lock drm2\n");
			exit(0);
		}
		else { return NULL; }
	}
	else {
		int test = drm_wait(drm2, &mytid);
		printf("Locked drm2, 2nd thread waiting...\n");
		sleep(2);
		printf("Thread 2 attempting to lock drm2 \n");
		test = drm_wait(drm1, &mytid);
		if(test == 0) {
			printf("Thread 2 failed to lock drm1\n");
			exit(0);
		}
		else { return NULL; }
	}
	return NULL;
}
