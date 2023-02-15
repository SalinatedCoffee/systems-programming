/**
 * Shell Lab
 * CS 241 - Spring 2019
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>

#include "format.h"
#include "shell.h"
#include "vector.h"
#include "sstring.h"

typedef struct process {
    char *command;
    pid_t pid;
} process;

// Pass a string on the heap to truncate its last character.
// The passed string itself will be modified.
void trunc_last_char(char*);

// Pass a vector to print all items in history format in format.h.
void print_hist_all(vector*);

// Pass a vector, FILE pointer, and starting index to append contents to file.
void save_hist_file(FILE*, vector*, size_t);

// Pass a FILE pointer to load all lines in it as history to a vector.
// Returns the number of commands read, or 0 if it was an empty file.
size_t  get_hist_from_file(FILE*, vector*);

// SIGINT handler.
// Kills foreground process, if it exists.
void signal_handler(int);

// SIGCHLD handler.
// Waits on any children.
void reaper(int);

// Pass pointers to the beginning and end of a command to exec() it.
// The passed string will not be changed, and exec_external will parse
// the whitespace delimited command.
void exec_external(const char*, const char*);

char* make_proc_path(pid_t, char*);

char* get_proc_command(pid_t);

static pid_t chldpid;
static int execstat;
static vector *chpids;

int shell(int argc, char *argv[]) {

	int options = 0;
	char *history_o = NULL, *script_o = NULL;
	FILE *history_f = NULL, *script_f = NULL;
	pid_t mypid = getpid();
	chldpid = 0;
	chpids = int_vector_create();
	char *wd = NULL, *cmd = NULL; //free these after use
	size_t lines_in_hist;
	size_t len;
	vector *hist = string_vector_create();
	opterr = 0; //supress getopt errors

	while((options = getopt(argc, argv, ":h:")) != -1) { //check history option
		if(options == 'h') {
			history_o = optarg;
		}
		else if(options == ':') {
			print_history_file_error();
		}
	}

	optind = 0; //reset optind for next getopt

	while((options = getopt(argc, argv, ":f:")) != -1) { //check file option
		if(options == 'f') {
			script_o = optarg;
		}
		else if(options == ':') {
			print_usage();
			exit(1);
		}
	}


	if(history_o) { //check parsed -h
		if(!strcmp(history_o, "-f")) {
			print_history_file_error();
		}
		else {
			history_f = fopen(history_o, "a+");
			if(!history_f) { print_history_file_error(); }
			else {
				lines_in_hist = get_hist_from_file(history_f, hist);
			}
		}
	}

	if(script_o) { //check parsed -f
		script_f = fopen(script_o, "r");
		if(!script_f) {
			print_script_file_error();
			exit(0);
		}
	}

	signal(SIGINT, signal_handler);
	signal(SIGCHLD, reaper);

	int getline_ret = 0;
	int run_history = 0;
	char *prev_cmd = NULL;
	int blank = 0;
	pid_t descend = 0;

	char *lhs = NULL;
	char *rhs = NULL;
	char *operator = NULL;

	FILE *read_from = NULL;

	int oper_type = 0; //1 for &&, 2 for ||, 3 for ;

	if(!script_f) {
		read_from = stdin;
	}
	else {
		read_from = script_f;
	}

	while(1) {
		wd = getcwd(NULL, 0);

		if(!run_history && !lhs && !rhs) {
			if(read_from == stdin) {
				print_prompt(wd, mypid);
			}
			getline_ret = getline(&cmd, &len, read_from);
			if(read_from != stdin && getline_ret >= 0) {
				print_prompt(wd, mypid);
				printf("%s", cmd);
			}
			trunc_last_char(cmd);
		}
		else if(run_history) {
			cmd = strdup(prev_cmd);
		}

		if(!oper_type) {
			if(cmd && (operator = strstr(cmd, "&&"))) {
				lhs = strndup(cmd, operator - cmd);
				rhs = strdup(operator+2);
				oper_type = 1;
			}
			else if(cmd && (operator = strstr(cmd, "||"))) {
				lhs = strndup(cmd, operator - cmd);
				rhs = strdup(operator+2);
				oper_type = 2;
			}
			else if(cmd && (operator = strstr(cmd, ";"))) {
				lhs = strndup(cmd, operator - cmd);
				rhs = strdup(operator+1);
				oper_type = 3;
			}
		}

		if(oper_type) { // if we have operation
			if(lhs) { // if lhs isn't null
				vector_push_back(hist, cmd);
				free(cmd);
				cmd = lhs;
				lhs = NULL;
			}
			else if(oper_type == 1) { // and
				if(WIFEXITED(execstat) && WEXITSTATUS(execstat) == 0) { // first cmd normally exited
					cmd = rhs;
					rhs = NULL;
				}
				else {
					free(rhs);
					free(wd);
					rhs = NULL;
					wd = NULL;
					oper_type = 0;
					continue;
				}
			}
			else if(oper_type == 2) { // or
				if(WIFEXITED(execstat) && WEXITSTATUS(execstat) != 0) { // first cmd abnormally exited
					cmd = rhs;
					rhs = NULL;
				}
				else {
					free(rhs);
					free(wd);
					rhs = NULL;
					wd = NULL;
					oper_type = 0;
					continue;
				}
			}
			else if(oper_type == 3) { // sequential
				cmd = rhs;
				rhs = NULL;
			}
		}

		if((getline_ret < 0) || !strcmp(cmd, "exit")) {
			save_hist_file(history_f, hist, lines_in_hist);
			vector_destroy(hist);
			if(cmd != prev_cmd) {
				free(cmd);
			}
			free(wd);
			while((descend = wait(&blank)) > 0);
			return 0;
		}
		else if((strlen(cmd) == 8) && !strcmp(cmd, "!history")) {
			print_hist_all(hist);
		}
		else if(*cmd == '\0') {
			continue;
		}
		else if(!strncmp(cmd, "ps", 2)) {
			make_proc_path(atoi(cmd+3), "cmdline");
			continue;
		}
		else if(!strncmp(cmd, "kill", 4)) {
			char* k_cmd = get_proc_command(atoi(cmd+5));
			int k_stat = kill(atoi(cmd+5), SIGKILL);
			if(k_stat == -1) {
				print_no_process_found(atoi(cmd+5));
			}
			else {
				print_killed_process(atoi(cmd+5), k_cmd);
			}
			free(k_cmd);
		}
		else if(!strncmp(cmd, "stop", 4)) {
			char* s_cmd = get_proc_command(atoi(cmd+5));
			int s_stat = kill(atoi(cmd+5), SIGSTOP);
			if(s_stat == -1) {
				print_no_process_found(atoi(cmd+5));
			}
			else {
				print_stopped_process(atoi(cmd+5), s_cmd);
			}
			free(s_cmd);
		}
		else if(!strncmp(cmd, "cont", 4)) {
			char* c_cmd = get_proc_command(atoi(cmd+5));
			int c_stat = kill(atoi(cmd+5), SIGCONT);
			if(c_stat == -1) {
				print_no_process_found(atoi(cmd+5));
			}
			free(c_cmd);
		}
		else if(!strncmp(cmd, "cd", 2)) {
			vector_push_back(hist, cmd);
			int check_chd = chdir(cmd+3);
			if(check_chd == -1) {
				print_no_directory(cmd+3);
			}
		}
		else if(*cmd == '!') {
			int i = (int) vector_size(hist) - 1;
			size_t pref_len = strlen(cmd+1);
			for(; i > 0 && vector_at(hist, i); i--) {
				if(!strncmp(cmd+1, vector_get(hist, i), pref_len)) {
					run_history = 1;
					prev_cmd = vector_get(hist, i);
					free(cmd);
					cmd = NULL;
					print_command(prev_cmd);
					break;
				}
			}
			if(!run_history) {
				print_no_history_match();
			}
		}
		else if(*cmd == '#') {
			int hist_idx = atoi(cmd+1);
			if((hist_idx > (int)vector_size(hist) - 1) && hist_idx >= 0) {
				print_invalid_index();
			}
			else {
				free(cmd);
				free(wd);
				wd = NULL;
				cmd = NULL;
				prev_cmd = vector_get(hist, hist_idx);
				run_history = 1;
				print_command(prev_cmd);
			}
		}
		else {
			if(oper_type != 0 && rhs == NULL) { // if we're done with rhs
				oper_type = 0;
			}
			else if (!oper_type && !lhs && !rhs) { // if we don't have logic op and no lhs and rhs
				vector_push_back(hist, cmd);
			}
			exec_external(cmd, cmd+strlen(cmd));
		}

		free(wd);
		wd = NULL;

		if(!run_history) {
			free(cmd);
			cmd = NULL;
		}
		else if(cmd) {
			run_history = 0;
			free(cmd);
			cmd = NULL;
		}
	}

    return 0;
}

char* make_proc_path(pid_t pid, char* dirf) {
	char path[1024];
	sprintf(path, "/proc/%d/%s", pid, dirf);

	char* ret = malloc(strlen(path) + 1);
	strcpy(ret, path);

	return ret;
}

char* get_proc_command(pid_t pid) {
	char buffer[1024];
	char* path = make_proc_path(pid, "cmdline");
	FILE* cmdline = fopen(path, "r");
	if(cmdline == NULL) { return NULL; }
	size_t len = fread(buffer, 1, sizeof(buffer), cmdline);
	fclose(cmdline);
	free(path);
	if(len == 0 || len == sizeof(buffer)) { return NULL; }
	buffer[len] = '\0';
	while(len > 0) {
		len--;
		if(buffer[len] == '\0') { buffer[len] = ' '; }
	}

	char* ret = malloc(strlen(buffer) + 1);
	strcpy(ret, buffer);

	return ret;
}

void exec_external(const char* command, const char* end) {
	int bg = 0;
	size_t length = end - command + 1;
	char* cmd = malloc(length);
	char* cmd_parsed;

	strncpy(cmd, command, length - 1);
	cmd[length - 1] = '\0';

	if(cmd[length - 2] == '&') {
		trunc_last_char(cmd);
		bg = 1;
	}

	vector* vec_cmds = string_vector_create();

	cmd_parsed = strtok(cmd, " ");

	while(cmd_parsed) {
		vector_push_back(vec_cmds, cmd_parsed);
		cmd_parsed = strtok(NULL, " ");
	}

	size_t cmd_num = vector_size(vec_cmds);
	char **cmd_args = malloc(sizeof(char*)*(cmd_num + 1));

	size_t i;
	for(i = 0; i < cmd_num; i++) {
		cmd_args[i] = vector_get(vec_cmds, i);
	}
	
	cmd_args[cmd_num] = NULL;

	chldpid = fork();

	if(chldpid == 0) {
		print_command_executed(getpid());
		execvp(cmd_args[0], cmd_args);
		print_exec_failed(command);
		exit(1);
	}
	else if(chldpid == -1) {
		print_fork_failed();
		exit(1);
	}
	else if(!bg) {
		if(setpgid(chldpid, chldpid) == -1) {
			print_setpgid_failed();
			exit(1);
		}
		int wait_f = waitpid(chldpid, &execstat, 0);
		if(wait_f == -1) {
			print_wait_failed();
		}
	}
	else if(bg == 1) {
		vector_push_back(chpids, chldpid);
		if(setpgid(chldpid, chldpid) == -1) {
			print_setpgid_failed();
			exit(1);
		}
	}

	free(cmd);
	free(cmd_args);
	vector_destroy(vec_cmds);
}

void reaper(int signal) {
	int status = 0;
	wait(&status);
}

void signal_handler(int signal) {
	kill(chldpid, SIGINT);
}

void trunc_last_char(char *str) {
	int len = strlen(str);
	str[len-1] = '\0';
}

void save_hist_file(FILE *hist, vector *hist_vec, size_t bookmark) {
	if(!hist) { return; }
	size_t vec_size = vector_size(hist_vec);
	while(bookmark < vec_size) {
		fprintf(hist, "%s\n", vector_get(hist_vec, bookmark));
		fflush(hist);
		bookmark++;
	}
}

void print_hist_all(vector *hist_vec) {
	size_t len = vector_size(hist_vec);
	size_t i;

	for(i = 0; i < len; i++) {
		print_history_line(i, vector_get(hist_vec, i));
	}
}

size_t get_hist_from_file(FILE *hist, vector *hist_vec) {
	char* cmd = NULL;
	size_t len;

	if(getline(&cmd, &len, hist) == -1) { return 0; } //EOF / fail on first line
	else {
		trunc_last_char(cmd);
		vector_push_back(hist_vec, cmd);
		free(cmd);
		cmd = NULL;

		while(getline(&cmd, &len, hist) != -1) {
			trunc_last_char(cmd);
			vector_push_back(hist_vec, cmd);
			free(cmd);
			cmd = NULL;
		}
	}
	#ifdef DEBUG
	printf("Read %lu lines from history file.\n", vector_size(hist_vec));
	#endif
	return vector_size(hist_vec);
}
