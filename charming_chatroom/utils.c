/**
 * Charming Chatroom
 * CS 241 - Spring 2019
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"
static const size_t MESSAGE_SIZE_DIGITS = 4;

char *create_message(char *name, char *message) {
    int name_len = strlen(name);
    int msg_len = strlen(message);
    char *msg = calloc(1, msg_len + name_len + 4);
    sprintf(msg, "%s: %s", name, message);

    return msg;
}

ssize_t get_message_size(int socket) {
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}

// You may assume size won't be larger than a 4 byte integer
ssize_t write_message_size(size_t size, int socket) {
	int32_t size_local = htonl(size);
	return write_all_to_socket(socket, (char*) &size_local, 4);
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
	ssize_t remaining = count;
	ssize_t read_from = 0;
	ssize_t check = 0;

	while(read_from != remaining) {
		check = read(socket, buffer + read_from, count - read_from);
		if(!check) { return read_from + check; }
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
