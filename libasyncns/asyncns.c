#define _GNU_SOURCE
#define HAVE_PR_SET_PDEATHSIG
#define HAVE_SETRESUID

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>

#ifdef HAVE_PR_SET_PDEATHSIG
#include <sys/prctl.h>
#endif

#include "asyncns.h"

#define MAX_WORKERS 16
#define MAX_QUERIES 256
#define BUFSIZE (10240)

typedef enum {
    REQUEST_ADDRINFO,
    RESPONSE_ADDRINFO,
    REQUEST_NAMEINFO,
    RESPONSE_NAMEINFO
} query_type_t;

struct aar {
    int in_fd, out_fd;

    pid_t workers[MAX_WORKERS];

    unsigned current_id, current_index;
    aar_query_t* queries[MAX_QUERIES];

    aar_query_t *done_head, *done_tail;

    int n_queries;
};

struct aar_query {
    aar_t *aar;
    int done;
    unsigned id;
    query_type_t type;
    aar_query_t *done_next, *done_prev;
    int ret;
    struct addrinfo *addrinfo;
    char *serv, *host;
    void *userdata;
};

typedef struct rheader {
    query_type_t type;
    unsigned id;
    size_t length;
} rheader_t;

typedef struct addrinfo_request {
    struct rheader header;
    int hints_is_null;
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    size_t node_len, service_len;
} addrinfo_request_t;

typedef struct addrinfo_response {
    struct rheader header;
    int ret;
} addrinfo_response_t;

typedef struct addrinfo_serialization {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    size_t ai_addrlen;
    size_t canonname_len;
} addrinfo_serialization_t;

typedef struct nameinfo_request {
    struct rheader header;
    int flags;
    socklen_t sockaddr_len;
    int gethost, getserv;
} nameinfo_request_t;

typedef struct nameinfo_response {
    struct rheader header;
    size_t hostlen, servlen;
    int ret;
} nameinfo_response_t;

static int fd_nonblock(int fd) {
    int i;
    assert(fd >= 0);

    if ((i = fcntl(fd, F_GETFL)) < 0)
        return -1;

    if (i & O_NONBLOCK)
        return 0;

    return fcntl(fd, F_SETFL, i | O_NONBLOCK);
}

static void *serialize_addrinfo(void *p, const struct addrinfo *ai, size_t *length, size_t maxlength) {
    addrinfo_serialization_t *s = p;
    size_t cnl, l;
    assert(p);
    assert(ai);
    assert(length);
    assert(*length <= maxlength);

    cnl = (ai->ai_canonname ? strlen(ai->ai_canonname)+1 : 0);
    l = sizeof(addrinfo_serialization_t) + ai->ai_addrlen + cnl;

    if (*length + l > maxlength)
        return NULL;

    s->ai_flags = ai->ai_flags;
    s->ai_family = ai->ai_family;
    s->ai_socktype = ai->ai_socktype;
    s->ai_protocol = ai->ai_protocol;
    s->ai_addrlen = ai->ai_addrlen;
    s->canonname_len = cnl;

    memcpy((uint8_t*) p + sizeof(addrinfo_serialization_t), ai->ai_addr, ai->ai_addrlen);

    if (ai->ai_canonname)
        strcpy((char*) p + sizeof(addrinfo_serialization_t) + ai->ai_addrlen, ai->ai_canonname);

    *length += l;
    return (uint8_t*) p + l;
}

static int send_addrinfo_reply(int out_fd, unsigned id, int ret, struct addrinfo *ai) {
    assert(out_fd >= 0);
    uint8_t data[BUFSIZE];
    addrinfo_response_t *resp = (addrinfo_response_t*) data;

    resp->header.type = RESPONSE_ADDRINFO;
    resp->header.id = id;
    resp->header.length = sizeof(addrinfo_response_t);
    resp->ret = ret;

    if (ret == 0 && ai) {
        void *p = data + sizeof(addrinfo_response_t);

        while (ai && p) {
            p = serialize_addrinfo(p, ai, &resp->header.length, BUFSIZE);
            ai = ai->ai_next;
        }
    }

    return send(out_fd, resp, resp->header.length, 0);
}

static int send_nameinfo_reply(int out_fd, unsigned id, int ret, const char *host, const char *serv) {
    assert(out_fd >= 0);
    uint8_t data[BUFSIZE];
    size_t hl, sl;
    nameinfo_response_t *resp = (nameinfo_response_t*) data;

    sl = serv ? strlen(serv)+1 : 0;
    hl = host ? strlen(host)+1 : 0;

    resp->header.type = RESPONSE_NAMEINFO;
    resp->header.id = id;
    resp->header.length = sizeof(nameinfo_response_t) + hl + sl;
    resp->ret = ret;
    resp->hostlen = hl;
    resp->servlen = sl;

    assert(sizeof(data) >= resp->header.length);
    
    if (host)
        memcpy(data + sizeof(nameinfo_response_t), host, hl);

    if (serv)
        memcpy(data + sizeof(nameinfo_response_t) + hl, serv, sl);
    
    return send(out_fd, resp, resp->header.length, 0);
}

static int handle_request(int out_fd, const rheader_t *req, size_t length) {
    assert(out_fd >= 0);
    assert(req);
    assert(length >= sizeof(rheader_t));
    assert(length == req->length);

    switch (req->type) {
        case REQUEST_ADDRINFO: {
            struct addrinfo ai, *result = NULL;
            const addrinfo_request_t *ai_req = (addrinfo_request_t*) req;
            const char *node, *service;
            int ret;

            assert(length >= sizeof(addrinfo_request_t));
            assert(length == sizeof(addrinfo_request_t) + ai_req->node_len + ai_req->service_len);

            memset(&ai, 0, sizeof(ai));
            ai.ai_flags = ai_req->ai_flags;
            ai.ai_family = ai_req->ai_family;
            ai.ai_socktype = ai_req->ai_socktype;
            ai.ai_protocol = ai_req->ai_protocol;

            node = ai_req->node_len ? (const char*) req + sizeof(addrinfo_request_t) : NULL;
            service = ai_req->service_len ? (const char*) req + sizeof(addrinfo_request_t) + ai_req->node_len : NULL;

            ret = getaddrinfo(node, service,
                              ai_req->hints_is_null ? NULL : &ai,
                              &result);
            ret = send_addrinfo_reply(out_fd, req->id, ret, result);

            if (result)
                freeaddrinfo(result);

            return ret;
        }

        case REQUEST_NAMEINFO: {
            int ret;
            const nameinfo_request_t *ni_req = (nameinfo_request_t*) req;
            char hostbuf[NI_MAXHOST], servbuf[NI_MAXSERV];
            const struct sockaddr *sa;
            
            assert(length >= sizeof(nameinfo_request_t));
            assert(length == sizeof(nameinfo_request_t) + ni_req->sockaddr_len);

            sa = (const struct sockaddr*) ((const char*) req + sizeof(nameinfo_request_t));
            
            ret = getnameinfo(sa, ni_req->sockaddr_len,
                              ni_req->gethost ? hostbuf : NULL, ni_req->gethost ? sizeof(hostbuf) : 0,
                              ni_req->getserv ? servbuf : NULL, ni_req->getserv ? sizeof(servbuf) : 0,
                              ni_req->flags);

            return send_nameinfo_reply(out_fd, req->id, ret,
                                       ret == 0 && ni_req->gethost ? hostbuf : NULL,
                                       ret == 0 && ni_req->getserv ? servbuf : NULL);
        }

        default:
            ;
    }

    return 0;
}

static int worker(int in_fd, int out_fd) {
    int r = 0;
    assert(in_fd > 2);
    assert(out_fd > 2);
    
    close(0);
    close(1);
    close(2);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);

    chdir("/");

    if (geteuid() == 0) {
        struct passwd *pw;

        if ((pw = getpwnam("nobody"))) {
#ifdef HAVE_SETRESUID            
            setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid);
#elif HAVE_SETREUID
            setuid(pw->pw_uid, pw->pw_uid);
#else
            setuid(pw->pw_uid);
            seteuid(pw->pw_uid);
#endif
        }
    }

    signal(SIGTERM, SIG_DFL);

    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);

#ifdef HAVE_PR_SET_PDEATHSIG
    prctl(PR_SET_PDEATHSIG, SIGTERM);
#else
    fd_nonblock(in_fd);
#endif
    
    while (getppid() != 1) { /* if the parent PID is 1 our parent process died. */
        char buf[BUFSIZE];
        ssize_t length;

#ifndef HAVE_PR_SET_PDEATHSIG
        fd_set fds;
        struct timeval tv = { 0, 500000 } ;

        FD_ZERO(&fds);
        FD_SET(in_fd, &fds);

        if (select(in_fd+1, &fds, NULL, NULL, &tv) < 0)
            goto fail;

        if (getppid() == 1)
            break;
#endif
        
        if ((length = recv(in_fd, buf, sizeof(buf), 0)) <= 0) {

            if (length < 0 && errno == EAGAIN)
                continue;
            
            goto fail;
        }

        if (handle_request(out_fd, (rheader_t*) buf, (size_t) length) < 0)
            goto fail;
    }

fail:

    close(in_fd);
    close(out_fd);
    
    return r;
}

aar_t* aar_new(int n_proc) {
    aar_t *aar = NULL;
    int fd1[2] = { -1, -1 }, fd2[2] = { -1, -1 }, p;

    assert(n_proc >= 1);

    if (!(aar = malloc(sizeof(aar_t))))
        goto fail;

    aar->in_fd = aar->out_fd = -1;
    memset(aar->workers, 0, sizeof(aar->workers));
    
    if (socketpair(PF_UNIX, SOCK_DGRAM, 0, fd1) < 0)
        goto fail;

    if (socketpair(PF_UNIX, SOCK_DGRAM, 0, fd2) < 0)
        goto fail;

    if (n_proc > MAX_WORKERS)
        n_proc = MAX_WORKERS;

    for (p = 0; p < n_proc; p++) {

        if ((aar->workers[p] = fork()) < 0)
            goto fail;
        else if (aar->workers[p] == 0) {
            close(fd1[0]);
            close(fd2[1]);
            _exit(worker(fd2[0], fd1[1]));
        }
    }

    close(fd2[0]);
    close(fd1[1]);
    aar->in_fd = fd1[0];
    aar->out_fd = fd2[1];

    aar->current_index = aar->current_id = 0;

    for (p = 0; p < MAX_QUERIES; p++)
        aar->queries[p] = NULL;
/*     memset(aar->queries, 0, sizeof(aar->queries)); */

    aar->done_head = aar->done_tail = NULL;
    aar->n_queries = 0;

    fd_nonblock(aar->in_fd);

    return aar;

fail:

    if (fd1[0] >= 0) close(fd1[0]);
    if (fd1[1] >= 0) close(fd1[1]);
    if (fd2[0] >= 0) close(fd2[0]);
    if (fd2[1] >= 0) close(fd2[1]);

    if (aar) 
        aar_free(aar);

    return NULL;
}

void aar_free(aar_t *aar) {
    int p;
    assert(aar);
    
    if (aar->in_fd >= 0)
        close(aar->in_fd);
    if (aar->out_fd >= 0)
        close(aar->out_fd);
    
    for (p = 0; p < MAX_WORKERS; p++)
        if (aar->workers[p] >= 1) {
            kill(aar->workers[p], SIGTERM);
            waitpid(aar->workers[p], NULL, 0);
        }

    for (p = 0; p < MAX_QUERIES; p++)
        if (aar->queries[p])
            aar_cancel(aar, aar->queries[p]);
    
    free(aar);
}

int aar_fd(aar_t *aar) {
    assert(aar);

    return aar->in_fd;
}

static aar_query_t *lookup_query(aar_t *aar, unsigned id) {
    aar_query_t *q;
    assert(aar);

    if ((q = aar->queries[id % MAX_QUERIES]))
        if (q->id == id)
            return q;

    return NULL;
}

static void complete_query(aar_t *aar, aar_query_t *q) {
    assert(aar);
    assert(q);
    assert(!q->done);

    q->done = 1;
    
    if ((q->done_prev = aar->done_tail))
        aar->done_tail->done_next = q;
    else
        aar->done_head = q;

    aar->done_tail = q;
    q->done_next = NULL;
}

static void *unserialize_addrinfo(void *p, struct addrinfo **ret_ai, size_t *length) {
    addrinfo_serialization_t *s = p;
    size_t l;
    struct addrinfo *ai;
    assert(p);
    assert(ret_ai);
    assert(index);

    if (*length < sizeof(addrinfo_serialization_t))
        return NULL;

    l = sizeof(addrinfo_serialization_t) + s->ai_addrlen + s->canonname_len;
    if (*length < l)
        return NULL;

    if (!(ai = malloc(sizeof(struct addrinfo))))
        goto fail;
    
    ai->ai_addr = NULL;
    ai->ai_canonname = NULL;
    ai->ai_next = NULL;

    if (s->ai_addrlen && !(ai->ai_addr = malloc(s->ai_addrlen)))
        goto fail;
    
    if (s->canonname_len && !(ai->ai_canonname = malloc(s->canonname_len)))
        goto fail;

    ai->ai_flags = s->ai_flags;
    ai->ai_family = s->ai_family;
    ai->ai_socktype = s->ai_socktype;
    ai->ai_protocol = s->ai_protocol;
    ai->ai_addrlen = s->ai_addrlen;

    if (ai->ai_addr)
        memcpy(ai->ai_addr, (uint8_t*) p + sizeof(addrinfo_serialization_t), s->ai_addrlen);

    if (ai->ai_canonname)
        memcpy(ai->ai_canonname, (uint8_t*) p + sizeof(addrinfo_serialization_t) + s->ai_addrlen, s->canonname_len);

    *length -= l;
    *ret_ai = ai;
    
    return (uint8_t*) p + l;


fail:
    if (ai)
        aar_freeaddrinfo(ai);

    return NULL;
}

static int handle_response(aar_t *aar, rheader_t *resp, size_t length) {
    aar_query_t *q;
    assert(aar);
    assert(resp);
    assert(length >= sizeof(rheader_t));
    assert(length == resp->length);

    if (!(q = lookup_query(aar, resp->id)))
        return 0;
    
    switch (resp->type) {
        case RESPONSE_ADDRINFO: {
            const addrinfo_response_t *ai_resp = (addrinfo_response_t*) resp;
            void *p;
            size_t l;
            struct addrinfo *prev = NULL;

            assert(length >= sizeof(addrinfo_response_t));
            assert(q->type == REQUEST_ADDRINFO);

            q->ret = ai_resp->ret;
            l = length - sizeof(addrinfo_response_t);
            p = (uint8_t*) resp + sizeof(addrinfo_response_t);

            while (l > 0 && p) {
                struct addrinfo *ai = NULL;
                p = unserialize_addrinfo(p, &ai, &l);

                if (!ai)
                    break;

                if (prev)
                    prev->ai_next = ai;
                else
                    q->addrinfo = ai;

                prev = ai;
            }

            complete_query(aar, q);
            break;
        }

        case RESPONSE_NAMEINFO: {
            const nameinfo_response_t *ni_resp = (nameinfo_response_t*) resp;

            assert(length >= sizeof(nameinfo_response_t));
            assert(q->type = REQUEST_NAMEINFO);

            q->ret = ni_resp->ret;

            if (ni_resp->hostlen)
                q->host = strndup((char*) ni_resp + sizeof(nameinfo_response_t), ni_resp->hostlen-1);

            if (ni_resp->servlen)
                q->serv = strndup((char*) ni_resp + sizeof(nameinfo_response_t) + ni_resp->hostlen, ni_resp->servlen-1);
                    

            complete_query(aar, q);
            break;
        }
            
        default:
            ;
    }
    
    return 0;
}

int aar_wait(aar_t *aar, int block) {
    assert(aar);

    for (;;) {
        char buf[BUFSIZE];
        ssize_t l;

        if (((l = recv(aar->in_fd, buf, sizeof(buf), 0)) < 0)) {
            fd_set fds;

            if (errno != EAGAIN)
                return -1;

            if (!block)
                return 0;
                
            FD_ZERO(&fds);
            FD_SET(aar->in_fd, &fds);

            if (select(aar->in_fd+1, &fds, NULL, NULL, NULL) < 0)
                return -1;
                
            continue;
        }


        return handle_response(aar, (rheader_t*) buf, (size_t) l);
    }
}

static aar_query_t *alloc_query(aar_t *aar) {
    aar_query_t *q;
    assert(aar);

    if (aar->n_queries >= MAX_QUERIES)
        return NULL;

    while (aar->queries[aar->current_index]) {

        aar->current_index++;
        aar->current_id++;

        while (aar->current_index >= MAX_QUERIES)
            aar->current_index -= MAX_QUERIES;
    }
        
    if (!(q = aar->queries[aar->current_index] = malloc(sizeof(aar_query_t))))
        return NULL;

    aar->n_queries++;

    q->aar = aar;
    q->done = 0;
    q->id = aar->current_id;
    q->done_next = q->done_prev = NULL;
    q->ret = 0;
    q->addrinfo = NULL;
    q->userdata = NULL;
    q->host = q->serv = NULL;

    return q;
}

aar_query_t* aar_getaddrinfo(aar_t *aar, const char *node, const char *service, const struct addrinfo *hints) {
    uint8_t data[BUFSIZE];
    addrinfo_request_t *req = (addrinfo_request_t*) data;
    aar_query_t *q;
    assert(aar);
    assert(node || service);

    if (!(q = alloc_query(aar)))
        return NULL;

    memset(req, 0, sizeof(addrinfo_request_t));
    
    req->node_len = node ? strlen(node)+1 : 0;
    req->service_len = service ? strlen(service)+1 : 0;
    
    req->header.id = q->id;
    req->header.type = q->type = REQUEST_ADDRINFO;
    req->header.length = sizeof(addrinfo_request_t) + req->node_len + req->service_len;

    if (req->header.length > BUFSIZE)
        goto fail;

    if (!(req->hints_is_null = !hints)) {
        req->ai_flags = hints->ai_flags; 
        req->ai_family = hints->ai_family;
        req->ai_socktype = hints->ai_socktype;
        req->ai_protocol = hints->ai_protocol;
    }

    if (node)
        strcpy((char*) req + sizeof(addrinfo_request_t), node);

    if (service)
        strcpy((char*) req + sizeof(addrinfo_request_t) + req->node_len, service);

    if (send(aar->out_fd, req, req->header.length, 0) < 0)
        goto fail;
    
    return q;

fail:
    if (q)
        aar_cancel(aar, q);

    return NULL;
}

int aar_getaddrinfo_done(aar_t *aar, aar_query_t* q, struct addrinfo **ret_res) {
    int ret;
    assert(aar);
    assert(q);
    assert(q->aar == aar);
    assert(q->type == REQUEST_ADDRINFO);

    if (!q->done)
        return EAI_AGAIN;

    *ret_res = q->addrinfo;
    q->addrinfo = NULL;
    ret = q->ret;
    aar_cancel(aar, q);

    return ret;
}

aar_query_t* aar_getnameinfo(aar_t *aar, const struct sockaddr *sa, socklen_t salen, int flags, int gethost, int getserv) {
    uint8_t data[BUFSIZE];
    nameinfo_request_t *req = (nameinfo_request_t*) data;
    aar_query_t *q;

    assert(aar);
    assert(sa);
    assert(salen > 0);

    if (!(q = alloc_query(aar)))
        return NULL;

    memset(req, 0, sizeof(nameinfo_request_t));
    
    req->header.id = q->id;
    req->header.type = q->type = REQUEST_NAMEINFO;
    req->header.length = sizeof(nameinfo_request_t) + salen;

    if (req->header.length > BUFSIZE)
        goto fail;

    req->flags = flags;
    req->sockaddr_len = salen;
    req->gethost = gethost;
    req->getserv = getserv;

    memcpy((uint8_t*) req + sizeof(nameinfo_request_t), sa, salen);

    if (send(aar->out_fd, req, req->header.length, 0) < 0)
        goto fail;
    
    return q;

fail:
    if (q)
        aar_cancel(aar, q);

    return NULL;
}

int aar_getnameinfo_done(aar_t *aar, aar_query_t* q, char *ret_host, size_t hostlen, char *ret_serv, size_t servlen) {
    int ret;
    assert(aar);
    assert(q);
    assert(q->aar == aar);
    assert(q->type == REQUEST_NAMEINFO);
    assert(!ret_host || hostlen);
    assert(!ret_serv || servlen);

    if (!q->done)
        return EAI_AGAIN;

    if (ret_host && q->host) {
        strncpy(ret_host, q->host, hostlen);
        ret_host[hostlen-1] = 0;
    }

    if (ret_serv && q->serv) {
        strncpy(ret_serv, q->serv, servlen);
        ret_serv[servlen-1] = 0;
    }
    
    ret = q->ret;
    aar_cancel(aar, q);

    return ret;
}

aar_query_t* aar_getnext(aar_t *aar) {
    assert(aar);
    return aar->done_head;
}

int aar_getnqueries(aar_t *aar) {
    assert(aar);
    return aar->n_queries;
}

void aar_cancel(aar_t *aar, aar_query_t* q) {
    int i;
    assert(aar);
    assert(q);
    assert(q->aar == aar);
    assert(aar->n_queries > 0);

    if (q->done) {

        if (q->done_prev)
            q->done_prev->done_next = q->done_next;
        else 
            aar->done_head = q->done_next;

        if (q->done_next)
            q->done_next->done_prev = q->done_prev;
        else
            aar->done_tail = q->done_prev;
    }


    i = q->id % MAX_QUERIES;
    assert(aar->queries[i] == q);
    aar->queries[i] = NULL;

    aar_freeaddrinfo(q->addrinfo);
    free(q->addrinfo);
    free(q->host);
    free(q->serv);

    aar->n_queries--;
    free(q);
}

void aar_freeaddrinfo(struct addrinfo *ai) {
    if (!ai)
        return;

    while (ai) {
        struct addrinfo *next = ai->ai_next;

        free(ai->ai_addr);
        free(ai->ai_canonname);
        free(ai);

        ai = next;
    }
}

int aar_isdone(aar_t *aar, aar_query_t*q) {
    assert(aar);
    assert(q);
    assert(q->aar == aar);

    return q->done;
}

void aar_setuserdata(aar_t *aar, aar_query_t *q, void *userdata) {
    assert(q);
    assert(aar);
    assert(q->aar = aar);
    
    q->userdata = userdata;
}

void* aar_getuserdata(aar_t *aar, aar_query_t *q) {
    assert(q);
    assert(aar);
    assert(q->aar = aar);

    return q->userdata;
}


