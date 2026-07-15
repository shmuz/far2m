#include <windows.h>

#include <lua.h>
#include <lauxlib.h>

#include "lf_bit64.h"
#include "lf_flags.h"
#include "lf_luafar.h"
#include "lf_service.h"
#include "lf_string.h"
#include "lf_util.h"

static int DoAdvControl (lua_State *L, FARAPIADVCONTROL PtrAdvControl, int Command, int Delta)
{
	int pos2 = 2-Delta;
	TPluginData* pd = GetPluginData(L);
	intptr_t int1;
	wchar_t buf[300];
	COORD coord;

	if (Delta == 0)
		Command = (int) check_env_flag(L, 1);

	switch(Command)
	{
		default:
			return luaL_argerror(L, 1, "command not supported");

		case ACTL_GETCONFIRMATIONS:
		case ACTL_GETDESCSETTINGS:
		case ACTL_GETDIALOGSETTINGS:
		case ACTL_GETINTERFACESETTINGS:
		case ACTL_GETPANELSETTINGS:
		case ACTL_GETPLUGINMAXREADDATA:
		case ACTL_GETSYSTEMSETTINGS:
		case ACTL_GETWINDOWCOUNT:
		case ACTL_COMMIT:
		case ACTL_REDRAWALL:
			int1 = PtrAdvControl(pd->ModuleNumber, Command, NULL, NULL);
			return lua_pushinteger(L, int1), 1;

		case ACTL_QUIT:
			int1 = PtrAdvControl(pd->ModuleNumber, Command, (void*)luaL_optinteger(L,pos2,0), NULL);
			return lua_pushinteger(L, int1), 1;

		case ACTL_SETCURRENTWINDOW:
			int1 = luaL_checkinteger(L, pos2) - 1;
			int1 = PtrAdvControl(pd->ModuleNumber, ACTL_SETCURRENTWINDOW, (void*)int1, NULL);
			if (int1)
				PtrAdvControl(pd->ModuleNumber, ACTL_COMMIT, NULL, NULL);
			return lua_pushinteger(L, int1), 1;

		case ACTL_WAITKEY:
		{
			if (lua_isnumber(L, pos2))
				int1 = lua_tointeger(L, pos2);
			else
				int1 = OptFlags(L, pos2, -1);

			if (int1 < -1) //this prevents program freeze
				int1 = -1;

			lua_pushinteger(L, PtrAdvControl(pd->ModuleNumber, Command, (void*)int1, NULL));
			return 1;
		}

		case ACTL_GETCOLOR:
		{
			uint64_t color;
			uintptr_t index = check_env_flag(L, pos2);
			if (PtrAdvControl(pd->ModuleNumber, Command, (void*)index, &color))
				PushFarColor(L, color);
			else
				lua_pushnil(L);

			return 1;
		}

		case ACTL_SYNCHRO:
			if (lua_isfunction(L, pos2)) {
				TSynchroData *sd = CreateSynchroData(SYNCHRO_FUNCTION, 0, NULL);
				int top = lua_gettop(L);
				sd->narg = top - pos2 + 1;
				lua_newtable(L);
				for (int i=pos2,j=1; i <= top; ) {
					lua_pushvalue(L, i++);
					lua_rawseti(L, -2, j++);
				}
				sd->ref = luaL_ref(L, LUA_REGISTRYINDEX);
				lua_pushinteger(L, PtrAdvControl(pd->ModuleNumber, Command, sd, NULL));
				return 1;
			}
			else {
				luaL_argcheck(L, lua_isnumber(L,pos2), pos2, "integer or function expected");
				TSynchroData *sd = CreateSynchroData(SYNCHRO_COMMON, lua_tointeger(L,pos2), NULL);
				lua_pushinteger(L, PtrAdvControl(pd->ModuleNumber, Command, sd, NULL));
				return 1;
			}

		case ACTL_SETPROGRESSSTATE:
			return 0;

		case ACTL_SETPROGRESSVALUE:
			return 0;

		case ACTL_GETARRAYCOLOR:
		{
			intptr_t size = PtrAdvControl(pd->ModuleNumber, Command, NULL, NULL);
			uint64_t *p = (uint64_t*) lua_newuserdata(L, size * sizeof(uint64_t));
			PtrAdvControl(pd->ModuleNumber, Command, (void*)size, p);
			lua_createtable(L, size, 0);
			for(int i=0; i < size; i++) {
				PushFarColor(L, p[i]);
				lua_rawseti(L, -2, i+1);
			}
			return 1;
		}

		case ACTL_GETFARVERSION:
		{
			DWORD n = PtrAdvControl(pd->ModuleNumber, Command, NULL, NULL);
			int v1 = (n >> 16);
			int v2 = n & 0xffff;
			if (lua_toboolean(L, pos2))
			{
				lua_pushinteger(L, v1);
				lua_pushinteger(L, v2);
				return 2;
			}
			lua_pushfstring(L, "%d.%d", v1, v2);
			return 1;
		}

		case ACTL_GETWINDOWINFO:
		{
			struct WindowInfo wi;
			memset(&wi, 0, sizeof(wi));
			wi.Pos = luaL_optinteger(L, pos2, 0) - 1;

			if (!PtrAdvControl(pd->ModuleNumber, Command, &wi, NULL))
				return lua_pushnil(L), 1;

			wi.TypeName = (wchar_t*)lua_newuserdata(L, (wi.TypeNameSize + wi.NameSize) * sizeof(wchar_t));
			wi.Name = wi.TypeName + wi.TypeNameSize;

			if (!PtrAdvControl(pd->ModuleNumber, Command, &wi, NULL))
				return lua_pushnil(L), 1;

			lua_createtable(L, 0, 6);

			switch(wi.Type)
			{
				case WTYPE_DIALOG:
					NewDialogData(L, (HANDLE)wi.Id, FALSE);
					lua_setfield(L, -2, "Id");
					break;

				default:
					PutIntToTable(L, "Id", (int)wi.Id);
					break;
			}

			PutIntToTable(L, "Pos", wi.Pos + 1);
			PutIntToTable(L, "Type", wi.Type);
			PutFlagsToTable(L, "Flags", wi.Flags);
			PutWStrToTable(L, "TypeName", wi.TypeName, -1);
			PutWStrToTable(L, "Name", wi.Name, -1);
			return 1;
		}

		case ACTL_SETARRAYCOLOR:
		{
			struct FarSetColors fsc;
			++pos2; // make compatible with far3
			luaL_checktype(L, pos2, LUA_TTABLE);
			lua_settop(L, pos2);
			fsc.StartIndex = GetOptIntFromTable(L, "StartIndex", 0);
			lua_getfield(L, pos2, "Flags");
			fsc.Flags = GetFlagCombination(L, -1, NULL);
			fsc.ColorCount = lua_objlen(L, pos2);
			fsc.Colors = (uint64_t*)lua_newuserdata(L, fsc.ColorCount * sizeof(uint64_t));
			for (int i=0; i < fsc.ColorCount; i++) {
				lua_rawgeti(L, pos2, i+1);
				fsc.Colors[i] = GetFarColor64(L, -1);
				lua_pop(L,1);
			}
			lua_pushinteger(L, PtrAdvControl(pd->ModuleNumber, Command, &fsc, NULL));
			return 1;
		}

		case ACTL_GETFARRECT:
		{
			SMALL_RECT sr;
			if (PtrAdvControl(pd->ModuleNumber, Command, &sr, NULL)) {
				lua_createtable(L, 0, 4);
				PutIntToTable(L, "Left",   sr.Left);
				PutIntToTable(L, "Top",    sr.Top);
				PutIntToTable(L, "Right",  sr.Right);
				PutIntToTable(L, "Bottom", sr.Bottom);
			}
			else
				lua_pushnil(L);

			return 1;
		}

		case ACTL_GETCURSORPOS:
			if (PtrAdvControl(pd->ModuleNumber, Command, &coord, NULL)) {
				lua_createtable(L, 0, 2);
				PutIntToTable(L, "X", coord.X);
				PutIntToTable(L, "Y", coord.Y);
			}
			else
				lua_pushnil(L);

			return 1;

		case ACTL_SETCURSORPOS:
			luaL_checktype(L, pos2, LUA_TTABLE);
			lua_getfield(L, pos2, "X");
			coord.X = lua_tointeger(L, -1);
			lua_getfield(L, pos2, "Y");
			coord.Y = lua_tointeger(L, -1);
			lua_pushinteger(L, PtrAdvControl(pd->ModuleNumber, Command, &coord, NULL));
			return 1;

		case ACTL_GETWINDOWTYPE:
		{
			struct WindowType wt = { sizeof(wt) };

			if (PtrAdvControl(pd->ModuleNumber, Command, 0, &wt))
			{
				lua_createtable(L, 0, 1);
				PutIntToTable(L, "Type", wt.Type);
			}
			else lua_pushnil(L);

			return 1;
		}

		case ACTL_GETSYSWORDDIV:
			PtrAdvControl(pd->ModuleNumber, Command, buf, NULL);
			return push_utf8_string(L,buf,-1), 1;

		case ACTL_GETFARCOMMITTIME:
		{
			uint64_t commit_time = 0;
			PtrAdvControl(pd->ModuleNumber, Command, &commit_time, NULL);
			lua_pushnumber(L, commit_time);
			return 1;
		}

		case ACTL_WINPORTBACKEND:
			PtrAdvControl(pd->ModuleNumber, Command, buf, NULL);
			return push_utf8_string(L,buf,-1), 1;

		//case ACTL_KEYMACRO:  //  not supported as it's replaced by separate functions far.MacroXxx
	}
}

static int far_AdvControl(lua_State *L) { return DoAdvControl(L, PSInfo.AdvControl, 0, 0); }

static int far_AdvControlAsync(lua_State *L) { return DoAdvControl(L, PSInfo.AdvControlAsync, 0, 0); }

#define AdvCommand(name,command) \
static int adv_##name(lua_State *L) { return DoAdvControl(L,PSInfo.AdvControl,command,1); }

AdvCommand( Commit,                 ACTL_COMMIT)
AdvCommand( GetArrayColor,          ACTL_GETARRAYCOLOR)
AdvCommand( GetColor,               ACTL_GETCOLOR)
AdvCommand( GetConfirmations,       ACTL_GETCONFIRMATIONS)
AdvCommand( GetCursorPos,           ACTL_GETCURSORPOS)
AdvCommand( GetDescSettings,        ACTL_GETDESCSETTINGS)
AdvCommand( GetDialogSettings,      ACTL_GETDIALOGSETTINGS)
AdvCommand( GetFarCommitTime,       ACTL_GETFARCOMMITTIME)
AdvCommand( GetFarManagerVersion,   ACTL_GETFARVERSION)
AdvCommand( GetFarRect,             ACTL_GETFARRECT)
AdvCommand( GetInterfaceSettings,   ACTL_GETINTERFACESETTINGS)
AdvCommand( GetPanelSettings,       ACTL_GETPANELSETTINGS)
AdvCommand( GetPluginMaxReadData,   ACTL_GETPLUGINMAXREADDATA)
AdvCommand( GetSystemSettings,      ACTL_GETSYSTEMSETTINGS)
AdvCommand( GetSysWordDiv,          ACTL_GETSYSWORDDIV)
AdvCommand( GetWindowCount,         ACTL_GETWINDOWCOUNT)
AdvCommand( GetWindowInfo,          ACTL_GETWINDOWINFO)
AdvCommand( GetWindowType,          ACTL_GETWINDOWTYPE)
AdvCommand( Quit,                   ACTL_QUIT)
AdvCommand( RedrawAll,              ACTL_REDRAWALL)
AdvCommand( SetArrayColor,          ACTL_SETARRAYCOLOR)
AdvCommand( SetCurrentWindow,       ACTL_SETCURRENTWINDOW)
AdvCommand( SetCursorPos,           ACTL_SETCURSORPOS)
AdvCommand( Synchro,                ACTL_SYNCHRO)
AdvCommand( WaitKey,                ACTL_WAITKEY)
AdvCommand( WinPortBackEnd,         ACTL_WINPORTBACKEND)

static const luaL_Reg far_funcs[] =
{
	PAIR( far, AdvControl),
	PAIR( far, AdvControlAsync),

	{NULL, NULL},
};

static const luaL_Reg actl_funcs[] =
{
	PAIR( adv, Commit),
	PAIR( adv, GetArrayColor),
	PAIR( adv, GetColor),
	PAIR( adv, GetConfirmations),
	PAIR( adv, GetCursorPos),
	PAIR( adv, GetDescSettings),
	PAIR( adv, GetDialogSettings),
	PAIR( adv, GetFarCommitTime),
	PAIR( adv, GetFarRect),
	PAIR( adv, GetFarManagerVersion),
	PAIR( adv, GetInterfaceSettings),
	PAIR( adv, GetPanelSettings),
	PAIR( adv, GetPluginMaxReadData),
	PAIR( adv, GetSystemSettings),
	PAIR( adv, GetSysWordDiv),
	PAIR( adv, GetWindowCount),
	PAIR( adv, GetWindowInfo),
	PAIR( adv, GetWindowType),
	PAIR( adv, Quit),
	PAIR( adv, RedrawAll),
	PAIR( adv, SetArrayColor),
	PAIR( adv, SetCurrentWindow),
	PAIR( adv, SetCursorPos),
	PAIR( adv, Synchro),
	PAIR( adv, WaitKey),
	PAIR( adv, WinPortBackEnd),

	{NULL, NULL},
};

int luaopen_actl(lua_State *L)
{
	int top = lua_gettop(L);
  luaL_register(L, "far", far_funcs);
	luaL_register(L, "actl", actl_funcs);
	lua_settop(L, top);
	return 0;
}
