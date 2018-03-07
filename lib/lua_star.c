#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>

#include "star.h"
#include "lua_star.h"
#include "star_seri.h"
#include "star_tcp.h"

extern Star *star;

extern int SIZEINT;
extern int SIZEPTR;

static int
l_server(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	lua_pushstring(L, "open");
	lua_gettable(L, -2);
	lua_setfield(L, LUA_REGISTRYINDEX, "socket_open");

	lua_pushstring(L, "data");
	lua_gettable(L, -2);
	lua_setfield(L, LUA_REGISTRYINDEX, "socket_data");

	lua_pushstring(L, "close");
	lua_gettable(L, -2);
	lua_setfield(L, LUA_REGISTRYINDEX, "socket_close");

	lua_settop(L, 0);

	return 0;
}



static int
l_xcall(lua_State *L)
{
	Queue *q;
	char *data;
	int size;

	void *arg;
	int sz = 0;

	int n;
	char one;   // one return?

	if (lua_type(L,1) == LUA_TNUMBER) {
		one = 1;
		n = lua_rawlen(L, 2);
	} else {
		one = 0;
		n = lua_rawlen(L, 1);
	}

	//[one, L, n, count, arg, sz, ...]
	char *xbuf = malloc(1 + SIZEPTR + SIZEINT*2 + (SIZEPTR+SIZEINT)*n);
	char *ptr = xbuf;

	xbuf[0] = one;
	ptr += 1;

	memcpy(ptr, &L, SIZEPTR);
	ptr += SIZEPTR;

	memcpy(ptr, &n, SIZEINT);
	ptr += SIZEINT;

	memcpy(ptr, &sz, SIZEINT); // count = 0;
	ptr += SIZEINT;

	const char *cmd;
	int sub_n;
	int index;
	int i_task;

	for (i_task = 1; i_task <=n; ++i_task)
	{
		// init
		q = next_queue();
		arg = NULL;
		sz = 0;

		lua_rawgeti(L, -1, i_task); 		// push element
		sub_n = lua_rawlen(L, -1);
		lua_rawgeti(L, -1, 1); 		        // element[1]   #cmd
		cmd = lua_tostring(L, -1);
		lua_pop(L, 1);

		index = lua_gettop(L);

		if (sub_n > 1) {
			for (int i = 2; i <= sub_n; ++i)
			{
				lua_rawgeti(L, -1-(i-2), i);
			}
			star_pack(L, &arg, &sz, index);
		}

		//[xbuf, index, arg, sz, cmd]
		size = SIZEPTR*2 + SIZEINT*2 + strlen(cmd) + 1;
		data = malloc(size);
		memcpy(data, 					&xbuf, 	 SIZEPTR);
		memcpy(data+SIZEPTR,    		&i_task, SIZEINT);
		memcpy(data+SIZEINT+SIZEPTR, 	&arg, 	 SIZEPTR);
		memcpy(data+SIZEINT+SIZEPTR*2, 	&sz, 	 SIZEINT);
		strcpy(data+SIZEPTR*2+SIZEINT*2, cmd);
		qpush(q, STAR_XCALL, data, size);
		if (one)
			lua_settop(L, 2);
		else
			lua_settop(L, 1);
	}
	return lua_yield(L, 0);
}


static inline void
star_send(lua_State *L)
{
	int proc;
	const char *cmd;
	void *arg = NULL;
	int sz = 0;
	Queue *q;

	int n = lua_gettop(L);

	// whata func thread's queue?
	if (lua_type(L, 1) == LUA_TNUMBER) {

		proc = lua_tointeger(L, 1);
		proc = proc%star->conf->nthread;
		q = star->func[proc]->queue;

		cmd = luaL_checkstring(L, 2);
		if (n > 2)
			star_pack(L, &arg, &sz, 2);
	} else {

		q = next_queue();
		cmd = luaL_checkstring(L, 1);
		if (n > 1)
			star_pack(L, &arg, &sz, 1);
	}


	//[L, arg, sz, cmd]
	uint32_t size = SIZEPTR * 2 + SIZEINT + strlen(cmd) + 1;
	char *buf = malloc(size);
	char *ptr = buf;

	memcpy(ptr, &L, SIZEPTR);
	ptr += SIZEPTR;

	memcpy(ptr, &arg, SIZEPTR);
	ptr += SIZEPTR;

	memcpy(ptr, &sz, SIZEINT);
	ptr += SIZEINT;

	strcpy(ptr, cmd);

	qpush(q, STAR_CALL, buf, size);
}


static int
l_call(lua_State *L)
{
	star_send(L);
	return lua_yield(L, 0);
}


static int
l_send(lua_State *L)
{
	star_send(L);
	return 0;
}


static int
l_sleep(lua_State *L)
{
	int ti = luaL_checkinteger(L, 1);

	if (L == star->main->L) {
		usleep(ti*1000);
		return 0;
	}

	Queue *timer_queue = star->timer->queue;
	char *data = malloc(SIZEPTR + SIZEINT);

	//[L, delay]
	memcpy(data, &L, SIZEPTR);
	memcpy(data + SIZEPTR, &ti, SIZEINT);

	qpush(timer_queue, STAR_SLEEP, data, SIZEPTR + SIZEINT);

	return lua_yield(L, 0);
}


static int
l_time(lua_State *L)
{   
    struct timeval start;
    gettimeofday( &start, NULL );
    lua_pushinteger(L, 1000*start.tv_sec + start.tv_usec/1000);
    return 1;  /* number of results */
}


static int
l_version(lua_State *L)
{   
    lua_pushnumber(L, STAR_VERSION);
    return 1;
}


int
l_mode_core(lua_State* L)
{
	static const struct luaL_Reg l[] = {
		{"server", l_server},
		{"xcall", l_xcall},
		{"call", l_call},
		{"send", l_send},
		{"sleep", l_sleep},
		{"time", l_time},
		{"version",l_version},
	    {NULL, NULL}
	};

    luaL_newlib(L, l);
    return 1;
}


