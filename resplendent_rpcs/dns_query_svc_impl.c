/**
 * Resplendent RPCs
 * CS 241 - Spring 2019
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "common.h"
#include "common.h"
#include "dns_query.h"
#include "dns_query_svc_impl.h"

#define CACHE_FILE "cache_files/rpc_server_cache"

char *contact_nameserver(query *argp, char *host, int port) {
    // Your code here
    // Look in the header file for a few more comments
	int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in manifest;
	memset(&manifest, 0, sizeof(manifest));
	manifest.sin_family = AF_INET;
	manifest.sin_port = htons((uint16_t)port);
	inet_pton(AF_INET, host, &manifest.sin_addr.s_addr);
	int check;
	while(1) {
		check = sendto(socket_fd, argp->host, strlen(argp->host), 0, &manifest, (socklen_t) sizeof(manifest));
		if(check == -1 && errno == EINTR) { continue; }
		else { break; }
	}
	char *recv_ip = calloc(1, 16);
	while(1) {
		check = recvfrom(socket_fd, recv_ip, 15, 0, NULL, NULL);
		if(check == -1 && errno == EINTR) { continue; }
		else { break; }
	}
	if(strstr(recv_ip, "-1")) {
		free(recv_ip);
		return NULL;
	}
    return recv_ip;
}

void create_response(query *argp, char *ipv4_address, response *res) {
    // Your code here
    // As before there are comments in the header file
	char *quer_host = strdup(argp->host);
	char *resp_ipv4;
	int success;
	res->address = calloc(1, sizeof(host_address));
	if(!ipv4_address) {
		resp_ipv4 = calloc(1, 1);
		success = 0;
	}
	else {
		resp_ipv4 = strdup(ipv4_address);
		success = 1;
	}
	res->address->host = quer_host;
	res->address->host_ipv4_address = resp_ipv4;
	res->success = success;
}

// Stub code

response *dns_query_1_svc(query *argp, struct svc_req *rqstp) {
    printf("Resolving query...\n");
    // check its cache, 'rpc_server_cache'
    // if it's in cache, return with response object with the ip address
    char *ipv4_address = check_cache_for_address(argp->host, CACHE_FILE);
    if (ipv4_address == NULL) {
        // not in the cache. contact authoritative servers like a recursive dns
        // server
        printf("Domain not found in server's cache. Contacting authoritative "
               "servers...\n");
        char *host = getenv("NAMESERVER_HOST");
        int port = strtol(getenv("NAMESERVER_PORT"), NULL, 10);
        ipv4_address = contact_nameserver(argp, host, port);
    } else {
        // it is in the server's cache; no need to ask the authoritative
        // servers.
        printf("Domain found in server's cache!\n");
    }

    static response res;
    xdr_free(xdr_response, &res); // Frees old memory in the response struct
    create_response(argp, ipv4_address, &res);

    free(ipv4_address);

    return &res;
}
