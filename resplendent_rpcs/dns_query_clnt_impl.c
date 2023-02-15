/**
 * Resplendent RPCs
 * CS 241 - Spring 2019
 */
#include "common.h"
#include "dns_query.h"
#include "dns_query_clnt_impl.h"

#define CACHE_FILE "cache_files/rpc_client_cache"

void resolve_hostname(char *server_host, char *host_to_resolve) {
    // Your Code Here
    // Look in the header for more details
	char *check = check_cache_for_address(host_to_resolve, CACHE_FILE);
	if(check) {
		print_ipv4_address_in_cache(host_to_resolve, check);
		free(check);
		exit(0);
	}
	//query *new_query = calloc(1, sizeof(query));
	query new_query;
	new_query.host = host_to_resolve;
	response *res_from_query = send_query(&new_query, server_host);
	if(!res_from_query) {
		printf("CLIENT: send_query() returned NULL\n");
		exit(1);
	}
	else if(res_from_query->success) {
		print_ipv4_address(res_from_query->address->host, res_from_query->address->host_ipv4_address);
		xdr_free(xdr_response, res_from_query);
	}
	else {
		print_failure();
	}
	exit(0);
}

CLIENT *create_client_stub(char *host) {
    // clnt_create is defined in the generated client stub file,
    // dns_query_clnt.c
    CLIENT *clnt =
        clnt_create(host, DNS_QUERY_SERVICE, DNS_QUERY_SERVICE_V1, "udp");
    if (clnt == NULL) {
        clnt_pcreateerror(host);
        exit(1);
    }
    return clnt;
}

response *send_query(query *q, char *to_server) {
    // Create the client stub
    CLIENT *clnt = create_client_stub(to_server);
    // Execute the RPC function (defined in dns_query.h) to send the query over
    // the client stub
    response *result = dns_query_1(q, clnt);

    // Check if failed
    if (result == (response *)NULL) {
        printf("Call failed\n");
        clnt_perror(clnt, "call failed");
    }
    // Clean up
    clnt_destroy(clnt);
    return result;
}

void print_failure() {
    printf("Domain name resolution failed.\n");
}

void print_ipv4_address(char *host, char *ipv4_address) {
    printf("%s has ipv4 address %s\n", host, ipv4_address);
}
void print_ipv4_address_in_cache(char *host, char *ipv4_address) {
    printf("Found domain's IP in local cache! No need to send an RPC to "
           "the server.\n");
    print_ipv4_address(host, ipv4_address);
}

int main(int argc, char *argv[]) {
    char *server_host;
    char *host_to_resolve;
    if (argc < 3) {
        printf("Usage: %s server_host host_to_resolve\n", argv[0]);
        exit(1);
    }
    server_host = argv[1];
    host_to_resolve = argv[2];

    resolve_hostname(server_host, host_to_resolve);
    return 0;
}
