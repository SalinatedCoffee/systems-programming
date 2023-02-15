/**
 * Password Cracker Lab
 * CS 241 - Spring 2019
 */
 
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "queue.h"
#include <crypt.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct worker_thread_md {
	pthread_t real_tid;
	size_t my_tid;
} wthread;

typedef struct job_manifest {
	char *username;
	char *hash;
	char *problem;
	int known;
	int unknowns;
} manifest;

void *worker(void*);
void parse_into_job(char*);
void destroy_job(manifest*);
void replace_periods(char*);

queue *tasks = NULL;

int start(size_t thread_count) {
	tasks = queue_create(-1);
	char *line = NULL;
	size_t len = 0;
	while(getline(&line, &len, stdin) != -1) {
		parse_into_job(line);
		free(line);
		line = NULL;
	}

	free(line);
	line = NULL;

	for (size_t i = 0; i < thread_count; i++) {
		queue_push(tasks, NULL);
	}

	pthread_t tids[thread_count];

	for(size_t i = 0; i < thread_count; i++) {
		pthread_create(tids+i, NULL, &worker, ((void*) i));
	}
	for(size_t i = 0; i < thread_count; i++) {
		pthread_join(tids[i], NULL);
	}

	queue_destroy(tasks);

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}

void *worker(void *val) {
	struct crypt_data cdata;
	cdata.initialized = 0;
	char *hashed;
	double start_time;
	int max_iters;
	int job_iters;
	manifest *obj;

	wthread myid;
	myid.real_tid = pthread_self();
	myid.my_tid = (size_t) val + 1;

	while(1) {
		start_time = getThreadCPUTime();
		obj = queue_pull(tasks);
		if(!obj) {
			return NULL;
		} // nothing left in queue
		// prep job
		max_iters = (int) pow(26, obj->unknowns);
		replace_periods(obj->problem);
		v1_print_thread_start(myid.my_tid, obj->username);
		for(job_iters = 0; job_iters < max_iters; job_iters++) {
			hashed = crypt_r(obj->problem, "xx", &cdata);
			if(!strcmp(hashed, obj->hash)) {
				v1_print_thread_result((int) myid.my_tid, obj->username, obj->problem,
									   job_iters + 1, getThreadCPUTime() - start_time, 0);
				destroy_job(obj);
				obj = NULL;
				break;
			}
			incrementString(obj->problem);
		}
		if(job_iters == max_iters) {
			v1_print_thread_result((int)myid.my_tid, obj->username, obj->problem,
								   job_iters, getThreadCPUTime() - start_time, 1);
			destroy_job(obj);
			obj = NULL;
		}
	}
}

void parse_into_job(char *line) {
	manifest *new_job = malloc(sizeof(manifest));
	new_job->username = strdup(strsep(&line, " "));
	new_job->hash = strdup(strsep(&line, " "));
	char *temp = strsep(&line, " ");
	temp[strlen(temp) - 1] = '\0'; // last token requires special treatment
	new_job->problem = strdup(temp);
	new_job->known = getPrefixLength(new_job->problem);
	new_job->unknowns = strlen(new_job->problem) - new_job->known;
	queue_push(tasks, new_job);
}

void destroy_job(manifest *shred) {
	free(shred->username);
	free(shred->hash);
	free(shred->problem);
	free(shred);
}

void replace_periods(char *target) {
	int len = strlen(target);
	for(int i = 0; i < len; i++) {
		if(target[i] == '.') { target[i] = 'a'; }
	}
}
