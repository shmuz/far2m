#include <utils.h>
#include <sudo.h>

const char SudoClientRegionType[] = "SudoClientRegion";

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

static const luaL_Reg SudoClientRegion_methods[] = {
	{ "Close",        SudoClientRegion_Free },
	{ "__gc",         SudoClientRegion_Free },
	{ "__tostring",   SudoClientRegion_tostring },
	{ NULL, NULL },
};

int far_SudoClientRegion(lua_State *L)
{
	SudoClientRegion **scr = (SudoClientRegion**) lua_newuserdata(L, sizeof(SudoClientRegion*));
	*scr = new SudoClientRegion();
	luaL_getmetatable(L, SudoClientRegionType);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		luaL_newmetatable(L, SudoClientRegionType);
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		luaL_register(L, NULL, SudoClientRegion_methods);
	}
	lua_setmetatable(L, -2);
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

} // extern "C"
