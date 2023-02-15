/**
 * Utilities Unleashed Lab
 * CS 241 - Spring 2019
 */

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include "format.h"

//putenv() may be useful here
//process the env vars in the child process

int main(int argc, char *argv[]) {

	if(argc < 2) { print_env_usage(); } //no args, print usage

	char *name = NULL;
	char *value = NULL;
	char delim = '=';
	int status = 0;
	int count = 0;
	int flag = 0;

	for(;count < argc; count++) {
		if(strcmp(argv[count], "--") == 0) {
			flag = count;
			break;
		}
	}
	if(!flag) { print_env_usage(); } //"--" not found, print usage

	pid_t pid = fork();

	if(pid < 0) {			//fork failed
		print_fork_failed();
	}
	else if(pid) {				//parent
		waitpid(pid, &status, 0);
	}
	else {						//child
		int check;
		for(count = 1; count <= argc; count++) {	//iterate through argv
			if(count < flag) {				//before "--" (env)
				name = strtok(argv[count], &delim);
				value = strtok(NULL, &delim);
				if(*value == '%') {
					value = getenv(value + 1);
					if(!value) { print_environment_change_failed(); }
				}
				check = setenv(name, value, 1);
				if(check) { print_environment_change_failed(); }
			}
			else if(count > flag) {				//after "--" (args)
				execvp(argv[flag + 1], argv + (flag + 1));
				print_exec_failed();
			}
		}
	}

    return 0;
}
