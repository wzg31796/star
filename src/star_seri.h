#ifndef LUA_SERIALIZE_H
#define LUA_SERIALIZE_H

#include <lua.h>
#include "star_queue.h"



void
star_pack(lua_State *L, void **arg, int *sz, int index);


int
star_unpack(lua_State *L, void *arg, int sz);


int
star_unpack_to_table(lua_State *L, void *arg, int sz, int index);


#endif
