#include <windows.h>

#include <lua.h>
#include <lauxlib.h>

#include "lf_luafar.h"
#include "lf_service.h"
#include "lf_string.h"
#include "lf_util.h"

HANDLE OptHandle(lua_State *L)
{
	switch(lua_type(L,1))
	{
		case LUA_TNONE:
		case LUA_TNIL:
			break;

		case LUA_TNUMBER:
		{
			lua_Integer whatPanel = lua_tointeger(L,1);
			HANDLE hh = (HANDLE)whatPanel;
			return (hh==PANEL_PASSIVE || hh==PANEL_ACTIVE) ? hh : whatPanel%2 ? PANEL_ACTIVE:PANEL_PASSIVE;
		}

		case LUA_TLIGHTUSERDATA:
			return lua_touserdata(L,1);

		default:
			luaL_typerror(L, 1, "integer or light userdata");
	}
	return NULL;
}

static HANDLE OptHandle2(lua_State *L)
{
	return lua_isnoneornil(L,1) ? (luaL_checkinteger(L,2) % 2 ? PANEL_ACTIVE:PANEL_PASSIVE) : OptHandle(L);
}

static int panel_CheckPanelsExist(lua_State *L)
{
	HANDLE handle = OptHandle(L);
	lua_pushboolean(L, (int)PSInfo.Control(handle, FCTL_CHECKPANELSEXIST, 0, 0));
	return 1;
}

static int panel_ClosePanel(lua_State *L)
{
	HANDLE handle = OptHandle(L);
	const wchar_t *dir = opt_utf8_string(L, 2, NULL);
	lua_pushboolean(L, PSInfo.Control(handle, FCTL_CLOSEPLUGIN, 0, (LONG_PTR)dir));
	return 1;
}

static int panel_GetPanelInfo(lua_State *L)
{
	HANDLE handle = OptHandle2(L);
	struct PanelInfo pi;
	if (!PSInfo.Control(handle, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi))
		return lua_pushnil(L), 1;

	if (pi.Plugin)   pi.Flags |= PFLAGS_PLUGIN;
	if (pi.Visible)  pi.Flags |= PFLAGS_VISIBLE;
	if (pi.Focus)    pi.Flags |= PFLAGS_FOCUS;

	lua_createtable(L, 0, 13);
	//-------------------------------------------------------------------------
	PutIntToTable (L, "PanelType", pi.PanelType);
	//-------------------------------------------------------------------------
	PutRECTToTable(L, "PanelRect", pi.PanelRect);
	//-------------------------------------------------------------------------
	PutIntToTable (L, "ItemsNumber",  pi.ItemsNumber);
	PutIntToTable (L, "SelectedItemsNumber", pi.SelectedItemsNumber);
	PutIntToTable (L, "CurrentItem",  pi.CurrentItem + 1);
	PutIntToTable (L, "TopPanelItem", pi.TopPanelItem + 1);
	PutIntToTable (L, "ViewMode",     pi.ViewMode);
	PutIntToTable (L, "SortMode",     pi.SortMode);
	PutFlagsToTable(L, "Flags",       pi.Flags);
	PutNumToTable (L, "OwnerID",      pi.OwnerID);
	//-------------------------------------------------------------------------
	if (pi.PluginHandle) {
		lua_pushlightuserdata(L, pi.PluginHandle);
		lua_setfield(L, -2, "PluginHandle");
	}
	if (pi.OwnerHandle) {
		lua_pushlightuserdata(L, pi.OwnerHandle);
		lua_setfield(L, -2, "OwnerHandle");
	}
	if (pi.OwnerID == GetPluginData(L)->PluginId) {
		PushPluginObject(L, pi.PluginHandle);
		lua_setfield(L, -2, "PluginObject");
	}
	return 1;
}

static int get_panel_item(lua_State *L, int command)
{
	HANDLE handle = OptHandle2(L);
	int index = luaL_optinteger(L,3,1) - 1;
	if (index >= 0 || command == FCTL_GETCURRENTPANELITEM)
	{
		int size = PSInfo.Control(handle, command, index, 0);
		if (size) {
			struct PluginPanelItem* item = (struct PluginPanelItem*)lua_newuserdata(L, size);
			if (PSInfo.Control(handle, command, index, (LONG_PTR)item)) {
				PushOptPluginTable(L, handle);
				PushPanelItem(L, item);
				return 1;
			}
		}
	}
	return lua_pushnil(L), 1;
}

static int panel_GetPanelItem(lua_State *L)
{
	return get_panel_item(L, FCTL_GETPANELITEM);
}

static int panel_GetSelectedPanelItem(lua_State *L)
{
	return get_panel_item(L, FCTL_GETSELECTEDPANELITEM);
}

static int panel_GetCurrentPanelItem(lua_State *L)
{
	return get_panel_item(L, FCTL_GETCURRENTPANELITEM);
}

static int get_string_info(lua_State *L, int command)
{
	HANDLE handle = OptHandle2(L);
	int size = PSInfo.Control(handle, command, 0, 0);

	if (size)
	{
		wchar_t *buf = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));

		if (PSInfo.Control(handle, command, size, (LONG_PTR)buf))
		{
			push_utf8_string(L, buf, -1);
			return 1;
		}
	}
	return lua_pushnil(L), 1;
}

static int panel_GetPanelFormat(lua_State *L)
{
	return get_string_info(L, FCTL_GETPANELFORMAT);
}

static int panel_GetPanelHostFile(lua_State *L)
{
	return get_string_info(L, FCTL_GETPANELHOSTFILE);
}

static int panel_GetColumnTypes(lua_State *L)
{
	return get_string_info(L, FCTL_GETCOLUMNTYPES);
}

static int panel_GetColumnWidths(lua_State *L)
{
	return get_string_info(L, FCTL_GETCOLUMNWIDTHS);
}

static int panel_GetPanelPrefix(lua_State *L)
{
	return get_string_info(L, FCTL_GETPANELPREFIX);
}

static int panel_RedrawPanel(lua_State *L)
{
	HANDLE handle = OptHandle2(L);
	LONG_PTR param2 = 0;
	struct PanelRedrawInfo pri;
	if (lua_istable(L, 3))
	{
		param2 = (LONG_PTR)&pri;
		lua_getfield(L, 3, "CurrentItem");
		pri.CurrentItem = lua_tointeger(L, -1) - 1;
		lua_getfield(L, 3, "TopPanelItem");
		pri.TopPanelItem = lua_tointeger(L, -1) - 1;
	}
	lua_pushboolean(L, PSInfo.Control(handle, FCTL_REDRAWPANEL, 0, param2));
	return 1;
}

static int SetPanelBooleanProperty(lua_State *L, int command)
{
	HANDLE handle = OptHandle2(L);
	int param1 = lua_toboolean(L,3);
	lua_pushboolean(L, PSInfo.Control(handle, command, param1, 0));
	return 1;
}

static int SetPanelIntegerProperty(lua_State *L, int command)
{
	HANDLE handle = OptHandle2(L);
	int param1 = check_env_flag(L,3);
	lua_pushboolean(L, PSInfo.Control(handle, command, param1, 0));
	return 1;
}

static int panel_SetCaseSensitiveSort(lua_State *L)
{
	return SetPanelBooleanProperty(L, FCTL_SETCASESENSITIVESORT);
}

static int panel_SetNumericSort(lua_State *L)
{
	return SetPanelBooleanProperty(L, FCTL_SETNUMERICSORT);
}

static int panel_SetSortOrder(lua_State *L)
{
	return SetPanelBooleanProperty(L, FCTL_SETSORTORDER);
}

static int panel_SetDirectoriesFirst(lua_State *L)
{
	return SetPanelBooleanProperty(L, FCTL_SETDIRECTORIESFIRST);
}

static int panel_UpdatePanel(lua_State *L)
{
	return SetPanelBooleanProperty(L, FCTL_UPDATEPANEL);
}

static int panel_SetSortMode(lua_State *L)
{
	return SetPanelIntegerProperty(L, FCTL_SETSORTMODE);
}

static int panel_SetViewMode(lua_State *L)
{
	return SetPanelIntegerProperty(L, FCTL_SETVIEWMODE);
}

static int panel_GetPanelDirectory(lua_State *L)
{
	HANDLE handle = OptHandle2(L);
	int size = PSInfo.Control(handle, FCTL_GETPANELDIR_V2, 0, 0);
	if (size)
	{
		struct FarPanelDirectory *fpd = (struct FarPanelDirectory*) lua_newuserdata(L, size);
		fpd->StructSize = sizeof(*fpd);
		if (PSInfo.Control(handle, FCTL_GETPANELDIR_V2, size, (LONG_PTR)fpd))
		{
			lua_createtable(L, 0, 4);
			PutWStrToTable(L, "Name",     fpd->Name,  -1);
			PutWStrToTable(L, "Param",    fpd->Param, -1);
			PutWStrToTable(L, "File",     fpd->File,  -1);
			PutIntToTable (L, "PluginId", fpd->PluginId);
			return 1;
		}
	}
	return lua_pushnil(L), 1;
}

static int panel_SetPanelDirectory(lua_State *L)
{
	HANDLE handle = OptHandle2(L);
	LONG_PTR param2 = 0;

	if (lua_istable(L, 3)) {
		lua_getfield(L, 3, "Name");
		if (lua_isstring(L, -1)) {
			param2 = (LONG_PTR)check_utf8_string(L, -1, NULL);
		}
	}
	else if (lua_isstring(L, 3)) {
		param2 = (LONG_PTR)check_utf8_string(L, 3, NULL);
	}
	else
		luaL_argerror(L, 3, "table or string");

	int ret = param2 ? PSInfo.Control(handle, FCTL_SETPANELDIR, 0, param2) : 0;
	if (ret) {
		PSInfo.Control(handle, FCTL_REDRAWPANEL, 0, 0); //not required in Far3
	}
	lua_pushboolean(L, ret);
	return 1;
}

static int panel_SetPanelLocation(lua_State *L)
{
	HANDLE handle = OptHandle2(L);
	struct FarPanelLocation fpl = {};
	luaL_checktype(L, 3, LUA_TTABLE);

	lua_getfield(L, 3, "PluginName");
	if (lua_isstring(L,-1)) fpl.PluginName = check_utf8_string(L,-1,NULL);

	lua_getfield(L, 3, "HostFile");
	if (lua_isstring(L,-1)) fpl.HostFile = check_utf8_string(L,-1,NULL);

	lua_getfield(L, 3, "Item");
	fpl.Item = (LONG_PTR)lua_tointeger(L,-1);

	lua_getfield(L, 3, "Path");
	if (lua_isstring(L,-1)) fpl.Path = check_utf8_string(L,-1,NULL);

	int ret = PSInfo.Control(handle, FCTL_SETPANELLOCATION, 0, (LONG_PTR)&fpl);
	if (ret)
		PSInfo.Control(handle, FCTL_REDRAWPANEL, 0, 0); //not required in Far3
	lua_pushboolean(L, ret);
	return 1;
}

static int panel_GetCmdLine(lua_State *L)
{
	HANDLE handle = OptHandle(L);
	int size = PSInfo.Control(handle, FCTL_GETCMDLINE, 0, 0);
	wchar_t *buf = (wchar_t*) malloc(size*sizeof(wchar_t));
	PSInfo.Control(handle, FCTL_GETCMDLINE, size, (LONG_PTR)buf);
	push_utf8_string(L, buf, -1);
	free(buf);
	return 1;
}

static int panel_SetCmdLine(lua_State *L)
{
	HANDLE handle = OptHandle(L);
	const wchar_t* str = check_utf8_string(L, 2, NULL);
	lua_pushboolean(L, PSInfo.Control(handle, FCTL_SETCMDLINE, 0, (LONG_PTR)str));
	return 1;
}

static int panel_GetCmdLinePos(lua_State *L)
{
	HANDLE handle = OptHandle(L);
	int pos;
	PSInfo.Control(handle, FCTL_GETCMDLINEPOS, 0, (LONG_PTR)&pos) ?
		lua_pushinteger(L, pos+1) : lua_pushnil(L);
	return 1;
}

static int panel_SetCmdLinePos(lua_State *L)
{
	HANDLE handle = OptHandle(L);
	int pos = luaL_checkinteger(L, 2) - 1;
	int ret = PSInfo.Control(handle, FCTL_SETCMDLINEPOS, pos, 0);
	return lua_pushboolean(L, ret), 1;
}

static int panel_InsertCmdLine(lua_State *L)
{
	HANDLE handle = OptHandle(L);
	const wchar_t* str = check_utf8_string(L, 2, NULL);
	lua_pushboolean(L, PSInfo.Control(handle, FCTL_INSERTCMDLINE, 0, (LONG_PTR)str));
	return 1;
}

static int panel_GetCmdLineSelection(lua_State *L)
{
	HANDLE handle = OptHandle(L);
	struct CmdLineSelect cms;
	if (PSInfo.Control(handle, FCTL_GETCMDLINESELECTION, 0, (LONG_PTR)&cms)) {
		if (cms.SelStart < 0) cms.SelStart = 0;
		if (cms.SelEnd < 0) cms.SelEnd = 0;
		lua_pushinteger(L, cms.SelStart + 1);
		lua_pushinteger(L, cms.SelEnd);
		return 2;
	}
	return lua_pushnil(L), 1;
}

static int panel_SetCmdLineSelection(lua_State *L)
{
	HANDLE handle = OptHandle(L);
	struct CmdLineSelect cms;
	cms.SelStart = luaL_checkinteger(L, 2) - 1;
	cms.SelEnd = luaL_checkinteger(L, 3);
	if (cms.SelStart < -1) cms.SelStart = -1;
	if (cms.SelEnd < -1) cms.SelEnd = -1;
	int ret = PSInfo.Control(handle, FCTL_SETCMDLINESELECTION, 0, (LONG_PTR)&cms);
	return lua_pushboolean(L, ret), 1;
}

// CtrlSetSelection   (handle, items, selection)
// CtrlClearSelection (handle, items)
//   handle:       handle
//   items:        either number of an item, or a list of item numbers
//   selection:    boolean
static int ChangePanelSelection(lua_State *L, BOOL op_set)
{
	HANDLE handle = OptHandle2(L);
	int itemindex = -1;
	if (lua_isnumber(L,3)) {
		itemindex = lua_tointeger(L,3) - 1;
		if (itemindex < 0) return luaL_argerror(L, 3, "non-positive index");
	}
	else if (!lua_istable(L,3))
		return luaL_typerror(L, 3, "number or table");
	int state = op_set ? lua_toboolean(L,4) : 0;

	// get panel info
	struct PanelInfo pi;
	if (!PSInfo.Control(handle, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi) ||
		 (pi.PanelType != PTYPE_FILEPANEL))
		return lua_pushboolean(L,0), 1;
	//---------------------------------------------------------------------------
	int numItems = op_set ? pi.ItemsNumber : pi.SelectedItemsNumber;
	int command  = op_set ? FCTL_SETSELECTION : FCTL_CLEARSELECTION;
	if (itemindex >= 0 && itemindex < numItems)
		PSInfo.Control(handle, command, itemindex, state);
	else {
		int len = lua_objlen(L,3);
		for (int i=1; i<=len; i++) {
			lua_pushinteger(L, i);
			lua_gettable(L,3);
			if (lua_isnumber(L,-1)) {
				itemindex = lua_tointeger(L,-1) - 1;
				if (itemindex >= 0 && itemindex < numItems)
					PSInfo.Control(handle, command, itemindex, state);
			}
			lua_pop(L,1);
		}
	}
	//---------------------------------------------------------------------------
	return lua_pushboolean(L,1), 1;
}

static int panel_SetSelection(lua_State *L)
{
	return ChangePanelSelection(L, TRUE);
}

static int panel_ClearSelection(lua_State *L)
{
	return ChangePanelSelection(L, FALSE);
}

static int panel_BeginSelection(lua_State *L)
{
	int res = PSInfo.Control(OptHandle2(L), FCTL_BEGINSELECTION, 0, 0);
	return lua_pushboolean(L, res), 1;
}

static int panel_EndSelection(lua_State *L)
{
	int res = PSInfo.Control(OptHandle2(L), FCTL_ENDSELECTION, 0, 0);
	return lua_pushboolean(L, res), 1;
}

static int panel_SetUserScreen(lua_State *L)
{
	HANDLE handle = OptHandle(L);
	int ret = PSInfo.Control(handle, FCTL_SETUSERSCREEN, 0, 0);
	return lua_pushboolean(L, ret), 1;
}

static int panel_GetUserScreen(lua_State *L)
{
	HANDLE handle = OptHandle(L);
	int ret = PSInfo.Control(handle, FCTL_GETUSERSCREEN, 0, 0);
	return lua_pushboolean(L, ret), 1;
}

static int panel_IsActivePanel(lua_State *L)
{
	HANDLE handle = OptHandle(L);
	return lua_pushboolean(L, PSInfo.Control(handle, FCTL_ISACTIVEPANEL, 0, 0)), 1;
}

static int panel_SetActivePanel(lua_State *L)
{
	HANDLE handle = OptHandle2(L);
	return lua_pushboolean(L, PSInfo.Control(handle, FCTL_SETACTIVEPANEL, 0, 0)), 1;
}

static int panel_GetPanelPluginHandle(lua_State *L)
{
	HANDLE plug_handle;
	PSInfo.Control(OptHandle2(L), FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)&plug_handle);
	if (plug_handle == INVALID_HANDLE_VALUE)
		lua_pushnil(L);
	else
		lua_pushlightuserdata(L, plug_handle);
	return 1;
}

static const luaL_Reg panel_funcs[] =
{
	PAIR( panel, BeginSelection),
	PAIR( panel, CheckPanelsExist),
	PAIR( panel, ClearSelection),
	PAIR( panel, ClosePanel),
	PAIR( panel, EndSelection),
	PAIR( panel, GetCmdLine),
	PAIR( panel, GetCmdLinePos),
	PAIR( panel, GetCmdLineSelection),
	PAIR( panel, GetColumnTypes),
	PAIR( panel, GetColumnWidths),
	PAIR( panel, GetCurrentPanelItem),
	PAIR( panel, GetPanelDirectory),
	PAIR( panel, GetPanelFormat),
	PAIR( panel, GetPanelHostFile),
	PAIR( panel, GetPanelInfo),
	PAIR( panel, GetPanelItem),
	PAIR( panel, GetPanelPluginHandle),
	PAIR( panel, GetPanelPrefix),
	PAIR( panel, GetSelectedPanelItem),
	PAIR( panel, GetUserScreen),
	PAIR( panel, InsertCmdLine),
	PAIR( panel, IsActivePanel),
	PAIR( panel, RedrawPanel),
	PAIR( panel, SetActivePanel),
	PAIR( panel, SetCaseSensitiveSort),
	PAIR( panel, SetCmdLine),
	PAIR( panel, SetCmdLinePos),
	PAIR( panel, SetCmdLineSelection),
	PAIR( panel, SetDirectoriesFirst),
	PAIR( panel, SetNumericSort),
	PAIR( panel, SetPanelDirectory),
	PAIR( panel, SetPanelLocation),
	PAIR( panel, SetSelection),
	PAIR( panel, SetSortMode),
	PAIR( panel, SetSortOrder),
	PAIR( panel, SetUserScreen),
	PAIR( panel, SetViewMode),
	PAIR( panel, UpdatePanel),

	{NULL, NULL},
};

int luaopen_panel(lua_State *L)
{
	luaL_register(L, "panel", panel_funcs);
	lua_pop(L, 1);
	return 0;
}
