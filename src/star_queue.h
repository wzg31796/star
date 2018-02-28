#ifndef STAR_QUEUE_H
#define STAR_QUEUE_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <stdbool.h>

typedef struct _message Message;
typedef struct _queue Queue;



Queue *
q_initialize();


bool
qpush(Queue *queue, lua_State *L, char *cmd, void *arg, int length);


bool
qpop(Queue *queue, lua_State **L, char**cmd, void **arg, int *length);


void
qfree(Queue *queue);



#endif