#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "star_proc.h"
#include "lua_star.h"
#include "star_seri.h"
#include "star_main.h"

extern Star *star;

Process *
new_proc()
{
	Process *p = malloc(sizeof(Process));
	p->L = luaL_newstate();
	p->queue = q_initialize();
	
	return p;
}


void
run_mainproc(Process *p, char *file)
{
	lua_State *L = p->L;

	// mian.lua will run in main coroutine
	lua_State *mainCo = lua_newthread(L);

	luaL_openlibs(L);
	luaL_requiref(L, "star.core", l_mode_core, 0);
	lua_pop(L, 1);

	if (luaL_loadfile(mainCo, file) != 0 ) {
		luaL_error(L, "Load %s error: %s", file, lua_tostring(L, -1));	
	}

	lua_resume(mainCo, L, 0);

	bool b;
	lua_State *coL;
	char *cmd;
	void *data;
	int sz;
	int nunpack;

	for (;;) {

		usleep(10);
		b = qpop(p->queue, &coL, &cmd, &data, &sz);

		if (b) {
			if (sz > 0)
				nunpack = star_unpack(coL, data, sz);
			else
				nunpack = 0;

			lua_resume(coL, L, nunpack);
		}
	}

}


static void *
l_thread(void *arg)
{
	Process *proc = (Process *)arg;
	lua_State *L = proc->L;
	
	// lua lib
	luaL_openlibs(L);
	luaL_requiref(L, "star.core", l_mode_core, 0);
	lua_pop(L, 1);

	if (lua_pcall(L, 0, 0, 0) != 0) {
		fprintf(stderr, "Run func error: %s\n", lua_tostring(L, -1));
	}

	bool b;
	lua_State *coL;
	char *cmd;
	void *data;
	int sz;

	int npack;
	int nret;

	Queue *main_q = star->main->queue;

	for (;;) {
		usleep(10);
		b = qpop(proc->queue, &coL, &cmd, &data, &sz);
		if (b) {

			lua_getglobal(L, cmd);
			free(cmd);

			npack = 0;
			if (sz > 0)
				npack = star_unpack(L, data, sz);

			lua_call(L, npack, LUA_MULTRET);

			nret = lua_gettop(L);
			if (nret > 0) {
				star_pack(L, &data, &sz, 0);
				qpush(main_q, coL, NULL, data, sz);
				lua_pop(L, nret);
			}
		}
	}

	return NULL;
}


void
run_funcproc(Process *p, char *file)
{
	pthread_t thread;

	if (luaL_loadfile(p->L, file) != 0 ) {
		luaL_error(p->L, "error starting thread: %s", lua_tostring(p->L, -1));	
	}

	if (pthread_create(&thread, NULL, l_thread, p) != 0)
		luaL_error(p->L, "unable to create new state");

	pthread_detach(thread);
}


void
free_proc(Process *p)
{
	qfree(p->queue);
	lua_close(p->L);
	free(p);
}
