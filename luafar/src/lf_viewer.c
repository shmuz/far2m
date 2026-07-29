#include <windows.h>
#include <dirent.h> //opendir
#include <stdlib.h>
#include <ctype.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "far3parts.h"
#include "lf_bit64.h"
#include "lf_farlibs.h"
#include "lf_flags.h"
#include "lf_service.h"
#include "lf_string.h"
#include "lf_util.h"

static int viewer_SetKeyBar(lua_State *L)
{
	return SetKeyBar(L, FALSE);
}

static int push_viewer_filename(lua_State *L, int Id)
{
	int size = PSInfo.ViewerControlV2(Id, VCTL_GETFILENAME, NULL);
	if (!size) return 0;

	wchar_t* fname = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
	size = PSInfo.ViewerControlV2(Id, VCTL_GETFILENAME, fname);

	if (size)
	{
		push_utf8_string(L, fname, -1);
		lua_remove(L, -2);
		return 1;
	}

	lua_pop(L,1);
	return 0;
}

static int viewer_Viewer(lua_State *L)
{
	const wchar_t* FileName = check_utf8_string(L, 1, NULL);
	const wchar_t* Title    = opt_utf8_string(L, 2, NULL);
	int X1 = luaL_optinteger(L, 3, 0);
	int Y1 = luaL_optinteger(L, 4, 0);
	int X2 = luaL_optinteger(L, 5, -1);
	int Y2 = luaL_optinteger(L, 6, -1);
	int Flags = OptFlags(L, 7, 0);
	int CodePage = luaL_optinteger(L, 8, CP_AUTODETECT);
	int ret = PSInfo.Viewer(FileName, Title, X1, Y1, X2, Y2, Flags, CodePage);
	lua_pushboolean(L, ret);
	return 1;
}

static int viewer_GetFileName(lua_State *L)
{
	int viewerId = luaL_optinteger(L,1,-1);

	if (!push_viewer_filename(L, viewerId)) lua_pushnil(L);

	return 1;
}

static int viewer_GetInfo(lua_State *L)
{
	int ViewerId = luaL_optinteger(L, 1, -1);
	struct ViewerInfo vi = { sizeof(vi) };

	if (PSInfo.ViewerControlV2(ViewerId, VCTL_GETINFO, &vi))
	{
		lua_createtable(L, 0, 10);
		PutNumToTable(L, "ViewerID", vi.ViewerID);

		if (push_viewer_filename(L, ViewerId))
			lua_setfield(L, -2, "FileName");

		PutNumToTable(L,  "FileSize", (double) vi.FileSize);
		PutNumToTable(L,  "FilePos", (double) vi.FilePos);
		PutNumToTable(L,  "WindowSizeX", vi.WindowSizeX);
		PutNumToTable(L,  "WindowSizeY", vi.WindowSizeY);
		PutNumToTable(L,  "Options", vi.Options);
		PutNumToTable(L,  "TabSize", vi.TabSize);
		PutNumToTable(L,  "LeftPos", vi.LeftPos + 1);

		flags_t Flags = (vi.CurMode.Wrap ? VMF_WRAP : 0) | (vi.CurMode.WordWrap ? VMF_WORDWRAP : 0);
		lua_createtable(L, 0, 4);
		PutNumToTable(L, "CodePage", vi.CurMode.CodePage);
		PutFlagsToTable(L, "Flags",  Flags);
		PutNumToTable(L, "ViewMode", vi.CurMode.Hex ? VMT_HEX : VMT_TEXT);
		PutBoolToTable (L, "Processed",  vi.CurMode.Processed);
		lua_setfield(L, -2, "CurMode");
	}
	else
		lua_pushnil(L);

	return 1;
}

static int viewer_Quit(lua_State *L)
{
	int ViewerId = luaL_optinteger(L, 1, -1);
	lua_pushboolean(L, PSInfo.ViewerControlV2(ViewerId, VCTL_QUIT, NULL));
	return 1;
}

static int viewer_Redraw(lua_State *L)
{
	int ViewerId = luaL_optinteger(L, 1, -1);
	PSInfo.ViewerControlV2(ViewerId, VCTL_REDRAW, NULL);
	return 0;
}

static int viewer_Select(lua_State *L)
{
	int ViewerId = luaL_optinteger(L,1,-1);
	struct ViewerSelect vs;
	vs.BlockStartPos = (int64_t)luaL_checknumber(L,2);
	vs.BlockLen = luaL_checkinteger(L,3);
	lua_pushboolean(L, PSInfo.ViewerControlV2(ViewerId, VCTL_SELECT, &vs));
	return 1;
}

static int viewer_SetPosition(lua_State *L)
{
	int viewerId = luaL_optinteger(L,1,-1);
	struct ViewerSetPosition vsp;
	if (lua_istable(L, 2)) {
		lua_settop(L, 2);
		vsp.StartPos = (int64_t)GetOptNumFromTable(L, "StartPos", 0);
		vsp.LeftPos = (int64_t)GetOptNumFromTable(L, "LeftPos", 1) - 1;
		vsp.Flags   = GetFlagsFromTable(L, -1, "Flags");
	}
	else {
		vsp.StartPos = (int64_t)luaL_optnumber(L,2,0);
		vsp.LeftPos = (int64_t)luaL_optnumber(L,3,1) - 1;
		vsp.Flags = OptFlags(L,4,0);
	}
	if (PSInfo.ViewerControlV2(viewerId, VCTL_SETPOSITION, &vsp))
		lua_pushnumber(L, (double)vsp.StartPos);
	else
		lua_pushnil(L);
	return 1;
}

static int viewer_SetMode(lua_State *L)
{
	int success;
	struct ViewerSetMode vsm = {};
	int ViewerId = luaL_optinteger(L, 1, -1);
	luaL_checktype(L, 2, LUA_TTABLE);
	lua_getfield(L, 2, "Type");
	vsm.Type = get_env_flag(L, -1, &success);

	if (!success)
		return lua_pushboolean(L,0), 1;

	lua_getfield(L, 2, "iParam");

	if (lua_isnumber(L, -1))
		vsm.Param.iParam = lua_tointeger(L, -1);
	else
		return lua_pushboolean(L,0), 1;

	lua_getfield(L, 2, "Flags");
	vsm.Flags = GetFlagCombination (L, -1, &success);

	if (!success)
		return lua_pushboolean(L,0), 1;

	lua_pushboolean(L, PSInfo.ViewerControlV2(ViewerId, VCTL_SETMODE, &vsm));
	return 1;
}

static const luaL_Reg viewer_funcs[] =
{
	PAIR( viewer, GetFileName),
	PAIR( viewer, GetInfo),
	PAIR( viewer, Quit),
	PAIR( viewer, Redraw),
	PAIR( viewer, Select),
	PAIR( viewer, SetKeyBar),
	PAIR( viewer, SetMode),
	PAIR( viewer, SetPosition),
	PAIR( viewer, Viewer),

	{NULL, NULL},
};

int luaopen_viewer(lua_State *L)
{
	luaL_register(L, "viewer", viewer_funcs);
	return 0;
}
