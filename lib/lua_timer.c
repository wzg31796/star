#include <inttypes.h>
#include "star.h"


extern Star *star;
extern int SIZEINT;
extern int SIZEPTR;


static int timer_id;


static int
l_timeout(lua_State *L)
{
	char lua_timer_id[9];
	char *data;
	int delay = luaL_checkinteger(L, 1);
	int iterations = 1;   // default

	if (lua_gettop(L) == 3)
		iterations = luaL_checkinteger(L, 3);

	lua_getfield(L, LUA_REGISTRYINDEX, "star_timer");

	// get an no used timer_id
	while(1) {
		bzero(lua_timer_id, 9);
		timer_id = timer_id%99999999 + 1;

		sprintf(lua_timer_id, "%d", timer_id);

		lua_pushstring(L, lua_timer_id);
		lua_gettable(L, -2);
		if (lua_type(L, -1) == LUA_TNIL)
			break;
		else
			lua_pop(L, 1);
	}

	lua_pop(L, 1); // pop nil

	lua_pushstring(L, lua_timer_id);
	lua_pushvalue(L, 2); 
	lua_settable(L, -3); 		// star_timer[lua_timer_id] = callback

	//[id, delay, iterations]
	data = malloc(SIZEINT*3);

	memcpy(data, 				&timer_id, 		SIZEINT);
	memcpy(data + SIZEINT, 		&delay, 		SIZEINT);
	memcpy(data + SIZEINT*2, 	&iterations, 	SIZEINT);

	qpush(star->timer->queue, STAR_TIMER_CREATE, data, SIZEINT*3);
	lua_pushstring(L, lua_timer_id);
	return 1;
}


static int
l_cancel(lua_State *L)
{
	int timer_id;
	char *data;
	const char *id = luaL_checkstring(L, 1);

	lua_getfield(L, LUA_REGISTRYINDEX, "star_timer");
	lua_pushstring(L, id);
	lua_pushnil(L);
	lua_settable(L, -3);

	timer_id = strtoimax(id, NULL, 10);
	
	//[id]
	data = malloc(SIZEINT);
	memcpy(data, &timer_id, SIZEINT);
	qpush(star->timer->queue, STAR_TIMER_CANCEL, data, SIZEINT);
	return 0;
}


int
l_mode_timer(lua_State* L)
{
	static const struct luaL_Reg l[] = {
		{"timeout", l_timeout},
		{"cancel", l_cancel},
	    {NULL, NULL}
	};

    luaL_newlib(L, l);
    return 1;
}
