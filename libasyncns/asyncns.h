#ifndef fooasyncnshfoo
#define fooasyncnshfoo

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

struct asyncns;
typedef struct asyncns asyncns_t;

struct asyncns_query;
typedef struct asyncns_query asyncns_query_t;

asyncns_t* asyncns_new(int n_proc);
void asyncns_free(asyncns_t *asyncns);
int asyncns_fd(asyncns_t *asyncns);
int asyncns_wait(asyncns_t *asyncns, int block);

asyncns_query_t* asyncns_getaddrinfo(asyncns_t *asyncns, const char *node, const char *service, const struct addrinfo *hints);
int asyncns_getaddrinfo_done(asyncns_t *asyncns, asyncns_query_t* q, struct addrinfo **ret_res);

asyncns_query_t* asyncns_getnameinfo(asyncns_t *asyncns, const struct sockaddr *sa, socklen_t salen, int flags, int gethost, int getserv);
int asyncns_getnameinfo_done(asyncns_t *asyncns, asyncns_query_t* q, char *ret_host, size_t hostlen, char *ret_serv, size_t servlen);

asyncns_query_t* asyncns_getnext(asyncns_t *asyncns);
int asyncns_getnqueries(asyncns_t *asyncns);

void asyncns_cancel(asyncns_t *asyncns, asyncns_query_t* q);

void asyncns_freeaddrinfo(struct addrinfo *ai);

int asyncns_isdone(asyncns_t *asyncns, asyncns_query_t*q);

void asyncns_setuserdata(asyncns_t *asyncns, asyncns_query_t *q, void *userdata);
void* asyncns_getuserdata(asyncns_t *asyncns, asyncns_query_t *q);

#endif
