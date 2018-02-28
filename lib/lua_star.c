#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>

#include "lua_star.h"
#include "star_main.h"
#include "star_seri.h"

extern Star *star;


static inline void
star_send(lua_State *L)
{
	int n = lua_gettop(L);
	void *arg = NULL;
	int sz = 0;

	const char *cmd = luaL_checkstring(L, 1);
	char *q_cmd = malloc(strlen(cmd) + 1);
	strcpy(q_cmd, cmd);

	if (n > 1)
		star_pack(L, &arg, &sz, 1);

	Queue *q = next_queue();
	qpush(q, L, q_cmd, arg, sz);
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
	double ti = luaL_checknumber(L, 1);
	usleep(ti*1000);
	return 0;
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


