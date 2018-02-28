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
	printf("main   thread:  %s\n", conf->main);
	printf("func   thread:  %s\n", conf->func);
	printf("\n");
}


void
free_conf(Conf *conf)
{
	free(conf->main);
	free(conf->func);
	free(conf);
}