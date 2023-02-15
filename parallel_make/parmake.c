/**
 * Parallel Make Lab
 * CS 241 - Spring 2019
 */
 
#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "vector.h"
#include "set.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

// try recursive solution while keeping track of whether a rule has already been successfully completed:

int parmake(char *makefile, size_t num_threads, char **targets) {

	graph *dep_graph = parser_parse_makefile(makefile, targets);
	rule_t *sentinel = graph_get_vertex_value(dep_graph, "");
	vector *goals = graph_neighbors(dep_graph, sentinel);

	while() { // loop for every element in goals
		// search for cycles in dependencies
		// if no dependencies found attempt to run commands for all dependencies
		// remember to check if target or dependencies are files on disk
	}

    return 0;
}

bool detect_cycle(void *node, set *visited) {
	if(set_contains(visited, node)) { return true; }

	vector *adj = graph_neighbors(rag, node);

	for(size_t i = 0; i < vector_size(adj); i++) {
		set_add(visited, node);
		int eval = detect_cycle(vector_get(adj, i), visited);
		if(eval) {
			vector_destroy(adj);
			return true;
		}
	}
	vector_destroy(adj);
	return false;
}

void run_commands(rule_t *rule) {
}
