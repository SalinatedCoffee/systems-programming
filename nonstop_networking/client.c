/**
 * Nonstop Networking
 * CS 241 - Spring 2019
 */
#include "common.h"
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>

char **parse_args(int argc, char **argv);
verb check_args(char **args);
size_t handle_response(int, verb);
ssize_t read_all_from_socket(int, char*, size_t);
ssize_t write_all_to_socket(int, const char*, size_t);

static int reached_eof;

// TODO: fix issues with GET

int main(int argc, char **argv) {
	// parse and validate arguments
	char **parsed_args = parse_args(argc, argv);
	verb req_method = check_args(parsed_args);

	// set up connection to server (code from chatroom lab)
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	int check = getaddrinfo(parsed_args[0], parsed_args[1], &hints, &result);
	if(check) {
		fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(check));
		exit(1);
	}
	check = connect(socket_fd, result->ai_addr, result->ai_addrlen);
	if(check == -1) {
		perror("connect()");
		exit(1);
	}
	freeaddrinfo(result);

	// method implementation
	if(req_method == GET) {
		char *remote_filename = parsed_args[3];
		char *local_filename = parsed_args[4];
		dprintf(socket_fd, "GET %s\n", remote_filename);
		shutdown(socket_fd, SHUT_WR);
		size_t file_size = handle_response(socket_fd, GET); // TODO: for filesizes of zero, we don't create file
		fprintf(stderr, "GET (client): Got filesize of %zu\n", file_size);
		if(file_size) {
			int local_file_fd = open(local_filename, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
			if(local_file_fd == -1) {
				perror("open()");
				exit(1);
			}
			char *file_buffer = calloc(1, 4096);
			size_t recv_size = 0;
			size_t recv_size_iter = 0;
			while(!reached_eof) {
				recv_size_iter = read_all_from_socket(socket_fd, file_buffer, 4096);
				write(local_file_fd, file_buffer, recv_size_iter);
				memset(file_buffer, 0, 4096);
				recv_size += recv_size_iter;
				recv_size_iter = 0;
			}
			free(file_buffer);
			close(local_file_fd);
			if(recv_size < file_size) {
				print_too_little_data();
				remove(local_filename);
			}
			else if(recv_size > file_size) {
				print_received_too_much_data();
				remove(local_filename);
			}
		}
	}
	else if(req_method == PUT) {
		char *remote_filename = parsed_args[3];
		char *local_filename = parsed_args[4];
		FILE *local_file = fopen(local_filename, "r");
		if(!local_file) {
			perror("fopen()");
			exit(1);
		}
		struct stat f_stats;
		int check = stat(local_filename, &f_stats);
		if(check == -1) {
			perror("stat()");
			exit(1);
		}
		size_t file_size = f_stats.st_size;
		dprintf(socket_fd, "PUT %s\n", remote_filename);
		write(socket_fd, &file_size, 8);
		if(file_size) {
			size_t num_chunks = file_size / 4096;
			if(!num_chunks) {
				char *file_buffer;
				file_buffer = calloc(1, file_size);
				if(!file_buffer) {
					perror("calloc()");
					exit(1);
				}
				fread(file_buffer, file_size, 1, local_file);
				write_all_to_socket(socket_fd, file_buffer, file_size);
				free(file_buffer);
			}
			else {
				char *file_buffer;
				file_buffer = calloc(1, 4096);
				if(!file_buffer) {
					perror("calloc()");
					exit(1);
				}
				for(size_t i = 0; i < num_chunks; i++) {
					fread(file_buffer, 4096, 1, local_file);
					write_all_to_socket(socket_fd, file_buffer, 4096);
					memset(file_buffer, 0, 4096);
				}
				free(file_buffer);
				size_t rem = file_size % 4096;
				file_buffer = calloc(1, rem);
				fread(file_buffer, rem, 1, local_file);
				write_all_to_socket(socket_fd, file_buffer, rem);
				free(file_buffer);
			}
			shutdown(socket_fd, SHUT_WR);
		}
		handle_response(socket_fd, PUT);
		fclose(local_file);
	}
	else if(req_method == LIST) {
		dprintf(socket_fd, "LIST\n");
		shutdown(socket_fd, SHUT_WR);
		handle_response(socket_fd, LIST);
	}
	else if(req_method == DELETE) {
		char *remote_filename = parsed_args[3];
		dprintf(socket_fd, "DELETE %s\n", remote_filename);
		shutdown(socket_fd, SHUT_WR);
		handle_response(socket_fd, DELETE);
	}
	else{
		print_client_help();
		exit(1);
	}

	free(parsed_args);
	shutdown(socket_fd, SHUT_RD);
	close(socket_fd);
}

size_t handle_response(int fd, verb mthd) {
	char buffer[6];

	read(fd, buffer, 3);
	if(!strncmp(buffer, "OK\n", 3)) {
		size_t data_size;
		if(mthd == GET) {
			read(fd, &data_size, sizeof(size_t));
			return data_size;
		}
		else if(mthd == LIST) {
			read(fd, &data_size, sizeof(size_t));
			if(data_size) {
				char *list = calloc(1, data_size);
				read_all_from_socket(fd, list, data_size);
				fprintf(stdout, "%s", list);
				free(list);
			}
			return 0;
		}
		else {
			print_success();
			return 0;
		}
	}

	read(fd, buffer+3, 3);
	if(!strncmp(buffer, "ERROR\n", 6)) {
		char err_buffer[1024];
		memset(err_buffer, 0, 1024);
		read_all_from_socket(fd, err_buffer, 1024);
		print_error_message(err_buffer);
		return 0;
	}

	print_invalid_response();
	return 0;
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

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}
