#include <assert.h>
#include <sys/time.h>
#include <pthread.h>

#include "star.h"
#include "star_queue.h"
#include "star_timer.h"

#define MQUEUE star->main->queue

extern Star *star;
extern int SIZEINT;
extern int SIZEPTR;

// sleep list
struct _sleepnode
{
	lua_State *L;
	int delay;
	SleepNode *next;
};

struct _sleeplist
{
	SleepNode *head;
	SleepNode *tail;
};


// timer list
struct _timernode
{
	int id;
	int delay;
	int iterations;
	int dt;
	TimerNode *next;
};

struct _timerlist
{
	TimerNode *head;
	TimerNode *tail;
};


// util func
SleepList *
sleep_list_new()
{
	SleepList * sl = malloc(sizeof(SleepList));
	sl->head = NULL;
	sl->tail = NULL;
	return sl;
}

TimerList *
timer_list_new()
{
	TimerList * tl = malloc(sizeof(TimerList));
	tl->head = NULL;
	tl->tail = NULL;
	return tl;
}


void
sleep_list_free(SleepList *sl)
{
	SleepNode *temp, *head;
	head = sl->head;

	while (head) {
		temp = head->next;
		free(head);
		head = temp;
	}

	free(sl);
}


void
timer_list_free(TimerList *tl)
{
	TimerNode *temp, *head;
	head = tl->head;

	while(head) {
		temp = head->next;
		free(head);
		head = temp;
	}

	free(tl);
}

void
sleep_list_add(SleepList *sl, lua_State *L, int delay)
{
	SleepNode *sn;
	sn 			= malloc(sizeof(SleepNode));
	sn->L 		= L;
	sn->delay 	= delay;
	sn->next	= NULL;

	if (sl->tail) {
		sl->tail->next = sn;
		sl->tail = sn;
	} else {
		assert(sl->head == NULL);
		sl->head = sl->tail = sn;
	}
}

void
timer_list_add(TimerList *tl, int id, int delay, int iterations)
{
	TimerNode *tn;
	tn 				= malloc(sizeof(TimerNode));
	tn->id  		= id;
	tn->delay 		= delay;
	tn->iterations 	= iterations;
	tn->dt 			= 0;

	if (tl->tail) {
		tl->tail->next = tn;
		tl->tail = tn;
	} else {
		assert(tl->head == NULL);
		tl->head = tl->tail = tn;
	}
}

void
timer_list_remove(TimerList *list, int id)
{
	TimerNode *previous = NULL;
	TimerNode *node = list->head;

	while(node) {

		if (node->id == id)	{
			if (previous)
				previous->next = node->next;
			else
				list->head = node->next;

			if (node == list->tail)
				list->tail = previous;

			free(node);
		} else {
			previous = node;
			node = node->next;
		}	
	}
}


Timer *
timer_new()
{
	Timer *ti = malloc(sizeof(Timer));

	ti->thread 		= 0;
	ti->queue 		= q_initialize();
	ti->sleeplist 	= sleep_list_new();
	ti->timerlist 	= timer_list_new();
	ti->starttime 	= 0;

	return ti;
}


void timer_thread_stop(int signo)
{
	Timer *timer = star->timer;

	qfree(timer->queue);
	sleep_list_free(timer->sleeplist);
	timer_list_free(timer->timerlist);

	printf("timer thread exit\n");
	exit(0);
}


void
timer_free(Timer *timer)
{
	if (timer->thread > 0)
		pthread_kill(timer->thread, SIGQUIT);
}


static uint64_t
gettime() {
	uint64_t t;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	t = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return t;
}


// bool
// qpush(Queue *queue, unsigned char type, char *data, uint32_t size);

void 
sleep_tick(SleepList *sl, uint16_t dt)
{
	SleepNode *previous = NULL;
	SleepNode *temp;
	SleepNode *node = sl->head;

	char *data = malloc(SIZEPTR);
 	
	while(node) {
		node->delay -= dt;
		if (node->delay <= 0) {

			//[L]
			memcpy(data, &node->L, SIZEPTR);
			qpush(MQUEUE, STAR_WAKE, data, SIZEPTR);

			// remove this node
			if (previous)
				previous->next = node->next;
			else
				sl->head = node->next;

			if (node == sl->tail)
				sl->tail = previous;

			temp = node->next;
			free(node);
			node = temp;
		} else {
			previous = node;
			node = node->next;
		}
	}
}

void 
timer_tick(TimerList *tl, uint16_t dt)
{
	TimerNode *previous = NULL;
	TimerNode *temp;
	TimerNode *node = tl->head;

	char *data;
	char over = 0;

	while(node) {
		node->dt += dt;
		over = 0;
		if (node->dt >= node->delay) {
			node->dt -= node->delay;	// 超过部分, 通过缩短下一次timeout 来弥补

			if (node->iterations > 0) {
				node->iterations--;
				if (node->iterations == 0) {
	
					// this timer is done, now remove it
					over = 1;
					if (previous)
						previous->next = node->next;
					else
						tl->head = node->next;

					if (node == tl->tail)
						tl->tail = previous;

					temp = node; // used to free node
				}
			}

			//[id, over]
			data = malloc(SIZEINT + 1);
			memcpy(data, &node->id, SIZEINT);
			data[SIZEINT] = over;

			qpush(MQUEUE, STAR_TIMEOUT, data, SIZEINT + 1);
		}

		if (over == 1) {
			node = node->next;
			free(temp);
		}
		else {
			previous = node;
			node = node->next;
		}

	}
}


void *
star_thread_timer(void *_arg)
{
	signal(SIGQUIT, timer_thread_stop);


	Timer *timer = (Timer *)_arg;
	timer->thread = pthread_self();
	timer->starttime = gettime();

	uint64_t start_time;
	uint16_t dt;

	bool b;
	unsigned char type;
	char *data;
	uint32_t size;

	int id;
	int delay;
	int iterations;
	lua_State *L;

	for (;;)
	{
		start_time = gettime();
		usleep(1000);
		b = qpop(timer->queue, &type, &data, &size);
		if (b) {
			switch (type)
			{
			case STAR_SLEEP: {

				//[L, delay]
				L 		= *(lua_State **)(data);
				delay 	= *(int *)(data + SIZEPTR);

				free(data);
				sleep_list_add(timer->sleeplist, L, delay);
				break;
			}
			case STAR_TIMER_CREATE: {

				//[id, delay, iterations]
				id 			= *(int *)(data);
				delay 		= *(int *)(data + SIZEINT);
				iterations 	= *(int *)(data + SIZEINT*2);

				free(data);
				timer_list_add(timer->timerlist, id, delay, iterations);
				break;
			}
			case STAR_TIMER_CANCEL: {

				//[id]
				id = *(int *)data;
				
				free(data);
				timer_list_remove(timer->timerlist, id);
				break;
			}
			default:
				fprintf(stderr, "timer get invalid message type:%d\n", type);
				break;
			}
		}
		dt = gettime() - start_time;
		sleep_tick(timer->sleeplist, dt);
		timer_tick(timer->timerlist, dt);
	}

    return NULL;
}


void
timer_thread_run(Timer *timer)
{
    pthread_t thread;

    if (pthread_create(&thread, NULL, star_thread_timer, (void *)timer) != 0)
        perror("unable to create timer thread");

    pthread_detach(thread);
}

