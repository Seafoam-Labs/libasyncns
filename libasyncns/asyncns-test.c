/* $Id$ */

/***
  This file is part of libasyncns.
 
  libasyncns is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.
 
  libasyncns is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.
 
  You should have received a copy of the GNU Lesser General Public
  License along with libasyncns; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "asyncns.h"

int main(int argc, char *argv[]) {
    asyncns_t* asyncns = NULL;
    asyncns_query_t *q1, *q2;
    int r = 1, ret;
    struct addrinfo *ai, hints;
    struct sockaddr_in sa;
    char host[NI_MAXHOST] = "", serv[NI_MAXSERV] = "";

    if (!(asyncns = asyncns_new(10))) {
        fprintf(stderr, "asyncns_new() failed\n");
        goto fail;
    }

    /* Make a name -> address query */
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    q1 = asyncns_getaddrinfo(asyncns, argc >= 2 ? argv[1] : "www.heise.de", NULL, &hints);


    /* Make an address -> name query */
    
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(argc >= 3 ? argv[2] : "193.99.144.71");
    sa.sin_port = htons(80);
    
    q2 = asyncns_getnameinfo(asyncns, (struct sockaddr*) &sa, sizeof(sa), 0, 1, 1); 


    /* Wait until the two queries are completed */
    
    while (!asyncns_isdone(asyncns, q1) || !asyncns_isdone(asyncns, q2)) {
        if (asyncns_wait(asyncns, 1) < 0)
            goto fail;
    }

    /* Interpret the result of the name -> addr query */
    if ((ret = asyncns_getaddrinfo_done(asyncns, q1, &ai)))
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

        asyncns_freeaddrinfo(ai);
    }

    /* Interpret the result of the addr -> name query */
    if ((ret = asyncns_getnameinfo_done(asyncns, q2, host, sizeof(host), serv, sizeof(serv))))
        fprintf(stderr, "error: %s %i\n", gai_strerror(ret), ret);
    else
        printf("%s -- %s\n", host, serv);
    
    r = 0;

fail:

    if (asyncns)
        asyncns_free(asyncns);
    
    return r;
}
