#include <utils.h>
#include <sudo.h>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>

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

int far_NewSudoClientRegion(lua_State *L)
{
	lua_pushlightuserdata(L, new SudoClientRegion());
	return 1;
}

int far_DeleteSudoClientRegion(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	delete reinterpret_cast<SudoClientRegion*>(lua_touserdata(L,1));
	return 0;
}

} // extern "C"
