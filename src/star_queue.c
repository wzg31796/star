#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "star_queue.h"

#define CHECKNULL(x) ;if(x == NULL) { perror("Malloc failed."); exit(1); }

#define LQUEUE 128
#define LDEFAULT_SLOT 128

#define EMPTY 0
#define FILLED 1

struct _message{
	_Atomic char flag;		// 0 mean empty, 1 mean has data
	int sz;
	lua_State *L;
	char *cmd;
	void *arg;
};

struct _queue{
	Message *data[LQUEUE];
	_Atomic unsigned long long int readindex;
	_Atomic unsigned long long int writeindex;
};


Queue *
q_initialize()
{
	Queue * q 		= malloc(sizeof(Queue)) CHECKNULL(q)
	q->readindex 	= 0;
	q->writeindex 	= 0;

	Message *m = NULL;

	for (int i = 0; i < LQUEUE; ++i)
	{
		m 			 = malloc(sizeof(Message)) CHECKNULL(m)
		m->flag 	 = EMPTY;
		m->cmd 		 = NULL;
		m->arg 	 	 = NULL;
		m->sz 		 = 0;
		q->data[i] = m;
	}

	return q;
}


bool
qpush(Queue *queue, lua_State *L, char *cmd, void *arg, int length)
{
	int index;					
	int count = queue->writeindex - queue->readindex + 1; //2047	-	0    2048     2047 时超载

	if (count == LQUEUE) {
		fprintf(stderr, "queue overload... %llu %llu\n", queue->writeindex, queue->readindex);
		return false;
	}

	if ((count+1)%100 == 0)
		fprintf(stderr, "waring: queue length: %d\n", count+1);

	index = atomic_fetch_add(&queue->writeindex, 1);
	index = index%LQUEUE;
	
	Message *m = queue->data[index];
	m->L 	= L;
	m->cmd  = cmd;
	m->arg  = arg;
	m->sz   = length;
	m->flag = FILLED;

	return true;
}


bool
qpop(Queue *queue, lua_State **L, char**cmd, void **arg ,int *length)
{
	int index = queue->readindex % LQUEUE;

	Message *m = queue->data[index];

	if (m->flag == EMPTY) {
		return false;
	} else {
		*L   = m->L;
		*cmd = m->cmd;
		*arg = m->arg;
		*length = m->sz;
		m->flag = EMPTY;
		queue->readindex++;
		return true;
	}
}


void
qfree(Queue *queue)
{  
	for (int i = 0; i < LQUEUE; ++i)
	{
		free(queue->data[i]);
	}
	free(queue);
}