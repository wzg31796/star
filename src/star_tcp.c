#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>

#include "star.h"
#include "star_tcp.h"

#define MAXBUF 1024
#define MAXEPOLLSIZE star->conf->maxclient + 1
#define MQUEUE star->main->queue




extern Star *star;
extern int SIZEINT;
extern int SIZEPTR;

static int listener;

int setnonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0); 
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
        return -1;
    return 0;
}

int handle_message(int new_fd)
{
    char buf[MAXBUF + 1];
    int len;
    bzero(buf, MAXBUF + 1);

    char *data;

    len = recv(new_fd, buf, MAXBUF, 0);
    if (len > 0) {

        //[fd, str_sz(2), str]
        data = malloc(SIZEINT + 2 + len);

        memcpy(data, &new_fd, SIZEINT);
        data[SIZEINT]     = len/256;
        data[SIZEINT+1]   = len%256;
        memcpy(data + SIZEINT +2, buf, len);
        qpush(MQUEUE, STAR_SOCK_DATA, data, SIZEINT + 2 + len);
    }
    else {
        // printf("接受消息失败! 错误代码是%d, 错误信息是'%s'\n", errno, strerror(errno));
        close(new_fd);

        //[fd]
        data = malloc(SIZEINT);
        memcpy(data, &new_fd, SIZEINT);
        qpush(MQUEUE, STAR_SOCK_CLOSE, data, SIZEINT);
        return -1;
    }
    return len;
}



void star_tcp_stop(int signo)
{  
    if (listener != 0)
        close(listener); 

    printf("tcp thread exit\n");
    exit(0);  
}  


void *
star_thread_tcp(void *arg)
{
    int new_fd, kdpfd, nfds, n, ret, curfds;
    socklen_t len;
    struct sockaddr_in my_addr, their_addr;
    unsigned int lisnum;
    struct epoll_event ev;
    struct epoll_event events[MAXEPOLLSIZE];
    struct rlimit rt;
    lisnum = 2;

    char *data;
    int size;

    star->server = pthread_self();

    rt.rlim_max = rt.rlim_cur = MAXEPOLLSIZE;
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1) {
        perror("setrlimit");
        exit(1);
    }

    if ((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    setnonblocking(listener);
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = PF_INET;
    my_addr.sin_port = htons(star->conf->port);
    my_addr.sin_addr.s_addr = inet_addr(star->conf->ip);
    if (bind(listener, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(listener, lisnum) == -1) {
        perror("listen");
        exit(1);
    } else
        printf("Tcp server listen on: '%s:%d'\n", star->conf->ip, star->conf->port);

    kdpfd = epoll_create(MAXEPOLLSIZE);
    len = sizeof(struct sockaddr_in);

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listener;
    if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, listener, &ev) < 0) {
        fprintf(stderr, "epoll set insertion error: fd = %d\n", listener);
        return NULL;
    }

    curfds = 1;

    signal(SIGQUIT, star_tcp_stop);
    char *ip;
    int port;
    while(1) {

        nfds = epoll_wait(kdpfd, events, curfds, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        }

        for (n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == listener) {
                new_fd = accept(listener, (struct sockaddr *)&their_addr, &len);
                if (new_fd < 0) {
                    perror("accept");
                    continue;
                } else {
                    ip = inet_ntoa(their_addr.sin_addr);
                    port = ntohs(their_addr.sin_port);

                    //[fd,port,ip]
                    size = SIZEINT * 2 + strlen(ip) + 1;
                    data = malloc(size);

                    memcpy(data, &new_fd, SIZEINT);
                    memcpy(data+SIZEINT, &port, SIZEINT);
                    strcpy(data+SIZEINT*2, ip);
                    qpush(MQUEUE, STAR_SOCK_OPEN, data, size);
                    // printf("有连接来自于:%s:%d, 分配的socket为:%d\n",
                    //     inet_ntoa(their_addr.sin_addr), ntohs(their_addr.sin_port), new_fd);
                }

                setnonblocking(new_fd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = new_fd;
                if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, new_fd, &ev) < 0) {
                    fprintf(stderr, "把 socket '%d' 加入 epoll 失败!%s\n", new_fd, strerror(errno));
                    return NULL;
                }
                curfds++;
            } else {
                ret = handle_message(events[n].data.fd);
                if (ret < 1 && errno != 11) {
                    // printf("从epoll中移除 对%d的监听\n", events[n].data.fd);
                    epoll_ctl(kdpfd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
                    curfds--;
                }
            }
        } 
    }

    close(listener);
    return NULL;
}


void
tcp_thread_run()
{
    pthread_t thread;

    if (pthread_create(&thread, NULL, star_thread_tcp, NULL) != 0)
        perror("unable to create tcp thread");

    pthread_detach(thread);
}
