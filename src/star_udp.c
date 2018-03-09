#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "star.h"
 
#define MAXBUF 1024
#define MQUEUE star->main->queue

extern Star *star;
extern int SIZEINT;
extern int SIZEPTR;


int udp_listener;
 


void die
(char *s)
{
    perror(s);
    exit(1);
}


void
star_udp_stop(int signo)
{  
    if (udp_listener != 0)
        close(udp_listener); 

    printf("udp thread exit\n");
    exit(0);  
}  


void *
star_thread_udp(void *_arg)
{
    struct sockaddr_in si_me, si_other;

    socklen_t slen = sizeof(si_other); 
    int recv_len;
    char buf[MAXBUF];

    char *ip;
    int port;

    int size;
    char *data;
    char *ptr;

    signal(SIGQUIT, star_udp_stop);
    star->server = pthread_self();
    
    if ((udp_listener=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = star->conf->family;
    si_me.sin_port = htons(star->conf->port);
    si_me.sin_addr.s_addr = inet_addr(star->conf->ip);
     

    if( bind(udp_listener , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }
    
    printf("Udp server listen on: '%s:%d'\n", star->conf->ip, star->conf->port);

    while(1)
    {
        if ((recv_len = recvfrom(udp_listener, buf, MAXBUF, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom");
        }

        ip = inet_ntoa(si_other.sin_addr);
        port = ntohs(si_other.sin_port);

        //[port, ip, (buf_len 2) + buf]
        size = SIZEINT + strlen(ip) + 1 + 2 + recv_len;
        data = malloc(size);
        ptr = data;

        memcpy(ptr, &port, SIZEINT);
        ptr += SIZEINT;

        strcpy(ptr, ip);
        ptr += (strlen(ip) + 1);

        ptr[0]   = recv_len/256;
        ptr[1]   = recv_len%256;
        ptr += 2;

        memcpy(ptr, buf, recv_len);

        qpush(MQUEUE, STAR_SOCK_UDP_DATA, data, size);
    }
 
    close(udp_listener);
    return NULL;
}


void
udp_thread_run()
{
    pthread_t thread;

    if (pthread_create(&thread, NULL, star_thread_udp, NULL) != 0)
        perror("unable to create tcp thread");

    pthread_detach(thread);
}