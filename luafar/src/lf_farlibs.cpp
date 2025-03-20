#include <utils.h>
#include <sudo.h>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>

extern int traceback (lua_State *L);

int far_InMyConfig(lua_State *L)
{
	const char *subpath = luaL_optstring(L, 1, NULL);
	int create_path = lua_toboolean(L, 2);
	const std::string &ret = InMyConfig(subpath, create_path);
	lua_pushstring(L, ret.c_str());
	return 1;
}

int far_InMyCache(lua_State *L)
{
	const char *subpath = luaL_optstring(L, 1, NULL);
	int create_path = lua_toboolean(L, 2);
	const std::string &ret = InMyCache(subpath, create_path);
	lua_pushstring(L, ret.c_str());
	return 1;
}

int far_InMyTemp(lua_State *L)
{
	const char *subpath = luaL_optstring(L, 1, NULL);
	const std::string &ret = InMyTemp(subpath);
	lua_pushstring(L, ret.c_str());
	return 1;
}

int far_GetMyHome(lua_State *L)
{
	const std::string &ret = GetMyHome();
	lua_pushstring(L, ret.c_str());
	return 1;
}

int far_SudoCRCall(lua_State *L) // sudo client region call
{
	luaL_checktype(L, 1, LUA_TFUNCTION);

	const int TRACE_POS = 1;
	lua_pushcfunction(L, traceback);
	lua_insert(L, TRACE_POS);

	SudoClientRegion scr;
	if ( !lua_pcall(L, lua_gettop(L)-2, LUA_MULTRET, TRACE_POS) ) {
		lua_pushboolean(L, 1);
		lua_replace(L, TRACE_POS);
		return lua_gettop(L);
	}
	lua_pushboolean(L, 0);
	lua_pushvalue(L, -2);
	return 2;
}

int wrap_sdc_utimens(const char *filename, const struct timespec times[2])
{
	return sdc_utimens(filename, times);
}

} // extern "C"
