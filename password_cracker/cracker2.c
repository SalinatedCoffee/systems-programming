/**
 * Password Cracker Lab
 * CS 241 - Spring 2019
 */
 
#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include <pthread.h>
#include "queue.h"
#include <stdio.h>
#include <crypt.h>
#include <string.h>

#define print_lock pthread_mutex_lock(&print);
#define print_unlock pthread_mutex_unlock(&print);

typedef struct job_manifest {
	char *username;
	char *hash;
	char *problem;
	int known;
	int unknowns;
	int status;
} manifest;

typedef struct individual_job_manifest {
	char *username;
	char *hash;
	char *problem;
	long start;
	long iters;
	int finaliter;
	int itid;
	int status;
} manifest_t;

void *worker(void*);
void parse_into_job(char*);
void destroy_job(manifest*);
void destroy_job_t(manifest_t*);

int flag = 0; // raised when password found
int begin = 0; // raised when thread jobs are ready
int clean = 0;
size_t done = 0; // incremented when thread has reached end of iterations
size_t pool = 0; // initialized to thread count
char *answer = NULL; // points to cracked password in heap
queue *tasks = NULL;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t t_main = PTHREAD_COND_INITIALIZER;
pthread_cond_t t_work = PTHREAD_COND_INITIALIZER;
pthread_barrier_t sync_all;
pthread_barrier_t sync_threads;

int start(size_t thread_count) {
	pthread_barrier_init(&sync_all, NULL, thread_count + 1);
	pthread_barrier_init(&sync_threads, NULL, thread_count);
	manifest *work = NULL;
	pool = thread_count;
	tasks = queue_create(-1);
	manifest_t *jobs_t = malloc(thread_count * sizeof(manifest_t));

	char *line = NULL;
	size_t len = 0;
	int status = 0;

	while(getline(&line, &len, stdin) != -1) {
		parse_into_job(line);
		free(line);
		line = NULL;
	}
	free(line);
	line = NULL;
	queue_push(tasks, NULL);
	pthread_t tids[thread_count];
	for(size_t i = 0; i < thread_count; i++) {
		pthread_create(tids+i, NULL, &worker, ((void*) (jobs_t+i)));
	}

	long range_floor;
	long tries;
	double totalbusytime = 0;
	double wallclocktime = 0;
	int totalhashes = 0;

	while(1) { // main thread server loop
		work = queue_pull(tasks);
		if(!work) {
			pthread_mutex_lock(&mtx);
			clean = 1;
			begin = 1;
			pthread_mutex_unlock(&mtx);
			pthread_cond_broadcast(&t_work);
			break;
		}
		status = 1;
		wallclocktime = getTime();
		totalbusytime = getCPUTime();
		print_lock
		v2_print_start_user(work->username);
		print_unlock
		for(size_t i = 0; i < thread_count; i++) {
			getSubrange(work->unknowns, thread_count, i + 1, &range_floor, &tries);
			jobs_t[i].username = strdup(work->username);
			jobs_t[i].hash = strdup(work->hash);
			jobs_t[i].problem = strdup(work->problem);
			setStringPosition(&jobs_t[i].problem[work->known], range_floor);
			jobs_t[i].itid = i + 1;
			jobs_t[i].start = range_floor;
			jobs_t[i].iters = tries;
			jobs_t[i].status = 0;
		}

		pthread_mutex_lock(&mtx);
		begin = 1;
		pthread_mutex_unlock(&mtx);
		pthread_cond_broadcast(&t_work);

		pthread_mutex_lock(&mtx);
		while(!flag) {
			pthread_cond_wait(&t_main, &mtx);
		}
		pthread_mutex_unlock(&mtx);

		pthread_barrier_wait(&sync_all);

		for(size_t i = 0; i < thread_count; i++) {
			totalhashes += jobs_t[i].finaliter;
			if(jobs_t[i].status == 1) {
				status = 0;
			}
		}

		wallclocktime = getTime() - wallclocktime;
		totalbusytime = getCPUTime() - totalbusytime;
		print_lock
		v2_print_summary(work->username, answer, totalhashes, wallclocktime, totalbusytime, status);
		print_unlock

		totalhashes = 0;

		pthread_mutex_lock(&mtx);
		flag = 0;
		begin = 0;
		done = 0;
		pthread_mutex_unlock(&mtx);

		free(answer);
		answer = NULL;

		destroy_job(work);
	}

	for(size_t i = 0; i < thread_count; i++) {
		pthread_join(tids[i], NULL);
	}

	pthread_mutex_destroy(&mtx);
	pthread_mutex_destroy(&mtx2);
	pthread_mutex_destroy(&print);
	pthread_cond_destroy(&t_main);
	pthread_cond_destroy(&t_work);
	pthread_barrier_destroy(&sync_all);
	pthread_barrier_destroy(&sync_threads);
	queue_destroy(tasks);
	free(jobs_t);

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}

void *worker(void *val) {
	struct crypt_data cdata;
	cdata.initialized = 0;
	char *hashed;
	int i = 0;
	int max_iters = 0;
	manifest_t *mystats = val;

	while(1) {
		pthread_mutex_lock(&mtx);
		while(!begin) {
			pthread_cond_wait(&t_work, &mtx);
		}
		pthread_mutex_unlock(&mtx);

		pthread_barrier_wait(&sync_threads);
		pthread_mutex_lock(&mtx);
		if(clean) {
			pthread_mutex_unlock(&mtx);
			return NULL;
		}
		pthread_mutex_unlock(&mtx);

		print_lock
		v2_print_thread_start(mystats->itid, mystats->username, mystats->start, mystats->problem);
		print_unlock

		max_iters = mystats->iters;
		for(i = 1; i <= max_iters; i++) {
			pthread_mutex_lock(&mtx);
			if(flag) {
				pthread_mutex_unlock(&mtx);
				mystats->finaliter = i;
				print_lock
				v2_print_thread_result(mystats->itid, mystats->finaliter, 1);
				print_unlock
				destroy_job_t(mystats);
				pthread_barrier_wait(&sync_all);
				break;
			}
			pthread_mutex_unlock(&mtx);
			hashed = crypt_r(mystats->problem, "xx", &cdata);
			if(!strcmp(hashed, mystats->hash)) {
				pthread_mutex_lock(&mtx);
				flag = 1;
				pthread_mutex_unlock(&mtx);
				mystats->status = 1;
				answer = strdup(mystats->problem);
				print_lock
				mystats->finaliter = i;
				v2_print_thread_result(mystats->itid, mystats->finaliter, 0);
				print_unlock
				pthread_cond_broadcast(&t_main);
				destroy_job_t(mystats);
				pthread_barrier_wait(&sync_all);
				break;
			}
			else if(i == max_iters) {
				mystats->finaliter = i;
				print_lock
				v2_print_thread_result(mystats->itid, mystats->finaliter, 2);
				print_unlock
				pthread_mutex_lock(&mtx);
				done++;
				if(done == pool) {
					flag = 1;
					pthread_mutex_unlock(&mtx);
					pthread_cond_broadcast(&t_main);
				}
				pthread_mutex_unlock(&mtx);
				destroy_job_t(mystats);
				pthread_barrier_wait(&sync_all);
				break;
			}
			incrementString(mystats->problem);
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
	new_job->status = 0;
	queue_push(tasks, new_job);
}

void destroy_job_t(manifest_t *shred) {
	free(shred->username);
	free(shred->hash);
	free(shred->problem);
}

void destroy_job(manifest *shred) {
	free(shred->username);
	free(shred->hash);
	free(shred->problem);
	free(shred);
}
