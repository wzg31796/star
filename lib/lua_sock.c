#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "star.h"

extern Star *star;
extern int SIZEINT;
extern int SIZEPTR;

extern int udp_listener;



static void
tcp_send(lua_State *L)
{
	int fd, r;
	size_t sz;
	const char *data;

	fd = luaL_checkinteger(L, 1);
	data = luaL_checklstring(L, 2, &sz);

	r = send(fd, data, (int)sz, 0);

	lua_pushinteger(L, r);
}


static void
udp_send(lua_State *L)
{
	int r;
	struct sockaddr_in si_other;
	char *ip;
	int port;
	char *from;
	size_t sz;
	const char *data;

	from = (char *)lua_touserdata(L, 1);
	data = luaL_checklstring(L, 2, &sz);

	port = *(int *)from;
	ip = from + SIZEINT;
	
	si_other.sin_family 	  = star->conf->family;
	si_other.sin_addr.s_addr  = inet_addr(ip);
	si_other.sin_port 		  = htons(port);

	r = sendto(udp_listener, data, sz, 0, (struct sockaddr*) &si_other, sizeof(si_other));
	lua_pushinteger(L, r);
}


static int
l_send(lua_State *L)
{
	int r;
	if (lua_type(L, 1) == LUA_TNUMBER)
		tcp_send(L);
	else
		udp_send(L);

	return 1;
}


static int
l_udp_address(lua_State *L)
{
	char *from = (char *)lua_touserdata(L, 1);
	int port = *(int *)from;

	lua_pushstring(L, from + SIZEINT);
	lua_pushinteger(L, port);
	return 2;
}



int
l_mode_sock(lua_State* L)
{
	static const struct luaL_Reg l[] = {
		{"udp_address", l_udp_address},
		{"send", l_send},
	    {NULL, NULL}
	};

    luaL_newlib(L, l);
    return 1;
}


