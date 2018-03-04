#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "star.h"
#include "star_proc.h"
#include "star_seri.h"
#include "star_tcp.h"

#include "lua_star.h"
#include "lua_sock.h"
#include "lua_timer.h"

extern Star *star;
extern int SIZEPTR;
extern int SIZEINT;

const char *socket_callback[] = {"", "socket_open", "socket_data", "socket_close"};

Process *
new_proc()
{
	Process *p = malloc(sizeof(Process));
	p->L = luaL_newstate();
	p->queue = q_initialize();
	
	return p;
}


lua_State * new_coroutine(lua_State *L)
{
	lua_State *l = lua_newthread(L);
	lua_getfield(L, LUA_REGISTRYINDEX, "star_co");
	lua_pushlightuserdata(L, (void *)l);
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pop(L, 2);
	return l;
}

void resume_coroutine(lua_State *co, lua_State *L, int n)
{
	int r = lua_resume(co, NULL, n);

	if (r == 0) {
		lua_getfield(L, LUA_REGISTRYINDEX, "star_co");
		lua_pushlightuserdata(L, (void *)co);
		lua_pushnil(L);
		lua_settable(L, -3);
		lua_pop(L, 1);
	} else {
		if (r != 1)
			fprintf(stderr, "Error resume coroutine :%s\n", lua_tostring(co, -1));
		lua_settop(co, 0);
	}
}


void main_lua_init(lua_State *L, char *file)
{
	// init
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "star_co");
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "star_timer");

	if (luaL_loadfile(L, file) != 0 ) {
		luaL_error(L, "Load %s error: %s", file, lua_tostring(L, -1));	
	}

	luaL_openlibs(L);
	luaL_requiref(L, "star.core", l_mode_core, 0);
	lua_pop(L, 1);
	luaL_requiref(L, "star.sock", l_mode_sock, 0);
	lua_pop(L, 1);
	luaL_requiref(L, "star.timer", l_mode_timer, 0);
	lua_pop(L, 1);

	if (lua_pcall(L, 0, 0, 0) != 0) {
		fprintf(stderr, "Run %s error: %s\n", file, lua_tostring(L, -1));
	}
}


void
run_mainproc(Process *p, char *file)
{
	lua_State *L = p->L;

	main_lua_init(L, file);

	bool b;
	unsigned char type;
	char *data;
	uint32_t size;

	lua_State *coL;
	void *arg;
	int sz;
	int nunpack;

	// server
	int fd;
	int len_data;
	char *ip;
	int port;

	// timer
	int timer_id;
	char over;
	char lua_timer_id[9];

	for (;;) {

		usleep(10);
		b = qpop(p->queue, &type, &data, &size);

		if (b) {
			switch(type)
			{
			case STAR_RETURN: {
				//[L, arg, sz]
				coL = *(lua_State **)(data);
				arg = *(void **)(data+SIZEPTR);
				sz = *(int *)(data + SIZEPTR * 2);

				free(data);

				// lua_settop(coL, 0);
				if (sz > 0)
					nunpack = star_unpack(coL, arg, sz);
				else
					nunpack = 0;

				resume_coroutine(coL, L, nunpack);
				break;
			}
			case STAR_SOCK_OPEN: {
				coL = new_coroutine(L);
				fd = *(int *)(data);
				port = *(int *)(data+SIZEINT);
				ip = data+2*SIZEINT;

				lua_getfield(coL, LUA_REGISTRYINDEX, socket_callback[STAR_SOCK_OPEN]);
				lua_pushinteger(coL, fd);
				lua_pushstring(coL, ip);
				lua_pushinteger(coL, port);
				free(data);
				resume_coroutine(coL, L, 3);
				break;
			}
			case STAR_SOCK_DATA: {
				//[fd, str_sz(2), str]
				coL = new_coroutine(L);
				fd = *(int *)(data);
				len_data = data[SIZEINT] * 256 + data[SIZEINT+1];

				lua_getfield(coL, LUA_REGISTRYINDEX, socket_callback[STAR_SOCK_DATA]);
				lua_pushinteger(coL, fd);
				lua_pushlstring(coL, data+2+SIZEINT, len_data);
				free(data);
				resume_coroutine(coL, L, 2);
				break;
			}
			case STAR_SOCK_CLOSE: {
				coL = new_coroutine(L);
				fd = *(int *)(data);

				free(data);
				lua_getfield(coL, LUA_REGISTRYINDEX, socket_callback[STAR_SOCK_CLOSE]);
				lua_pushinteger(coL, fd);
				resume_coroutine(coL, L, 1);
				break;
			}
			case STAR_WAKE: {
				//[L]
				coL = *(lua_State **)(data);
				free(data);
				resume_coroutine(coL, L, 0);
				break;
			}
			case STAR_TIMEOUT: {
				//[id, over]
				timer_id = *(int *)data;
				over = data[SIZEINT];

				free(data);
				bzero(lua_timer_id, 9);
				sprintf(lua_timer_id, "%d", timer_id);

				lua_getfield(L, LUA_REGISTRYINDEX, "star_timer");
				lua_pushstring(L, lua_timer_id);
				lua_gettable(L, -2);

				if(lua_pcall(L, 0, 0, 0) != 0) {
					fprintf(stderr, "timeout call error: %s\n", lua_tostring(L, -1));
					lua_pop(L, 1);
				}
				if (over) {
					lua_pushstring(L, lua_timer_id);
					lua_pushnil(L);
					lua_settable(L, -3);
				}
				lua_pop(L, 1);	// pop table(star_timer)
				break;
			}
			default:
				break;
			}
		}
	}

}


static void *
l_thread(void *_arg)
{
	Process *proc = (Process *)_arg;
	lua_State *L = proc->L;
	
	// lua lib
	luaL_openlibs(L);
	luaL_requiref(L, "star.core", l_mode_core, 0);
	lua_pop(L, 1);
	luaL_requiref(L, "star.sock", l_mode_sock, 0);
	lua_pop(L, 1);

	if (lua_pcall(L, 0, 0, 0) != 0) {
		fprintf(stderr, "Run func error: %s\n", lua_tostring(L, -1));
	}

	bool b;
	unsigned char type;
	char *data;
	char *ptr;
	uint32_t size;

	lua_State *coL;
	char *cmd;
	void *arg;
	int sz;

	int npack;
	int nret;

	Queue *main_q = star->main->queue;

	for (;;) {
		usleep(10);
		b = qpop(proc->queue, &type, &data, &size);
		if (b) {

			//[L, arg, sz, cmd]
			coL = *(lua_State **)data;
			arg = *(void **)(data+SIZEPTR);
			sz = *(int *)(data + SIZEPTR * 2);
			cmd = data + SIZEPTR * 2 + SIZEINT;

			lua_getglobal(L, cmd);

			npack = 0;
			if (sz > 0)
				npack = star_unpack(L, arg, sz);

			lua_call(L, npack, LUA_MULTRET);

			nret = lua_gettop(L);
			if (nret > 0) {
				star_pack(L, &arg, &sz, 0);

				//[L, arg, sz]
				size = SIZEPTR * 2 + SIZEINT;
				data = realloc(data, size);
				ptr = data;

				memcpy(ptr, &coL, SIZEPTR);
				ptr += SIZEPTR;

				memcpy(ptr, &arg, SIZEPTR);
				ptr += SIZEPTR;

				memcpy(ptr, &sz, SIZEINT);
				ptr += SIZEINT;

				qpush(main_q, STAR_RETURN, data, size);
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
