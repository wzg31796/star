#include <sys/socket.h>
#include "star.h"

extern Star *star;



static int
l_send(lua_State *L)
{
	int fd, r;
	size_t sz;
	const char *data;

	fd = luaL_checkinteger(L, 1);
	data = luaL_checklstring(L, 2, &sz);

	r = send(fd, data, (int)sz, 0);

	lua_pushinteger(L, r);
	return 1;
}



int
l_mode_sock(lua_State* L)
{
	static const struct luaL_Reg l[] = {
		{"send", l_send},
	    {NULL, NULL}
	};

    luaL_newlib(L, l);
    return 1;
}


