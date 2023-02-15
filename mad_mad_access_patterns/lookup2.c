/**
 * Mad Mad Access Patterns
 * CS 241 - Spring 2019
 */
#include "tree.h"
#include "utils.h"
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/

static char *d_file;

void traverse(char*, const char*, size_t);

int main(int argc, char **argv) {
	if(argc < 3) {
		printArgumentUsage();
		exit(1);
	}

	d_file = argv[1];
	int d_file_fd = open(d_file, O_RDONLY);
	if(d_file_fd == -1) {
		openFail(d_file);
		exit(2);
	}

	struct stat f_info;
	fstat(d_file_fd, &f_info);
	char *d_mmap;
	d_mmap = mmap(0, f_info.st_size, PROT_READ, MAP_SHARED, d_file_fd, 0);
	if(d_mmap == MAP_FAILED) {
		mmapFail(d_file);
		exit(2);
	}

	close(d_file_fd);

	if(strncmp("BTRE", d_mmap, 4) != 0) {
		formatFail(d_file);
		exit(2);
	}

	argv = &argv[2];
	while(*argv) {
		traverse(d_mmap, *argv, 4);
		argv++;
	}

    return 0;
}

void traverse(char *d_mmap, const char *word_query, size_t offset) {
	char *my_d_mmap = d_mmap + offset;
	BinaryTreeNode *node;
	node = (BinaryTreeNode*) my_d_mmap;
	int cmp = strncmp(word_query, node->word, strlen(word_query));
	if(!cmp) {
		printFound(word_query, node->count, node->price);
		return;
	}
	else if(cmp < 0) {
		if(!node->left_child) {
			printNotFound(word_query);
			return;
		}
		traverse(d_mmap, word_query, node->left_child);
	}
	else {
		if(!node->right_child) {
			printNotFound(word_query);
			return;
		}
		traverse(d_mmap, word_query, node->right_child);
	}

}
