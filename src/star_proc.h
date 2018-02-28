#ifndef STAR_PROCESS_H
#define STAR_PROCESS_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "star_queue.h"

typedef struct
{
	lua_State *L;
	Queue *queue;
} Process;



Process *
new_proc();


void
run_mainproc(Process *p, char *file);


void
run_funcproc(Process *p, char *file);


void
free_proc(Process *p);

#endif