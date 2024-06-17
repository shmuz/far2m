#ifndef LUAFAR_FARLIBS_H
#define LUAFAR_FARLIBS_H

int far_InMyConfig(lua_State *L);
int far_InMyCache(lua_State *L);
int far_InMyTemp(lua_State *L);
int far_GetMyHome(lua_State *L);
int far_SudoCRCall(lua_State *L);

int wrap_sdc_utimens(const char *filename, const struct timespec times[2]);

#endif
