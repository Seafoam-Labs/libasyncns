#ifndef fooasyncnshfoo
#define fooasyncnshfoo

#include <sys/socket.h>
#include <netdb.h>

struct aar;
typedef struct aar aar_t;

struct aar_query;
typedef struct aar_query aar_query_t;

aar_t* aar_new(int n_proc);
void aar_free(aar_t *aar);
int aar_fd(aar_t *aar);
int aar_wait(aar_t *aar, int block);

aar_query_t* aar_getaddrinfo(aar_t *aar, const char *node, const char *service, const struct addrinfo *hints);
int aar_getaddrinfo_done(aar_t *aar, aar_query_t* q, struct addrinfo **ret_res);

aar_query_t* aar_getnameinfo(aar_t *aar, const struct sockaddr *sa, socklen_t salen, int flags, int gethost, int getserv);
int aar_getnameinfo_done(aar_t *aar, aar_query_t* q, char *ret_host, size_t hostlen, char *ret_serv, size_t servlen);

aar_query_t* aar_getnext(aar_t *aar);
int aar_getnqueries(aar_t *aar);

void aar_cancel(aar_t *aar, aar_query_t* q);

void aar_freeaddrinfo(struct addrinfo *ai);

int aar_isdone(aar_t *aar, aar_query_t*q);

void aar_setuserdata(aar_t *aar, aar_query_t *q, void *userdata);
void* aar_getuserdata(aar_t *aar, aar_query_t *q);

#endif
