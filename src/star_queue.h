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
qpush(Queue *queue, unsigned char type, char *data, uint32_t size);


bool
qpop(Queue *queue, unsigned char *type, char **data, uint32_t *size);


void
qfree(Queue *queue);



#endif