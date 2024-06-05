#ifndef LUAFAR_BIT64_H
#define LUAFAR_BIT64_H

#include <lua.h>

int  bit64_getvalue(lua_State *L, int pos, int64_t *target);
int  bit64_push(lua_State *L, int64_t v);
int64_t check64(lua_State *L, int pos, int* success);
int  luaopen_bit64 (lua_State *L);

#endif

