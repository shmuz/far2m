#ifndef LUAFAR_FARLIBS_H
#define LUAFAR_FARLIBS_H

int far_InMyConfig(lua_State *L);
int far_InMyCache(lua_State *L);
int far_InMyTemp(lua_State *L);
int far_GetMyHome(lua_State *L);
int far_SudoClientRegion(lua_State *L);

#endif
