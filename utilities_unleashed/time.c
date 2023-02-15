/**
 * Utilities Unleashed Lab
 * CS 241 - Spring 2019
 */
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "format.h"

double timecalc(struct timespec*);

int main(int argc, char *argv[]) {

	if(argc<2) { print_time_usage(); } //no args, print usage

	struct timespec start;
	struct timespec end;
	int status;

	pid_t pid = fork();

	if(pid<0) {		//fork failed
		print_fork_failed();
	}
	else if(pid>0) {	//parent
		clock_gettime(CLOCK_MONOTONIC, &start);
		waitpid(pid, &status, 0);
		clock_gettime(CLOCK_MONOTONIC, &end);

		if(WIFEXITED(status) && WEXITSTATUS(status) != 42) {			//child terminated normally
			double res = timecalc(&end) - timecalc(&start);
			display_results(argv, res);
		}
		else if(WIFEXITED(status) && WEXITSTATUS(status) == 42) {
			print_exec_failed();
		}
		else { exit(1); }		//child exited abnormally
	}
	else {	//child
		execvp(argv[1], argv+1);
		return 42;
	}

    return 0;
}

double timecalc(struct timespec *ts) {
	return (double) ts->tv_sec + (double) ts->tv_nsec / 1000000000;
}
