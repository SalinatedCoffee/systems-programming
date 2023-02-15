/**
 * Savvy Scheduler
 * CS 241 - Spring 2019
 */
#include "libpriqueue/libpriqueue.h"
#include "libscheduler.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "vector.h"
#include "print_functions.h"
typedef struct _job_info {
    int id;
	double priority;
	double arrival_time;
	double running_time;
	double rem_running_time;
    /* Add whatever other bookkeeping you need into this struct. */
} job_info;

priqueue_t pqueue;
scheme_t pqueue_scheme;
comparer_t comparision_func;
vector *existing_jobs;
double total_waiting_time;
double total_turnaround_time;
double total_response_time;
int total_number_jobs;

void scheduler_start_up(scheme_t s) {
    switch (s) {
    case FCFS:
        comparision_func = comparer_fcfs;
        break;
    case PRI:
        comparision_func = comparer_pri;
        break;
    case PPRI:
        comparision_func = comparer_ppri;
        break;
    case PSRTF:
        comparision_func = comparer_psrtf;
        break;
    case RR:
        comparision_func = comparer_rr;
        break;
    case SJF:
        comparision_func = comparer_sjf;
        break;
    default:
        printf("Did not recognize scheme\n");
        exit(1);
    }
    priqueue_init(&pqueue, comparision_func);
    pqueue_scheme = s;
    // Put any set up code you may need here
	existing_jobs = shallow_vector_create();
}

static int break_tie(const void *a, const void *b) {
    return comparer_fcfs(a, b);
}

int comparer_fcfs(const void *a, const void *b) {
	job *job_a = a;
	job *job_b = b;
	job_info *metadata_a = job_a->metadata;
	job_info *metadata_b = job_b->metadata;
	if(metadata_a->arrival_time > metadata_b->arrival_time) { return 1; }
	else if(metadata_a->arrival_time < metadata_b->arrival_time) { return -1; }
    else if(metadata_a->id > metadata_b->id) { return 1; }
	else if(metadata_a->id < metadata_b->id) { return -1; }
	else { return 0; }
}

int comparer_ppri(const void *a, const void *b) {
	job *job_a = a;
	job *job_b = b;

    // Complete as is
    return comparer_pri(a, b);
}

int comparer_pri(const void *a, const void *b) {
	job *job_a = a;
	job *job_b = b;
	job_info *metadata_a = job_a->metadata;
	job_info *metadata_b = job_b->metadata;

	if(metadata_a->priority > metadata_b->priority) { return 1; }
	else if(metadata_a->priority < metadata_b->priority) { return -1; }
	else { return break_tie(a, b); }
}

int comparer_psrtf(const void *a, const void *b) {
	job *job_a = a;
	job *job_b = b;
	job_info *metadata_a = job_a->metadata;
	job_info *metadata_b = job_b->metadata;

	if(metadata_a->rem_running_time > metadata_b->rem_running_time) { return 1; }
	else if(metadata_a->rem_running_time < metadata_b->rem_running_time) { return -1; }
	else { return break_tie(a, b); }
}

int comparer_rr(const void *a, const void *b) {
	job *job_a = a;
	job *job_b = b;
	
    return comparer_pri(a, b);
}

int comparer_sjf(const void *a, const void *b) {
	job *job_a = a;
	job *job_b = b;
	job_info *metadata_a = job_a->metadata;
	job_info *metadata_b = job_b->metadata;

	if(metadata_a->running_time > metadata_b->running_time) { return 1; }
	else if(metadata_a->running_time < metadata_b->running_time) { return -1; }
	else { return break_tie(a, b); }
}

// Do not allocate stack space or initialize ctx. These will be overwritten by
// gtgo
void scheduler_new_job(job *newjob, int job_number, double time,
                       scheduler_info *sched_data) {
    // TODO complete me!
	job_info *manifest = malloc(sizeof(job_info));
	newjob->metadata = manifest;
	manifest->priority = sched_data->priority;
	manifest->arrival_time = time;
	manifest->running_time = sched_data->running_time;
	manifest->rem_running_time = sched_data->running_time;
	manifest->id = job_number;
	
	vector_push_back(existing_jobs, newjob);
}

job *scheduler_quantum_expired(job *job_evicted, double time) {
    // TODO complete me!
	switch (pqueue_scheme) {
	} // or maybe an if() will do
    return NULL;
}

void scheduler_job_finished(job *job_done, double time) {
    // TODO complete me!
}

static void print_stats() {
    fprintf(stderr, "turnaround     %f\n", scheduler_average_turnaround_time());
    fprintf(stderr, "total_waiting  %f\n", scheduler_average_waiting_time());
    fprintf(stderr, "total_response %f\n", scheduler_average_response_time());
}

double scheduler_average_waiting_time() {
    // TODO complete me!
    return 9001;
}

double scheduler_average_turnaround_time() {
    // TODO complete me!
    return 9001;
}

double scheduler_average_response_time() {
    // TODO complete me!
    return 9001;
}

void scheduler_show_queue() {
    // Implement this if you need it!
}

void scheduler_clean_up() {
    priqueue_destroy(&pqueue);
    print_stats();
}
