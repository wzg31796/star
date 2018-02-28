#include "star_main.h"

Star *star;


Queue *
next_queue()
{
	Queue *q;
	star->i++;
	if (star->i == star->n)
		star->i = 0;
	q = star->proc[star->i]->queue;
	return q;
}

Star *
new_star(Conf *conf)
{
	Star *s = malloc(sizeof(Star) + conf->nthread * sizeof(Process *));

	s->conf = conf;
	s->n = conf->nthread;
	s->i = -1;

	s->main = new_proc();

	for (int i = 0; i < s->n; ++i)
	{
		s->proc[i] = new_proc();
	}

	return s;
}

void
free_star(Star *star)
{
	for (int i = 0; i < star->n; ++i)
	{
		free_proc(star->proc[i]);
	}

	free_proc(star->main);

	free_conf(star->conf);
	free(star);
}


void
read_conf(const char *file, Conf *conf)
{
	lua_State *L = luaL_newstate();

	if (luaL_loadfile(L, file) != 0 ) {
		lua_close(L);
	 	fprintf(stderr, "Load conf error from %s: %s", file, lua_tostring(L, -1));
	 	exit(1);
	}

	luaL_openlibs(L);

	if (lua_pcall(L, 0, 0, 0) != 0) {
		lua_close(L);
		fprintf(stderr, "Run conf error: %s\n", lua_tostring(L, -1));
		exit(1);
	}

	// thread number
	lua_getglobal(L, "thread");
	int n = (int)lua_tointeger(L, -1);
	if (n == 0) {n = 4;}
	conf->nthread = n;
	lua_pop(L, 1);

	// main thread
	lua_getglobal(L, "main");
	size_t sz;
	const char * main1 = luaL_checklstring(L, -1, &sz);
	char *main2 = malloc(sz + 1);
	strcpy(main2, main1);
	conf->main = main2;
	lua_pop(L, 1);

	// func thread
	lua_getglobal(L, "func");
	const char * func1 = luaL_checklstring(L, -1, &sz);
	char *func2 = malloc(sz + 1);
	strcpy(func2, func1);
	conf->func = func2;

	lua_close(L);
}


void
star_run()
{
	// printf("run......\n");
	for (int i = 0; i < star->n; ++i)
	{
		run_funcproc(star->proc[i], star->conf->func);
	}

	run_mainproc(star->main, star->conf->main);
}


void
start(Conf *conf)
{
	dump_conf(conf);
	star = new_star(conf);
	star_run();
}


int
main(int argc, char const *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: star main.lua\n");
		return 1;
	}

	Conf *conf = new_conf();
	read_conf(argv[1], conf);

	start(conf);

	free_star(star);

	return 0;
}