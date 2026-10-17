#ifndef LUAFAR_FARLIBS_H
#define LUAFAR_FARLIBS_H

int far_InMyConfig(lua_State *L);
int far_InMyCache(lua_State *L);
int far_InMyTemp(lua_State *L);
int far_GetMyHome(lua_State *L);
int far_SudoCRCall(lua_State *L);

int wrap_sdc_utimens(const char *filename, const struct timespec times[2]);
void Push_MB2Wide(lua_State *L, const char *src, int len);
void Push_Wide2MB(lua_State *L, const wchar_t *src, int len);

#endif
