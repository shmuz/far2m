#include <utils.h>
#include <sudo.h>

const char SudoClientRegionType[] = "SudoClientRegion";

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

static SudoClientRegion ** CheckSudoClientRegion(lua_State* L, int pos)
{
	return (SudoClientRegion **)luaL_checkudata(L, pos, SudoClientRegionType);
}

static int SudoClientRegion_Free(lua_State *L)
{
	SudoClientRegion **scr = CheckSudoClientRegion(L, 1);
	if (*scr) {
		delete(*scr);
		*scr = NULL;
	}
	return 0;
}

static int SudoClientRegion_tostring (lua_State *L)
{
	SudoClientRegion **scr = CheckSudoClientRegion(L, 1);
	if (*scr)
		lua_pushfstring(L, "%s (%p)", SudoClientRegionType, *scr);
	else
		lua_pushfstring(L, "%s (closed)", SudoClientRegionType);
	return 1;
}

int far_NewSudoClientRegion(lua_State *L)
{
	SudoClientRegion **scr = (SudoClientRegion**) lua_newuserdata(L, sizeof(SudoClientRegion*));
	*scr = new SudoClientRegion();
	luaL_getmetatable(L, SudoClientRegionType);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		luaL_newmetatable(L, SudoClientRegionType);
		lua_pushcfunction(L, SudoClientRegion_Free);
		lua_setfield(L, -2, "__gc");
		lua_pushcfunction(L, SudoClientRegion_tostring);
		lua_setfield(L, -2, "__tostring");
	}
	lua_setmetatable(L, -2);
	return 1;
}

int far_DeleteSudoClientRegion(lua_State *L)
{
	return SudoClientRegion_Free(L);
}

} // extern "C"
