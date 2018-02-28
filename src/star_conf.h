#ifndef STAR_CONF_H
#define STAR_CONF_H

typedef struct
{
	char *main;
	char *func;
	int nthread;
} Conf;



Conf *
new_conf();


void
dump_conf(Conf *conf);


void
free_conf(Conf *conf);


#endif