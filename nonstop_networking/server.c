/**
 * Nonstop Networking
 * CS 241 - Spring 2019
 */
#include <stdio.h>
#include "vector.h"
#include "format.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>

// TODO: fix PUT (intermittent success)
// TODO: for PUT, don't push filename in vector if it already exists
// TODO: review housekeeping (open fds, etc)

static vector *file_list;
// size + 1 to accomodate newline for every file
static size_t file_list_transmit_size = 0;
static int reached_eof;
static int flag;
static char *incoming;
static char buffer[1024*1024];

// code to handle SIGINT and SIGPIPE
void sig_handler(int);
// establish connection to new clients
void establish_connection(int, struct epoll_event*);
// process data to/from connected client
void data_transaction(int, struct epoll_event*);
// send appropriate error message to client
void send_err_to_client();
ssize_t read_all_from_socket(int, char*, size_t);
ssize_t write_all_to_socket(int, const char*, size_t);
size_t populate_buffer(int);

int main(int argc, char **argv) {

	if(argc != 2) {
		print_server_usage();
		exit(1);
	}

	// set up signal handler
	struct sigaction sa_template;
	sa_template.sa_handler = sig_handler;
	int check = sigaction(SIGINT, &sa_template, NULL);
	if(check == -1) {
		perror("sigaction()");
		exit(1);
	}

	file_list = string_vector_create();

	// set up connection to clients 
	int socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	check = getaddrinfo(NULL, argv[1], &hints, &result);
	if(check) {
		////fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(check));
		exit(1);
	}
	int optval = 1;
	check = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if(check == -1) {
		perror("setsockopt()");
		exit(1);
	}
	check = bind(socket_fd, result->ai_addr, result->ai_addrlen);
	if(check == -1) {
		perror("bind()");
		exit(1);
	}
	check = listen(socket_fd, 8);
	if(check == -1) {
		perror("listen()");
		exit(1);
	}

	int epfd = epoll_create(1);
	if(epfd == -1) {
		perror("epoll_create()");
		exit(1);
	}
	struct epoll_event event;
	event.data.fd = socket_fd;
	event.events = EPOLLIN | EPOLLET;
	check = epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &event);
	if(check == -1) {
		perror("epoll_ctl()");
		exit(1);
	}

	// create temporary directory
	incoming = strdup("XXXXXX");
	mkdtemp(incoming);
	if(!incoming) {
		perror("mkdtemp()");
		exit(1);
	}
	print_temp_directory(incoming);
	chdir(incoming);

	// accept incoming connections
	while(1) {
		struct epoll_event inst;
		check = epoll_wait(epfd, &inst, 1, -1);
		if(check == -1) {
			perror("epoll_wait()");
			exit(1);
		}
		else if(check > 0) {
			if(inst.data.fd == socket_fd) {
				establish_connection(epfd, &inst);
			}
			else {
				data_transaction(epfd, &inst);
			}
		}
	}
}

void sig_handler(int signum) {
	////fprintf(stderr, "Cleaning up\n");
	int check = 0;
	if(signum == SIGINT) {
		for(size_t i = 0; i < vector_size(file_list); i++) {
			check = unlink(vector_get(file_list, i));
			if(check == -1) {
				perror("unlink()");
			}
		}
		check = chdir("..");
		if(check == -1) { perror("chdir()"); }
		check = rmdir(incoming);
		if(check == -1) { perror("rmdir()"); }
		exit(0);
	}
	else if(signum == SIGPIPE) {
	}
	else {
	}
}

size_t populate_buffer(int clfd) {
	ssize_t bytes_read_iter = 0;
	size_t bytes_read = 0;
	while(1) {
		bytes_read_iter = read(clfd, buffer + bytes_read, 1024*1024 - bytes_read);
		if(bytes_read_iter > 0) {
			bytes_read += bytes_read_iter;
		}
		else if(bytes_read_iter == -1 && errno == EAGAIN) {
			continue;
		}
		else if(bytes_read_iter == 0) {
			break;
		}
	}
	if(bytes_read > (size_t) (strstr(buffer, "\n") - buffer)) { flag = 1; }
	//fprintf(stderr, "pop_buff got %zu bytes\n", bytes_read);
	return bytes_read;
}

void data_transaction(int epfd, struct epoll_event *client_event) {
	////fprintf(stderr, "data_trans entry\n");
	//read(client_event->data.fd, buffer, 1024*1024);
	//size_t actual_read = read_all_from_socket(client_event->data.fd, buffer, 1024*1024);
	size_t init_read = populate_buffer(client_event->data.fd);

	// parse & identify request
	if(!strncmp(buffer, "GET ", 4)) {
		char *newline = strstr(buffer, "\n");
		*newline = 0;
		char *fn_str = strdup(buffer+4);
		char *fn_found = NULL;
		for(size_t i = 0; i < vector_size(file_list); i++) {
			if(!strcmp(vector_get(file_list, i), fn_str)) {
				fn_found = vector_get(file_list, i);
				break;
			}
		}
		free(fn_str);
		if(!fn_found) {
			dprintf(client_event->data.fd, "ERROR\n%s\n", err_no_such_file);
		}
		else {
			dprintf(client_event->data.fd, "OK\n");
			int send_fd = open(fn_found, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
			struct stat metadata;
			stat(fn_found, &metadata);
			size_t filesize = metadata.st_size;
			write(client_event->data.fd, &filesize, 8);
			char *file_mmap = mmap(NULL, filesize, PROT_READ, MAP_FILE | MAP_SHARED, send_fd, 0);
			write_all_to_socket(client_event->data.fd, file_mmap, filesize);
			munmap(file_mmap, filesize);
			close(send_fd);
		}
		memset(buffer, 0, 512);
		flag = 0;
		close(client_event->data.fd);
	}
	else if(!strncmp(buffer, "PUT ", 4)) {
		char *newline = strstr(buffer, "\n");
		*newline = 0;
		char *fn_str = strdup(buffer+4);
		size_t filesize, actual_read = 0;
		if(!flag) {
			//fprintf(stderr, "PUT: flag was not set, reading again\n");
			memset(buffer, 0, 1024*1024);
			actual_read = populate_buffer(client_event->data.fd);
			memcpy(&filesize, buffer, 8);
		}
		else {
			actual_read = init_read - strlen(fn_str) - 13;
			memcpy(&filesize, newline+1, 8);
		}
		//fprintf(stderr, "PUT: Got %zu bytes filesize and filename of %s\n", filesize, fn_str);
		if(filesize > actual_read) {
			print_too_little_data();
			dprintf(client_event->data.fd, "ERROR\n%s\n", err_bad_file_size);
		}
		else if(filesize < actual_read) {
			print_received_too_much_data();
			dprintf(client_event->data.fd, "ERROR\n%s\n", err_bad_file_size);
		}
		else {
			int recv_file = open(fn_str, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
			if(!flag) {
				write(recv_file, buffer, filesize);
			}
			else {
				write(recv_file, buffer+13+strlen(fn_str), filesize);
				flag = 0;
			}
			dprintf(client_event->data.fd, "OK\n");
			vector_push_back(file_list, fn_str);
			file_list_transmit_size += (strlen(fn_str) + 1);
			////fprintf(stderr, "Adding %s to vector, current flts is %zu\n", fn_str, file_list_transmit_size);
			close(recv_file);
			close(client_event->data.fd);
		}
		memset(buffer, 0, 1024*1024);
	}
	else if(!strncmp(buffer, "LIST\n", 5)) {
		if(!vector_empty(file_list)) {
			dprintf(client_event->data.fd, "OK\n");
			size_t temp_size = file_list_transmit_size - 1;
			write(client_event->data.fd, &temp_size, 8);
			if(vector_size(file_list) > 1) {
				for(size_t i = 0; i + 1 < vector_size(file_list); i++) {
					char *filename = vector_get(file_list, i);
					write(client_event->data.fd, filename, strlen(filename));
					char nl_char = '\n';
					write(client_event->data.fd, &nl_char, 1);
				}
			}
			char *last_fn = vector_get(file_list, vector_size(file_list)-1);
			write(client_event->data.fd, last_fn, strlen(last_fn));
		}
		else {
			dprintf(client_event->data.fd, "OK\n");
			size_t empty = 0;
			char null_char = 0;
			write(client_event->data.fd, &empty, 8);
			write(client_event->data.fd, &null_char, 1);
		}
		memset(buffer, 0, 1024*1024);
		flag = 0;
		close(client_event->data.fd);
	}
	else if(!strncmp(buffer, "DELETE ", 7)) {
		char *newline = strstr(buffer, "\n");
		*newline = 0;
		char *fn_str = strdup(buffer+7);
		////fprintf(stderr, "DELETE: Got filename of %s\n", fn_str);
		for(size_t i = 0; i < vector_size(file_list); i++) {
			if(!strcmp(vector_get(file_list, i), fn_str)) {
				vector_erase(file_list, i);
				file_list_transmit_size -= strlen(fn_str) + 1;
				unlink(fn_str);
				free(fn_str);
				fn_str = NULL;
			}
		}
		if(fn_str) {
			dprintf(client_event->data.fd, "ERROR\n%s\n", err_no_such_file);
		}
		else {
			dprintf(client_event->data.fd, "OK\n");
		}
		flag = 0;
	}
	else {
		print_invalid_response();
		dprintf(client_event->data.fd, "ERROR\n%s\n", err_bad_request);
		flag = 0;
	}
	memset(buffer, 0, 1024*1024);
	close(client_event->data.fd);
}

void establish_connection(int epfd, struct epoll_event *new_event) {
	while(1) {
		struct sockaddr_in new_connection_addr;
		socklen_t nca_len = sizeof(new_connection_addr);
		int check, this_fd;
		this_fd = accept(new_event->data.fd, (struct sockaddr*) &new_connection_addr, &nca_len);
		if(this_fd == -1) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) { break; }
			else {
				perror("accept()");
				exit(1);
			}
		}
		struct epoll_event ev;
		int fd_flags = fcntl(this_fd, F_GETFL, 0) | O_NONBLOCK;
		fcntl(this_fd, F_SETFL, fd_flags);
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = this_fd;
		check = epoll_ctl(epfd, EPOLL_CTL_ADD, this_fd, &ev);
		if(check == -1) {
			perror("epoll_ctl: establish_connection");
			exit(1);
		}
	}
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
	ssize_t remaining = count;
	ssize_t read_from = 0;
	ssize_t check = 0;

	while(read_from != remaining) {
		check = read(socket, buffer + read_from, count - read_from);
		if(!check) {
			reached_eof = 1;
			return read_from + check;
		}
		else if(check > 0) { read_from += check; }
		else if(check == -1 && errno == EINTR) { continue; }
		else { return read_from + check; }
	}

    return read_from;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
	ssize_t remaining = count;
	ssize_t written = 0;
	ssize_t check = 0;

	while(written != remaining) {
		check = write(socket, buffer + written, count - written);
		if(!check) { return written + check; }
		else if(check > 0) { written += check; }
		else if(check == -1 && errno == EINTR) { continue; }
		else { return written + check; }
	}

    return written;
}
