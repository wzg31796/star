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
#include "star_timer.h"

#define STAR_VERSION 0.03


#define STAR_DEBUG 0         // none <-> none  # Used in the next version

#define STAR_SOCK_OPEN 1	 // main <- tcp/udp
#define STAR_SOCK_DATA 2	 // main <- tcp/udp
#define STAR_SOCK_CLOSE 3	 // main <- tcp/udp

#define STAR_CALL 4			 // main -> func
#define STAR_RETURN 5		 // main <- func 

#define STAR_SLEEP 6		 // main -> timer
#define STAR_WAKE 7			 // main <- timer
#define STAR_TIMER_CREATE 8	 // main -> timer
#define STAR_TIMER_CANCEL 9  // main -> timer
#define STAR_TIMER_PAUSE 10  // main -> timer  # Used in the next version
#define STAR_TIMER_RESUME 11 // main -> timer  # Used in the next version
#define STAR_TIMEOUT 12		 // main <- timer



typedef struct
{
	Conf *conf;
	int n;
	int i;
	pthread_t server;
	Timer *timer;
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