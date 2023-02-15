/**
 * Mad Mad Access Patterns
 * CS 241 - Spring 2019
 */
#include "tree.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/

static char *d_file;

// recursively search for word (const char*) in file (FILE*) at node (size_t)
void traverse(FILE*, const char*, size_t);

int main(int argc, char **argv) {
	if(argc < 3) {
		printArgumentUsage();
		exit(1);
	}

	d_file = argv[1];
	FILE *d_file_f = fopen(d_file, "r");
	if(!d_file_f) {
		openFail(d_file);
		exit(2);
	}

	char header[5];
	header[4] = '\0';
	fread(header, 4, 1, d_file_f);
	if(strcmp(header, "BTRE") != 0) {
		formatFail(d_file);
		exit(2);
	}

	argv = &argv[2];
	while(*argv) {
		traverse(d_file_f, *argv, 4);
		argv++;
	}

	fclose(d_file_f);

    return 0;
}

void traverse(FILE *d_file_f, const char *word_query, size_t offset) {
	fseek(d_file_f, offset, SEEK_SET);
	BinaryTreeNode node;
	fread(&node, sizeof(BinaryTreeNode), 1, d_file_f);
	char buffer[51];
	buffer[50] = '\0';
	fgets(buffer, 50, d_file_f);
	int cmp = strncmp(word_query, buffer, strlen(word_query));
	if(!cmp) {
		printFound(word_query, node.count, node.price);
		return;
	}
	else if(cmp < 0) {
		if(!node.left_child) {
			printNotFound(word_query);
			return;
		}
		traverse(d_file_f, word_query, node.left_child);
	}
	else {
		if(!node.right_child) {
			printNotFound(word_query);
			return;
		}
		traverse(d_file_f, word_query, node.right_child);
	}
}
