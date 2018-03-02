#include <stdio.h>
#include <stdlib.h>
#include "star_conf.h"


Conf *
new_conf()
{
	Conf *conf = malloc(sizeof(Conf));
	return conf;
}


void
dump_conf(Conf *conf)
{	
	printf("\n");
	printf("*************************************\n");
	printf("*             star conf             *\n");
	printf("*************************************\n");
	printf("thread number:  %d\n", conf->nthread);
	printf("main   thread:  '%s'\n", conf->main);
	printf("func   thread:  '%s'\n", conf->func);
	printf("\n");
	printf("server       :  '%s'\n", conf->server);
	printf("ip           :  '%s'\n", conf->ip);
	printf("port         :  %d\n", conf->port);
	printf("maxclient    :  %d\n", conf->maxclient);
	printf("\n");
}


void
free_conf(Conf *conf)
{
	free(conf->main);
	free(conf->func);
	free(conf);
}