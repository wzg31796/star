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
	// printf(">> co1 %p size:%d\n", co, lua_gettop(co));

	int r = lua_resume(co, NULL, n);

	// printf("=================== resume over r: %d %p\n", r, co);

	if (r == 0) {
		// lua_getfield(L, LUA_REGISTRYINDEX, "star_co");
		// lua_pushlightuserdata(L, (void *)co);
		// lua_pushnil(L);
		// lua_settable(L, -3);
		// printf("-----------> lua_settable over\n");
		// lua_pop(L, 1);
	} else {
		if (r != 1)
			fprintf(stderr, "Error resume coroutine :%s\n", lua_tostring(co, -1));
		lua_settop(co, 0);
	}

	// printf(">> co2 %p size:%d\n", co, lua_gettop(co));
}

void
run_mainproc(Process *p, char *file)
{
	lua_State *L = p->L;

	// init
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "star_co");

	if (luaL_loadfile(L, file) != 0 ) {
		luaL_error(L, "Load %s error: %s", file, lua_tostring(L, -1));	
	}

	luaL_openlibs(L);
	luaL_requiref(L, "star.core", l_mode_core, 0);
	lua_pop(L, 1);
	luaL_requiref(L, "star.sock", l_mode_sock, 0);
	lua_pop(L, 1);

	if (lua_pcall(L, 0, 0, 0) != 0) {
		fprintf(stderr, "Run %s error: %s\n", file, lua_tostring(L, -1));
	}

	bool b;
	unsigned char type;
	char *data;
	uint32_t size;

	lua_State *coL;
	void *arg;
	int sz;
	int nunpack;

	// server
	unsigned char sock_type;
	int fd;
	int len_data;
	char *ip;
	int port;

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

				// printf("unpack 之前 %d\n", lua_gettop(coL));
				lua_settop(coL, 0);
				if (sz > 0)
					nunpack = star_unpack(coL, arg, sz);
				else
					nunpack = 0;

				// printf("will resume %p %d  %d\n", coL, lua_gettop(coL), nunpack);
				resume_coroutine(coL, L, nunpack);
				break;
			}
			case STAR_SOCKET: {
				coL = new_coroutine(L);
				sock_type = data[0];
				fd = *(int *)(data+1);
				lua_getfield(coL, LUA_REGISTRYINDEX, socket_callback[sock_type]);
				lua_pushinteger(coL, fd);

				switch(sock_type)
				{
				case SOCKET_OPEN: {
					//[type,fd,port,ip]
					port = *(int *)(data+1+SIZEINT);
					ip = data+1+2*SIZEINT;
					lua_pushstring(coL, ip);
					lua_pushinteger(coL, port);
					resume_coroutine(coL, L, 3);
					break;
				}
				case SOCKET_DATA: {
					len_data = data[1+SIZEINT] * 256 + data[2+SIZEINT];
					lua_pushlstring(coL, data+3+SIZEINT, len_data);
					// printf("will resume sock_data %p %d\n", coL, lua_gettop(coL));
					resume_coroutine(coL, L, 2);
					break;
				}
				case SOCKET_CLOSE: {
					resume_coroutine(coL, L, 1);
					break;
				}
				default:
					fprintf(stderr, "Invalid sock_type:%d\n", sock_type);
					break;
				}
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
