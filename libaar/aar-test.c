#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "aar.h"

int main(int argc, char *argv[]) {
    aar_t* aar = NULL;
    aar_query_t *q1, *q2;
    int r = 1, ret;
    struct addrinfo *ai, hints;
    struct sockaddr_in sa;
    char host[NI_MAXHOST] = "", serv[NI_MAXSERV] = "";

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (!(aar = aar_new(5))) {
        fprintf(stderr, "aar_new() failed\n");
        goto fail;
    }


    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr("192.168.50.1");
    sa.sin_port = htons(80);
    
    q1 = aar_getaddrinfo(aar, argc >= 2 ? argv[1] : "www.heise.de", NULL, &hints);
    q2 = aar_getnameinfo(aar, (struct sockaddr*) &sa, sizeof(sa), 0, 1, 1); 

    while (!aar_isdone(aar, q1) || !aar_isdone(aar, q2)) {
        if (aar_wait(aar, 1) < 0)
            goto fail;
    }

    ret = aar_getaddrinfo_done(aar, q1, &ai);

    if (ret)
        fprintf(stderr, "error: %s %i\n", gai_strerror(ret), ret);
    else {
        struct addrinfo *i;

        for (i = ai; i; i = i->ai_next) {
            char t[256];
            const char *p = NULL;

            if (i->ai_family == PF_INET) 
                p = inet_ntop(AF_INET, &((struct sockaddr_in*) i->ai_addr)->sin_addr, t, sizeof(t));
            else if (i->ai_family == PF_INET6)
                p = inet_ntop(AF_INET6, &((struct sockaddr_in6*) i->ai_addr)->sin6_addr, t, sizeof(t));
                          
            printf("%s\n", p);
        }

        aar_freeaddrinfo(ai);
    }

    ret = aar_getnameinfo_done(aar, q2, host, sizeof(host), serv, sizeof(serv));

    if (ret)
        fprintf(stderr, "error: %s %i\n", gai_strerror(ret), ret);
    else {
        printf("%s -- %s\n", host, serv);
    }
    
    
    r = 0;

fail:

    if (aar)
        aar_free(aar);
    
    return r;
}
