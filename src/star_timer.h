#ifndef STAR_TIMER_H
#define STAR_TIMER_H


typedef struct _sleepnode SleepNode;
typedef struct _timernode TimerNode;

typedef struct _sleeplist SleepList;
typedef struct _timerlist TimerList;


typedef struct
{
	pthread_t thread;
	Queue *queue;
	SleepList *sleeplist;
	TimerList *timerlist;
	uint64_t starttime;
} Timer;



Timer *
timer_new();


void
timer_free();


void
timer_thread_run(Timer *timer);



#endif