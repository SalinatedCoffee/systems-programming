/**
 * Deadlock Demolition Lab
 * CS 241 - Spring 2019
 */
 
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <pthread.h>

struct drm_t {
	pthread_mutex_t lock;
};

// included graph is not MT-Safe

static graph *rag = NULL;
static pthread_mutex_t rag_lock = PTHREAD_MUTEX_INITIALIZER;

bool detect_cycle(void*, set*);

drm_t *drm_init() {
	drm_t *ret = malloc(sizeof(drm_t));
	pthread_mutex_init(&ret->lock, NULL);

	pthread_mutex_lock(&rag_lock);
	if(!rag) { rag = shallow_graph_create(); }
	graph_add_vertex(rag, ret);
	pthread_mutex_unlock(&rag_lock);

    return ret;
}

int drm_post(drm_t *drm, pthread_t *thread_id) {
	pthread_mutex_lock(&rag_lock); // lock graph mutex
	if(!graph_contains_vertex(rag, thread_id)) { // if vertex thread_id dne, return 0
		pthread_mutex_unlock(&rag_lock);
		return 0;
	}
	if(graph_adjacent(rag, drm, thread_id)) { // if edge from drm to thread_id exists, unlock, remove edge, return 1
		pthread_mutex_unlock(&drm->lock);
		graph_remove_edge(rag, drm, thread_id);
		pthread_mutex_unlock(&rag_lock);
		return 1;
	}
    return 0;
}

int drm_wait(drm_t *drm, pthread_t *thread_id) {
	pthread_mutex_lock(&rag_lock); // lock graph mutex
	if(!graph_contains_vertex(rag, thread_id)) { graph_add_vertex(rag, thread_id); } // add thread_id if it doesn't exist in graph
	if(graph_adjacent(rag, drm, thread_id)) { // if drm already owned by thread_id, return 0
		pthread_mutex_unlock(&rag_lock);
		return 0;
	}
	graph_add_edge(rag, thread_id, drm); // add requesting edge to graph
	set *visited = shallow_set_create();
	if(detect_cycle(thread_id, visited)) { // if cycle exists
		graph_remove_edge(rag, thread_id, drm); // remove edge, return 0
		pthread_mutex_unlock(&rag_lock);
		return 0;
	}
	pthread_mutex_unlock(&rag_lock); // this thread may wait on mutex lock, so relinquish lock on graph
	pthread_mutex_lock(&drm->lock); // no cycle detected, lock mutex
	pthread_mutex_lock(&rag_lock); // relock graph mutex
	graph_add_edge(rag, drm, thread_id); // add ownership edge
	graph_remove_edge(rag, thread_id, drm); // remove requesting edge
	pthread_mutex_unlock(&rag_lock);
	set_destroy(visited);
    return 1;
}

void drm_destroy(drm_t *drm) {
	pthread_mutex_lock(&rag_lock);
	graph_remove_vertex(rag, drm);
	pthread_mutex_unlock(&rag_lock);

	pthread_mutex_destroy(&drm->lock);
	free(drm);
    return;
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
