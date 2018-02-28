#ifndef STAR_MAIN_H
#define STAR_MAIN_H

#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "star_conf.h"
#include "star_proc.h"

#define STAR_VERSION 0.01


typedef struct
{
	Conf *conf;
	int n;
	int i;
	Process *main;
	Process *proc[];
} Star;



Star *
new_star(Conf *conf);


Queue *
next_queue();


void
free_star(Star *star);




#endif