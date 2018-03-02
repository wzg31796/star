#ifndef STAR_MAIN_H
#define STAR_MAIN_H

#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "star_conf.h"
#include "star_proc.h"

#define STAR_VERSION 0.02
/*
	0.01：base
	0.02: add tcp suport, abstract star_queue
	will do:
		add timer suport
		add udp suport
*/

// #define STAR_DEBUG 0
#define STAR_CALL 1
#define STAR_RETURN 2
#define STAR_REGTIMER 3
#define STAR_TIMEOUT 4
#define STAR_SOCKET 5


typedef struct
{
	Conf *conf;
	int n;
	int i;
	pthread_t server;
	Process *main;
	Process *func[];
} Star;



Star *
new_star(Conf *conf);


Queue *
next_queue();


void
free_star(Star *star);




#endif