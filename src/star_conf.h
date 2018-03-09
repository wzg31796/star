#ifndef STAR_CONF_H
#define STAR_CONF_H

typedef struct
{
	int nthread;
	char *main;
	char *func;
	
	char *server;
	short family;
	char *ip;
	int port;
	int maxclient;
} Conf;



Conf *
new_conf();


void
dump_conf(Conf *conf);


void
free_conf(Conf *conf);


#endif