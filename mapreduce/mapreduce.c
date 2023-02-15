/**
 * mapreduce Lab
 * CS 241 - Spring 2019
 */
 
#include "utils.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

// ./mapreduce <input_file> <output_file> <mapper_executable> <reducer_executable> <mapper_count>

// Input filename, total number of splitters, current number of splitter, pipe to use as stdout
void start_splitter(char*, int, int, int);

// Mapper filename, pipe to use as stdin, pipe to use as stdout
void start_mapper(char*, int, int);

// Reducer filename, pipe to use as stdin, pipe to use as stdout
void start_reducer(char*, int, int);

int main(int argc, char **argv) {
	assert(argc == 6);
	char *input_f = strdup(argv[1]);
	char *output_f = strdup(argv[2]);
	char *mapper_e = strdup(argv[3]);
	char *reducer_e = strdup(argv[4]);
	int mapper_c = atoi(argv[5]);
	pid_t pids_m[mapper_c];
	pid_t pids_s[mapper_c];
	pid_t pid_r;
	int pipes_m[2*mapper_c];
	int pipes_r[2];
	int output_fd;
	mode_t mode = S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH; 

    // Create an input pipe for each mapper.
	for(int i = 0; i < mapper_c; i++) {
		pipe(pipes_m + 2*i);
		descriptors_add(*(pipes_m+2*i));
		descriptors_add(*(pipes_m+2*i+1));
	}

    // Create one input pipe for the reducer.
	pipe(pipes_r);
	descriptors_add(pipes_r[0]);
	descriptors_add(pipes_r[1]);
	
    // Open the output file.
	output_fd = open(output_f, O_WRONLY | O_CREAT, mode);
	descriptors_add(output_fd);

    // Start a splitter process for each mapper.
	for(int i = 0; i < mapper_c; i++) {
		pids_s[i] = fork();
		if(pids_s[i] == 0) { // child
			start_splitter(input_f, mapper_c, i, pipes_m[2*i+1]);
		}
	}

    // Start all the mapper processes.
	for(int i = 0; i < mapper_c; i++) {
		pids_m[i] = fork();
		if(pids_m[i] == 0) { // child
			start_mapper(mapper_e, pipes_m[2*i], pipes_r[1]);
		}
	}

    // Start the reducer process.
	pid_r = fork();
	if(pid_r == 0) { // child
		start_reducer(reducer_e, pipes_r[0], output_fd);
	}

	// - done handling pipes in the parent process, nuke all fds
	descriptors_closeall();
	descriptors_destroy();

    // Wait for the reducer to finish.
	int r_stat = 0;
	waitpid(pid_r, &r_stat, 0);

	// - also wait for mappers and splitters
	int m_stat[mapper_c];
	int s_stat[mapper_c];
	for(int i = 0; i < mapper_c; i++) {
		waitpid(pids_m[i], m_stat + i, 0);
		waitpid(pids_s[i], s_stat + i, 0);
	}

    // Print nonzero subprocess exit codes.
	if(WIFEXITED(r_stat) && WEXITSTATUS(r_stat)) {
		print_nonzero_exit_status(reducer_e, WEXITSTATUS(r_stat));
	}

	for(int i = 0; i< mapper_c; i++) {
		if(WIFEXITED(m_stat[i]) && WEXITSTATUS(m_stat[i])) {
			print_nonzero_exit_status(mapper_e, WEXITSTATUS(m_stat[i]));
		}
		if(WIFEXITED(s_stat[i]) && WEXITSTATUS(s_stat[i])) {
			print_nonzero_exit_status("./splitter", WEXITSTATUS(s_stat[i]));
		}
	}

    // Count the number of lines in the output file.
	print_num_lines(output_f);

	// cleanup
	free(input_f);
	free(output_f);
	free(mapper_e);
	free(reducer_e);

    return 0;
}

void start_splitter(char *filename, int num_procs, int my_num, int my_pipe) {
	dup2(my_pipe, 1); // swap stdout for my_pipe (this should go to mapper stdin)
	char num_procs_s[10];
	char my_num_s[10];
	sprintf(num_procs_s, "%d", num_procs);
	sprintf(my_num_s, "%d", my_num);
	descriptors_closeall();
	descriptors_destroy();
	execlp("./splitter", "./splitter", filename, num_procs_s, my_num_s, NULL);

}

void start_mapper(char *filename, int my_pipe_r, int my_pipe_w) {
	dup2(my_pipe_r, 0); // this should be splitter stdout
	dup2(my_pipe_w, 1); // this should be reducer stdin
	descriptors_closeall();
	descriptors_destroy();
	execlp(filename, filename);
}

void start_reducer(char *filename, int my_pipe_r, int my_pipe_w) {
	dup2(my_pipe_r, 0); // this should be mapper stdout
	dup2(my_pipe_w, 1); // this should be output file
	descriptors_closeall();
	descriptors_destroy();
	execlp(filename, filename);
}
