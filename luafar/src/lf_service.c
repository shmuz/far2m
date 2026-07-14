//---------------------------------------------------------------------------

#include <windows.h>
#include <dirent.h> //opendir
#include <stdlib.h>
#include <ctype.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "far3parts.h"
#include "lf_flags.h"
#include "lf_farlibs.h"
#include "lf_util.h"
#include "lf_string.h"
#include "lf_bit64.h"
#include "lf_service.h"

struct PluginStartupInfo PSInfo; // DON'T ever use fields ModuleName and ModuleNumber of PSInfo
                                 // because they contain data of the 1-st loaded LuaFAR plugin.
                                 // Instead, get them via GetPluginData(L).
struct FarStandardFunctions FSF;

extern int luaopen_dialog(lua_State *L);
extern int luaopen_editor(lua_State *L);
extern int luaopen_panel(lua_State *L);
extern int luaopen_far_host(lua_State *L);
extern int luaopen_regex(lua_State*);
extern int luaopen_usercontrol(lua_State*);
extern int luaopen_timer(lua_State *L);
extern int luaopen_unicode(lua_State *L);
extern int luaopen_utf8(lua_State *L);
extern int luaopen_sysutils(lua_State *L);
extern int luaopen_win(lua_State *L);
extern int luaopen_lpeg(lua_State *L);

extern void push_flags_table(lua_State *L);
extern int far_MacroCallFar(lua_State *L);
extern int far_MacroCallToLua(lua_State *L);
extern void PackMacroValues(lua_State* L, size_t Count, const struct FarMacroValue* Values);
extern void PushPluginTable(lua_State* L, HANDLE hPlugin);
extern BOOL RunDefaultScript(lua_State* L, int ForFirstTime);

const char FarFileFilterType[] = "FarFileFilter";
const char PluginHandleType[]  = "FarPluginHandle";
const char AddMacroDataType[]  = "FarAddMacroData";
const char SavedScreenType[]   = "FarSavedScreen";

const char FAR_PLUGINDATA[]    = "far.plugindata";
const char FAR_VIRTUALKEYS[]   = "far.virtualkeys";
const char FAR_FLAGSTABLE[]    = "far.Flags";

const char* VirtualKeyStrings[256] =
{
	// 0x00
	NULL, "LBUTTON", "RBUTTON", "CANCEL",
	"MBUTTON", "XBUTTON1", "XBUTTON2", NULL,
	"BACK", "TAB", NULL, NULL,
	"CLEAR", "RETURN", NULL, NULL,
	// 0x10
	"SHIFT", "CONTROL", "MENU", "PAUSE",
	"CAPITAL", "KANA", NULL, "JUNJA",
	"FINAL", "HANJA", NULL, "ESCAPE",
	NULL, "NONCONVERT", "ACCEPT", "MODECHANGE",
	// 0x20
	"SPACE", "PRIOR", "NEXT", "END",
	"HOME", "LEFT", "UP", "RIGHT",
	"DOWN", "SELECT", "PRINT", "EXECUTE",
	"SNAPSHOT", "INSERT", "DELETE", "HELP",
	// 0x30
	"0", "1", "2", "3",
	"4", "5", "6", "7",
	"8", "9", NULL, NULL,
	NULL, NULL, NULL, NULL,
	// 0x40
	NULL, "A", "B", "C",
	"D", "E", "F", "G",
	"H", "I", "J", "K",
	"L", "M", "N", "O",
	// 0x50
	"P", "Q", "R", "S",
	"T", "U", "V", "W",
	"X", "Y", "Z", "LWIN",
	"RWIN", "APPS", NULL, "SLEEP",
	// 0x60
	"NUMPAD0", "NUMPAD1", "NUMPAD2", "NUMPAD3",
	"NUMPAD4", "NUMPAD5", "NUMPAD6", "NUMPAD7",
	"NUMPAD8", "NUMPAD9", "MULTIPLY", "ADD",
	"SEPARATOR", "SUBTRACT", "DECIMAL", "DIVIDE",
	// 0x70
	"F1", "F2", "F3", "F4",
	"F5", "F6", "F7", "F8",
	"F9", "F10", "F11", "F12",
	"F13", "F14", "F15", "F16",
	// 0x80
	"F17", "F18", "F19", "F20",
	"F21", "F22", "F23", "F24",
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	// 0x90
	"NUMLOCK", "SCROLL", "OEM_NEC_EQUAL", "OEM_FJ_MASSHOU",
	"OEM_FJ_TOUROKU", "OEM_FJ_LOYA", "OEM_FJ_ROYA", NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	// 0xA0
	"LSHIFT", "RSHIFT", "LCONTROL", "RCONTROL",
	"LMENU", "RMENU", "BROWSER_BACK", "BROWSER_FORWARD",
	"BROWSER_REFRESH", "BROWSER_STOP", "BROWSER_SEARCH", "BROWSER_FAVORITES",
	"BROWSER_HOME", "VOLUME_MUTE", "VOLUME_DOWN", "VOLUME_UP",
	// 0xB0
	"MEDIA_NEXT_TRACK", "MEDIA_PREV_TRACK", "MEDIA_STOP", "MEDIA_PLAY_PAUSE",
	"LAUNCH_MAIL", "LAUNCH_MEDIA_SELECT", "LAUNCH_APP1", "LAUNCH_APP2",
	NULL, NULL, "OEM_1", "OEM_PLUS",
	"OEM_COMMA", "OEM_MINUS", "OEM_PERIOD", "OEM_2",
	// 0xC0
	"OEM_3", NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	// 0xD0
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "OEM_4",
	"OEM_5", "OEM_6", "OEM_7", "OEM_8",
	// 0xE0
	NULL, NULL, "OEM_102", NULL,
	NULL, "PROCESSKEY", NULL, "PACKET",
	NULL, "OEM_RESET", "OEM_JUMP", "OEM_PA1",
	"OEM_PA2", "OEM_PA3", "OEM_WSCTRL", NULL,
	// 0xF0
	NULL, NULL, NULL, NULL,
	NULL, NULL, "ATTN", "CRSEL",
	"EXSEL", "EREOF", "PLAY", "ZOOM",
	"NONAME", "PA1", "OEM_CLEAR", NULL,
};

TSynchroData* CreateSynchroData(int type, int data, TTimerData *td)
{
	TSynchroData* SD = (TSynchroData*) malloc(sizeof(TSynchroData));
	SD->type = type;
	SD->data = data;
	SD->ref = LUA_REFNIL;
	SD->timerData = td;
	return SD;
}

TPluginData* GetPluginData(lua_State* L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, FAR_PLUGINDATA);
	TPluginData* pd = (TPluginData*) lua_touserdata(L, -1);
	if (pd)
		lua_pop(L, 1);
	else
		luaL_error (L, "TPluginData is not available.");
	return pd;
}

static void PushPluginHandle(lua_State *L, HANDLE Handle)
{
	if (Handle)
	{
		HANDLE *p = (HANDLE*)lua_newuserdata(L, sizeof(HANDLE));
		*p = Handle;
		luaL_getmetatable(L, PluginHandleType);
		lua_setmetatable(L, -2);
	}
	else
		lua_pushnil(L);
}

static int PluginHandle_rawhandle(lua_State *L)
{
	void* Handle = *(void**)luaL_checkudata(L, 1, PluginHandleType);
	lua_pushlightuserdata(L, Handle);
	return 1;
}

void ConvertLuaValue (lua_State *L, int pos, struct FarMacroValue *target)
{
	int64_t val64;
	HANDLE val_handle;
	int type = lua_type(L, pos);
	pos = abs_index(L, pos);
	target->Type = FMVT_UNKNOWN;

	if (type == LUA_TNUMBER)
	{
		target->Type = FMVT_DOUBLE;
		target->Value.Double = lua_tonumber(L, pos);
	}
	else if (type == LUA_TSTRING)
	{
		target->Type = FMVT_STRING;
		target->Value.String = check_utf8_string(L, pos, NULL);
	}
	else if (type == LUA_TTABLE)
	{
		lua_getfield(L, pos, TKEY_BINARY);
		if (lua_type(L, -1) == LUA_TSTRING)
		{
			target->Type = FMVT_BINARY;
			target->Value.Binary.Data = (void*)lua_tolstring(L, -1, &target->Value.Binary.Size);
		}
		else
		{
			target->Type = FMVT_TABLE;
			target->Value.Integer = pos;
		}
		lua_pop(L, 1);
	}
	else if (type == LUA_TBOOLEAN)
	{
		target->Type = FMVT_BOOLEAN;
		target->Value.Boolean = lua_toboolean(L, pos);
	}
	else if (type == LUA_TNIL)
	{
		target->Type = FMVT_NIL;
	}
	else if (type == LUA_TLIGHTUSERDATA)
	{
		target->Type = FMVT_POINTER;
		target->Value.Pointer = lua_touserdata(L, pos);
	}
	else if (bit64_getvalue(L, pos, &val64))
	{
		target->Type = FMVT_INTEGER;
		target->Value.Integer = val64;
	}
	else if (Dialog_getvalue(L, pos, &val_handle))
	{
		target->Type = FMVT_DIALOG;
		target->Value.Pointer = val_handle;
	}
}

GUID OptGuid(lua_State *L, int pos)
{
	GUID guid = {};
	if (lua_type(L,pos) == LUA_TSTRING && lua_objlen(L,pos) == sizeof(GUID))
		return *(const GUID*)lua_tostring(L,pos);
	else
		return guid;
}

static int _GetFileProperty (lua_State *L, int Owner)
{
	wchar_t Target[512] = {0};
	const wchar_t *Computer = opt_utf8_string (L, 1, NULL);
	const wchar_t *Name = check_utf8_string (L, 2, NULL);
	if (Owner)
		FSF.GetFileOwner (Computer, Name, Target, ARRAYSIZE(Target));
	else
		FSF.GetFileGroup (Computer, Name, Target, ARRAYSIZE(Target));
	if (*Target)
		push_utf8_string(L, Target, -1);
	else
		lua_pushnil(L);
	return 1;
}

static int far_GetFileOwner (lua_State *L) { return _GetFileProperty(L,1); }
static int far_GetFileGroup (lua_State *L) { return _GetFileProperty(L,0); }

static int far_GetNumberOfLinks (lua_State *L)
{
	const wchar_t *Name = check_utf8_string (L, 1, NULL);
	int num = FSF.GetNumberOfLinks (Name);
	return lua_pushinteger (L, num), 1;
}

static void GetMouseEvent(lua_State *L, MOUSE_EVENT_RECORD* rec)
{
	rec->dwMousePosition.X = GetOptIntFromTable(L, "MousePositionX", 0);
	rec->dwMousePosition.Y = GetOptIntFromTable(L, "MousePositionY", 0);
	rec->dwButtonState = GetOptIntFromTable(L, "ButtonState", 0);
	rec->dwControlKeyState = GetOptIntFromTable(L, "ControlKeyState", 0);
	rec->dwEventFlags = GetOptIntFromTable(L, "EventFlags", 0);
}

void PutMouseEvent(lua_State *L, const MOUSE_EVENT_RECORD* rec, BOOL table_exist)
{
	if (!table_exist)
		lua_createtable(L, 0, 5);

	PutNumToTable(L, "MousePositionX", rec->dwMousePosition.X);
	PutNumToTable(L, "MousePositionY", rec->dwMousePosition.Y);
	PutNumToTable(L, "ButtonState", rec->dwButtonState);
	PutNumToTable(L, "ControlKeyState", rec->dwControlKeyState);
	PutNumToTable(L, "EventFlags", rec->dwEventFlags);
}

// convert a string from utf-8 to wide char and put it into a table,
// to prevent stack overflow and garbage collection
static const wchar_t* StoreTempString(lua_State *L, int store_stack_pos)
{
	const wchar_t *s = check_utf8_string(L,-1,NULL);
	luaL_ref(L, store_stack_pos);
	return s;
}

void PushEditorSetPosition(lua_State *L, const struct EditorSetPosition *esp)
{
	lua_createtable(L, 0, 6);
	PutIntToTable(L, "CurLine",       esp->CurLine + 1);
	PutIntToTable(L, "CurPos",        esp->CurPos + 1);
	PutIntToTable(L, "CurTabPos",     esp->CurTabPos + 1);
	PutIntToTable(L, "TopScreenLine", esp->TopScreenLine + 1);
	PutIntToTable(L, "LeftPos",       esp->LeftPos + 1);
	PutIntToTable(L, "Overtype",      esp->Overtype);
}

void FillEditorSetPosition(lua_State *L, struct EditorSetPosition *esp)
{
	esp->CurLine   = GetOptIntFromTable(L, "CurLine", 0) - 1;
	esp->CurPos    = GetOptIntFromTable(L, "CurPos", 0) - 1;
	esp->CurTabPos = GetOptIntFromTable(L, "CurTabPos", 0) - 1;
	esp->TopScreenLine = GetOptIntFromTable(L, "TopScreenLine", 0) - 1;
	esp->LeftPos   = GetOptIntFromTable(L, "LeftPos", 0) - 1;
	esp->Overtype  = GetOptIntFromTable(L, "Overtype", -1);
}

//a table expected on Lua stack top
static void PushFarFindData(lua_State *L, const struct FAR_FIND_DATA *wfd)
{
	PutAttrToTable     (L,                       wfd->dwFileAttributes);
	PutNumToTable      (L, "FileSize",           (double)wfd->nFileSize);
	PutNumToTable      (L, "PhysicalSize",       (double)wfd->nPhysicalSize);
	PutFileTimeToTable (L, "LastWriteTime",      wfd->ftLastWriteTime);
	PutFileTimeToTable (L, "LastAccessTime",     wfd->ftLastAccessTime);
	PutFileTimeToTable (L, "CreationTime",       wfd->ftCreationTime);
	PutWStrToTable     (L, "FileName",           wfd->lpwszFileName, -1);
	PutNumToTable      (L, "UnixMode",           wfd->dwUnixMode);
}

// on entry : the table's on the stack top
// on exit  : 2 strings added to the stack top (don't pop them!)
static void GetFarFindData(lua_State *L, struct FAR_FIND_DATA *wfd)
{
	memset(wfd, 0, sizeof(*wfd));

	wfd->dwFileAttributes = GetAttrFromTable(L);
	wfd->nFileSize        = GetFileSizeFromTable(L, "FileSize");
	wfd->nPhysicalSize    = GetFileSizeFromTable(L, "PhysicalSize");
	wfd->ftLastWriteTime  = GetFileTimeFromTable(L, "LastWriteTime");
	wfd->ftLastAccessTime = GetFileTimeFromTable(L, "LastAccessTime");
	wfd->ftCreationTime   = GetFileTimeFromTable(L, "CreationTime");
	wfd->dwUnixMode       = GetOptIntFromTable  (L, "UnixMode", 0);

	lua_getfield(L, -1, "FileName"); // +1
	wfd->lpwszFileName = opt_utf8_string(L, -1, L""); // +1
}
//---------------------------------------------------------------------------

void PushOptPluginTable(lua_State *L, HANDLE handle)
{
	HANDLE plug_handle = handle;
	if (handle == PANEL_ACTIVE || handle == PANEL_PASSIVE)
		PSInfo.Control(handle, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)&plug_handle);
	if (plug_handle == INVALID_HANDLE_VALUE)
		lua_pushnil(L);
	else
		PushPluginTable(L, plug_handle);
}

// either nil or plugin table is on stack top
void PushPanelItem(lua_State *L, const struct PluginPanelItem *PanelItem)
{
	lua_newtable(L); // "PanelItem"

	PushFarFindData(L, &PanelItem->FindData);
	PutFlagsToTable(L, "Flags", PanelItem->Flags);
	PutNumToTable(L, "NumberOfLinks", PanelItem->NumberOfLinks);
	PutNumToTable(L, "CRC32", PanelItem->CRC32);

	if (PanelItem->Description)    PutWStrToTable(L, "Description",  PanelItem->Description, -1);
	if (PanelItem->Owner)          PutWStrToTable(L, "Owner",  PanelItem->Owner, -1);
	if (PanelItem->Group)          PutWStrToTable(L, "Group",  PanelItem->Group, -1);

	if (PanelItem->CustomColumnNumber > 0) {
		lua_createtable (L, PanelItem->CustomColumnNumber, 0);
		for(int j=0; j < PanelItem->CustomColumnNumber; j++)
			PutWStrToArray(L, j+1, PanelItem->CustomColumnData[j], -1);
		lua_setfield(L, -2, "CustomColumnData");
	}

	if (PanelItem->UserData && lua_istable(L, -2)) {
		lua_getfield(L, -2, COLLECTOR_UD);
		if (lua_istable(L,-1)) {
			lua_rawgeti(L, -1, (int)PanelItem->UserData);
			lua_setfield(L, -3, "UserData");
		}
		lua_pop(L,1);
	}
	else {
		lua_pushlightuserdata(L, (void*)PanelItem->UserData);
		lua_setfield(L, -2, "UserData");
	}
}

void PutRECTToTable(lua_State *L, const char* key, RECT rect)
{
	lua_createtable(L, 0, 4);
	PutIntToTable(L, "left", rect.left);
	PutIntToTable(L, "top", rect.top);
	PutIntToTable(L, "right", rect.right);
	PutIntToTable(L, "bottom", rect.bottom);
	lua_setfield(L, -2, key);
}
//---------------------------------------------------------------------------

void PushPanelItems(lua_State *L, HANDLE handle, const struct PluginPanelItem *PanelItems, int ItemsNumber)
{
	lua_createtable(L, ItemsNumber, 0);    //+1 "PanelItems"
	PushOptPluginTable(L, handle);         //+2

	for(int i=0; i < ItemsNumber; i++)
	{
		PushPanelItem(L, PanelItems + i);
		lua_rawseti(L, -3, i+1);
	}
	lua_pop(L, 1);                         //+1
}
//---------------------------------------------------------------------------

static int far_PluginStartupInfo(lua_State *L)
{
	TPluginData *pd = GetPluginData(L);
	lua_createtable(L, 0, 5);
	PutWStrToTable(L, "ModuleName", pd->ModuleName, -1);

	const wchar_t *slash = wcsrchr(pd->ModuleName, GOOD_SLASH);
	if (slash)
		PutWStrToTable(L, "ModuleDir", pd->ModuleName, slash - pd->ModuleName);

	lua_pushlightuserdata(L, (void*)pd->ModuleNumber);
	lua_setfield(L, -2, "ModuleNumber");

	lua_pushinteger(L, pd->PluginId);
	lua_setfield(L, -2, "PluginId");

	PutStrToTable(L, "ShareDir", pd->ShareDir);
	return 1;
}

static int far_GetPluginId(lua_State *L)
{
	lua_pushinteger(L, GetPluginData(L)->PluginId);
	return 1;
}

static void PushGlobalInfo(lua_State *L, const struct GlobalInfo *info)
{
	lua_createtable(L, 0, 7);
	PutNumToTable  (L, "StructSize", info->StructSize);
	PutNumToTable  (L, "SysID", info->SysID);
	PutWStrToTable (L, "Title", info->Title, -1);
	PutWStrToTable (L, "Description", info->Description, -1);
	PutWStrToTable (L, "Author", info->Author, -1);
	PutBoolToTable (L, "UseMenuGuids", info->UseMenuGuids);

	lua_createtable(L, 4, 0);
	PutIntToArray(L, 1, info->Version.Major);
	PutIntToArray(L, 2, info->Version.Minor);
	PutIntToArray(L, 3, info->Version.Revision);
	PutIntToArray(L, 4, info->Version.Build);
	lua_setfield(L, -2, "Version");
}

static int far_GetPluginGlobalInfo(lua_State *L)
{
	struct GlobalInfo info;
	GetPluginData(L)->GetGlobalInfo(&info);
	PushGlobalInfo(L, &info);
	return 1;
}

static int far_GetCurrentDirectory (lua_State *L)
{
	int size = FSF.GetCurrentDirectory(0, NULL);
	wchar_t* buf = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
	FSF.GetCurrentDirectory(size, buf);
	push_utf8_string(L, buf, -1);
	return 1;
}

int SetKeyBar(lua_State *L, BOOL IsEditor)
{
	enum {
		POS_FRAME = 1,
		POS_PARAM = 2,
		POS_STORE = 3,
	}; // stack positions

	enum {
		REDRAW  = -1,
		RESTORE = 0
	}; // corresponds to FAR API

	void* param = NULL;
	struct KeyBarTitles kbt;
	int frameId = luaL_optinteger(L, POS_FRAME, -1);
	BOOL argfail = FALSE;

	if (lua_isstring(L, POS_PARAM)) {
		const char* p = lua_tostring(L, POS_PARAM);
		if (0 == strcmp("redraw", p))
			param = (void*)REDRAW;
		else if (0 == strcmp("restore", p))
			param = (void*)RESTORE;
		else
			argfail = TRUE;
	}
	else if (lua_istable(L, POS_PARAM)) {
		param = &kbt;
		memset(&kbt, 0, sizeof(kbt));
		struct { const char* key; wchar_t** trg; } pairs[] = {
			{"Titles",          kbt.Titles},
			{"CtrlTitles",      kbt.CtrlTitles},
			{"AltTitles",       kbt.AltTitles},
			{"ShiftTitles",     kbt.ShiftTitles},
			{"CtrlShiftTitles", kbt.CtrlShiftTitles},
			{"AltShiftTitles",  kbt.AltShiftTitles},
			{"CtrlAltTitles",   kbt.CtrlAltTitles},
		};
		lua_settop(L, POS_PARAM);
		lua_newtable(L); // POS_STORE
		for (size_t i=0; i < ARRAYSIZE(pairs); i++) {
			lua_getfield (L, POS_PARAM, pairs[i].key);
			if (lua_istable (L, -1)) {
				for (size_t j=0; j < ARRAYSIZE(kbt.Titles); j++) {
					lua_pushinteger(L,j+1);
					lua_gettable(L,-2);
					if (lua_isstring(L,-1))
						pairs[i].trg[j] = (wchar_t*)StoreTempString(L, POS_STORE);
					else
						lua_pop(L,1);
				}
			}
			lua_pop (L, 1);
		}
	}
	else
		argfail = TRUE;

	if (argfail)
		return luaL_argerror(L, POS_PARAM, "must be 'redraw', 'restore', or table");

	int result = IsEditor ? PSInfo.EditorControlV2(frameId, ECTL_SETKEYBAR, param) :
	                        PSInfo.ViewerControlV2(frameId, VCTL_SETKEYBAR, param);
	lua_pushboolean(L, result != 0);
	return 1;
}

static int viewer_SetKeyBar(lua_State *L)
{
	return SetKeyBar(L, FALSE);
}

void PushFarColor(lua_State *L, uint64_t value)
{
	DWORD Flags = 0;

	lua_newtable(L);
	if (value >> 16)
	{
		PutIntToTable(L, "ForegroundColor", (value >> 16) & 0xFFFFFF);
		PutIntToTable(L, "BackgroundColor", (value >> 40) & 0xFFFFFF);

		if (value & COMMON_LVB_UNDERSCORE) Flags |= FCF_FG_UNDERLINE_MASK;
		if (value & COMMON_LVB_STRIKEOUT)  Flags |= FCF_FG_STRIKEOUT;
	}
	else
	{
		PutIntToTable(L, "ForegroundColor", value & 0x0F);
		PutIntToTable(L, "BackgroundColor", (value >> 4) & 0x0F);
		Flags |= (FCF_FG_INDEX | FCF_BG_INDEX);
	}

	PutIntToTable(L, "Flags", Flags);
}

static void FarTrueColorFromRGB(struct FarTrueColor *out, DWORD rgb, int used)
{
	out->Flags = used ? 1 : 0;
	out->R = rgb & 0xff;
	out->G = (rgb >> 8) & 0xff;
	out->B = (rgb >> 16) & 0xff;
}

DWORD RGBFromFarTrueColor(const struct FarTrueColor *tc)
{
	return (tc->R) | (tc->G << 8) | (tc->B << 16);
}

// partially taken from the Colorer plugin
uint64_t GetFarColor(lua_State *L, int pos, struct FarTrueColorForeAndBack *fullcolor,
		int *basecolor, int *isTrueColor)
{
	memset(fullcolor, 0, sizeof(*fullcolor));
	*basecolor = 0;
	*isTrueColor = 0;

	if (lua_istable(L, pos))
	{
		lua_pushvalue(L, pos);
		uint64_t FgColor = GetOptIntFromTable(L, "ForegroundColor", 0) & 0x00FFFFFF;
		uint64_t BgColor = GetOptIntFromTable(L, "BackgroundColor", 0) & 0x00FFFFFF;
		lua_pop(L, 1);

		FAR3COLORFLAGS Flags = CheckFlagsFromTable(L, pos, "Flags");
		if (Flags & FCF_INDEXMASK)
		{
			*basecolor = (FgColor & 0x0F) | ((BgColor & 0x0F) << 4);
		}
		else if (FgColor || BgColor)
		{
			*isTrueColor = 1;
			struct FarTrueColor *Fore = &fullcolor->Fore;
			struct FarTrueColor *Back = &fullcolor->Back;

			FarTrueColorFromRGB(Fore, FgColor, 1);
			FarTrueColorFromRGB(Back, BgColor, 1);

			*basecolor |= (FOREGROUND_TRUECOLOR | BACKGROUND_TRUECOLOR);

			if (Fore->R > 0x10)  *basecolor |= FOREGROUND_RED;
			if (Fore->G > 0x10)  *basecolor |= FOREGROUND_GREEN;
			if (Fore->B > 0x10)  *basecolor |= FOREGROUND_BLUE;

			if (Back->R > 0x10)  *basecolor |= BACKGROUND_RED;
			if (Back->G > 0x10)  *basecolor |= BACKGROUND_GREEN;
			if (Back->B > 0x10)  *basecolor |= BACKGROUND_BLUE;

			if (Flags & FCF_FG_UNDERLINE_MASK)  *basecolor |= COMMON_LVB_UNDERSCORE;
			if (Flags & FCF_FG_STRIKEOUT)       *basecolor |= COMMON_LVB_STRIKEOUT;

			return (BgColor << 40) | (FgColor << 16) | (*basecolor & 0xFFFF);
		}
	}
	else if (lua_isnumber(L, pos))
	{
		*basecolor = lua_tointeger(L, pos) & 0xFF;
	}
	else if (lua_isnoneornil(L, pos))
	{
		*basecolor = 0x0F;
	}
	else
		return luaL_argerror(L, pos, "table or number expected");

	return *basecolor;
}

uint64_t GetFarColor64(lua_State *L, int pos)
{
	struct FarTrueColorForeAndBack fullcolor;
	int basecolor, istruecolor;
	return GetFarColor(L, pos, &fullcolor, &basecolor, &istruecolor);
}

static int far_KeyToName (lua_State *L);

static void PushNameFromInputRecord(lua_State *L, const INPUT_RECORD *Rec)
{
	lua_pushcfunction(L, far_KeyToName);
	lua_pushinteger(L, FSF.FarInputRecordToKey(Rec));
	lua_call(L, 1, 1);
}

static void PushNameFromKey(lua_State *L, FarKey Key)
{
	lua_pushcfunction(L, far_KeyToName);
	lua_pushinteger(L, Key);
	lua_call(L, 1, 1);
}

void PushInputRecord (lua_State* L, const INPUT_RECORD* ir)
{
	lua_newtable(L);                   //+2: Func,Tbl
	PutIntToTable(L, "EventType", ir->EventType);

	switch(ir->EventType)
	{
		case KEY_EVENT:
			PutBoolToTable(L,"KeyDown", ir->Event.KeyEvent.bKeyDown);
			PutNumToTable(L, "RepeatCount", ir->Event.KeyEvent.wRepeatCount);
			PutNumToTable(L, "VirtualKeyCode", ir->Event.KeyEvent.wVirtualKeyCode);
			PutNumToTable(L, "VirtualScanCode", ir->Event.KeyEvent.wVirtualScanCode);
			PutWStrToTable(L, "UnicodeChar", &ir->Event.KeyEvent.uChar.UnicodeChar, 1);
			PutNumToTable(L, "ControlKeyState", ir->Event.KeyEvent.dwControlKeyState);
			PushNameFromInputRecord(L, ir);
			lua_setfield(L, -2, "FarKeyName");
			break;

		case MOUSE_EVENT:
			PutMouseEvent(L, &ir->Event.MouseEvent, TRUE);
			break;

		case WINDOW_BUFFER_SIZE_EVENT:
			PutNumToTable(L, "SizeX", ir->Event.WindowBufferSizeEvent.dwSize.X);
			PutNumToTable(L, "SizeY", ir->Event.WindowBufferSizeEvent.dwSize.Y);
			break;

		case MENU_EVENT:
			PutNumToTable(L, "CommandId", ir->Event.MenuEvent.dwCommandId);
			break;

		case FOCUS_EVENT:
			PutBoolToTable(L,"SetFocus", ir->Event.FocusEvent.bSetFocus);
			break;

		default:
			break;
	}
}

void FillInputRecord(lua_State *L, int pos, INPUT_RECORD *ir)
{
	int success = 0;
	pos = abs_index(L, pos);
	luaL_checktype(L, pos, LUA_TTABLE);
	memset(ir, 0, sizeof(INPUT_RECORD));
	// determine event type
	lua_getfield(L, pos, "EventType");
	ir->EventType = get_env_flag(L, -1, &success);
	if (success)
	{
		if (ir->EventType == 0)
		{
			ir->EventType = KEY_EVENT;
		}
		success = ir->EventType == KEY_EVENT
			|| ir->EventType == MOUSE_EVENT
			|| ir->EventType == WINDOW_BUFFER_SIZE_EVENT
			|| ir->EventType == MENU_EVENT
			|| ir->EventType == FOCUS_EVENT;
	}
	if (!success)
		luaL_error(L, "invalid 'EventType' specified");

	lua_pop(L, 1);
	lua_pushvalue(L, pos);

	switch(ir->EventType)
	{
		case KEY_EVENT:
			ir->Event.KeyEvent.bKeyDown = GetOptBoolFromTable(L, "KeyDown", TRUE);
			ir->Event.KeyEvent.wRepeatCount = GetOptIntFromTable(L, "RepeatCount", 1);
			ir->Event.KeyEvent.wVirtualKeyCode = GetOptIntFromTable(L, "VirtualKeyCode", 0);
			ir->Event.KeyEvent.wVirtualScanCode = GetOptIntFromTable(L, "VirtualScanCode", 0);
			lua_getfield(L, -1, "UnicodeChar");
			if (lua_type(L,-1) == LUA_TSTRING) {
				size_t size;
				wchar_t* ptr = utf8_to_wcstring(L, -1, &size);
				if (ptr && size>=1)
					ir->Event.KeyEvent.uChar.UnicodeChar = ptr[0];
			}
			lua_pop(L, 1);
			ir->Event.KeyEvent.dwControlKeyState = GetOptIntFromTable(L, "ControlKeyState", 0);
			break;

		case MOUSE_EVENT:
			GetMouseEvent(L, &ir->Event.MouseEvent);
			break;

		case WINDOW_BUFFER_SIZE_EVENT:
			ir->Event.WindowBufferSizeEvent.dwSize.X = GetOptIntFromTable(L, "SizeX", 0);
			ir->Event.WindowBufferSizeEvent.dwSize.Y = GetOptIntFromTable(L, "SizeY", 0);
			break;

		case MENU_EVENT:
			ir->Event.MenuEvent.dwCommandId = GetOptIntFromTable(L, "CommandId", 0);
			break;

		case FOCUS_EVENT:
			ir->Event.FocusEvent.bSetFocus = GetOptBoolFromTable(L, "SetFocus", FALSE);
			break;
	}

	lua_pop(L, 1);
}

typedef struct {
	lua_State *L;
	int nargs;
	int was_error;
} MENU_DATA;

static int FarMenuCallback(void *Data, int MenuPos, FarKey Key)
{
	if (Key != KEY_IDLE) {
		MENU_DATA *mdata = (MENU_DATA*) Data;
		lua_State *L = mdata->L;
		int pos_func = lua_gettop(L) - mdata->nargs;

		lua_pushvalue(L, pos_func);          // Callback function
		lua_pushinteger(L, MenuPos + 1);     // MenuPos
		PushNameFromKey(L, Key);

		for (int idx = 1; idx <= mdata->nargs; idx++)
			lua_pushvalue(L, pos_func + idx);

		if (0 == lua_pcall(L, 2 + mdata->nargs, 1, 0)) {
			int ret = lua_toboolean(L,-1) ? lua_tointeger(L,-1) : FMCB_PROCESSKEY;
			lua_pop(L, 1);
			return ret;
		}

		mdata->was_error = 1;
		return FMCB_CANCELMENU;
	}
	return FMCB_PROCESSKEY;
}

// Item, Position = Menu (Properties, Items, Breakkeys, Callback, ...)
// Parameters:
//   Properties -- a table
//   Items      -- an array (each array item may be either a table or a string)
//   BreakKeys  -- (optional) either an array of tables or a string
//   Callback   -- (optional) a callback function
//   ...        -- (optional) zero or more callback parameters of any type
// Return value:
//   Item:
//     a table  -- the table of selected item (or of breakkey) is returned
//     a nil    -- menu canceled by the user
//   Position:
//     a number -- position of selected menu item
//     a nil    -- menu canceled by the user
static int far_Menu(lua_State *L)
{
	enum {
		POS_PROPS = 1, // properties
		POS_ITEMS = 2, // items
		POS_BKEYS = 3, // break keys
		POS_CBACK = 4, // callback
	};

	if (lua_gettop(L) < POS_CBACK) // don't remove this!
		lua_settop(L, POS_CBACK);

	MENU_DATA mdata = { L, lua_gettop(L) - POS_CBACK, 0 };

	luaL_checktype(L, POS_PROPS, LUA_TTABLE);
	luaL_checktype(L, POS_ITEMS, LUA_TTABLE);
	if (lua_toboolean(L,POS_BKEYS) && !lua_istable(L,POS_BKEYS) && !lua_isstring(L,POS_BKEYS))
		return luaL_argerror(L, POS_BKEYS, "must be table, string or nil");

	if (lua_toboolean(L,POS_CBACK) && !lua_isfunction(L,POS_CBACK))
		return luaL_argerror(L, POS_CBACK, "must be function or nil");

	lua_newtable(L); // temporary store
	const int POS_STORE = lua_gettop(L);

	// Properties
	lua_pushvalue (L,POS_PROPS);  // push Properties on top
	int X = GetOptIntFromTable(L, "X", -1);
	int Y = GetOptIntFromTable(L, "Y", -1);
	int MaxHeight = GetOptIntFromTable(L, "MaxHeight", 0);

	int Flags = FMENU_WRAPMODE;
	lua_getfield(L, POS_PROPS, "Flags");
	if (!lua_isnil(L, -1)) Flags = CheckFlags(L, -1);

	const wchar_t *Title = L"Menu";
	lua_getfield(L, POS_PROPS, "Title");
	if (lua_isstring(L,-1))    Title = StoreTempString(L, POS_STORE);

	const wchar_t *Bottom = NULL;
	lua_getfield(L, POS_PROPS, "Bottom");
	if (lua_isstring(L,-1))    Bottom = StoreTempString(L, POS_STORE);

	const wchar_t *HelpTopic = NULL;
	lua_getfield(L, POS_PROPS, "HelpTopic");
	if (lua_isstring(L,-1))    HelpTopic = StoreTempString(L, POS_STORE);

	lua_getfield(L, POS_PROPS, "SelectIndex");
	int SelectIndex = lua_tointeger(L,-1) - 1;

	lua_getfield(L, POS_PROPS, "Id");
	GUID MenuGuid = OptGuid(L,-1);

	int ItemsNumber = lua_objlen(L, POS_ITEMS);
	if (!(SelectIndex >= 0 && SelectIndex < ItemsNumber))
		SelectIndex = -1;

	lua_settop (L, POS_STORE);

	// Items
	struct FarMenuItemEx *Items =
		(struct FarMenuItemEx*) lua_newuserdata(L, ItemsNumber*sizeof(struct FarMenuItemEx));
	luaL_ref(L, POS_STORE);
	memset(Items, 0, ItemsNumber*sizeof(struct FarMenuItemEx));
	struct FarMenuItemEx *pItem = Items;

	for(int i=0; i < ItemsNumber; i++,pItem++,lua_pop(L,1))
	{
		static const char key[] = "text";
		lua_pushinteger(L, i+1);
		lua_gettable(L, POS_ITEMS);

		if (lua_isstring(L, -1)) { // convert a string to a table element
			lua_createtable(L, 0, 1);
			lua_insert(L, -2);
			lua_setfield(L, -2, key);
		}
		else if (!lua_istable(L, -1))
			return luaLF_SlotError(L, i+1, "string or table");

		//-------------------------------------------------------------------------
		lua_getfield(L, -1, key);

		if (lua_isstring(L,-1))  pItem->Text = StoreTempString(L, POS_STORE);
		else if (!lua_isnil(L,-1)) return luaLF_FieldError(L, key, "string");

		if (!pItem->Text)
			lua_pop(L, 1);

		//-------------------------------------------------------------------------
		lua_getfield(L,-1,"checked");

		if (lua_type(L,-1) == LUA_TSTRING)
		{
			const wchar_t* s = utf8_to_wcstring(L,-1,NULL);
			if (s) pItem->Flags |= s[0];
		}
		else if (lua_toboolean(L,-1)) pItem->Flags |= MIF_CHECKED;
		lua_pop(L,1);
		//-------------------------------------------------------------------------
		if (SelectIndex == -1) {
			lua_getfield(L,-1,"selected");
			if (lua_toboolean(L,-1)) {
				pItem->Flags |= MIF_SELECTED;
				SelectIndex = i;
			}
			lua_pop(L,1);
		}
		//-------------------------------------------------------------------------
		if (GetBoolFromTable(L, "separator")) pItem->Flags |= MIF_SEPARATOR;

		if (GetBoolFromTable(L, "disable"))   pItem->Flags |= MIF_DISABLE;

		if (GetBoolFromTable(L, "grayed"))    pItem->Flags |= MIF_GRAYED;

		if (GetBoolFromTable(L, "hidden"))    pItem->Flags |= MIF_HIDDEN;
		//-------------------------------------------------------------------------
		lua_getfield(L, -1, "AccelKey");
		if (lua_isnumber(L,-1)) pItem->AccelKey = lua_tointeger(L,-1);
		lua_pop(L, 1);
	}
	if (SelectIndex != -1)
		Items[SelectIndex].Flags |= MIF_SELECTED;

	// Break Keys
	int BreakCode = 0;
	int NumBreakCodes = 0;
	int *pBreakKeys = NULL;
	int *pBreakCode = NULL;
	if (lua_type(L, POS_BKEYS) == LUA_TSTRING)
	{
		const char *ptr = lua_tostring(L, POS_BKEYS);
		lua_newtable(L);
		while (*ptr)
		{
			while (isspace(*ptr)) ptr++;
			if (*ptr == 0) break;
			const char *q = ptr++;
			while(*ptr && !isspace(*ptr)) ptr++;
			lua_createtable(L,0,1);
			lua_pushlstring(L,q,ptr-q);
			lua_setfield(L,-2,"BreakKey");
			lua_rawseti(L,-2,++NumBreakCodes);
		}
		lua_replace(L, POS_BKEYS);
	}
	else
		NumBreakCodes = lua_istable(L,POS_BKEYS) ? (int)lua_objlen(L,POS_BKEYS) : 0;

	if (NumBreakCodes)
	{
		int* BreakKeys = (int*)lua_newuserdata(L, (1+NumBreakCodes)*sizeof(int));
		luaL_ref(L, POS_STORE);
		// get virtualkeys table from the registry; push it on top
		lua_pushstring(L, FAR_VIRTUALKEYS);
		lua_rawget(L, LUA_REGISTRYINDEX);
		// push breakkeys table on top
		lua_pushvalue(L, POS_BKEYS);        // vk=-2; bk=-1;

		int ind_target = 0;
		for (int ind=0; ind < NumBreakCodes; ind++)
		{
			// get next break key (optional modifier plus virtual key)
			lua_pushinteger(L,ind+1);       // vk=-3; bk=-2;
			lua_gettable(L,-2);             // vk=-3; bk=-2;

			if (!lua_istable(L,-1)) { lua_pop(L,1); continue; }

			lua_getfield(L, -1, "BreakKey");// vk=-4; bk=-3;

			if (!lua_isstring(L,-1)) { lua_pop(L,2); continue; }

			// first try to use "Far key names" instead of "virtual key names"
			if (utf8_to_wcstring(L, -1, NULL))
			{
				INPUT_RECORD Rec;
				if (FSF.FarNameToInputRecord((const wchar_t*)lua_touserdata(L,-1), &Rec)
					&& Rec.EventType == KEY_EVENT)
				{
					int mod = 0;
					DWORD Code = Rec.Event.KeyEvent.wVirtualKeyCode;
					DWORD State = Rec.Event.KeyEvent.dwControlKeyState;
					if (State & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) mod |= PKF_CONTROL;
					if (State & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))   mod |= PKF_ALT;
					if (State & SHIFT_PRESSED)                          mod |= PKF_SHIFT;
					BreakKeys[ind_target++] = (Code & 0xFFFF) | (mod << 16);
					lua_pop(L, 2);
					continue; // success
				}
				// restore the original string
				lua_pop(L, 1);
				lua_getfield(L, -1, "BreakKey");// vk=-4; bk=-3;bki=-2;bknm=-1;
			}

			// separate modifier and virtual key strings
			const char* s = lua_tostring(L,-1);

			char buf[32];
			int mod = 0;
			if (strlen(s) >= sizeof(buf)) { lua_pop(L,2); continue; }

			char* vk = buf;
			do *vk++ = toupper(*s); while(*s++); // copy and convert to upper case
			vk = strchr(buf, '+');  // virtual key

			if (vk)
			{
				*vk++ = '\0';

				if (strchr(buf,'C')) mod |= PKF_CONTROL;

				if (strchr(buf,'A')) mod |= PKF_ALT;

				if (strchr(buf,'S')) mod |= PKF_SHIFT;

				mod <<= 16;
				// replace on stack: break key name with virtual key name
				lua_pop(L, 1);
				lua_pushstring(L, vk);
			}
			// get virtual key and break key values
			lua_rawget(L,-4);               // vk=-4; bk=-3;
			int tmp = lua_tointeger(L,-1) | mod;
			if (tmp) BreakKeys[ind_target++] = tmp;
			lua_pop(L,2);                   // vk=-2; bk=-1;
		}
		BreakKeys[ind_target] = 0; // required by FAR API
		pBreakKeys = BreakKeys;
		pBreakCode = &BreakCode;
	}

	FARMENUCALLBACK callback = NULL;
	if (lua_isfunction(L, POS_CBACK)) {
		callback = FarMenuCallback;
		lua_settop(L, POS_STORE);
		lua_replace(L, POS_PROPS);
	}

	TPluginData *pd = GetPluginData(L);
	int ret = PSInfo.MenuV2(
		pd->ModuleNumber, &MenuGuid, X, Y, MaxHeight, Flags|FMENU_USEEXT,
		Title, Bottom, HelpTopic, pBreakKeys, pBreakCode,
		(const struct FarMenuItem *)Items, ItemsNumber, callback, &mdata);

	if (mdata.was_error)
	{
		const char *msg = lua_tostring(L, -1);
		msg = msg ? msg : "error occured in callback";
		return luaL_error(L, msg);
	}
	else if (NumBreakCodes && (BreakCode != -1))
	{
		lua_pushinteger(L, BreakCode+1);
		lua_gettable(L, POS_BKEYS);
	}
	else if (ret == -1)
		return lua_pushnil(L), 1;
	else
	{
		lua_pushinteger(L, ret+1);
		lua_gettable(L, POS_ITEMS);
	}

	lua_pushinteger(L, ret+1);
	return 2;
}

// Return:   -1 if escape pressed, else - button number chosen (0 based).
int LF_Message(lua_State* L,
	const wchar_t* aMsg,      // if multiline, then lines must be separated by '\n'
	const wchar_t* aTitle,
	const wchar_t* aButtons,  // if multiple, then captions must be separated by ';'
	const char*    aFlags,
	const wchar_t* aHelpTopic,
	const GUID*    aMessageGuid)
{
	TPluginData *pd = GetPluginData(L);
	SMALL_RECT sr;
	int ret = PSInfo.AdvControl(pd->ModuleNumber, ACTL_GETFARRECT, &sr, NULL);
	const int max_len = ret ? sr.Right - sr.Left + 1 - 14 : 66;
	const int max_lines = ret ? sr.Bottom - sr.Top + 1 - 5 : 20;

	int num_lines = 0, num_buttons = 0;

	// Buttons
	wchar_t *BtnCopy = NULL;
	int wrap = !(aFlags && strchr(aFlags, 'n'));
	uint64_t Flags = 0;

	if (*aButtons == L';')
	{
		const wchar_t* p = aButtons + 1;

		if (!wcscasecmp(p, L"Ok"))                    Flags = FMSG_MB_OK;
		else if (!wcscasecmp(p, L"OkCancel"))         Flags = FMSG_MB_OKCANCEL;
		else if (!wcscasecmp(p, L"AbortRetryIgnore")) Flags = FMSG_MB_ABORTRETRYIGNORE;
		else if (!wcscasecmp(p, L"YesNo"))            Flags = FMSG_MB_YESNO;
		else if (!wcscasecmp(p, L"YesNoCancel"))      Flags = FMSG_MB_YESNOCANCEL;
		else if (!wcscasecmp(p, L"RetryCancel"))      Flags = FMSG_MB_RETRYCANCEL;
		else
			while(*aButtons == L';') aButtons++;
	}

	if (Flags == 0)
	{
		// Buttons: 1-st pass, determining number of buttons
		BtnCopy = _wcsdup(aButtons);
		wchar_t *ptr = BtnCopy;

		while(*ptr && (num_buttons < 64))
		{
			while(*ptr == L';')
				ptr++; // skip semicolons

			if (*ptr)
			{
				++num_buttons;
				ptr = wcschr(ptr, L';');

				if (!ptr) break;
			}
		}
	}

	const wchar_t **items = (const wchar_t**) malloc((1+max_lines+num_buttons) * sizeof(wchar_t*));
	wchar_t **allocLines = (wchar_t**) malloc(max_lines * sizeof(wchar_t*)); // array of pointers to allocated lines
	int nAlloc = 0; // number of allocated lines

	// Title
	const wchar_t **pItems = items;
	*pItems++ = aTitle;

	// Message lines
	wchar_t *lastDelim = NULL;
	wchar_t *MsgCopy = _wcsdup(aMsg);
	wchar_t *start = MsgCopy, *pos = MsgCopy;

	while(num_lines < max_lines)
	{
		if (*pos == 0)                          // end of the entire message
		{
			*pItems++ = start;
			++num_lines;
			break;
		}
		else if (*pos == L'\n')                 // end of a message line
		{
			*pItems++ = start;
			*pos = L'\0';
			++num_lines;
			start = ++pos;
			lastDelim = NULL;
		}
		else if (pos-start < max_len)            // characters inside the line
		{
			if (wrap && !iswalnum(*pos) && *pos != L'_' && *pos != L'\'' && *pos != L'\"')
				lastDelim = pos;

			pos++;
		}
		else if (wrap)                          // the 1-st character beyond the line
		{
			pos = lastDelim ? lastDelim+1 : pos;
			size_t len = pos - start;
			wchar_t **q = &allocLines[nAlloc++]; // line allocation is needed
			*pItems++ = *q = (wchar_t*) malloc((len+1)*sizeof(wchar_t));
			wcsncpy(*q, start, len);
			(*q)[len] = L'\0';
			++num_lines;
			start = pos;
			lastDelim = NULL;
		}
		else
			pos++;
	}

	if (*aButtons != L';')
	{
		// Buttons: 2-nd pass.
		wchar_t *ptr = BtnCopy;

		for(int i=0; i < num_buttons; i++)
		{
			while(*ptr == L';')
				++ptr;

			if (*ptr)
			{
				*pItems++ = ptr;
				ptr = wcschr(ptr, L';');

				if (ptr)
					*ptr++ = 0;
				else
					break;
			}
			else break;
		}
	}

	// Flags
	if (aFlags)
	{
		if (strchr(aFlags, 'w')) Flags |= FMSG_WARNING;
		if (strchr(aFlags, 'e')) Flags |= FMSG_ERRORTYPE;
		if (strchr(aFlags, 'k')) Flags |= FMSG_KEEPBACKGROUND;
		if (strchr(aFlags, 'l')) Flags |= FMSG_LEFTALIGN;
	}

	ret = PSInfo.MessageV3(pd->ModuleNumber, aMessageGuid, Flags, aHelpTopic, items,
			1 + num_lines + num_buttons, num_buttons);

	free(BtnCopy);
	while (nAlloc) {
		free(allocLines[--nAlloc]);
	}
	free(allocLines);
	free(MsgCopy);
	free(items);

	return ret;
}

// Taken from Lua 5.1 (luaL_gsub) and modified
const wchar_t *LF_Gsub (lua_State *L, const wchar_t *s, const wchar_t *p, const wchar_t *r)
{
	const wchar_t *wild;
	size_t l = wcslen(p);
	size_t l2 = sizeof(wchar_t) * wcslen(r);
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	while ((wild = wcsstr(s, p)) != NULL) {
		luaL_addlstring(&b, (void*)s, sizeof(wchar_t) * (wild - s));  /* push prefix */
		luaL_addlstring(&b, (void*)r, l2);  /* push replacement in place of pattern */
		s = wild + l;  /* continue after `p' */
	}
	luaL_addlstring(&b, (void*)s, sizeof(wchar_t) * wcslen(s));  /* push last suffix */
	luaL_addlstring(&b, (void*)L"\0", sizeof(wchar_t));  /* push L'\0' */
	luaL_pushresult(&b);
	return (wchar_t*) lua_tostring(L, -1);
}

void LF_Error(lua_State *L, const wchar_t* aMsg)
{
	TPluginData *pd = GetPluginData(L);
	if (!aMsg) aMsg = L"<non-string error message>";

	lua_pushlstring(L, (const char*)pd->ModuleName, wcslen(pd->ModuleName)*sizeof(wchar_t));
	lua_pushlstring(L, (const char*)L":\n", sizeof(wchar_t) * 2);
	LF_Gsub(L, aMsg, L"\n\t", L"\n   ");
	lua_concat(L, 3);
	LF_Message(L, (const wchar_t*)lua_tostring(L,-1), L"Error", L"OK", "wl", NULL, NULL);
	lua_pop(L, 1);
}

// 1-st param: message text (if multiline, then lines must be separated by '\n')
// 2-nd param: message title (if absent or nil, then "Message" is used)
// 3-rd param: buttons (if multiple, then captions must be separated by ';';
//             if absent or nil, then one button "OK" is used).
// 4-th param: flags
// 5-th param: help topic
// 6-th param: Id
// Return: -1 if escape pressed, else - button number chosen (1 based).
static int far_Message(lua_State *L)
{
	luaL_checkany(L,1);
	lua_settop(L,6);

	size_t MsgLen;
	const char *str = global_tolstring(L, 1, &MsgLen);
	char *copy = malloc(MsgLen);
	for (size_t i=0; i < MsgLen; i++) {
		copy[i] = str[i] ? str[i] : ' ';  // replace '\0' with a space
	}
	lua_pop(L, 1);
	lua_pushlstring(L, copy, MsgLen);
	free(copy);

	const wchar_t *Msg = check_utf8_string(L, -1, NULL);
	lua_replace(L,1);

	const wchar_t *Title     = opt_utf8_string(L, 2, L"Message");
	const wchar_t *Buttons   = opt_utf8_string(L, 3, L";OK");
	const char *Flags        = luaL_optstring(L, 4, "");
	const wchar_t *HelpTopic = opt_utf8_string(L, 5, NULL);
	GUID Id                  = OptGuid(L, 6);

	int ret = LF_Message(L, Msg, Title, Buttons, Flags, HelpTopic, &Id);
	lua_pushinteger(L, ret<0 ? ret : ret+1);
	return 1;
}

// GetDirList (Dir)
//   Dir:     Name of the directory to scan (full pathname).
static int far_GetDirList (lua_State *L)
{
	const wchar_t *Dir = check_utf8_string (L, 1, NULL);
	struct FAR_FIND_DATA *PanelItems;
	int ItemsNumber;
	int ret = PSInfo.GetDirList (Dir, &PanelItems, &ItemsNumber);
	if (ret) {
		lua_createtable(L, ItemsNumber, 0); // "PanelItems"
		for(int i=0; i < ItemsNumber; i++) {
			lua_newtable(L);
			PushFarFindData (L, PanelItems + i);
			lua_rawseti(L, -2, i+1);
		}
		PSInfo.FreeDirList (PanelItems, ItemsNumber);
		return 1;
	}
	return lua_pushnil(L), 1;
}

// GetPluginDirList (hPanel, Dir)
//   hPanel:    panel handle
//   Dir:       name of the directory to scan (full pathname)
static int far_GetPluginDirList (lua_State *L)
{
	struct PanelInfo pInfo;
	HANDLE hPanel = OptHandle(L);
	const wchar_t *Dir = opt_utf8_string (L, 2, NULL);

	if (PSInfo.Control(hPanel, FCTL_GETPANELINFO, 0, (LONG_PTR)&pInfo) && pInfo.OwnerHandle)
	{
		struct PluginPanelItem *PanelItems;
		int ItemsNumber;
		if (PSInfo.GetPluginDirList((INT_PTR)pInfo.OwnerHandle, hPanel, Dir, &PanelItems, &ItemsNumber))
		{
			PushPanelItems (L, hPanel, PanelItems, ItemsNumber);
			PSInfo.FreePluginDirList (PanelItems, ItemsNumber);
			return 1;
		}
	}
	return lua_pushnil(L), 1;
}

static int SavedScreen_tostring (lua_State *L)
{
	void **pp = (void**)luaL_checkudata(L, 1, SavedScreenType);
	if (*pp)
		lua_pushfstring(L, "%s (%p)", SavedScreenType, *pp);
	else
		lua_pushfstring(L, "%s (freed)", SavedScreenType);
	return 1;
}

// RestoreScreen (handle)
//   handle:    handle of saved screen.
static int far_RestoreScreen (lua_State *L)
{
	if (lua_isnoneornil(L, 1))
		PSInfo.RestoreScreen(NULL);
	else
	{
		void **pp = (void**)luaL_checkudata(L, 1, SavedScreenType);
		if (*pp)
		{
			PSInfo.RestoreScreen(*pp);
			*pp = NULL;
		}
	}
	return 0;
}

// FreeScreen (handle)
//   handle:    handle of saved screen.
static int far_FreeScreen(lua_State *L)
{
	void **pp = (void**)luaL_checkudata(L, 1, SavedScreenType);
	if (*pp)
	{
		PSInfo.FreeScreen(*pp);
		*pp = NULL;
	}
	return 0;
}

// handle = SaveScreen (X1,Y1,X2,Y2)
//   handle:    handle of saved screen, [lightuserdata]
static int far_SaveScreen (lua_State *L)
{
	int X1 = luaL_optinteger(L,1,0);
	int Y1 = luaL_optinteger(L,2,0);
	int X2 = luaL_optinteger(L,3,-1);
	int Y2 = luaL_optinteger(L,4,-1);

	*(void**)lua_newuserdata(L, sizeof(void*)) = PSInfo.SaveScreen(X1,Y1,X2,Y2);
	luaL_getmetatable(L, SavedScreenType);
	lua_setmetatable(L, -2);
	return 1;
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

static int far_ShowHelp(lua_State *L)
{
	const wchar_t *ModuleName = opt_utf8_string (L,1,NULL);
	const wchar_t *HelpTopic = opt_utf8_string (L,2,NULL);
	int Flags = OptFlags(L,3,0);
	BOOL ret = PSInfo.ShowHelp (ModuleName, HelpTopic, Flags);
	return lua_pushboolean(L, ret), 1;
}

// DestText = far.InputBox(Guid,Title,Prompt,HistoryName,SrcText,DestLength,HelpTopic,Flags)
// all arguments are optional
static int far_InputBox(lua_State *L)
{
	GUID Guid                  = OptGuid(L, 1);
	const wchar_t *Title       = opt_utf8_string (L, 2, L"Input Box");
	const wchar_t *Prompt      = opt_utf8_string (L, 3, L"Enter the text:");
	const wchar_t *HistoryName = opt_utf8_string (L, 4, NULL);
	const wchar_t *SrcText     = opt_utf8_string (L, 5, L"");
	int DestLength             = luaL_optinteger (L, 6, 1024);
	const wchar_t *HelpTopic   = opt_utf8_string (L, 7, NULL);
	flags_t Flags = OptFlags (L, 8, FIB_ENABLEEMPTY|FIB_BUTTONS|FIB_NOAMPERSAND);

	if (DestLength < 1) DestLength = 1;
	wchar_t *DestText = (wchar_t*) malloc(sizeof(wchar_t)*DestLength);
	int res = PSInfo.InputBoxV3(GetPluginData(L)->ModuleNumber, &Guid, Title, Prompt, HistoryName,
			SrcText, DestText, DestLength, HelpTopic, Flags);

	if (res) push_utf8_string (L, DestText, -1);
	else lua_pushnil(L);

	free(DestText);
	return 1;
}

static int far_GetMsg(lua_State *L)
{
	TPluginData* pd = GetPluginData(L);
	int MsgId = (int)luaL_checkinteger(L, 1);
	DWORD SysId = (DWORD)luaL_optinteger(L, 2, pd->PluginId);
	const wchar_t* Msg = NULL;

	if (MsgId >= 0) {
		intptr_t modNumber = (SysId == pd->PluginId) ? pd->ModuleNumber
				: PSInfo.PluginsControlV3(NULL, PCTL_FINDPLUGIN, PFM_SYSID, &SysId);
		if (modNumber)
			Msg = PSInfo.GetMsg(modNumber, MsgId);
	}

	if (Msg) push_utf8_string(L, Msg, -1);
	else lua_pushnil(L);
	return 1;
}

static int far_Text(lua_State *L)
{
	struct ColorDialogData Data = { 0 };

	int X = luaL_optinteger(L, 1, 0);
	int Y = luaL_optinteger(L, 2, 0);
	Data.Color = GetFarColor64(L, 3);
	const wchar_t *Str = opt_utf8_string(L, 4, NULL);
	PSInfo.TextV2(X, Y, &Data, Str);
	return 0;
}

static int far_CopyToClipboard(lua_State *L)
{
	const wchar_t *str = check_utf8_string(L,1,NULL);
	int ret = FSF.CopyToClipboard(str);
	return lua_pushboolean(L, ret), 1;
}

static int far_PasteFromClipboard (lua_State *L)
{
	wchar_t* str = FSF.PasteFromClipboard();
	if (str) {
		push_utf8_string(L, str, -1);
		FSF.DeleteBuffer(str);
	}
	else lua_pushnil(L);
	return 1;
}

static int far_KeyToName (lua_State *L)
{
	wchar_t buf[256];
	FarKey Key = (FarKey)luaL_checkinteger(L,1);
	if (Key == KEY_NONE)
		lua_pushstring(L, "None"); // because FarKeyToName(KEY_NONE) produces character \x1
	else if (FSF.FarKeyToName(Key, buf, ARRAYSIZE(buf)-1))
		push_utf8_string(L, buf, -1);
	else
		lua_pushnil(L);
	return 1;
}

static int far_NameToKey (lua_State *L)
{
	const wchar_t* str = check_utf8_string(L,1,NULL);
	if (!wcscmp(str, L"None"))
		lua_pushinteger(L, KEY_NONE);
	else {
		FarKey Key = FSF.FarNameToKey(str);
		if (Key == KEY_INVALID)
			lua_pushnil(L);
		else
			lua_pushinteger(L, Key);
	}
	return 1;
}

static int far_InputRecordToKey (lua_State *L)
{
	INPUT_RECORD ir;
	FillInputRecord(L, 1, &ir);
	lua_pushinteger(L, FSF.FarInputRecordToKey(&ir));
	return 1;
}

static int far_NameToInputRecord(lua_State *L)
{
	INPUT_RECORD ir;
	const wchar_t* str = check_utf8_string(L, 1, NULL);

	if (FSF.FarNameToInputRecord(str, &ir))
		PushInputRecord(L, &ir);
	else
		lua_pushnil(L);

	return 1;
}

static int far_LStricmp(lua_State *L)
{
	const wchar_t* s1 = check_utf8_string(L, 1, NULL);
	const wchar_t* s2 = check_utf8_string(L, 2, NULL);
	lua_pushinteger(L, FSF.LStricmp(s1, s2));
	return 1;
}

static int far_LStrnicmp(lua_State *L)
{
	const wchar_t* s1 = check_utf8_string(L, 1, NULL);
	const wchar_t* s2 = check_utf8_string(L, 2, NULL);
	int num = luaL_checkinteger(L, 3);
	if (num < 0) num = 0;
	lua_pushinteger(L, FSF.LStrnicmp(s1, s2, num));
	return 1;
}

static int far_LStrcmp(lua_State *L)
{
	const wchar_t* s1 = check_utf8_string(L, 1, NULL);
	const wchar_t* s2 = check_utf8_string(L, 2, NULL);
	lua_pushinteger(L, FSF.LStrcmp(s1, s2));
	return 1;
}

static int far_LStrncmp(lua_State *L)
{
	const wchar_t* s1 = check_utf8_string(L, 1, NULL);
	const wchar_t* s2 = check_utf8_string(L, 2, NULL);
	int num = luaL_checkinteger(L, 3);
	if (num < 0) num = 0;
	lua_pushinteger(L, FSF.LStrncmp(s1, s2, num));
	return 1;
}

static int _ProcessName (lua_State *L, int Op)
{
	int pos2=2, pos3=3, pos4=4;
	if (Op == -1)
		Op = CheckFlags(L, 1);
	else {
		--pos2, --pos3, --pos4;
		if (Op == PN_CHECKMASK)
			--pos4;
	}
	const wchar_t* Mask = check_utf8_string(L, pos2, NULL);
	const wchar_t* Name = (Op == PN_CHECKMASK) ? L"" : check_utf8_string(L, pos3, NULL);
	int Flags = Op | OptFlags(L, pos4, 0);

	if (Op == PN_CMPNAME || Op == PN_CMPNAMELIST || Op == PN_CHECKMASK) {
		int result = FSF.ProcessName(Mask, (wchar_t*)Name, 0, Flags);
		lua_pushboolean(L, result);
	}
	else if (Op == PN_GENERATENAME) {
		const int BUFSIZE = 1024;
		wchar_t* buf = (wchar_t*)lua_newuserdata(L, BUFSIZE * sizeof(wchar_t));
		wcsncpy(buf, Mask, BUFSIZE-1);
		buf[BUFSIZE-1] = 0;

		int result = FSF.ProcessName(Name, buf, BUFSIZE, Flags);
		if (result)
			push_utf8_string(L, buf, -1);
		else
			lua_pushboolean(L, result);
	}
	else
		luaL_argerror(L, 1, "command not supported");

	return 1;
}

static int far_ProcessName  (lua_State *L) { return _ProcessName(L, -1);              }
static int far_CmpName      (lua_State *L) { return _ProcessName(L, PN_CMPNAME);      }
static int far_CmpNameList  (lua_State *L) { return _ProcessName(L, PN_CMPNAMELIST);  }
static int far_CheckMask    (lua_State *L) { return _ProcessName(L, PN_CHECKMASK);    }
static int far_GenerateName (lua_State *L) { return _ProcessName(L, PN_GENERATENAME); }

static int far_GetReparsePointInfo(lua_State *L)
{
	const wchar_t* Src = check_utf8_string(L, 1, NULL);
	int size = FSF.GetReparsePointInfo(Src, NULL, 0);
	if (size <= 0)
		return lua_pushnil(L), 1;
	wchar_t* Dest = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
	FSF.GetReparsePointInfo(Src, Dest, size);
	return push_utf8_string(L, Dest, -1), 1;
}

static int far_LIsAlpha(lua_State *L)
{
	const wchar_t* str = check_utf8_string(L, 1, NULL);
	return lua_pushboolean(L, FSF.LIsAlpha(*str)), 1;
}

static int far_LIsAlphanum(lua_State *L)
{
	const wchar_t* str = check_utf8_string(L, 1, NULL);
	return lua_pushboolean(L, FSF.LIsAlphanum(*str)), 1;
}

static int far_LIsLower(lua_State *L)
{
	const wchar_t* str = check_utf8_string(L, 1, NULL);
	return lua_pushboolean(L, FSF.LIsLower(*str)), 1;
}

static int far_LIsUpper(lua_State *L)
{
	const wchar_t* str = check_utf8_string(L, 1, NULL);
	return lua_pushboolean(L, FSF.LIsUpper(*str)), 1;
}

static int convert_buf(lua_State *L, int command)
{
	const wchar_t* src = check_utf8_string(L, 1, NULL);
	int len;
	if (lua_isnoneornil(L,2))
		len = wcslen(src);
	else if (lua_isnumber(L,2)) {
		len = lua_tointeger(L,2);
		if (len < 0) len = 0;
	}
	else
		return luaL_typerror(L, 3, "optional number");
	wchar_t* dest = (wchar_t*)lua_newuserdata(L, (len+1)*sizeof(wchar_t));
	wcsncpy(dest, src, len+1);
	if (command=='l')
		FSF.LLowerBuf(dest,len);
	else
		FSF.LUpperBuf(dest,len);
	return push_utf8_string(L, dest, -1), 1;
}

static int far_LLowerBuf(lua_State *L)
{
	return convert_buf(L, 'l');
}

static int far_LUpperBuf(lua_State *L)
{
	return convert_buf(L, 'u');
}

static int far_MkTemp(lua_State *L)
{
	const wchar_t* prefix = opt_utf8_string(L, 1, NULL);
	const int dim = 4096;
	wchar_t* dest = (wchar_t*)lua_newuserdata(L, dim * sizeof(wchar_t));
	if (FSF.MkTemp(dest, dim, prefix))
		push_utf8_string(L, dest, -1);
	else
		lua_pushnil(L);
	return 1;
}

static int far_MkLink(lua_State *L)
{
	const wchar_t* target = check_utf8_string(L, 1, NULL);
	const wchar_t* linkname = check_utf8_string(L, 2, NULL);
	DWORD linktype = OptFlags(L, 3, FLINK_SYMLINK);
	flags_t flags = OptFlags(L, 4, 0);
	flags = (linktype & 0x0000FFFF) | (flags & 0xFFFF0000);
	lua_pushboolean(L, FSF.MkLink(target, linkname, flags));
	return 1;
}

static int truncstring (lua_State *L, int op)
{
	const wchar_t* Src = check_utf8_string(L, 1, NULL);
	int MaxLen = luaL_checkinteger(L, 2);
	int SrcLen = wcslen(Src);

	if (MaxLen < 0) MaxLen = 0;

	wchar_t* Trg = (wchar_t*)lua_newuserdata(L, (1 + SrcLen) * sizeof(wchar_t));
	wcscpy(Trg, Src);
	const wchar_t* ptr = (op == 'p') ?
		FSF.TruncPathStr(Trg, MaxLen) : FSF.TruncStr(Trg, MaxLen);
	return push_utf8_string(L, ptr, -1), 1;
}

static int far_TruncPathStr(lua_State *L)
{
	return truncstring(L, 'p');
}

static int far_TruncStr(lua_State *L)
{
	return truncstring(L, 's');
}

typedef struct
{
	lua_State *L;
	int nparams;
	int err;
	DWORD attr_incl;
	DWORD attr_excl;
} FrsData;

static int WINAPI FrsUserFunc(const struct FAR_FIND_DATA *FData, const wchar_t *FullName,
                              void *Param)
{
	FrsData *Data = (FrsData*)Param;
	lua_State *L = Data->L;
	int nret = lua_gettop(L);

	if ((FData->dwFileAttributes & Data->attr_excl) != 0 || (FData->dwFileAttributes & Data->attr_incl) != Data->attr_incl)
		return TRUE; // attributes mismatch

	lua_pushvalue(L, 3); // push the Lua function
	lua_newtable(L);
	PushFarFindData(L, FData);
	push_utf8_string(L, FullName, -1);
	for (int i=1; i<=Data->nparams; i++)
		lua_pushvalue(L, 4+i);

	Data->err = lua_pcall(L, 2+Data->nparams, LUA_MULTRET, 0);

	nret = lua_gettop(L) - nret;
	if (!Data->err && (nret==0 || lua_toboolean(L,-nret)==0))
	{
		lua_pop(L, nret);
		return TRUE;
	}
	return FALSE;
}

static int far_RecursiveSearch(lua_State *L)
{
	const wchar_t *InitDir = check_utf8_string(L, 1, NULL);
	wchar_t *Mask = check_utf8_string(L, 2, NULL);
	luaL_checktype(L, 3, LUA_TFUNCTION);
	FrsData Data = { L,0,0,0,0 };

	wchar_t *MaskEnd = wcsstr(Mask, L">>");
	if (MaskEnd)
	{
		*MaskEnd = 0;
		SetAttrWords(MaskEnd+2, &Data.attr_incl, &Data.attr_excl);
	}

	flags_t Flags = OptFlags(L, 4, 0);
	if (lua_gettop(L) == 3)
		lua_pushnil(L);

	Data.nparams = lua_gettop(L) - 4;
	lua_checkstack(L, 256);

	FSF.FarRecursiveSearch(InitDir, Mask, FrsUserFunc, Flags, &Data);

	if (Data.err)
		LF_Error(L, check_utf8_string(L, -1, NULL));

	return Data.err ? 0 : lua_gettop(L) - Data.nparams - 4;
}

static int far_ConvertPath(lua_State *L)
{
	const wchar_t *Src = check_utf8_string(L, 1, NULL);
	enum CONVERTPATHMODES Mode = lua_isnoneornil(L,2) ?
		CPM_FULL : (enum CONVERTPATHMODES)check_env_flag(L,2);
	size_t Size = FSF.ConvertPath(Mode, Src, NULL, 0);
	wchar_t* Target = (wchar_t*)lua_newuserdata(L, Size*sizeof(wchar_t));
	FSF.ConvertPath(Mode, Src, Target, Size);
	push_utf8_string(L, Target, -1);
	return 1;
}

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

static int far_MacroLoadAll(lua_State* L)
{
	TPluginData *pd = GetPluginData(L);
	struct FarMacroLoad Data;
	Data.StructSize = sizeof(Data);
	Data.Path = opt_utf8_string(L, 1, NULL);
	Data.Flags = OptFlags(L, 2, 0);
	lua_pushboolean(L, PSInfo.MacroControl(pd->PluginId, MCTL_LOADALL, 0, &Data) != 0);
	return 1;
}

static int far_MacroSaveAll(lua_State* L)
{
	TPluginData *pd = GetPluginData(L);
	lua_pushboolean(L, PSInfo.MacroControl(pd->PluginId, MCTL_SAVEALL, 0, 0) != 0);
	return 1;
}

static int far_MacroGetState(lua_State* L)
{
	TPluginData *pd = GetPluginData(L);
	lua_pushinteger(L, PSInfo.MacroControl(pd->PluginId, MCTL_GETSTATE, 0, 0));
	return 1;
}

static int far_MacroGetArea(lua_State* L)
{
	TPluginData *pd = GetPluginData(L);
	lua_pushinteger(L, PSInfo.MacroControl(pd->PluginId, MCTL_GETAREA, 0, 0));
	return 1;
}

static int MacroSendString(lua_State* L, int Param1)
{
	TPluginData *pd = GetPluginData(L);
	struct MacroSendMacroText smt;
	memset(&smt, 0, sizeof(smt));
	smt.StructSize = sizeof(smt);
	smt.SequenceText = check_utf8_string(L, 1, NULL);
	smt.Flags = OptFlags(L, 2, 0);
	if (Param1 == MSSC_POST)
	{
		smt.AKey = (lua_type(L,3) == LUA_TSTRING) ?
			(DWORD)FSF.FarNameToKey(check_utf8_string(L,3,NULL)) :
			(DWORD)luaL_optinteger(L,3,0);
	}

	lua_pushboolean(L, PSInfo.MacroControl(pd->PluginId, MCTL_SENDSTRING, Param1, &smt) != 0);
	return 1;
}

static int far_MacroPost(lua_State* L)
{
	return MacroSendString(L, MSSC_POST);
}

static int far_MacroCheck(lua_State* L)
{
	return MacroSendString(L, MSSC_CHECK);
}

static int far_MacroGetLastError(lua_State* L)
{
	TPluginData *pd = GetPluginData(L);
	intptr_t size = PSInfo.MacroControl(pd->PluginId, MCTL_GETLASTERROR, 0, NULL);

	if (size)
	{
		struct MacroParseResult *mpr = (struct MacroParseResult*)lua_newuserdata(L, size);
		mpr->StructSize = sizeof(*mpr);
		PSInfo.MacroControl(pd->PluginId, MCTL_GETLASTERROR, size, mpr);
		lua_createtable(L, 0, 4);
		PutIntToTable(L, "ErrCode", mpr->ErrCode);
		PutIntToTable(L, "ErrPosX", mpr->ErrPos.X);
		PutIntToTable(L, "ErrPosY", mpr->ErrPos.Y);
		PutWStrToTable(L, "ErrSrc", mpr->ErrSrc, -1);
	}
	else
		lua_pushboolean(L, 0);

	return 1;
}

typedef struct
{
	lua_State *L;
	int funcref;
} MacroAddData;

static intptr_t WINAPI MacroAddCallback (void* Id, DWORD Flags)
{
	lua_State *L;
	int result = TRUE;
	MacroAddData *data = (MacroAddData*)Id;
	if ((L = data->L) == NULL)
		return FALSE;

	lua_rawgeti(L, LUA_REGISTRYINDEX, data->funcref);

	if (lua_type(L,-1) == LUA_TFUNCTION)
	{
		lua_pushlightuserdata(L, Id);
		lua_rawget(L, LUA_REGISTRYINDEX);
		lua_pushnumber(L, Flags);
		result = !lua_pcall(L, 2, 1, 0) && lua_toboolean(L, -1);
	}

	lua_pop(L, 1);
	return result;
}

static int far_MacroAdd(lua_State* L)
{
	TPluginData *pd = GetPluginData(L);
	struct MacroAddMacro data;
	memset(&data, 0, sizeof(data));
	data.StructSize = sizeof(data);
	data.Area = OptFlags(L, 1, MACROAREA_COMMON);
	data.Flags = OptFlags(L, 2, 0);
	data.AKey = check_utf8_string(L, 3, NULL);
	data.SequenceText = check_utf8_string(L, 4, NULL);
	data.Description = opt_utf8_string(L, 5, L"");
	lua_settop(L, 7);
	if (lua_toboolean(L, 6))
	{
		luaL_checktype(L, 6, LUA_TFUNCTION);
		data.Callback = MacroAddCallback;
	}
	data.Id = lua_newuserdata(L, sizeof(MacroAddData));
	data.Priority = luaL_optinteger(L, 7, 50);

	if (PSInfo.MacroControl(pd->PluginId, MCTL_ADDMACRO, 0, &data))
	{
		MacroAddData* Id = (MacroAddData*)data.Id;
		lua_isfunction(L, 6) ? lua_pushvalue(L, 6) : lua_pushboolean(L, 1);
		Id->funcref = luaL_ref(L, LUA_REGISTRYINDEX);
		Id->L = pd->MainLuaState;
		luaL_getmetatable(L, AddMacroDataType);
		lua_setmetatable(L, -2);
		lua_pushlightuserdata(L, Id); // Place it in the registry to protect from gc. It should be collected only at lua_close().
		lua_pushvalue(L, -2);
		lua_rawset(L, LUA_REGISTRYINDEX);
	}
	else
		lua_pushnil(L);

	return 1;
}

static int far_MacroDelete(lua_State* L)
{
	int result = FALSE;
	MacroAddData *Id = (MacroAddData*)luaL_checkudata(L, 1, AddMacroDataType);

	if (Id->L)
	{
		TPluginData *pd = GetPluginData(L);
		result = (int)PSInfo.MacroControl(pd->PluginId, MCTL_DELMACRO, 0, Id);
		if (result)
		{
			luaL_unref(L, LUA_REGISTRYINDEX, Id->funcref);
			Id->L = NULL;
			lua_pushlightuserdata(L, Id);
			lua_pushnil(L);
			lua_rawset(L, LUA_REGISTRYINDEX);
		}
	}

	lua_pushboolean(L, result);
	return 1;
}

static int AddMacroData_gc(lua_State* L)
{
	far_MacroDelete(L);
	return 0;
}

static int far_MacroExecute(lua_State* L)
{
	TPluginData *pd = GetPluginData(L);
	int top = lua_gettop(L);

	struct MacroExecuteString Data;
	Data.StructSize = sizeof(Data);
	Data.SequenceText = check_utf8_string(L, 1, NULL);
	Data.Flags = OptFlags(L,2,0);
	Data.InCount = 0;

	if (top > 2)
	{
		Data.InCount = top-2;
		Data.InValues = (struct FarMacroValue*)lua_newuserdata(L, Data.InCount*sizeof(struct FarMacroValue));
		memset(Data.InValues, 0, Data.InCount*sizeof(struct FarMacroValue));
		for (size_t i=0; i<Data.InCount; i++)
			ConvertLuaValue(L, (int)i+3, Data.InValues+i);
	}

	if (PSInfo.MacroControl(pd->PluginId, MCTL_EXECSTRING, 0, &Data))
		PackMacroValues(L, Data.OutCount, Data.OutValues);
	else
		lua_pushnil(L);

	return 1;
}

static int far_CPluginStartupInfo(lua_State *L)
{
	lua_pushlightuserdata(L, &PSInfo);
	return 1;
}

static int far_MakeMenuItems (lua_State *L)
{
	int argn = lua_gettop(L);
	lua_createtable(L, argn, 0);               //+1 (items)

	if (argn > 0)
	{
		int item = 1;
		char delim[] = { 226,148,130,0 };        // Unicode char 9474 in UTF-8
		char buf_prefix[64], buf_space[64], buf_format[64];
		int maxno = 0;

		for (int i=argn; i; i/=10) ++maxno;
		size_t len_prefix = sprintf(buf_space, "%*s%s ", maxno, "", delim);
		sprintf(buf_format, "%%%dd%%s ", maxno);

		for(int i=1; i<=argn; i++)
		{
			size_t len_arg;
			const char *start = global_tolstring(L, i, &len_arg); //+2
			sprintf(buf_prefix, buf_format, i, delim);
			char *str = (char*) malloc(len_arg + 1);
			memcpy(str, start, len_arg + 1);
			lua_pop(L, 1);                         //+1 (items)

			for (size_t j=0; j<len_arg; j++)
				if (str[j] == '\0') str[j] = ' ';

			for (start=str; start; )
			{
				lua_newtable(L);                     //+2 (items,curr_item)
				const char* nl = strchr(start, '\n');
				size_t len_text = nl ? (nl++) - start : (str+len_arg) - start;
				char *line = (char*) malloc(len_prefix + len_text);
				memcpy(line, buf_prefix, len_prefix);
				memcpy(line + len_prefix, start, len_text);

				lua_pushlstring(L, line, len_prefix + len_text);
				free(line);
				lua_setfield(L, -2, "text");         //+2
				lua_pushvalue(L, i);
				lua_setfield(L, -2, "arg");          //+2
				lua_rawseti(L, -2, item++);          //+1 (items)
				strcpy(buf_prefix, buf_space);
				start = nl;
			}

			free(str);
		}
	}

	return 1;
}

static int far_Show(lua_State *L)
{
	const char* f =
		"local items, n = ...\n"
		"local bot = n==0 and 'No arguments' or n==1 and '1 argument' or n..' arguments'\n"
		"local it, pos = far.Menu({Title=''; Bottom=bot; Flags='FMENU_SHOWAMPERSAND FMENU_WRAPMODE'},\n"
		"  items, 'Space CtrlC CtrlIns')\n"
		"if items[pos] and it ~= items[pos] and (it.BreakKey=='CtrlC' or it.BreakKey=='CtrlIns') then\n"
		"  far.CopyToClipboard(tostring(items[pos].arg)) end\n"
		"return it, pos";
	int argn = lua_gettop(L);
	far_MakeMenuItems(L);

	if (luaL_loadstring(L, f) != 0)
		luaL_error(L, lua_tostring(L, -1));

	lua_pushvalue(L, -2);
	lua_pushinteger(L, argn);

	if (lua_pcall(L, 2, LUA_MULTRET, 0) != 0)
		luaL_error(L, lua_tostring(L, -1));

	return lua_gettop(L) - argn - 1;
}

static int far_InputRecordToName(lua_State* L)
{
	INPUT_RECORD Rec;
	FillInputRecord(L, 1, &Rec);
	if (Rec.EventType == 0 || Rec.EventType == KEY_EVENT)
		PushNameFromInputRecord(L, &Rec);
	else
		lua_pushnil(L);

	return 1;
}

void NewVirtualKeyTable(lua_State* L, BOOL twoways)
{
	lua_createtable(L, twoways ? 256:0, 200);

	for(int i=0; i<256; i++)
	{
		const char* str = VirtualKeyStrings[i];

		if (str)
		{
			lua_pushinteger(L, i);
			lua_setfield(L, -2, str);
		}

		if (twoways)
		{
			lua_pushstring(L, str ? str : "");
			lua_rawseti(L, -2, i);
		}
	}
}

HANDLE* CheckFileFilter(lua_State* L, int pos)
{
	return (HANDLE*)luaL_checkudata(L, pos, FarFileFilterType);
}

HANDLE CheckValidFileFilter(lua_State* L, int pos)
{
	HANDLE h = *CheckFileFilter(L, pos);
	luaL_argcheck(L,h != INVALID_HANDLE_VALUE,pos,"attempt to access invalid file filter");
	return h;
}

static int far_CreateFileFilter(lua_State *L)
{
	HANDLE hHandle = (luaL_checkinteger(L,1) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
	int filterType = check_env_flag(L,2);
	HANDLE* pOutHandle = (HANDLE*)lua_newuserdata(L, sizeof(HANDLE));

	if (PSInfo.FileFilterControl(hHandle, FFCTL_CREATEFILEFILTER, filterType, (LONG_PTR)pOutHandle))
	{
		luaL_getmetatable(L, FarFileFilterType);
		lua_setmetatable(L, -2);
	}
	else
		lua_pushnil(L);

	return 1;
}

static int filefilter_Free(lua_State *L)
{
	HANDLE *h = CheckFileFilter(L, 1);

	if (*h != INVALID_HANDLE_VALUE)
	{
		lua_pushboolean(L, PSInfo.FileFilterControl(*h, FFCTL_FREEFILEFILTER, 0, 0));
		*h = INVALID_HANDLE_VALUE;
	}
	else
		lua_pushboolean(L,0);

	return 1;
}

static int filefilter_gc(lua_State *L)
{
	filefilter_Free(L);
	return 0;
}

static int filefilter_tostring(lua_State *L)
{
	HANDLE *h = CheckFileFilter(L, 1);
	if (*h != INVALID_HANDLE_VALUE)
		lua_pushfstring(L, "%s (%p)", FarFileFilterType, h);
	else
		lua_pushfstring(L, "%s (closed)", FarFileFilterType);
	return 1;
}

static int filefilter_OpenMenu(lua_State *L)
{
	HANDLE h = CheckValidFileFilter(L, 1);
	lua_pushboolean(L, PSInfo.FileFilterControl(h, FFCTL_OPENFILTERSMENU, 0, 0));
	return 1;
}

static int filefilter_Starting(lua_State *L)
{
	HANDLE h = CheckValidFileFilter(L, 1);
	lua_pushboolean(L, PSInfo.FileFilterControl(h, FFCTL_STARTINGTOFILTER, 0, 0));
	return 1;
}

static int filefilter_IsFileInFilter(lua_State *L)
{
	struct FAR_FIND_DATA ffd;
	HANDLE h = CheckValidFileFilter(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);
	lua_settop(L, 2);         // +2
	GetFarFindData(L, &ffd);  // +4
	lua_pushboolean(L, PSInfo.FileFilterControl(h, FFCTL_ISFILEINFILTER, 0, (LONG_PTR)&ffd));
	return 1;
}

static int plugin_load(lua_State *L, enum FAR_PLUGINS_CONTROL_COMMANDS command)
{
	int param1 = check_env_flag(L, 1);
	void *param2 = check_utf8_string(L, 2, NULL);
	intptr_t result = PSInfo.PluginsControlV3(INVALID_HANDLE_VALUE, command, param1, param2);

	if (result) PushPluginHandle(L, (void*)result);
	else lua_pushnil(L);

	return 1;
}

static int far_LoadPlugin(lua_State *L) { return plugin_load(L, PCTL_LOADPLUGIN); }
static int far_ForcedLoadPlugin(lua_State *L) { return plugin_load(L, PCTL_FORCEDLOADPLUGIN); }

static int far_UnloadPlugin(lua_State *L)
{
	void* Handle = *(void**)luaL_checkudata(L, 1, PluginHandleType);
	lua_pushboolean(L, PSInfo.PluginsControlV3(Handle, PCTL_UNLOADPLUGIN, 0, 0) != 0);
	return 1;
}

static int far_FindPlugin(lua_State *L)
{
	int param1 = check_env_flag(L, 1);
	void *param2 = NULL;
	DWORD SysID;

	if (param1 == PFM_MODULENAME)
	{
		param2 = check_utf8_string(L, 2, NULL);
	}
	else if (param1 == PFM_SYSID)
	{
		SysID = (DWORD)luaL_checkinteger(L, 2);
		param2 = &SysID;
	}

	if (param2)
	{
		intptr_t handle = PSInfo.PluginsControlV3(NULL, PCTL_FINDPLUGIN, param1, param2);

		if (handle)
		{
			PushPluginHandle(L, (void*)handle);
			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}

static int far_ClearPluginCache(lua_State *L)
{
	int param1 = check_env_flag(L, 1);
	void* param2 = (void*)check_utf8_string(L, 2, NULL);
	intptr_t result = PSInfo.PluginsControlV3(INVALID_HANDLE_VALUE, PCTL_CACHEFORGET, param1, param2);
	lua_pushboolean(L, result);
	return 1;
}

static void PutPluginMenuItemToTable(lua_State *L, const char* Field, const wchar_t* const* Strings, int Count)
{
	lua_createtable(L, 0, 2);
	PutIntToTable(L, "Count", Count);
	lua_createtable(L, Count, 0);
	if (Strings) {
		for (int i=0; i<Count; i++)
			PutWStrToArray(L, i+1, Strings[i], -1);
	}
	lua_setfield(L, -2, "Strings");
	lua_setfield(L, -2, Field);
}

static int far_GetPluginInformation(lua_State *L)
{
	struct FarGetPluginInformation *pi;
	HANDLE Handle = *(HANDLE*)luaL_checkudata(L, 1, PluginHandleType);
	size_t size = PSInfo.PluginsControlV3(Handle, PCTL_GETPLUGININFORMATION, 0, 0);

	if (size == 0) return lua_pushnil(L), 1;

	pi = (struct FarGetPluginInformation *)lua_newuserdata(L, size);
	pi->StructSize = sizeof(*pi);

	if (!PSInfo.PluginsControlV3(Handle, PCTL_GETPLUGININFORMATION, size, pi))
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 4);
	{
		PutWStrToTable(L, "ModuleName", pi->ModuleName, -1);
		PutFlagsToTable(L, "Flags", pi->Flags);
		lua_createtable(L, 0, 6); // PInfo
		{
			PutNumToTable(L, "StructSize", pi->PInfo->StructSize);
			PutFlagsToTable(L, "Flags", pi->PInfo->Flags);
			PutNumToTable(L, "SysID", pi->PInfo->SysID);
			PutPluginMenuItemToTable(L, "DiskMenu", pi->PInfo->DiskMenuStrings, pi->PInfo->DiskMenuStringsNumber);
			PutPluginMenuItemToTable(L, "PluginMenu", pi->PInfo->PluginMenuStrings, pi->PInfo->PluginMenuStringsNumber);
			PutPluginMenuItemToTable(L, "PluginConfig", pi->PInfo->PluginConfigStrings, pi->PInfo->PluginConfigStringsNumber);

			if (pi->PInfo->CommandPrefix)
				PutWStrToTable(L, "CommandPrefix", pi->PInfo->CommandPrefix, -1);

			lua_setfield(L, -2, "PInfo");
		}
		PushGlobalInfo(L, pi->GInfo);
		lua_setfield(L, -2, "GInfo");
	}

	return 1;
}

static int far_GetPlugins(lua_State *L)
{
	int count = (int)PSInfo.PluginsControlV3(INVALID_HANDLE_VALUE, PCTL_GETPLUGINS, 0, 0);
	lua_createtable(L, count, 0);

	if (count > 0)
	{
		HANDLE *handles = lua_newuserdata(L, count*sizeof(HANDLE));
		count = (int)PSInfo.PluginsControlV3(INVALID_HANDLE_VALUE, PCTL_GETPLUGINS, count, handles);

		for(int i=0; i<count; i++)
		{
			PushPluginHandle(L, handles[i]);
			lua_rawseti(L, -3, i+1);
		}

		lua_pop(L, 1);
	}

	return 1;
}

static int far_IsPluginLoaded(lua_State *L)
{
	int result = 0;
	DWORD SysId = (DWORD)luaL_checkinteger(L, 1);;

	intptr_t handle = PSInfo.PluginsControlV3(NULL, PCTL_FINDPLUGIN, PFM_SYSID, &SysId);
	if (handle)
	{
		size_t size = PSInfo.PluginsControlV3((HANDLE)handle, PCTL_GETPLUGININFORMATION, 0, 0);
		if (size)
		{
			struct FarGetPluginInformation *pi = (struct FarGetPluginInformation *)malloc(size);
			pi->StructSize = sizeof(*pi);
			if (PSInfo.PluginsControlV3((HANDLE)handle, PCTL_GETPLUGININFORMATION, size, pi))
				result = (pi->Flags & FPF_LOADED) ? 1:0;

			free(pi);
		}
	}
	lua_pushboolean(L, result);
	return 1;
}

static int far_XLat(lua_State *L)
{
	size_t size;
	wchar_t *Line = check_utf8_string(L, 1, &size);
	int StartPos = luaL_optinteger(L, 2, 1) - 1;
	int EndPos = luaL_optinteger(L, 3, size);
	int Flags = OptFlags(L, 4, 0);
	Line = FSF.XLat(Line, StartPos, EndPos, Flags);
	Line ? push_utf8_string(L, Line, -1) : lua_pushnil(L);
	return 1;
}

static int far_FormatFileSize(lua_State *L)
{
	uint64_t Size = (uint64_t) luaL_checknumber(L, 1);
	int Width = (int) luaL_checkinteger(L, 2);
	if (abs(Width) > 10000)
		return luaL_error(L, "the 'Width' argument exceeds 10000");

	flags_t Flags = OptFlags(L, 3, 0) & ~FFFS_MINSIZEINDEX_MASK;
	Flags |= luaL_optinteger(L, 4, 0) & FFFS_MINSIZEINDEX_MASK;

	size_t bufsize = FSF.FormatFileSize(Size, Width, Flags, NULL, 0);
	wchar_t *buf = (wchar_t*) lua_newuserdata(L, bufsize*sizeof(wchar_t));

	FSF.FormatFileSize(Size, Width, Flags, buf, bufsize);
	push_utf8_string(L, buf, -1);
	return 1;
}

static int far_Execute(lua_State *L)
{
	const wchar_t *CmdStr = check_utf8_string(L, 1, NULL);
	int ExecFlags = OptFlags(L, 2, 0);
	lua_pushinteger(L, FSF.Execute(CmdStr, ExecFlags));
	return 1;
}

static int far_ExecuteLibrary(lua_State *L)
{
	const wchar_t *Library = check_utf8_string(L, 1, NULL);
	const wchar_t *Symbol  = check_utf8_string(L, 2, NULL);
	const wchar_t *CmdStr  = check_utf8_string(L, 3, NULL);
	int ExecFlags = OptFlags(L, 4, 0);
	lua_pushinteger(L, FSF.ExecuteLibrary(Library, Symbol, CmdStr, ExecFlags));
	return 1;
}

static int far_DisplayNotification(lua_State *L)
{
	const wchar_t *action = check_utf8_string(L, 1, NULL);
	const wchar_t *object  = check_utf8_string(L, 2, NULL);
	FSF.DisplayNotification(action, object);
	return 0;
}

static int far_DispatchInterThreadCalls(lua_State *L)
{
	lua_pushinteger(L, FSF.DispatchInterThreadCalls());
	return 1;
}

static int far_BackgroundTask(lua_State *L)
{
	const wchar_t *Info = check_utf8_string(L, 1, NULL);
	BOOL Started = lua_toboolean(L, 2);
	FSF.BackgroundTask(Info, Started);
	return 0;
}

static int far_StrCellsCount(lua_State *L)
{
	size_t Len;
	const wchar_t *Str = check_utf8_string(L, 1, &Len);
	size_t CharsCount = luaL_optinteger(L, 2, Len);

	if (CharsCount > Len)
		CharsCount = Len;

	lua_pushinteger(L, FSF.StrCellsCount(Str, CharsCount));
	return 1;
}

static int far_StrSizeOfCells(lua_State *L)
{
	size_t Len;
	const wchar_t *Str = check_utf8_string(L, 1, &Len);
	lua_Integer tmp = luaL_checkinteger(L, 2);
	size_t CellsCount = tmp >= 0 ? tmp : 0;
	int RoundUp = lua_toboolean(L, 3);

	lua_pushinteger(L, FSF.StrSizeOfCells(Str, Len, &CellsCount, RoundUp));
	lua_pushinteger(L, CellsCount);
	return 2;
}

static int far_Log(lua_State *L)
{
	const char* txt = luaL_optstring(L, 1, "log message");
	lua_pushinteger(L, Log(L, "%s", txt));
	return 1;
}

static int far_ColorDialog(lua_State *L)
{
	TPluginData* pd = GetPluginData(L);
	struct ColorDialogData Data = { 0 };

	Data.Color = GetFarColor64(L, 1);
	flags_t Flags = OptFlags(L, 2, 0);

	if (PSInfo.ColorDialogV2(pd->ModuleNumber, &Data, Flags))
		PushFarColor(L, Data.Color);
	else
		lua_pushnil(L);

	return 1;
}

static int far_WriteConsole(lua_State *L)
{
	HANDLE h_out = NULL; //stdout; //GetStdHandle(STD_OUTPUT_HANDLE);
	const wchar_t* src = opt_utf8_string(L, 1, L"");

	TPluginData* pd = GetPluginData(L);
	SMALL_RECT sr;
	PSInfo.AdvControl(pd->ModuleNumber, ACTL_GETFARRECT, &sr, NULL);
	size_t FarWidth = sr.Right - sr.Left + 1;

	for (;;)
	{
		const wchar_t *ptr1 = wcschr(src, L'\n');
		const wchar_t *ptr2 = ptr1 ? ptr1 : src + wcslen(src);
		size_t nCharsToWrite = ptr2 - src;
		int wrap = nCharsToWrite > FarWidth ? 1 : 0;
		if (wrap)
			nCharsToWrite = FarWidth;

		PSInfo.Control(PANEL_ACTIVE, FCTL_GETUSERSCREEN, 0, 0);

		DWORD nCharsWritten;
		BOOL bResult = (nCharsToWrite == 0)
			|| WINPORT(WriteConsole)(h_out, src, (DWORD)nCharsToWrite, &nCharsWritten, NULL);

		PSInfo.Control(PANEL_ACTIVE, FCTL_SETUSERSCREEN, 0, 0);

		if (!bResult)
			return SysErrorReturn(L);

		if (!wrap && !ptr1)
			break;

		src += nCharsToWrite + (wrap ? 0:1);
	}

	lua_pushboolean(L, 1);
	return 1;
}

static int far_RunDefaultScript(lua_State *L)
{
	lua_pushboolean(L, RunDefaultScript(L, 0));
	return 1;
}

static int far_SplitCmdLine(lua_State *L)
{
	int numargs = 0;
	const char *str = luaL_checkstring(L,1), *p=str;

	char *arg = (char*)lua_newuserdata(L, strlen(str)+1);

	lua_newtable(L);
	for (const char *q=p; *p; p=q)
	{
		char *trg = arg;
		while (isspace(*q))
			q++;
		if (*q == 0)
			break;

		int quoted = *q == '\"';
		if (quoted)
			q++;

		for (p=q; ; q++)
		{
			if (*q == '\\')
			{
				if (*++q == 0)
					break;
			}
			else if (!quoted && (*q==0 || isspace(*q)))
				break;
			else if (quoted && (*q==0 || *q == '\"'))
			{
				if (*q == '\"') q++;
				break;
			}
			*trg++ = *q;
		}
		lua_pushlstring(L, arg, trg-arg);
		lua_rawseti(L, -2, ++numargs);
	}
	return 1;
}

static int far_DetectCodePage(lua_State *L)
{
	struct DetectCodePageInfo Info;
	Info.StructSize = sizeof(Info);
	Info.FileName = check_utf8_string(L, 1, NULL);
	int codepage = FSF.DetectCodePage(&Info);
	if (codepage)
		lua_pushinteger(L, codepage);
	else
		lua_pushnil(L);
	return 1;
}

static int far_VTEnumBackground(lua_State *L)
{
	size_t count = FSF.VTEnumBackground(NULL, 0);
	HANDLE *hnds = malloc((count+1) * sizeof(HANDLE));
	FSF.VTEnumBackground(hnds, count);
	lua_createtable(L, count, 0);
	for (size_t i=0; i < count; i++) {
		lua_pushlightuserdata(L, hnds[i]);
		lua_rawseti(L, -2, i + 1);
	}
	free(hnds);
	return 1;
}

static int far_VTLogExport(lua_State *L)
{
	HANDLE hnd = lua_isnoneornil(L,1) ? NULL : lua_touserdata(L,1);
	const wchar_t *file = check_utf8_string(L,2,NULL);
	DWORD flags = OptFlags(L,3,0);
	luaL_argcheck(L, *file, 3, "empty string not allowed"); //empty string here means a buffer - not needed
	lua_pushboolean(L, FSF.VTLogExport(hnd, flags, file));
	return 1;
}

static const luaL_Reg filefilter_methods[] =
{
	{"__gc",             filefilter_gc},
	{"__tostring",       filefilter_tostring},
	{"FreeFileFilter",   filefilter_Free},
	{"OpenFiltersMenu",  filefilter_OpenMenu},
	{"StartingToFilter", filefilter_Starting},
	{"IsFileInFilter",   filefilter_IsFileInFilter},

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

static const luaL_Reg far_funcs[] =
{
	PAIR( far, AdvControl),
	PAIR( far, AdvControlAsync),
	PAIR( far, BackgroundTask),
	PAIR( far, CheckMask),
	PAIR( far, ClearPluginCache),
	PAIR( far, CmpName),
	PAIR( far, CmpNameList),
	PAIR( far, ColorDialog),
	PAIR( far, ConvertPath),
	PAIR( far, CopyToClipboard),
	PAIR( far, CPluginStartupInfo),
	PAIR( far, CreateFileFilter),
	PAIR( far, DetectCodePage),
	PAIR( far, DispatchInterThreadCalls),
	PAIR( far, DisplayNotification),
	PAIR( far, Execute),
	PAIR( far, ExecuteLibrary),
	PAIR( far, FindPlugin),
	PAIR( far, ForcedLoadPlugin),
	PAIR( far, FormatFileSize),
	PAIR( far, FreeScreen),
	PAIR( far, GenerateName),
	PAIR( far, GetCurrentDirectory),
	PAIR( far, GetDirList),
	PAIR( far, GetFileGroup),
	PAIR( far, GetFileOwner),
	PAIR( far, GetMsg),
	PAIR( far, GetMyHome),
	PAIR( far, GetNumberOfLinks),
	PAIR( far, GetPluginDirList),
	PAIR( far, GetPluginGlobalInfo),
	PAIR( far, GetPluginId),
	PAIR( far, GetPluginInformation),
	PAIR( far, GetPlugins),
	PAIR( far, GetReparsePointInfo),
	PAIR( far, InMyCache),
	PAIR( far, InMyConfig),
	PAIR( far, InMyTemp),
	PAIR( far, InputBox),
	PAIR( far, InputRecordToKey),
	PAIR( far, InputRecordToName),
	PAIR( far, IsPluginLoaded),
	PAIR( far, KeyToName),
	PAIR( far, LIsAlpha),
	PAIR( far, LIsAlphanum),
	PAIR( far, LIsLower),
	PAIR( far, LIsUpper),
	PAIR( far, LLowerBuf),
	PAIR( far, LoadPlugin),
	PAIR( far, Log),
	PAIR( far, LStricmp),
	PAIR( far, LStrnicmp),
	PAIR( far, LStrcmp),
	PAIR( far, LStrncmp),
	PAIR( far, LUpperBuf),
	PAIR( far, MacroAdd),
	PAIR( far, MacroCheck),
	PAIR( far, MacroDelete),
	PAIR( far, MacroExecute),
	PAIR( far, MacroGetArea),
	PAIR( far, MacroGetLastError),
	PAIR( far, MacroGetState),
	PAIR( far, MacroLoadAll),
	PAIR( far, MacroPost),
	PAIR( far, MacroSaveAll),
	PAIR( far, MakeMenuItems),
	PAIR( far, Menu),
	PAIR( far, Message),
	PAIR( far, MkLink),
	PAIR( far, MkTemp),
	PAIR( far, NameToInputRecord),
	PAIR( far, NameToKey),
	PAIR( far, PasteFromClipboard),
	PAIR( far, PluginStartupInfo),
	PAIR( far, ProcessName),
	PAIR( far, RecursiveSearch),
	PAIR( far, RestoreScreen),
	PAIR( far, RunDefaultScript),
	PAIR( far, SaveScreen),
	PAIR( far, Show),
	PAIR( far, ShowHelp),
	PAIR( far, SplitCmdLine),
	PAIR( far, StrCellsCount),
	PAIR( far, StrSizeOfCells),
	PAIR( far, SudoCRCall),
	PAIR( far, Text),
	PAIR( far, TruncPathStr),
	PAIR( far, TruncStr),
	PAIR( far, UnloadPlugin),
	PAIR( far, VTEnumBackground),
	PAIR( far, VTLogExport),
	PAIR( far, WriteConsole),
	PAIR( far, XLat),

	{NULL, NULL}
};

static const char utf8_reformat[] =
"function utf8.reformat (patt, ...)\n"
  "local args = { ... }\n"
  "local function Subst (i, m, f)\n"
    "i = tonumber(i)\n"
    "f = f:match('[^s]')\n"
    "return args[i] and ('%' .. m .. (f or 's')):format(f and args[i] or tostring(args[i])) or ''\n"
  "end\n"

  "patt = patt:gsub('%f[%%{]{(%d+):?(%-?%d*%.?%d*)([A-Za-z]?)}', Subst):gsub('%%{', '{')\n"
  "return patt:format(...)\n"
"end";

static int luaopen_far (lua_State *L)
{
	TPluginData* pd = GetPluginData(L);

	NewVirtualKeyTable(L, FALSE);
	lua_setfield(L, LUA_REGISTRYINDEX, FAR_VIRTUALKEYS);
	luaL_register(L, "far", far_funcs);
	PutStrToTable (L, "Flavor", "far2m");

	luaopen_far_host(L);
	lua_setfield(L, -2, "Host");

	if (pd->Private && pd->PluginId == LuamacroId)
	{
		lua_pushcfunction(L, far_MacroCallFar);
		lua_setfield(L, -2, "MacroCallFar");
		lua_pushcfunction(L, far_MacroCallToLua);
		lua_setfield(L, -2, "MacroCallToLua");
	}

	push_flags_table(L);
	lua_pushvalue(L, -1);
	lua_setfield(L, -3, "Flags");
	lua_pushvalue(L, -1);           // for compatibility with Far3 scripts
	lua_setfield(L, -3, "Colors");  // +++
	lua_setfield(L, LUA_REGISTRYINDEX, FAR_FLAGSTABLE);

#if !defined(__DragonFly__) && !defined(__ANDROID__) && !defined(__APPLE__)
	lua_pushcfunction(L, luaopen_timer);
	lua_call(L, 0, 1);
	lua_setfield(L, -2, "Timer");
#endif

	lua_newtable(L);
	lua_setglobal(L, "export");

	luaopen_dialog(L);
	luaopen_editor(L);
	luaopen_panel(L);
	luaopen_regex(L);
	luaL_register(L, "viewer", viewer_funcs);
	luaL_register(L, "actl",   actl_funcs);

	luaL_newmetatable(L, FarFileFilterType);
	lua_pushvalue(L,-1);
	lua_setfield(L, -2, "__index");
	luaL_register(L, NULL, filefilter_methods);

	lua_pushcfunction(L, luaopen_usercontrol);
	lua_call(L, 0, 0);

	luaL_newmetatable(L, PluginHandleType);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, PluginHandle_rawhandle);
	lua_setfield(L, -2, "rawhandle");

	luaL_newmetatable(L, AddMacroDataType);
	lua_pushcfunction(L, AddMacroData_gc);
	lua_setfield(L, -2, "__gc");

	luaL_newmetatable(L, SavedScreenType);
	lua_pushcfunction(L, far_FreeScreen);
	lua_setfield(L, -2, "__gc");
	lua_pushcfunction(L, SavedScreen_tostring);
	lua_setfield(L, -2, "__tostring");

	return 0;
}

void LF_RunLuafarInit(lua_State* L)
{
	int top = lua_gettop(L);
	lua_pushcfunction(L, far_InMyConfig);
	lua_pushliteral(L, "luafar_init.lua");
	lua_call(L, 1, 1); //+1

	const char *fname = lua_tostring(L, -1);
	struct stat St;
	if (0 == stat(fname, &St)) {
		if (luaL_loadfile(L,fname) || lua_pcall(L,0,0,0)) {
			LF_Error(L, check_utf8_string(L, -1, NULL));
		}
	}
	lua_settop(L, top);
}

static void InitLuaState (lua_State *L, TPluginData *aPlugData, lua_CFunction aOpenLibs)
{
	lua_CFunction func_arr[] = {
		luaopen_far,
		luaopen_bit64,
		luaopen_unicode,
		luaopen_utf8,
		luaopen_win,
		luaopen_sysutils,
		luaopen_lpeg,
	};

	// open Lua libraries
	luaL_openlibs(L);

	if (aOpenLibs) {
		lua_pushcfunction(L, aOpenLibs);
		lua_call(L, 0, 0);
	}

	// open more libraries
	for (int idx=0; idx < ARRAYSIZE(func_arr); idx++) {
		lua_pushcfunction(L, func_arr[idx]);
		lua_call(L, 0, 0);
	}

	lua_getglobal(L, "utf8");                   //+1
	lua_getglobal(L, "string");                 //+2
	// utf8.dump = string.dump
	lua_getfield(L, -1, "dump");                //+3
	lua_setfield(L, -3, "dump");                //+2
	// utf8.rep = string.rep
	lua_getfield(L, -1, "rep");                 //+3
	lua_setfield(L, -3, "rep");                 //+2
	// getmetatable("").__index = utf8
	lua_pushliteral(L, "");                     //+3
	lua_getmetatable(L, -1);                    //+4
	lua_pushvalue(L, -4);                       //+5
	lua_setfield(L, -2, "__index");             //+4
	lua_pop(L, 4);                              //+0
	// add utf8.reformat
	(void) luaL_dostring(L, utf8_reformat);

	{ // modify package.path
		const char *farhome;
		lua_getglobal  (L, "package");            //+1
		int pos = lua_gettop(L);
		if (aPlugData->Flags & PDF_SETPACKAGEPATH) {
			lua_pushstring (L, aPlugData->ShareDir);
			lua_pushstring (L, "/?.lua;");
		}
		if ((farhome = getenv("FARHOME"))) {
			lua_pushstring (L, farhome);
			lua_pushstring (L, "/Plugins/luafar/lua_share/?.lua;");
			lua_pushstring (L, farhome);
			lua_pushstring (L, "/Plugins/luafar/lua_share/?/init.lua;");
		}
		lua_getfield (L, pos, "path");
		lua_concat   (L, lua_gettop(L) - pos);    //+2
		lua_setfield (L, pos, "path");            //+1
		lua_pop      (L, 1);                      //+0
	}
}

// Initialize the interpreter
int LF_LuaOpen (const struct PluginStartupInfo *aInfo, TPluginData* aPlugData, lua_CFunction aOpenLibs)
{
	if (PSInfo.StructSize == 0) {
		if (aInfo->StructSize < sizeof(*aInfo) || aInfo->FSF->StructSize < sizeof(*aInfo->FSF)) {
			return 0; // Far is too old
		}
		PSInfo = *aInfo;
		FSF = *aInfo->FSF;
		PSInfo.FSF = &FSF;
		PSInfo.ModuleName = NULL;
		PSInfo.ModuleNumber = 0;
		PSInfo.Private = NULL;
	}

	// create Lua State
	lua_State *L = lua_open();
	if (L) {
		// place pointer to plugin data in the L's registry -
		aPlugData->MainLuaState = L;
		lua_pushlightuserdata(L, aPlugData);
		lua_setfield(L, LUA_REGISTRYINDEX, FAR_PLUGINDATA);

		// Evaluate the path where the scripts are (ShareDir)
		// It may (or may not) be the same as ModuleDir.
		const char *s1=  "/lib/far2m/Plugins/luafar/";
		const char *s2="/share/far2m/Plugins/luafar/";
		push_utf8_string(L, aPlugData->ModuleName, -1);                  //+1
		aPlugData->ShareDir = (char*) malloc(lua_objlen(L,-1) + 8);
		strcpy(aPlugData->ShareDir, luaL_gsub(L, lua_tostring(L,-1), s1, s2)); //+2
		strrchr(aPlugData->ShareDir, GOOD_SLASH)[0] = '\0';

		DIR* dir = opendir(aPlugData->ShareDir); // a "patch" for PPA installations
		if (dir)
			closedir(dir);
		else {
			strcpy(aPlugData->ShareDir, lua_tostring(L,-2));
			strrchr(aPlugData->ShareDir, GOOD_SLASH)[0] = '\0';
		}
		lua_pop(L,2);                                                    //+0

		InitLuaState(L, aPlugData, aOpenLibs);
		return 1;
	}

	return 0;
}

int LF_InitOtherLuaState (lua_State *L, lua_State *Lplug, lua_CFunction aOpenLibs)
{
	if (L != Lplug) {
		TPluginData *PluginData = GetPluginData(Lplug);
		TPluginData *pd = (TPluginData*)lua_newuserdata(L, sizeof(TPluginData));
		lua_setfield(L, LUA_REGISTRYINDEX, FAR_PLUGINDATA);
		memcpy(pd, PluginData, sizeof(TPluginData));
		pd->MainLuaState = L;
		InitLuaState(L, pd, aOpenLibs);
	}
	return 0;
}
