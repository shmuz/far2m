//coding: utf-8
//---------------------------------------------------------------------------

#include <windows.h>
#include <dirent.h> //opendir
#include <ctype.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "bit64.h"
#include "farlibs.h"
#include "ustring.h"
#include "util.h"
#include "version.h"
#include "service.h"

extern void add_flags (lua_State *L); // from generated file farflags.c

extern int  far_MacroCallFar(lua_State *L);
extern int  far_MacroCallToLua(lua_State *L);

extern int  luaopen_far_host(lua_State *L);
extern int  luaopen_regex (lua_State*);
extern int  luaopen_timer (lua_State *L);
extern int  luaopen_unicode (lua_State *L);
extern int  luaopen_usercontrol (lua_State *L);
extern int  luaopen_utf8 (lua_State *L);
extern int  luaopen_win (lua_State *L);
extern int  luaopen_sysutils (lua_State *L);

extern void PackMacroValues(lua_State* L, size_t Count, const struct FarMacroValue* Values);
extern int  pcall_msg (lua_State* L, int narg, int nret);
extern void PushPluginTable(lua_State* L, HANDLE hPlugin);
extern BOOL RunDefaultScript(lua_State* L, int ForFirstTime);

struct PluginStartupInfo PSInfo; // DON'T ever use fields ModuleName and ModuleNumber of PSInfo
																 // because they contain data of the 1-st loaded LuaFAR plugin.
																 // Instead, get them via GetPluginData(L).
struct FarStandardFunctions FSF;

const char FarFileFilterType[] = "FarFileFilter";
const char FarDialogType[]     = "FarDialog";
const char AddMacroDataType[]  = "FarAddMacroData";
const char SavedScreenType[]   = "FarSavedScreen";

const char FAR_KEYINFO[]       = "far.info";
const char FAR_VIRTUALKEYS[]   = "far.virtualkeys";
const char FAR_DN_STORAGE[]    = "FAR_DN_STORAGE";

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

const char* FarKeyStrings[] = {
/* 0x00 */ NULL,    NULL,   NULL,   NULL,                NULL,    NULL,    NULL,    NULL,
/* 0x08 */ "BS",    "Tab",  NULL,   NULL,                NULL,    "Enter", NULL,    NULL,
/* 0x10 */ NULL,    NULL,   NULL,   NULL,                NULL,    NULL,    NULL,    NULL,
/* 0x18 */ NULL,    NULL,   NULL,   "Esc",               NULL,    NULL,    NULL,    NULL,
/* 0x20 */ "Space", "PgUp", "PgDn", "End",               "Home",  "Left",  "Up",    "Right",
/* 0x28 */ "Down",  NULL,   NULL,   NULL,                NULL,    "Ins",   "Del",   NULL,
/* 0x30 */ "0",     "1",    "2",    "3",                 "4",     "5",     "6",     "7",
/* 0x38 */ "8",     "9",    NULL,   NULL,                NULL,    NULL,    NULL,    NULL,
/* 0x40 */ NULL,    "A",    "B",    "C",                 "D",     "E",     "F",     "G",
/* 0x48 */ "H",     "I",    "J",    "K",                 "L",     "M",     "N",     "O",
/* 0x50 */ "P",     "Q",    "R",    "S",                 "T",     "U",     "V",     "W",
/* 0x58 */ "X",     "Y",    "Z",    NULL,                NULL,    NULL,    NULL,    NULL,
/* 0x60 */ "Num0",  "Num1", "Num2", "Num3",              "Num4",  "Clear", "Num6",  "Num7",
/* 0x68 */ "Num8",  "Num9", "Multiply", "Add",           NULL, "Subtract", "NumDel", "Divide",
/* 0x70 */ "F1",    "F2",   "F3",   "F4",                "F5",    "F6",    "F7",    "F8",
/* 0x78 */ "F9",    "F10",  "F11",  "F12",               "F13",   "F14",   "F15",   "F16",
/* 0x80 */ "F17",   "F18",  "F19",  "F20",               "F21",   "F22",   "F23",   "F24",
};

TSynchroData* CreateSynchroData(TTimerData *td, int data)
{
	TSynchroData* SD = (TSynchroData*) malloc(sizeof(TSynchroData));
	SD->timerData = td;
	SD->data = data;
	return SD;
}

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

flags_t GetFlags (lua_State *L, int stack_pos, int *success)
{
	int dummy, ok;
	flags_t trg = 0, flag;

	success = success ? success : &dummy;
	*success = TRUE;

	switch(lua_type(L,stack_pos))
	{
		case LUA_TNONE:
		case LUA_TNIL:
			break;

		case LUA_TNUMBER:
			trg = (flags_t)lua_tonumber(L, stack_pos);
			break;

		case LUA_TSTRING:
		{
			const char *p = lua_tostring(L, stack_pos), *q;
			for (; *p; p=q)
			{
				while (isspace(*p) || *p=='+') p++;
				if (*p == 0) break;
				for (q=p+1; *q && !isspace(*q) && *q!='+'; ) q++;
				lua_pushlstring(L, p, q-p);
				lua_getfield (L, LUA_ENVIRONINDEX, lua_tostring(L, -1));
				if (lua_isnumber(L, -1))
					trg |= (flags_t)lua_tonumber(L, -1);
				else
					*success = FALSE;
				lua_pop(L, 2);
			}
			break;
		}

		case LUA_TTABLE:
			stack_pos = abs_index (L, stack_pos);
			lua_pushnil(L);
			while (lua_next(L, stack_pos)) {
				if (lua_type(L,-2)==LUA_TSTRING && lua_toboolean(L,-1)) {
					flag = GetFlags (L, -2, &ok); // recursion
					if (ok)
						trg |= flag;
					else
						*success = FALSE;
				}
				lua_pop(L, 1);
			}
			break;

		default:
			*success = FALSE;
			break;
	}

	return trg;
}

flags_t check_env_flag (lua_State *L, int stack_pos)
{
	flags_t trg = 0;
	int success = FALSE;
	if (!lua_isnoneornil(L,stack_pos))
		trg = GetFlags(L,stack_pos,&success);
	if (!success)
		luaL_argerror(L, stack_pos, "invalid flag");
	return trg;
}

flags_t opt_env_flag (lua_State *L, int stack_pos, flags_t dflt)
{
	flags_t trg = dflt;
	if (!lua_isnoneornil(L,stack_pos)) {
		int success;
		trg = GetFlags(L,stack_pos,&success);
		if (!success)
			luaL_argerror(L, stack_pos, "invalid flag");
	}
	return trg;
}

flags_t CheckFlags(lua_State* L, int stackpos)
{
	int success;
	flags_t Flags = GetFlags(L, stackpos, &success);
	if (!success)
		luaL_error(L, "invalid flag combination");
	return Flags;
}

flags_t OptFlags(lua_State* L, int pos, flags_t dflt)
{
	return lua_isnoneornil(L, pos) ? dflt : CheckFlags(L, pos);
}

flags_t GetFlagsFromTable(lua_State *L, int pos, const char* key)
{
	flags_t f;
	lua_getfield(L, pos, key);
	f = GetFlags(L, -1, NULL);
	lua_pop(L, 1);
	return f;
}

TPluginData* GetPluginData(lua_State* L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, FAR_KEYINFO);
	TPluginData* pd = (TPluginData*) lua_touserdata(L, -1);
	if (pd)
		lua_pop(L, 1);
	else
		luaL_error (L, "TPluginData is not available.");
	return pd;
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

static int far_LuafarVersion (lua_State *L)
{
	if (lua_toboolean(L, 1)) {
		lua_pushinteger(L, VER_MAJOR);
		lua_pushinteger(L, VER_MINOR);
		lua_pushinteger(L, VER_MICRO);
		return 3;
	}
	lua_pushfstring(L, "%d.%d.%d", (int)VER_MAJOR, (int)VER_MINOR, (int)VER_MICRO);
	return 1;
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
const wchar_t* StoreTempString(lua_State *L, int store_stack_pos, int* index)
{
	const wchar_t *s = check_utf8_string(L,-1,NULL);
	lua_rawseti(L, store_stack_pos, ++(*index));
	return s;
}

static void PushEditorSetPosition(lua_State *L, const struct EditorSetPosition *esp)
{
	lua_createtable(L, 0, 6);
	PutIntToTable(L, "CurLine",       esp->CurLine + 1);
	PutIntToTable(L, "CurPos",        esp->CurPos + 1);
	PutIntToTable(L, "CurTabPos",     esp->CurTabPos + 1);
	PutIntToTable(L, "TopScreenLine", esp->TopScreenLine + 1);
	PutIntToTable(L, "LeftPos",       esp->LeftPos + 1);
	PutIntToTable(L, "Overtype",      esp->Overtype);
}

static void FillEditorSetPosition(lua_State *L, struct EditorSetPosition *esp)
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

static void PushOptPluginTable(lua_State *L, HANDLE handle)
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
	PutNumToTable(L, "Flags", PanelItem->Flags);
	PutNumToTable(L, "NumberOfLinks", PanelItem->NumberOfLinks);

	if (PanelItem->Description)    PutWStrToTable(L, "Description",  PanelItem->Description, -1);
	if (PanelItem->Owner)          PutWStrToTable(L, "Owner",  PanelItem->Owner, -1);
	if (PanelItem->Group)          PutWStrToTable(L, "Group",  PanelItem->Group, -1);

	if (PanelItem->CustomColumnNumber > 0) {
		int j;
		lua_createtable (L, PanelItem->CustomColumnNumber, 0);
		for(j=0; j < PanelItem->CustomColumnNumber; j++)
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
}

void PushPanelItems(lua_State *L, HANDLE handle, const struct PluginPanelItem *PanelItems, int ItemsNumber)
{
	int i;
	lua_createtable(L, ItemsNumber, 0);    //+1 "PanelItems"
	PushOptPluginTable(L, handle);         //+2
	for(i=0; i < ItemsNumber; i++) {
		PushPanelItem (L, PanelItems + i);
		lua_rawseti(L, -3, i+1);
	}
	lua_pop(L, 1);                         //+1
}
//---------------------------------------------------------------------------

static int far_PluginStartupInfo(lua_State *L)
{
	const wchar_t *slash;
	TPluginData *pd = GetPluginData(L);
	lua_createtable(L, 0, 3);
	PutWStrToTable(L, "ModuleName", pd->ModuleName, -1);

	slash = wcsrchr(pd->ModuleName, L'/');
	if (slash)
		PutWStrToTable(L, "ModuleDir", pd->ModuleName, slash - pd->ModuleName);

	lua_pushlightuserdata(L, (void*)pd->ModuleNumber);
	lua_setfield(L, -2, "ModuleNumber");

	PutWStrToTable(L, "RootKey", pd->RootKey, -1);

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

static int far_GetPluginGlobalInfo(lua_State *L)
{
	struct GlobalInfo info;
	GetPluginData(L)->GetGlobalInfo(&info);
	lua_createtable(L,0,6);
	PutNumToTable  (L, "SysID", info.SysID);
	PutWStrToTable (L, "Title", info.Title, -1);
	PutWStrToTable (L, "Description", info.Description, -1);
	PutWStrToTable (L, "Author", info.Author, -1);

	lua_createtable(L,4,0);
	PutIntToArray(L, 1, info.Version.Major);
	PutIntToArray(L, 2, info.Version.Minor);
	PutIntToArray(L, 3, info.Version.Revision);
	PutIntToArray(L, 4, info.Version.Build);
	lua_setfield(L, -2, "Version");
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

static int push_editor_filename(lua_State *L, int editorId)
{
	int size = PSInfo.EditorControlV2(editorId, ECTL_GETFILENAME, 0);
	if (!size) return 0;

	wchar_t* fname = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
	if (PSInfo.EditorControlV2(editorId, ECTL_GETFILENAME, fname)) {
		push_utf8_string(L, fname, -1);
		lua_remove(L, -2);
		return 1;
	}
	lua_pop(L,1);
	return 0;
}

static int editor_GetFileName(lua_State *L) {
	int editorId = luaL_optinteger(L,1,-1);
	if (!push_editor_filename(L, editorId)) lua_pushnil(L);
	return 1;
}

static int editor_GetInfo(lua_State *L)
{
	struct EditorInfo ei;
	int editorId = luaL_optinteger(L,1,-1);
	if (!PSInfo.EditorControlV2(editorId, ECTL_GETINFO, &ei))
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 18);
	PutNumToTable(L, "EditorID", ei.EditorID);

	if (push_editor_filename(L, editorId))
		lua_setfield(L, -2, "FileName");

	PutNumToTable(L, "WindowSizeX", ei.WindowSizeX);
	PutNumToTable(L, "WindowSizeY", ei.WindowSizeY);
	PutNumToTable(L, "TotalLines", ei.TotalLines);
	PutNumToTable(L, "CurLine", ei.CurLine + 1);
	PutNumToTable(L, "CurPos", ei.CurPos + 1);
	PutNumToTable(L, "CurTabPos", ei.CurTabPos + 1);
	PutNumToTable(L, "TopScreenLine", ei.TopScreenLine + 1);
	PutNumToTable(L, "LeftPos", ei.LeftPos + 1);
	PutNumToTable(L, "Overtype", ei.Overtype);
	PutNumToTable(L, "BlockType", ei.BlockType);
	PutNumToTable(L, "BlockStartLine", ei.BlockStartLine + 1);
	PutNumToTable(L, "Options", ei.Options);
	PutNumToTable(L, "TabSize", ei.TabSize);
	PutNumToTable(L, "BookmarkCount", ei.BookMarkCount);
	PutNumToTable(L, "SessionBookmarkCount", ei.SessionBookmarkCount);
	PutNumToTable(L, "CurState", ei.CurState);
	PutNumToTable(L, "CodePage", ei.CodePage);
	return 1;
}

/* t-rex:
 * Для тех кому плохо доходит описываю:
 * Редактор в фаре это двух связный список, указатель на текущюю строку
 * изменяется только при ECTL_SETPOSITION, при использовании любой другой
 * ECTL_* для которой нужно задавать номер строки если этот номер не -1
 * (т.е. текущаая строка) то фар должен найти эту строку в списке (а это
 * занимает дофига времени), поэтому если надо делать несколько ECTL_*
 * (тем более когда они делаются на последовательность строк
 * i,i+1,i+2,...) то перед каждым ECTL_* надо делать ECTL_SETPOSITION а
 * сами ECTL_* вызывать с -1.
 */
static BOOL FastGetString(int editorId, int string_num, struct EditorGetString *egs)
{
	struct EditorSetPosition esp;
	esp.CurLine   = string_num;
	esp.CurPos    = -1;
	esp.CurTabPos = -1;
	esp.TopScreenLine = -1;
	esp.LeftPos   = -1;
	esp.Overtype  = -1;

	if(!PSInfo.EditorControlV2(editorId, ECTL_SETPOSITION, &esp))
		return FALSE;

	egs->StringNumber = string_num;
	return PSInfo.EditorControlV2(editorId, ECTL_GETSTRING, egs) != 0;
}

// EditorGetString (EditorId, line_num, [mode])
//
//   line_num:  number of line in the Editor, a 1-based integer.
//
//   mode:      0 = returns: table LineInfo;        changes current position: no
//              1 = returns: table LineInfo;        changes current position: yes
//              2 = returns: StringText,StringEOL;  changes current position: yes
//              3 = returns: StringText,StringEOL;  changes current position: no
//
//   return:    either table LineInfo or StringText,StringEOL - depending on `mode` argument.
//
static int _EditorGetString(lua_State *L, int is_wide)
{
	int editorId = luaL_optinteger(L,1,-1);
	intptr_t line_num = luaL_optinteger(L, 2, 0) - 1;
	intptr_t mode = luaL_optinteger(L, 3, 0);
	BOOL res = 0;
	struct EditorGetString egs;

	if(mode == 0 || mode == 3)
	{
		egs.StringNumber = line_num;
		res = PSInfo.EditorControlV2(editorId, ECTL_GETSTRING, &egs) != 0;
	}
	else if(mode == 1 || mode == 2)
		res = FastGetString(editorId, line_num, &egs);

	if(res)
	{
		if(mode == 2 || mode == 3)
		{
			if(is_wide)
			{
				push_wcstring(L, egs.StringText, egs.StringLength);
				push_wcstring(L, egs.StringEOL, -1);
			}
			else
			{
				push_utf8_string(L, egs.StringText, egs.StringLength);
				push_utf8_string(L, egs.StringEOL, -1);
			}

			return 2;
		}
		else
		{
			lua_createtable(L, 0, 6);
			PutNumToTable(L, "StringNumber", (double)egs.StringNumber+1);
			PutNumToTable(L, "StringLength", (double)egs.StringLength);
			PutNumToTable(L, "SelStart", (double)egs.SelStart+1);
			PutNumToTable(L, "SelEnd", (double)egs.SelEnd);

			if(is_wide)
			{
				push_wcstring(L, egs.StringText, egs.StringLength);
				lua_setfield(L, -2, "StringText");
				push_wcstring(L, egs.StringEOL, -1);
				lua_setfield(L, -2, "StringEOL");
			}
			else
			{
				PutWStrToTable(L, "StringText",  egs.StringText, egs.StringLength);
				PutWStrToTable(L, "StringEOL",   egs.StringEOL, -1);
			}
		}

		return 1;
	}

	return lua_pushnil(L), 1;
}

static int editor_GetString(lua_State *L) { return _EditorGetString(L, 0); }
static int editor_GetStringW(lua_State *L) { return _EditorGetString(L, 1); }

static int _EditorSetString(lua_State *L, int is_wide)
{
	struct EditorSetString ess;
	size_t len;
	int editorId = luaL_optinteger(L,1,-1);
	ess.StringNumber = luaL_optinteger(L, 2, 0) - 1;

	if(is_wide)
	{
		ess.StringText = check_wcstring(L, 3, &len);
		ess.StringEOL = opt_wcstring(L, 4, NULL);

		if(ess.StringEOL)
		{
			lua_pushvalue(L, 4);
			lua_pushliteral(L, "\0\0\0\0");
			lua_concat(L, 2);
			ess.StringEOL = (wchar_t*) lua_tostring(L, -1);
		}
	}
	else
	{
		ess.StringText = check_utf8_string(L, 3, &len);
		ess.StringEOL = opt_utf8_string(L, 4, NULL);
	}

	ess.StringLength = len;
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_SETSTRING, &ess) != 0);
	return 1;
}

static int editor_SetString(lua_State *L) { return _EditorSetString(L, 0); }
static int editor_SetStringW(lua_State *L) { return _EditorSetString(L, 1); }

static int editor_InsertString(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	int indent = lua_toboolean(L, 2);
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_INSERTSTRING, &indent));
	return 1;
}

static int editor_DeleteString(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_DELETESTRING, NULL));
	return 1;
}

static int editor_InsertText(lua_State *L)
{
	int editorId, redraw, res;
	wchar_t* text;

	editorId = luaL_optinteger(L,1,-1);
	text = check_utf8_string(L,2,NULL);
	redraw = lua_toboolean(L,3);
	res = PSInfo.EditorControlV2(editorId, ECTL_INSERTTEXT_V2, text);
	if (res && redraw)
		PSInfo.EditorControlV2(editorId, ECTL_REDRAW, NULL);
	lua_pushboolean(L, res);
	return 1;
}

static int editor_InsertTextW(lua_State *L)
{
	int editorId, redraw, res;

	editorId = luaL_optinteger(L,1,-1);
	(void)luaL_checkstring(L,2);
	redraw = lua_toboolean(L,3);
	lua_pushvalue(L,2);
	lua_pushlstring(L, "\0\0\0\0", 4);
	lua_concat(L,2);
	res = PSInfo.EditorControlV2(editorId, ECTL_INSERTTEXT_V2, (void*)lua_tostring(L,-1));
	if (res && redraw)
		PSInfo.EditorControlV2(editorId, ECTL_REDRAW, NULL);
	lua_pushboolean(L, res);
	return 1;
}

static int editor_DeleteChar(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_DELETECHAR, NULL));
	return 1;
}

static int editor_DeleteBlock(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_DELETEBLOCK, NULL));
	return 1;
}

static int editor_UndoRedo(lua_State *L)
{
	struct EditorUndoRedo eur;
	int editorId = luaL_optinteger(L,1,-1);
	memset(&eur, 0, sizeof(eur));
	eur.Command = check_env_flag(L, 2);
	return lua_pushboolean (L, PSInfo.EditorControlV2(editorId, ECTL_UNDOREDO, &eur)), 1;
}

static int SetKeyBar(lua_State *L, BOOL editor)
{
	void* param;
	struct KeyBarTitles kbt;
	int frameId = luaL_optinteger(L,1,-1);

	enum { REDRAW=-1, RESTORE=0 }; // corresponds to FAR API
	BOOL argfail = FALSE;
	if (lua_isstring(L,2)) {
		const char* p = lua_tostring(L,2);
		if (0 == strcmp("redraw", p)) param = (void*)REDRAW;
		else if (0 == strcmp("restore", p)) param = (void*)RESTORE;
		else argfail = TRUE;
	}
	else if (lua_istable(L,2)) {
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
		lua_settop(L, 2);
		lua_newtable(L);
		int store = 0;
		size_t i;
		int j;
		for (i=0; i < ARRAYSIZE(pairs); i++) {
			lua_getfield (L, 2, pairs[i].key);
			if (lua_istable (L, -1)) {
				for (j=0; j<12; j++) {
					lua_pushinteger(L,j+1);
					lua_gettable(L,-2);
					if (lua_isstring(L,-1))
						pairs[i].trg[j] = (wchar_t*)StoreTempString(L, 3, &store);
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
		return luaL_argerror(L, 2, "must be 'redraw', 'restore', or table");

	int result = editor ? PSInfo.EditorControlV2(frameId, ECTL_SETKEYBAR, param) :
												PSInfo.ViewerControlV2(frameId, VCTL_SETKEYBAR, param);
	lua_pushboolean(L, result);
	return 1;
}

static int editor_SetKeyBar(lua_State *L)
{
	return SetKeyBar(L, TRUE);
}

static int viewer_SetKeyBar(lua_State *L)
{
	return SetKeyBar(L, FALSE);
}

static int editor_SetParam(lua_State *L)
{
	struct EditorSetParameter esp;
	int editorId = luaL_optinteger(L,1,-1);
	memset(&esp, 0, sizeof(esp));
	wchar_t buf[256];
	esp.Type = check_env_flag(L,2);
	//-----------------------------------------------------
	int tp = lua_type(L,3);
	if (tp == LUA_TNUMBER)
		esp.Param.iParam = lua_tointeger(L,3);
	else if (tp == LUA_TBOOLEAN)
		esp.Param.iParam = lua_toboolean(L,3);
	else if (tp == LUA_TSTRING)
		esp.Param.wszParam = (wchar_t*)check_utf8_string(L,3,NULL);
	//-----------------------------------------------------
	if(esp.Type == ESPT_GETWORDDIV) {
		esp.Param.wszParam = buf;
		esp.Size = ARRAYSIZE(buf);
	}
	//-----------------------------------------------------
	esp.Flags = GetFlags (L, 4, NULL);
	//-----------------------------------------------------
	int result = PSInfo.EditorControlV2(editorId, ECTL_SETPARAM, &esp);
	lua_pushboolean(L, result);
	if(result && esp.Type == ESPT_GETWORDDIV) {
		push_utf8_string(L,buf,-1); return 2;
	}
	return 1;
}

static int editor_SetPosition(lua_State *L)
{
	struct EditorSetPosition esp;
	int editorId = luaL_optinteger(L,1,-1);
	if (lua_istable(L, 2)) {
		lua_settop(L, 2);
		FillEditorSetPosition(L, &esp);
	}
	else {
		esp.CurLine   = luaL_optinteger(L, 2, 0) - 1;
		esp.CurPos    = luaL_optinteger(L, 3, 0) - 1;
		esp.CurTabPos = luaL_optinteger(L, 4, 0) - 1;
		esp.TopScreenLine = luaL_optinteger(L, 5, 0) - 1;
		esp.LeftPos   = luaL_optinteger(L, 6, 0) - 1;
		esp.Overtype  = luaL_optinteger(L, 7, -1);
	}
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_SETPOSITION, &esp));
	return 1;
}

static int editor_Redraw(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_REDRAW, NULL));
	return 1;
}

static int editor_ExpandTabs(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	int line_num = luaL_optinteger(L, 2, 0) - 1;
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_EXPANDTABS, &line_num));
	return 1;
}

static int PushBookmarks(lua_State *L, int editorId, int count, int command)
{
	if (count > 0) {
		struct EditorBookMarks ebm;
		ebm.Line = (long*)lua_newuserdata(L, 4 * count * sizeof(long));
		ebm.Cursor     = ebm.Line + count;
		ebm.ScreenLine = ebm.Cursor + count;
		ebm.LeftPos    = ebm.ScreenLine + count;
		if (PSInfo.EditorControlV2(editorId, command, &ebm)) {
			int i;
			lua_createtable(L, count, 0);
			for (i=0; i < count; i++) {
				lua_pushinteger(L, i+1);
				lua_createtable(L, 0, 4);
				PutIntToTable (L, "Line", ebm.Line[i] + 1);
				PutIntToTable (L, "Cursor", ebm.Cursor[i] + 1);
				PutIntToTable (L, "ScreenLine", ebm.ScreenLine[i] + 1);
				PutIntToTable (L, "LeftPos", ebm.LeftPos[i] + 1);
				lua_rawset(L, -3);
			}
			return 1;
		}
	}
	return lua_pushnil(L), 1;
}

static int editor_GetBookmarks(lua_State *L)
{
	struct EditorInfo ei;
	int editorId = luaL_optinteger(L,1,-1);
	if (!PSInfo.EditorControlV2(editorId, ECTL_GETINFO, &ei))
		return 0;
	return PushBookmarks(L, editorId, ei.BookMarkCount, ECTL_GETBOOKMARKS);
}

static int editor_GetSessionBookmarks(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	int count = PSInfo.EditorControlV2(editorId, ECTL_GETSTACKBOOKMARKS, NULL);
	return PushBookmarks(L, editorId, count, ECTL_GETSTACKBOOKMARKS);
}

static int editor_AddSessionBookmark(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_ADDSTACKBOOKMARK, NULL));
	return 1;
}

static int editor_ClearSessionBookmarks(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	lua_pushinteger(L, PSInfo.EditorControlV2(editorId, ECTL_CLEARSTACKBOOKMARKS, NULL));
	return 1;
}

static int editor_DeleteSessionBookmark(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	INT_PTR num = luaL_optinteger(L, 2, 0) - 1;
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_DELETESTACKBOOKMARK, (void*)num));
	return 1;
}

static int editor_NextSessionBookmark(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_NEXTSTACKBOOKMARK, NULL));
	return 1;
}

static int editor_PrevSessionBookmark(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_PREVSTACKBOOKMARK, NULL));
	return 1;
}

static int editor_SetTitle(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	const wchar_t* text = opt_utf8_string(L, 2, NULL);
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_SETTITLE, (wchar_t*)text));
	return 1;
}

static int editor_GetTitle(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	int size = PSInfo.EditorControlV2(editorId, ECTL_GETTITLE, NULL);
	lua_pushstring(L, "");
	if (size) {
		void* str = lua_newuserdata(L, size * sizeof(wchar_t));
		if (PSInfo.EditorControlV2(editorId, ECTL_GETTITLE, str))
			push_utf8_string(L, (wchar_t*)str, -1);
	}
	return 1;
}

static int editor_Quit(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_QUIT, NULL));
	return 1;
}

static int FillEditorSelect(lua_State *L, int pos_table, struct EditorSelect *es)
{
	int OK;
	lua_getfield(L, pos_table, "BlockType");
	es->BlockType = GetFlags(L, -1, &OK);
	if (!OK) {
		lua_pop(L,1);
		return 0;
	}
	lua_pushvalue(L, pos_table);
	es->BlockStartLine = GetOptIntFromTable(L, "BlockStartLine", 0) - 1;
	es->BlockStartPos  = GetOptIntFromTable(L, "BlockStartPos", 0) - 1;
	es->BlockWidth     = GetOptIntFromTable(L, "BlockWidth", -1);
	es->BlockHeight    = GetOptIntFromTable(L, "BlockHeight", -1);
	lua_pop(L,2);
	return 1;
}

static int editor_Select(lua_State *L)
{
	struct EditorSelect es;
	int result;
	int editorId = luaL_optinteger(L,1,-1);
	if (lua_istable(L, 2))
		result = FillEditorSelect(L, 2, &es);
	else {
		es.BlockType = GetFlags(L, 2, &result);
		if (result) {
			es.BlockStartLine = luaL_optinteger(L, 3, 0) - 1;
			es.BlockStartPos  = luaL_optinteger(L, 4, 0) - 1;
			es.BlockWidth     = luaL_optinteger(L, 5, -1);
			es.BlockHeight    = luaL_optinteger(L, 6, -1);
		}
	}
	result = result && PSInfo.EditorControlV2(editorId, ECTL_SELECT, &es);
	return lua_pushboolean(L, result), 1;
}

// This function is that long because FAR API does not supply needed
// information directly.
static int editor_GetSelection(lua_State *L)
{
	int BlockStartPos, h, from, to;
	struct EditorInfo EI;
	struct EditorGetString egs;
	struct EditorSetPosition esp;
	int editorId = luaL_optinteger(L,1,-1);
	PSInfo.EditorControlV2(editorId, ECTL_GETINFO, &EI);

	if(EI.BlockType == BTYPE_NONE || !FastGetString(editorId, EI.BlockStartLine, &egs))
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 5);
	PutIntToTable(L, "BlockType", EI.BlockType);
	PutIntToTable(L, "StartLine", EI.BlockStartLine+1);
	BlockStartPos = egs.SelStart;
	PutIntToTable(L, "StartPos", BlockStartPos+1);
	// binary search for a non-block line
	h = 100; // arbitrary small number
	from = EI.BlockStartLine;

	for(to = from+h; to < EI.TotalLines; to = from + (h*=2))
	{
		if(!FastGetString(editorId, to, &egs))
			return lua_pushnil(L), 1;

		if(egs.SelStart < 0)
			break;
	}

	if(to >= EI.TotalLines)
		to = EI.TotalLines - 1;

	// binary search for the last block line
	while(from != to)
	{
		int curr = (from + to + 1) / 2;

		if(!FastGetString(editorId, curr, &egs))
			return lua_pushnil(L), 1;

		if(egs.SelStart < 0)
		{
			if(curr == to)
				break;

			to = curr;      // curr was not selected
		}
		else
		{
			from = curr;    // curr was selected
		}
	}

	if(!FastGetString(editorId, from, &egs))
		return lua_pushnil(L), 1;

	PutIntToTable(L, "EndLine", from+1);
	PutIntToTable(L, "EndPos", egs.SelEnd);
	// restore current position, since FastGetString changed it
	esp.CurLine       = EI.CurLine;
	esp.CurPos        = EI.CurPos;
	esp.CurTabPos     = EI.CurTabPos;
	esp.TopScreenLine = EI.TopScreenLine;
	esp.LeftPos       = EI.LeftPos;
	esp.Overtype      = EI.Overtype;
	PSInfo.EditorControlV2(editorId, ECTL_SETPOSITION, &esp);
	return 1;
}

static int _EditorTabConvert(lua_State *L, int Operation)
{
	struct EditorConvertPos ecp;
	int editorId = luaL_optinteger(L,1,-1);
	ecp.StringNumber = luaL_optinteger(L, 2, 0) - 1;
	ecp.SrcPos = luaL_checkinteger(L, 3) - 1;
	if (PSInfo.EditorControlV2(editorId, Operation, &ecp))
		lua_pushinteger(L, ecp.DestPos+1);
	else
		lua_pushnil(L);
	return 1;
}

static int editor_TabToReal(lua_State *L)
{
	return _EditorTabConvert(L, ECTL_TABTOREAL);
}

static int editor_RealToTab(lua_State *L)
{
	return _EditorTabConvert(L, ECTL_REALTOTAB);
}

static int editor_TurnOffMarkingBlock(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	PSInfo.EditorControlV2(editorId, ECTL_TURNOFFMARKINGBLOCK, NULL);
	return 0;
}

static void FarTrueColorFromRGB(struct FarTrueColor *out, DWORD rgb, int used)
{
	out->Flags = used ? 1 : 0;
	out->R = rgb & 0xff;
	out->G = (rgb >> 8) & 0xff;
	out->B = (rgb >> 16) & 0xff;
}

DWORD FarTrueColorToRGB(const struct FarTrueColor *src)
{
	return src->R | (src->G << 8) | (src->B << 16) | (src->Flags << 24);
}

static int editor_AddColor(lua_State *L)
{
	const uint32_t
		MASK_COLOR  = 0x0000FFFF,
		MASK_ACTIVE = (0x1 << 24),
		COLOR_WHITE = 0xFFFFFF,
		COLOR_BLACK = 0x000000;

	struct EditorTrueColor etc;
	int editorId, Flags, isTrueColor;

	memset(&etc, 0, sizeof(etc));
	editorId              = luaL_optinteger(L,1,-1);
	etc.Base.StringNumber = luaL_optinteger(L,2,0) - 1;
	etc.Base.StartPos     = luaL_checkinteger(L,3) - 1;
	etc.Base.EndPos       = luaL_checkinteger(L,4) - 1;
	Flags                 = CheckFlags(L,5);
	isTrueColor           = lua_istable(L,6);

	if (isTrueColor)
	{
		DWORD fg, bg;
		lua_pushvalue(L,6);
		{
			etc.Base.Color // may contain COMMON_LVB_UNDERSCORE, etc.
			   = GetOptIntFromTable(L,"Attr",0) & MASK_COLOR;
			fg = GetOptIntFromTable(L,"Fore",COLOR_WHITE) | MASK_ACTIVE;
			bg = GetOptIntFromTable(L,"Back",COLOR_BLACK) | MASK_ACTIVE;
			FarTrueColorFromRGB(&etc.TrueColor.Fore, fg, 1);
			FarTrueColorFromRGB(&etc.TrueColor.Back, bg, 1);
		}
		lua_pop(L,1);
	}
	else
		etc.Base.Color = luaL_optinteger(L,6,0) & MASK_COLOR;

	if (etc.Base.Color == 0) // prevent color deletion
		etc.Base.Color = 0x0F;

	etc.Base.Color |= (Flags & ~MASK_COLOR);

	if (isTrueColor)
		lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_ADDTRUECOLOR, &etc));
	else
		lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_ADDCOLOR, &etc.Base));

	return 1;
}

static int editor_DelColor(lua_State *L)
{
	struct EditorColor ec;
	int editorId = luaL_optinteger(L,1,-1);
	memset(&ec, 0, sizeof(ec)); // set ec.Color = 0
	ec.StringNumber = luaL_optinteger  (L, 2, 0) - 1;
	ec.StartPos     = luaL_optinteger  (L, 3, 0) - 1;
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_ADDCOLOR, &ec)); // ECTL_ADDCOLOR (sic)
	return 1;
}

static int editor_GetColor(lua_State *L)
{
	struct EditorTrueColor etc;
	int editorId = luaL_optinteger(L,1,-1);
	memset(&etc, 0, sizeof(etc));
	etc.Base.StringNumber = luaL_optinteger(L, 2, 0) - 1;
	etc.Base.ColorItem    = luaL_checkinteger(L, 3) - 1;
	if (PSInfo.EditorControlV2(editorId, ECTL_GETTRUECOLOR, &etc))
	{
		lua_createtable(L, 0, 5);
		PutNumToTable(L, "StartPos", etc.Base.StartPos+1);
		PutNumToTable(L, "EndPos", etc.Base.EndPos+1);
		PutNumToTable(L, "BaseColor", etc.Base.Color);
		if (etc.TrueColor.Fore.Flags & 0x1)
			PutNumToTable(L, "TrueFore", etc.TrueColor.Fore.R | (etc.TrueColor.Fore.G << 8) | (etc.TrueColor.Fore.B << 16));
		if (etc.TrueColor.Back.Flags & 0x1)
			PutNumToTable(L, "TrueBack", etc.TrueColor.Back.R | (etc.TrueColor.Back.G << 8) | (etc.TrueColor.Back.B << 16));
	}
	else
		lua_pushnil(L);
	return 1;
}

static int editor_SaveFile(lua_State *L)
{
	struct EditorSaveFile esf;
	int editorId = luaL_optinteger(L,1,-1);
	esf.FileName = opt_utf8_string(L, 2, L"");
	esf.FileEOL = opt_utf8_string(L, 3, NULL);
	esf.CodePage = luaL_optinteger(L, 4, 0);
	if (esf.CodePage == 0) {
		struct EditorInfo ei;
		if (PSInfo.EditorControlV2(editorId, ECTL_GETINFO, &ei))
			esf.CodePage = ei.CodePage;
	}
	lua_pushboolean(L, PSInfo.EditorControlV2(editorId, ECTL_SAVEFILE, &esf));
	return 1;
}

void PushInputRecord (lua_State* L, const INPUT_RECORD *Rec)
{
	lua_newtable(L);                   //+2: Func,Tbl
	PutNumToTable(L, "EventType", Rec->EventType);
	switch (Rec->EventType) {
		case KEY_EVENT:
			PutBoolToTable(L,"KeyDown",         Rec->Event.KeyEvent.bKeyDown);
			PutNumToTable(L, "RepeatCount",     Rec->Event.KeyEvent.wRepeatCount);
			PutNumToTable(L, "VirtualKeyCode",  Rec->Event.KeyEvent.wVirtualKeyCode);
			PutNumToTable(L, "VirtualScanCode", Rec->Event.KeyEvent.wVirtualScanCode);
			PutWStrToTable(L, "UnicodeChar",   &Rec->Event.KeyEvent.uChar.UnicodeChar, 1);
			PutNumToTable(L, "ControlKeyState", Rec->Event.KeyEvent.dwControlKeyState);
			break;

		case MOUSE_EVENT:
			PutMouseEvent(L, &Rec->Event.MouseEvent, TRUE);
			break;

		case WINDOW_BUFFER_SIZE_EVENT:
			PutNumToTable(L, "SizeX", Rec->Event.WindowBufferSizeEvent.dwSize.X);
			PutNumToTable(L, "SizeY", Rec->Event.WindowBufferSizeEvent.dwSize.Y);
			break;

		case MENU_EVENT:
			PutNumToTable(L, "CommandId", Rec->Event.MenuEvent.dwCommandId);
			break;

		case FOCUS_EVENT:
			PutBoolToTable(L, "SetFocus", Rec->Event.FocusEvent.bSetFocus);
			break;
	}
}

static int editor_ReadInput(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, -1);
	INPUT_RECORD ir;

	if (PSInfo.EditorControlV2(EditorId, ECTL_READINPUT, &ir))
		PushInputRecord(L, &ir);
	else
		lua_pushnil(L);

	return 1;
}

void FillInputRecord(lua_State *L, int pos, INPUT_RECORD *ir)
{
	int ok;
	size_t size;

	pos = abs_index(L, pos);
	luaL_checktype(L, pos, LUA_TTABLE);
	memset(ir, 0, sizeof(INPUT_RECORD));

	// determine event type
	lua_getfield(L, pos, "EventType");
	ir->EventType = GetFlags(L, -1, &ok);
	if (!ok)
		luaL_argerror(L, pos, "EventType field is missing or invalid");
	lua_pop(L, 1);

	lua_pushvalue(L, pos);
	switch(ir->EventType) {
		case KEY_EVENT:
			ir->Event.KeyEvent.bKeyDown = GetOptBoolFromTable(L, "KeyDown", FALSE);
			ir->Event.KeyEvent.wRepeatCount = GetOptIntFromTable(L, "RepeatCount", 1);
			ir->Event.KeyEvent.wVirtualKeyCode = GetOptIntFromTable(L, "VirtualKeyCode", 0);
			ir->Event.KeyEvent.wVirtualScanCode = GetOptIntFromTable(L, "VirtualScanCode", 0);

			lua_getfield(L, -1, "UnicodeChar");
			if (lua_type(L,-1) == LUA_TSTRING) {
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

static int editor_ProcessInput(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	if (!lua_istable(L, 2))
		return 0;
	INPUT_RECORD ir;
	FillInputRecord(L, 2, &ir);
	if (PSInfo.EditorControlV2(editorId, ECTL_PROCESSINPUT, &ir))
		return lua_pushboolean(L, 1), 1;
	return 0;
}

static int editor_ProcessKey(lua_State *L)
{
	int editorId = luaL_optinteger(L,1,-1);
	INT_PTR key = luaL_checkinteger(L,2);
	PSInfo.EditorControlV2(editorId, ECTL_PROCESSKEY, (void*)key);
	return 0;
}

// Item, Position = Menu (Properties, Items [, Breakkeys])
// Parameters:
//   Properties -- a table
//   Items      -- an array of tables
//   BreakKeys  -- an array of strings with special syntax
// Return value:
//   Item:
//     a table  -- the table of selected item (or of breakkey) is returned
//     a nil    -- menu canceled by the user
//   Position:
//     a number -- position of selected menu item
//     a nil    -- menu canceled by the user
static int far_Menu(lua_State *L)
{
	TPluginData *pd = GetPluginData(L);
	int X = -1, Y = -1, MaxHeight = 0;
	int Flags;
	const wchar_t *Title = L"Menu", *Bottom = NULL, *HelpTopic = NULL;

	lua_settop (L, 3);    // cut unneeded parameters; make stack predictable
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checktype(L, 2, LUA_TTABLE);
	if (lua_toboolean(L,3) && !lua_istable(L,3) && !lua_isstring(L,3))
		return luaL_argerror(L, 3, "must be table, string or nil");

	lua_newtable(L); // temporary store; at stack position 4
	int store = 0;

	// Properties
	lua_pushvalue (L,1);  // push Properties on top (stack index 5)
	X = GetOptIntFromTable(L, "X", -1);
	Y = GetOptIntFromTable(L, "Y", -1);
	MaxHeight = GetOptIntFromTable(L, "MaxHeight", 0);
	lua_getfield(L, 1, "Flags");
	Flags = CheckFlags(L, -1);
	lua_getfield(L, 1, "Title");
	if(lua_isstring(L,-1))    Title = StoreTempString(L, 4, &store);
	lua_getfield(L, 1, "Bottom");
	if(lua_isstring(L,-1))    Bottom = StoreTempString(L, 4, &store);
	lua_getfield(L, 1, "HelpTopic");
	if(lua_isstring(L,-1))    HelpTopic = StoreTempString(L, 4, &store);
	lua_getfield(L, 1, "SelectIndex");
	int ItemsNumber = lua_objlen(L, 2);
	int SelectIndex = lua_tointeger(L,-1) - 1;
	if (!(SelectIndex >= 0 && SelectIndex < ItemsNumber))
		SelectIndex = -1;

	// Items
	int i;
	struct FarMenuItemEx* Items = (struct FarMenuItemEx*)
		lua_newuserdata(L, ItemsNumber*sizeof(struct FarMenuItemEx));
	memset(Items, 0, ItemsNumber*sizeof(struct FarMenuItemEx));
	struct FarMenuItemEx* pItem = Items;
	for(i=0; i < ItemsNumber; i++,pItem++,lua_pop(L,1)) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, 2);
		if (!lua_istable(L, -1))
			return luaLF_SlotError (L, i+1, "table");
		//-------------------------------------------------------------------------
		const char *key = "text";
		lua_getfield(L, -1, key);
		if (lua_isstring(L,-1))  pItem->Text = StoreTempString(L, 4, &store);
		else if(!lua_isnil(L,-1)) return luaLF_FieldError (L, key, "string");
		if (!pItem->Text)
			lua_pop(L, 1);
		//-------------------------------------------------------------------------
		lua_getfield(L,-1,"checked");
		if (lua_type(L,-1) == LUA_TSTRING) {
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
	int BreakCode;
	int *pBreakKeys=NULL, *pBreakCode=NULL;
	int NumBreakCodes = 0;
	if (lua_isstring(L,3))
	{
		const char *q, *ptr = lua_tostring(L,3);
		lua_newtable(L);
		while (*ptr)
		{
			while (isspace(*ptr)) ptr++;
			if (*ptr == 0) break;
			q = ptr++;
			while(*ptr && !isspace(*ptr)) ptr++;
			lua_createtable(L,0,1);
			lua_pushlstring(L,q,ptr-q);
			lua_setfield(L,-2,"BreakKey");
			lua_rawseti(L,-2,++NumBreakCodes);
		}
		lua_replace(L,3);
	}
	else
		NumBreakCodes = lua_istable(L,3) ? (int)lua_objlen(L,3) : 0;

	if (NumBreakCodes) {
		int* BreakKeys = (int*)lua_newuserdata(L, (1+NumBreakCodes)*sizeof(int));
		// get virtualkeys table from the registry; push it on top
		lua_pushstring(L, FAR_VIRTUALKEYS);
		lua_rawget(L, LUA_REGISTRYINDEX);
		// push breakkeys table on top
		lua_pushvalue(L, 3);              // vk=-2; bk=-1;
		char buf[32];
		int ind, out; // used outside the following loop

// Prevent an invalid break key from shifting or invalidating the following ones
#define INSERT_INVALID() do { BreakKeys[out++] = (0|PKF_ALT); } while (0)

		for(ind=0,out=0; ind < NumBreakCodes; ind++) {
			// get next break key (optional modifier plus virtual key)
			lua_pushinteger(L,ind+1);       // vk=-3; bk=-2;
			lua_gettable(L,-2);             // vk=-3; bk=-2;
			if(!lua_istable(L,-1))  { lua_pop(L,1); INSERT_INVALID(); continue; }
			lua_getfield(L, -1, "BreakKey");// vk=-4; bk=-3;
			if(!lua_isstring(L,-1)) { lua_pop(L,2); INSERT_INVALID(); continue; }

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
					BreakKeys[out++] = (Code & 0xFFFF) | (mod << 16);
					lua_pop(L, 2);
					continue; // success
				}
				// restore the original string
				lua_pop(L, 1);
				lua_getfield(L, -1, "BreakKey");// vk=-4; bk=-3;bki=-2;bknm=-1;
			}

			// separate modifier and virtual key strings
			int mod = 0;
			const char* s = lua_tostring(L,-1);
			if(strlen(s) >= sizeof(buf)) { lua_pop(L,2); INSERT_INVALID(); continue; }
			char* vk = buf;
			do *vk++ = toupper(*s); while(*s++); // copy and convert to upper case
			vk = strchr(buf, '+');  // virtual key
			if (vk) {
				*vk++ = '\0';
				if(strchr(buf,'C')) mod |= PKF_CONTROL;
				if(strchr(buf,'A')) mod |= PKF_ALT;
				if(strchr(buf,'S')) mod |= PKF_SHIFT;
				mod <<= 16;
				// replace on stack: break key name with virtual key name
				lua_pop(L, 1);
				lua_pushstring(L, vk);
			}
			// get virtual key and break key values
			lua_rawget(L,-4);               // vk=-4; bk=-3;
			int tmp = lua_tointeger(L,-1) | mod;
			if (tmp)
				BreakKeys[out++] = tmp;
			else
				INSERT_INVALID();
			lua_pop(L,2);                   // vk=-2; bk=-1;
		}
#undef INSERT_INVALID
		BreakKeys[out] = 0; // required by FAR API
		pBreakKeys = BreakKeys;
		pBreakCode = &BreakCode;
	}

	int ret = PSInfo.Menu(
		pd->ModuleNumber, X, Y, MaxHeight, Flags|FMENU_USEEXT,
		Title, Bottom, HelpTopic, pBreakKeys, pBreakCode,
		(const struct FarMenuItem *)Items, ItemsNumber);

	if (NumBreakCodes && (BreakCode != -1)) {
		lua_pushinteger(L, BreakCode+1);
		lua_gettable(L, 3);
	}
	else if (ret == -1)
		return lua_pushnil(L), 1;
	else {
		lua_pushinteger(L, ret+1);
		lua_gettable(L, 2);
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
	const wchar_t **items, **pItems;
	wchar_t** allocLines;
	int nAlloc;
	wchar_t *lastDelim, *MsgCopy, *start, *pos;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	int ret = WINPORT(GetConsoleScreenBufferInfo)(NULL, &csbi);//GetStdHandle(STD_OUTPUT_HANDLE)
	const int max_len   = ret ? csbi.srWindow.Right - csbi.srWindow.Left+1-14 : 66;
	const int max_lines = ret ? csbi.srWindow.Bottom - csbi.srWindow.Top+1-5 : 20;
	int num_lines = 0, num_buttons = 0;
	uint64_t Flags = 0;
	// Buttons
	wchar_t *BtnCopy = NULL, *ptr = NULL;
	int wrap = !(aFlags && strchr(aFlags, 'n'));

	if(*aButtons == L';')
	{
		const wchar_t* p = aButtons + 1;

		if(!wcscasecmp(p, L"Ok"))                    Flags = FMSG_MB_OK;
		else if(!wcscasecmp(p, L"OkCancel"))         Flags = FMSG_MB_OKCANCEL;
		else if(!wcscasecmp(p, L"AbortRetryIgnore")) Flags = FMSG_MB_ABORTRETRYIGNORE;
		else if(!wcscasecmp(p, L"YesNo"))            Flags = FMSG_MB_YESNO;
		else if(!wcscasecmp(p, L"YesNoCancel"))      Flags = FMSG_MB_YESNOCANCEL;
		else if(!wcscasecmp(p, L"RetryCancel"))      Flags = FMSG_MB_RETRYCANCEL;
		else
			while(*aButtons == L';') aButtons++;
	}
	if(Flags == 0)
	{
		// Buttons: 1-st pass, determining number of buttons
		BtnCopy = _wcsdup(aButtons);
		ptr = BtnCopy;

		while(*ptr && (num_buttons < 64))
		{
			while(*ptr == L';')
				ptr++; // skip semicolons

			if(*ptr)
			{
				++num_buttons;
				ptr = wcschr(ptr, L';');

				if(!ptr) break;
			}
		}
	}

	items = (const wchar_t**) malloc((1+max_lines+num_buttons) * sizeof(wchar_t*));
	allocLines = (wchar_t**) malloc(max_lines * sizeof(wchar_t*)); // array of pointers to allocated lines
	nAlloc = 0;                                                    // number of allocated lines
	pItems = items;
	// Title
	*pItems++ = aTitle;
	// Message lines
	lastDelim = NULL;
	MsgCopy = _wcsdup(aMsg);
	start = pos = MsgCopy;

	while(num_lines < max_lines)
	{
		if(*pos == 0)                          // end of the entire message
		{
			*pItems++ = start;
			++num_lines;
			break;
		}
		else if(*pos == L'\n')                 // end of a message line
		{
			*pItems++ = start;
			*pos = L'\0';
			++num_lines;
			start = ++pos;
			lastDelim = NULL;
		}
		else if(pos-start < max_len)            // characters inside the line
		{
			if (wrap && !iswalnum(*pos) && *pos != L'_' && *pos != L'\'' && *pos != L'\"')
				lastDelim = pos;

			pos++;
		}
		else if (wrap)                          // the 1-st character beyond the line
		{
			size_t len;
			wchar_t **q;
			pos = lastDelim ? lastDelim+1 : pos;
			len = pos - start;
			q = &allocLines[nAlloc++]; // line allocation is needed
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

	if(*aButtons != L';')
	{
		// Buttons: 2-nd pass.
		int i;
		ptr = BtnCopy;

		for(i=0; i < num_buttons; i++)
		{
			while(*ptr == L';')
				++ptr;

			if(*ptr)
			{
				*pItems++ = ptr;
				ptr = wcschr(ptr, L';');

				if(ptr)
					*ptr++ = 0;
				else
					break;
			}
			else break;
		}
	}

	// Flags
	if(aFlags)
	{
		if(strchr(aFlags, 'w')) Flags |= FMSG_WARNING;
		if(strchr(aFlags, 'e')) Flags |= FMSG_ERRORTYPE;
		if(strchr(aFlags, 'k')) Flags |= FMSG_KEEPBACKGROUND;
		if(strchr(aFlags, 'l')) Flags |= FMSG_LEFTALIGN;
	}

	ret = PSInfo.MessageV3(pd->ModuleNumber, aMessageGuid, Flags, aHelpTopic, items, 1+num_lines+num_buttons, num_buttons);
	free(BtnCopy);

	while(nAlloc) free(allocLines[--nAlloc]);

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
	lua_pushlstring(L, (void*)pd->ModuleName, sizeof(wchar_t) * wcslen(pd->ModuleName));
	lua_pushlstring(L, (void*)L":\n", sizeof(wchar_t) * 2);
	LF_Gsub(L, aMsg, L"\n\t", L"\n   ");
	lua_concat(L, 3);
	LF_Message(L, (void*)lua_tostring(L,-1), L"Error", L"OK", "w", NULL, NULL);
	lua_pop(L, 1);
}

static int SplitToTable(lua_State *L, const wchar_t *Text, wchar_t Delim, int StartIndex)
{
	int count = StartIndex;
	const wchar_t *p = Text;
	do {
		const wchar_t *q = wcschr(p, Delim);
		if (q == NULL) q = wcschr(p, L'\0');
		lua_pushinteger(L, ++count);
		lua_pushlstring(L, (const char*)p, (q-p)*sizeof(wchar_t));
		lua_rawset(L, -3);
		p = *q ? q+1 : NULL;
	} while(p);
	return count - StartIndex;
}

// 1-st param: message text (if multiline, then lines must be separated by '\n')
// 2-nd param: message title (if absent or nil, then "Message" is used)
// 3-rd param: buttons (if multiple, then captions must be separated by ';';
//             if absent or nil, then one button "OK" is used).
// 4-th param: flags
// 5-th param: help topic
// Return: -1 if escape pressed, else - button number chosen (1 based).
static int far_Message(lua_State *L)
{
	int ret;
	const wchar_t *Msg, *Title, *Buttons, *HelpTopic;
	const char *Flags;
	const GUID *Id;
	luaL_checkany(L,1);
	lua_settop(L,6);
	Msg = NULL;

	if (lua_isstring(L, 1))
		Msg = check_utf8_string(L, 1, NULL);
	else {
		lua_getglobal(L, "tostring");
		if (lua_isfunction(L,-1)) {
			lua_pushvalue(L,1);
			lua_call(L,1,1);
			Msg = check_utf8_string(L,-1,NULL);
		}
		if (Msg == NULL) luaL_argerror(L, 1, "cannot convert to string");
		lua_replace(L,1);
	}

	Title   = opt_utf8_string(L, 2, L"Message");
	Buttons = opt_utf8_string(L, 3, L";OK");
	Flags   = luaL_optstring(L, 4, "");
	HelpTopic = opt_utf8_string(L, 5, NULL);
	Id = (lua_type(L,6)==LUA_TSTRING && lua_objlen(L,6)==sizeof(GUID)) ?
	     (const GUID*)lua_tostring(L,6) : NULL;

	ret = LF_Message(L, Msg, Title, Buttons, Flags, HelpTopic, Id);
	lua_pushinteger(L, ret<0 ? ret : ret+1);
	return 1;
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

	lua_createtable(L, 0, 12);
	//-------------------------------------------------------------------------
	PutIntToTable (L, "PanelType", pi.PanelType);
	//-------------------------------------------------------------------------
	lua_createtable(L, 0, 4); // "PanelRect"
	PutIntToTable (L, "left",   pi.PanelRect.left);
	PutIntToTable (L, "top",    pi.PanelRect.top);
	PutIntToTable (L, "right",  pi.PanelRect.right);
	PutIntToTable (L, "bottom", pi.PanelRect.bottom);
	lua_setfield(L, -2, "PanelRect");
	//-------------------------------------------------------------------------
	PutIntToTable (L, "ItemsNumber",  pi.ItemsNumber);
	PutIntToTable (L, "SelectedItemsNumber", pi.SelectedItemsNumber);
	PutIntToTable (L, "CurrentItem",  pi.CurrentItem + 1);
	PutIntToTable (L, "TopPanelItem", pi.TopPanelItem + 1);
	PutIntToTable (L, "ViewMode",     pi.ViewMode);
	PutIntToTable (L, "SortMode",     pi.SortMode);
	PutIntToTable (L, "Flags",        pi.Flags);
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
	return 1;
}

static int get_panel_item(lua_State *L, int command)
{
	HANDLE handle = OptHandle2(L);
	int index = luaL_optinteger(L,3,1) - 1;
	if(index >= 0 || command == FCTL_GETCURRENTPANELITEM)
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

static int panel_GetPanelItem(lua_State *L) {
	return get_panel_item(L, FCTL_GETPANELITEM);
}

static int panel_GetSelectedPanelItem(lua_State *L) {
	return get_panel_item(L, FCTL_GETSELECTEDPANELITEM);
}

static int panel_GetCurrentPanelItem(lua_State *L) {
	return get_panel_item(L, FCTL_GETCURRENTPANELITEM);
}

static int get_string_info(lua_State *L, int command)
{
	HANDLE handle = OptHandle2(L);
	int size = PSInfo.Control(handle, command, 0, 0);
	if (size) {
		wchar_t *buf = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
		if (PSInfo.Control(handle, command, size, (LONG_PTR)buf)) {
			push_utf8_string(L, buf, -1);
			return 1;
		}
	}
	return lua_pushnil(L), 1;
}

static int panel_GetPanelDirectory(lua_State *L) {
	return get_string_info(L, FCTL_GETPANELDIR);
}

static int panel_GetPanelFormat(lua_State *L) {
	return get_string_info(L, FCTL_GETPANELFORMAT);
}

static int panel_GetPanelHostFile(lua_State *L) {
	return get_string_info(L, FCTL_GETPANELHOSTFILE);
}

static int panel_GetColumnTypes(lua_State *L) {
	return get_string_info(L, FCTL_GETCOLUMNTYPES);
}

static int panel_GetColumnWidths(lua_State *L) {
	return get_string_info(L, FCTL_GETCOLUMNWIDTHS);
}

static int panel_GetPanelPrefix(lua_State *L) {
	return get_string_info(L, FCTL_GETPANELPREFIX);
}

static int panel_RedrawPanel(lua_State *L)
{
	HANDLE handle = OptHandle2(L);
	LONG_PTR param2 = 0;
	struct PanelRedrawInfo pri;
	if (lua_istable(L, 3)) {
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

static int panel_SetCaseSensitiveSort(lua_State *L) {
	return SetPanelBooleanProperty(L, FCTL_SETCASESENSITIVESORT);
}

static int panel_SetNumericSort(lua_State *L) {
	return SetPanelBooleanProperty(L, FCTL_SETNUMERICSORT);
}

static int panel_SetSortOrder(lua_State *L) {
	return SetPanelBooleanProperty(L, FCTL_SETSORTORDER);
}

static int panel_SetDirectoriesFirst(lua_State *L)
{
	return SetPanelBooleanProperty(L, FCTL_SETDIRECTORIESFIRST);
}

static int panel_UpdatePanel(lua_State *L) {
	return SetPanelBooleanProperty(L, FCTL_UPDATEPANEL);
}

static int panel_SetSortMode(lua_State *L) {
	return SetPanelIntegerProperty(L, FCTL_SETSORTMODE);
}

static int panel_SetViewMode(lua_State *L) {
	return SetPanelIntegerProperty(L, FCTL_SETVIEWMODE);
}

static int panel_SetPanelDirectory(lua_State *L)
{
	HANDLE handle = OptHandle2(L);
	LONG_PTR param2 = 0;
	int ret;
	if (lua_isstring(L, 3)) {
		const wchar_t* dir = check_utf8_string(L, 3, NULL);
		param2 = (LONG_PTR)dir;
	}
	ret = PSInfo.Control(handle, FCTL_SETPANELDIR, 0, param2);
	if (ret)
		PSInfo.Control(handle, FCTL_REDRAWPANEL, 0, 0); //not required in Far3
	lua_pushboolean(L, ret);
	return 1;
}

static int panel_SetPanelLocation(lua_State *L)
{
	HANDLE handle = OptHandle2(L);
	struct FarPanelLocation fpl = {};
	int ret;
	luaL_checktype(L, 3, LUA_TTABLE);

	lua_getfield(L, 3, "PluginName");
	if (lua_isstring(L,-1)) fpl.PluginName = check_utf8_string(L,-1,NULL);

	lua_getfield(L, 3, "HostFile");
	if (lua_isstring(L,-1)) fpl.HostFile = check_utf8_string(L,-1,NULL);

	lua_getfield(L, 3, "Item");
	fpl.Item = (LONG_PTR)lua_tointeger(L,-1);

	lua_getfield(L, 3, "Path");
	if (lua_isstring(L,-1)) fpl.Path = check_utf8_string(L,-1,NULL);

	ret = PSInfo.Control(handle, FCTL_SETPANELLOCATION, 0, (LONG_PTR)&fpl);
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
		int i, len = lua_objlen(L,3);
		for (i=1; i<=len; i++) {
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

static int panel_SetSelection(lua_State *L) {
	return ChangePanelSelection(L, TRUE);
}

static int panel_ClearSelection(lua_State *L) {
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

// GetDirList (Dir)
//   Dir:     Name of the directory to scan (full pathname).
static int far_GetDirList (lua_State *L)
{
	const wchar_t *Dir = check_utf8_string (L, 1, NULL);
	struct FAR_FIND_DATA *PanelItems;
	int ItemsNumber;
	int ret = PSInfo.GetDirList (Dir, &PanelItems, &ItemsNumber);
	if(ret) {
		int i;
		lua_createtable(L, ItemsNumber, 0); // "PanelItems"
		for(i=0; i < ItemsNumber; i++) {
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
	intptr_t X1 = luaL_optinteger(L,1,0);
	intptr_t Y1 = luaL_optinteger(L,2,0);
	intptr_t X2 = luaL_optinteger(L,3,-1);
	intptr_t Y2 = luaL_optinteger(L,4,-1);

	*(void**)lua_newuserdata(L, sizeof(void*)) = PSInfo.SaveScreen(X1,Y1,X2,Y2);
	luaL_getmetatable(L, SavedScreenType);
	lua_setmetatable(L, -2);
	return 1;
}

static int GetDialogItemType(lua_State* L, int key, int item)
{
	int ok;
	lua_pushinteger(L, key);
	lua_gettable(L, -2);
	int iType = GetFlags(L, -1, &ok);
	if (!ok) {
		const char* sType = lua_tostring(L, -1);
		return luaL_error(L, "%s - unsupported type in dialog item %d", sType, item);
	}
	lua_pop(L, 1);
	return iType;
}

// the table is on lua stack top
flags_t GetItemFlags(lua_State* L, int flag_index, int item_index)
{
	flags_t flags;
	int ok;
	lua_pushinteger(L, flag_index);
	lua_gettable(L, -2);
	flags = GetFlags (L, -1, &ok);
	if (!ok)
		return luaL_error(L, "unsupported flag in dialog item %d", item_index);
	lua_pop(L, 1);
	return flags;
}

// list table is on Lua stack top
struct FarList* CreateList(lua_State *L, int historyindex)
{
	int i, n = (int)lua_objlen(L,-1);
	struct FarList* list = (struct FarList*)lua_newuserdata(L,
												 sizeof(struct FarList) + n*sizeof(struct FarListItem)); // +2
	int len = (int)lua_objlen(L, historyindex);
	lua_rawseti(L, historyindex, ++len);  // +1; put into "histories" table to avoid being gc'ed
	list->ItemsNumber = n;
	list->Items = (struct FarListItem*)(list+1);
	for(i=0; i<n; i++)
	{
		struct FarListItem *p = list->Items + i;
		lua_pushinteger(L, i+1); // +2
		lua_gettable(L,-2);      // +2
		if(lua_type(L,-1) != LUA_TTABLE)
			luaL_error(L, "value at index %d is not a table", i+1);
		p->Text = NULL;
		lua_getfield(L, -1, "Text"); // +3
		if(lua_isstring(L,-1))
		{
			lua_pushvalue(L,-1);                     // +4
			p->Text = check_utf8_string(L,-1,NULL);  // +4
			lua_rawseti(L, historyindex, ++len);     // +3
		}
		lua_pop(L, 1);                 // +2
		lua_getfield(L, -1, "Flags");  // +3
		p->Flags = CheckFlags(L,-1);
		lua_pop(L,2);                  // +1
	}
	return list;
}

// item table is on Lua stack top
static void SetFarDialogItem(lua_State *L, struct FarDialogItem* Item, int itemindex, int historyindex)
{
	flags_t Flags;

	memset(Item, 0, sizeof(struct FarDialogItem));
	Item->Type  = GetDialogItemType (L, 1, itemindex+1);
	Item->X1    = GetIntFromArray   (L, 2);
	Item->Y1    = GetIntFromArray   (L, 3);
	Item->X2    = GetIntFromArray   (L, 4);
	Item->Y2    = GetIntFromArray   (L, 5);

	Flags = GetItemFlags(L, 9, itemindex+1);
	Item->Focus = (Flags & DIF_FOCUS) ? 1:0;
	Item->DefaultButton = (Flags & DIF_DEFAULTBUTTON) ? 1:0;
	Item->Flags = Flags & 0xFFFFFFFF;

	if (Item->Type==DI_LISTBOX || Item->Type==DI_COMBOBOX) {
		lua_pushinteger(L, 6);   // +1
		lua_gettable(L, -2);     // +1
		if (lua_type(L,-1) != LUA_TTABLE)
			luaLF_SlotError (L, 7, "table");
		Item->ListItems = CreateList(L, historyindex);
		int SelectIndex = GetOptIntFromTable(L, "SelectIndex", -1);
		if (SelectIndex > 0 && SelectIndex <= (int)lua_objlen(L,-1))
			Item->ListItems->Items[SelectIndex-1].Flags |= LIF_SELECTED;
		lua_pop(L,1);                    // 0
	}
	else if (Item->Type == DI_USERCONTROL)
	{
		lua_rawgeti(L, -1, 6);
		if (lua_type(L,-1) == LUA_TUSERDATA)
		{
			TFarUserControl* fuc = CheckFarUserControl(L, -1);
			Item->VBuf = fuc->VBuf;
		}
		lua_pop(L,1);
	}
	else if (Item->Type == DI_CHECKBOX || Item->Type == DI_RADIOBUTTON) {
		lua_pushinteger(L, 6);
		lua_gettable(L, -2);
		if (lua_isnumber(L,-1))
			Item->Selected = lua_tointeger(L,-1);
		else
			Item->Selected = lua_toboolean(L,-1) ? BSTATE_CHECKED : BSTATE_UNCHECKED;
		lua_pop(L, 1);
	}
	else if (Item->Type == DI_EDIT) {
		if (Item->Flags & DIF_HISTORY) {
			lua_rawgeti(L, -1, 7);      // +1
			Item->History = opt_utf8_string (L, -1, NULL); // +1 --> Item->History and Item->Mask are aliases (union members)
			size_t len = lua_objlen(L, historyindex);
			lua_rawseti (L, historyindex, len+1); // +0; put into "histories" table to avoid being gc'ed
		}
	}
	else if (Item->Type == DI_FIXEDIT) {
		if (Item->Flags & DIF_MASKEDIT) {
			lua_rawgeti(L, -1, 8);      // +1
			Item->Mask = opt_utf8_string (L, -1, NULL); // +1 --> Item->History and Item->Mask are aliases (union members)
			size_t len = lua_objlen(L, historyindex);
			lua_rawseti (L, historyindex, len+1); // +0; put into "histories" table to avoid being gc'ed
		}
	}

	Item->MaxLen = GetOptIntFromArray(L, 11, 0);
	lua_pushinteger(L, 10); // +1
	lua_gettable(L, -2);    // +1
	if (lua_isstring(L, -1)) {
		Item->PtrData = check_utf8_string (L, -1, NULL); // +1
		size_t len = lua_objlen(L, historyindex);
		lua_rawseti (L, historyindex, len+1); // +0; put into "histories" table to avoid being gc'ed
	}
	else
		lua_pop(L, 1);
}

static void PushDlgItem (lua_State *L, const struct FarDialogItem* pItem, BOOL table_exist)
{
	flags_t Flags;

	if (! table_exist) {
		lua_createtable(L, 11, 0);
		if (pItem->Type == DI_LISTBOX || pItem->Type == DI_COMBOBOX) {
			lua_createtable(L, 0, 1);
			lua_rawseti(L, -2, 6);
		}
	}
	PutIntToArray  (L, 1, pItem->Type);
	PutIntToArray  (L, 2, pItem->X1);
	PutIntToArray  (L, 3, pItem->Y1);
	PutIntToArray  (L, 4, pItem->X2);
	PutIntToArray  (L, 5, pItem->Y2);

	if (pItem->Type == DI_LISTBOX || pItem->Type == DI_COMBOBOX) {
		lua_rawgeti(L, -1, 6);
		lua_pushinteger(L, pItem->ListPos+1);
		lua_setfield(L, -2, "SelectIndex");
		lua_pop(L,1);
	}
	else if (pItem->Type == DI_USERCONTROL)
	{
		lua_pushlightuserdata(L, pItem->VBuf);
		lua_rawseti(L, -2, 6);
	}
	else if (pItem->Type == DI_CHECKBOX || pItem->Type == DI_RADIOBUTTON)
	{
		lua_pushinteger(L, pItem->Selected);
		lua_rawseti(L, -2, 6);
	}
	else if (pItem->Type == DI_EDIT && (pItem->Flags & DIF_HISTORY))
	{
		PutWStrToArray(L, 7, pItem->History, -1);
	}
	else if (pItem->Type == DI_FIXEDIT && (pItem->Flags & DIF_MASKEDIT))
	{
		PutWStrToArray(L, 8, pItem->Mask, -1);
	}
	else
		PutIntToArray(L, 6, pItem->Selected);

	Flags = pItem->Flags;
	if (pItem->Focus) Flags |= DIF_FOCUS;
	if (pItem->DefaultButton) Flags |= DIF_DEFAULTBUTTON;
	PutNumToArray(L, 9, Flags);

	lua_pushinteger(L, 10);
	push_utf8_string(L, pItem->PtrData, -1);
	lua_settable(L, -3);
	PutIntToArray  (L, 11, pItem->MaxLen);
}

LONG_PTR SendDlgMessage(HANDLE hDlg, int Msg, int Param1, void *Param2)
{
	return PSInfo.SendDlgMessage(hDlg, Msg, Param1, (LONG_PTR)Param2);
}

static void PushDlgItemNum(lua_State *L, HANDLE hDlg, int numitem, int pos_table)
{
	int size = PSInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, numitem, 0);
	if (size > 0) {
		BOOL table_exist = lua_istable(L, pos_table);
		struct FarDialogItem* pItem = (struct FarDialogItem*) lua_newuserdata(L, size);
		SendDlgMessage(hDlg, DM_GETDLGITEM, numitem, pItem);
		if (table_exist)
			lua_pushvalue(L, pos_table);
		PushDlgItem(L, pItem, table_exist);
	}
	else
		lua_pushnil(L);
}

static int SetDlgItem (lua_State *L, HANDLE hDlg, int numitem, int pos_table)
{
	struct FarDialogItem DialogItem;
	lua_newtable(L);
	lua_replace(L,1);
	luaL_checktype(L, pos_table, LUA_TTABLE);
	lua_pushvalue(L, pos_table);
	SetFarDialogItem(L, &DialogItem, numitem, 1);
	lua_pushboolean(L, SendDlgMessage(hDlg, DM_SETDLGITEM, numitem, &DialogItem));
	return 1;
}

TDialogData* NewDialogData(lua_State* L, HANDLE hDlg, BOOL isOwned)
{
	TDialogData *dd = (TDialogData*) lua_newuserdata(L, sizeof(TDialogData));
	dd->L        = GetPluginData(L)->MainLuaState;
	dd->hDlg     = hDlg;
	dd->isOwned  = isOwned;
	dd->wasError = FALSE;
	dd->isModal  = TRUE;
	dd->dataRef  = LUA_REFNIL;
	luaL_getmetatable(L, FarDialogType);
	lua_setmetatable(L, -2);
	if (isOwned) {
		lua_newtable(L);
		lua_setfenv(L, -2);
	}
	return dd;
}

TDialogData* CheckDialog(lua_State* L, int pos)
{
	return (TDialogData*)luaL_checkudata(L, pos, FarDialogType);
}

TDialogData* CheckValidDialog(lua_State* L, int pos)
{
	TDialogData* dd = CheckDialog(L, pos);
	luaL_argcheck(L, dd->hDlg != INVALID_HANDLE_VALUE, pos, "closed dialog");
	return dd;
}

HANDLE CheckDialogHandle (lua_State* L, int pos)
{
	return CheckValidDialog(L, pos)->hDlg;
}

static int DialogHandleEqual(lua_State* L)
{
	TDialogData* dd1 = CheckDialog(L, 1);
	TDialogData* dd2 = CheckDialog(L, 2);
	lua_pushboolean(L, dd1->hDlg == dd2->hDlg);
	return 1;
}

static int Is_DM_DialogItem(int Msg)
{
	switch(Msg) {
		case DM_ADDHISTORY:
		case DM_EDITUNCHANGEDFLAG:
		case DM_ENABLE:
		case DM_GETCHECK:
		case DM_GETCOLOR:
		case DM_GETCOMBOBOXEVENT:
		case DM_GETCONSTTEXTPTR:
		case DM_GETCURSORPOS:
		case DM_GETCURSORSIZE:
		case DM_GETDLGITEM:
		case DM_GETEDITPOSITION:
		case DM_GETITEMDATA:
		case DM_GETITEMPOSITION:
		case DM_GETSELECTION:
		case DM_GETTEXT:
		case DM_GETTRUECOLOR:
		case DM_LISTADD:
		case DM_LISTADDSTR:
		case DM_LISTDELETE:
		case DM_LISTFINDSTRING:
		case DM_LISTGETCURPOS:
		case DM_LISTGETDATA:
		case DM_LISTGETDATASIZE:
		case DM_LISTGETITEM:
		case DM_LISTGETTITLES:
		case DM_LISTINFO:
		case DM_LISTINSERT:
		case DM_LISTSET:
		case DM_LISTSETCURPOS:
		case DM_LISTSETDATA:
		case DM_LISTSETMOUSEREACTION:
		case DM_LISTSETTITLES:
		case DM_LISTSORT:
		case DM_LISTUPDATE:
		case DM_SET3STATE:
		case DM_SETCHECK:
		case DM_SETCOLOR:
		case DM_SETCOMBOBOXEVENT:
		case DM_SETCURSORPOS:
		case DM_SETCURSORSIZE:
		case DM_SETDLGITEM:
		case DM_SETDROPDOWNOPENED:
		case DM_SETEDITPOSITION:
		case DM_SETFOCUS:
		case DM_SETHISTORY:
		case DM_SETITEMDATA:
		case DM_SETITEMPOSITION:
		case DM_SETMAXTEXTLENGTH:
		case DM_SETSELECTION:
		case DM_SETTEXT:
		case DM_SETTEXTPTR:
		case DM_SETTRUECOLOR:
		case DM_SHOWITEM:
		case DM_SETREADONLY:
			return 1;
	}
	return 0;
}

int PushDMParams (lua_State *L, int Msg, int Param1)
{
	if (! ((Msg>DM_FIRST && Msg<DN_FIRST) || Msg==DM_USER))
		return 0;

	// Msg
	lua_pushinteger(L, Msg);                             //+1

	// Param1
	if (Msg == DM_CLOSE)
		lua_pushinteger(L, Param1<=0 ? Param1 : Param1+1); //+2
	else if (Is_DM_DialogItem(Msg))
		lua_pushinteger(L, Param1+1);                      //+2
	else
		lua_pushinteger(L, Param1);                        //+2

	return 1;
}

LONG_PTR GetEnableFromLua (lua_State *L, int pos)
{
	LONG_PTR ret;
	if (lua_isnoneornil(L,pos)) //get state
		ret = -1;
	else if (lua_isnumber(L,pos))
		ret = lua_tointeger(L, pos);
	else
		ret = lua_toboolean(L, pos);
	return ret;
}

static void SetColorForeAndBack(lua_State *L, const struct FarTrueColorForeAndBack *fb, const char *name)
{
	lua_createtable(L,0,2);
	PutIntToTable(L, "ForegroundColor", FarTrueColorToRGB(&fb->Fore));
	PutIntToTable(L, "BackgroundColor", FarTrueColorToRGB(&fb->Back));
	lua_setfield(L, -2, name);
}

DWORD GetColorFromTable(lua_State *L, const char *field, int index)
{
	DWORD val;
	lua_getfield(L, -1, field);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_pushinteger(L, index);
		lua_gettable(L, -2);
	}
	val = (DWORD) lua_tointeger(L, -1);
	lua_pop(L, 1);
	return val;
}

static void FillColor(lua_State *L, struct FarTrueColorForeAndBack *fb)
{
	if (lua_istable(L, -1)) {
		DWORD val = GetColorFromTable(L, "ForegroundColor", 1);
		FarTrueColorFromRGB(&fb->Fore, val, 1);
		val = GetColorFromTable(L, "BackgroundColor", 2);
		FarTrueColorFromRGB(&fb->Back, val, 1);
	}
}

static void FillColorForeAndBack(lua_State *L, struct FarTrueColorForeAndBack *fb, const char *Name)
{
	lua_getfield(L, -1, Name);
	FillColor(L, fb);
	lua_pop(L,1);
}

static int DoSendDlgMessage (lua_State *L, int Msg, int delta)
{
	typedef struct { void *Id; int Ref; } listdata_t;
	TPluginData *pluginData = GetPluginData(L);
	int Param1=0, res=0, res_incr=0;
	LONG_PTR Param2=0;
	wchar_t buf[512];
	int pos2 = 2-delta, pos3 = 3-delta, pos4 = 4-delta;
	//---------------------------------------------------------------------------
	COORD coord;
	SMALL_RECT small_rect;
	//---------------------------------------------------------------------------
	lua_settop(L, pos4); //many cases below rely on top==pos4
	HANDLE hDlg = CheckDialogHandle(L, 1);
	if (delta == 0)
		Msg = check_env_flag (L, 2);

	// Param1
	switch(Msg)
	{
		case DM_CLOSE:
			Param1 = luaL_optinteger(L,pos3,-1);
			if (Param1>0) --Param1;
			break;

		case DM_ENABLEREDRAW:
			Param1 = GetEnableFromLua(L,pos3);
			break;

		case DM_SETDLGDATA:
			break;

		default:
			Param1 = Is_DM_DialogItem(Msg) ? luaL_optinteger(L,pos3,1)-1 : luaL_optinteger(L,pos3,0);
			break;
	}

	// res_incr
	switch(Msg)
	{
		case DM_GETFOCUS:
		case DM_LISTADDSTR:
			res_incr=1;
			break;
	}

	//Param2 and the rest
	switch(Msg)
	{
		default:
			luaL_argerror(L, pos2, "operation not implemented");
			break;

		case DM_CLOSE:
		case DM_EDITUNCHANGEDFLAG:
		case DM_GETCOMBOBOXEVENT:
		case DM_GETCURSORSIZE:
		case DM_GETDROPDOWNOPENED:
		case DM_GETFOCUS:
		case DM_GETITEMDATA:
		case DM_LISTSORT:
		case DM_REDRAW:               // alias: DM_SETREDRAW
		case DM_SET3STATE:
		case DM_SETCURSORSIZE:
		case DM_SETDROPDOWNOPENED:
		case DM_SETFOCUS:
		case DM_SETITEMDATA:
		case DM_SETMAXTEXTLENGTH:     // alias: DM_SETTEXTLENGTH
		case DM_SETMOUSEEVENTNOTIFY:
		case DM_SHOWDIALOG:
		case DM_SHOWITEM:
		case DM_USER:
			Param2 = luaL_optlong(L, pos4, 0);
			break;

		case DM_ENABLEREDRAW:
			break;

		case DM_ENABLE:
			Param2 = GetEnableFromLua(L, pos4);
			lua_pushboolean(L, PSInfo.SendDlgMessage(hDlg, Msg, Param1, Param2));
			return 1;

		case DM_GETCHECK:
			lua_pushinteger(L, PSInfo.SendDlgMessage(hDlg, Msg, Param1, 0));
			return 1;

		case DM_SETCHECK:
			if (lua_isnumber(L,pos4))
				Param2 = lua_tointeger(L,pos4);
			else
				Param2 = lua_toboolean(L,pos4) ? BSTATE_CHECKED : BSTATE_UNCHECKED;
			break;

		case DM_GETCOLOR:
		{
			DWORD dword;
			SendDlgMessage(hDlg, Msg, Param1, &dword);
			lua_pushinteger (L, dword & DIF_COLORMASK);
			return 1;
		}

		case DM_SETCOLOR:
			Param2 = luaL_checkinteger(L, pos4) | DIF_SETCOLOR;
			break;

		case DM_GETTRUECOLOR:
		{
			struct DialogItemTrueColors ditc;
			SendDlgMessage(hDlg, Msg, Param1, &ditc);
			lua_createtable(L, 0, 3);
			SetColorForeAndBack(L, &ditc.Normal,    "Normal");
			SetColorForeAndBack(L, &ditc.Hilighted, "Hilighted");
			SetColorForeAndBack(L, &ditc.Frame,     "Frame");
			return 1;
		}

		case DM_SETTRUECOLOR:
		{
			struct DialogItemTrueColors ditc;
			memset(&ditc, 0, sizeof(ditc));
			if (lua_istable(L, pos4)) {
				lua_pushvalue(L, pos4);
				FillColorForeAndBack(L, &ditc.Normal,    "Normal");
				FillColorForeAndBack(L, &ditc.Hilighted, "Hilighted");
				FillColorForeAndBack(L, &ditc.Frame,     "Frame");
				lua_pop(L,1);
			}
			lua_pushinteger (L, SendDlgMessage(hDlg, Msg, Param1, &ditc));
			return 1;
		}

		case DM_LISTADDSTR:
		case DM_ADDHISTORY:
		case DM_SETTEXTPTR:
			Param2 = (LONG_PTR) check_utf8_string(L, pos4, NULL);
			break;

		case DM_SETHISTORY:
			Param2 = (LONG_PTR) opt_utf8_string(L, pos4, NULL);
			break;

		case DM_LISTSETMOUSEREACTION:
			Param2 = GetFlags (L, pos4, NULL);
			break;

		case DM_GETCURSORPOS:
			if (SendDlgMessage(hDlg, Msg, Param1, &coord)) {
				lua_createtable(L,0,2);
				PutNumToTable(L, "X", coord.X);
				PutNumToTable(L, "Y", coord.Y);
				return 1;
			}
			return lua_pushnil(L), 1;

		case DM_GETDIALOGINFO:
		{
			struct DialogInfo dlg_info;
			dlg_info.StructSize = sizeof(dlg_info);
			if (SendDlgMessage(hDlg, Msg, Param1, &dlg_info))
			{
				lua_createtable(L,0,2);
				PutLStrToTable(L, "Id", &dlg_info.Id, 16);
				PutNumToTable(L, "Owner", dlg_info.Owner);
				return 1;
			}
			return lua_pushnil(L), 1;
		}

		case DM_GETDLGDATA: {
			TDialogData *dd = (TDialogData*) PSInfo.SendDlgMessage(hDlg,DM_GETDLGDATA,0,0);
			lua_rawgeti(L, LUA_REGISTRYINDEX, dd->dataRef);
			return 1;
		}

		case DM_SETDLGDATA: {
			TDialogData *dd = (TDialogData*) PSInfo.SendDlgMessage(hDlg,DM_GETDLGDATA,0,0);
			lua_settop(L, pos3);
			lua_rawseti(L, LUA_REGISTRYINDEX, dd->dataRef);
			return 0;
		}

		case DM_GETDLGRECT:
		case DM_GETITEMPOSITION:
			if (SendDlgMessage(hDlg, Msg, Param1, &small_rect)) {
				lua_createtable(L,0,4);
				PutNumToTable(L, "Left", small_rect.Left);
				PutNumToTable(L, "Top", small_rect.Top);
				PutNumToTable(L, "Right", small_rect.Right);
				PutNumToTable(L, "Bottom", small_rect.Bottom);
				return 1;
			}
			return lua_pushnil(L), 1;

		case DM_GETEDITPOSITION:
		{
			struct EditorSetPosition esp;

			if (SendDlgMessage(hDlg, Msg, Param1, &esp))
				return PushEditorSetPosition(L, &esp), 1;

			return lua_pushnil(L), 1;
		}

		case DM_GETSELECTION:
		{
			struct EditorSelect es;

			if (SendDlgMessage(hDlg, Msg, Param1, &es))
			{
				lua_createtable(L,0,5);
				PutNumToTable(L, "BlockType", (double) es.BlockType);
				PutNumToTable(L, "BlockStartLine", (double) es.BlockStartLine+1);
				PutNumToTable(L, "BlockStartPos", (double) es.BlockStartPos+1);
				PutNumToTable(L, "BlockWidth", (double) es.BlockWidth);
				PutNumToTable(L, "BlockHeight", (double) es.BlockHeight);
				return 1;
			}
			return lua_pushnil(L), 1;
		}

		case DM_SETSELECTION:
		{
			struct EditorSelect es;
			luaL_checktype(L, pos4, LUA_TTABLE);

			if(FillEditorSelect(L, pos4, &es))
				lua_pushinteger(L, SendDlgMessage(hDlg, Msg, Param1, &es));
			else
				lua_pushinteger(L,0);

			return 1;
		}

		case DM_GETTEXT:
		{
			struct FarDialogItemData fdid;
			size_t size;
			fdid.PtrLength = (size_t) PSInfo.SendDlgMessage(hDlg, Msg, Param1, 0);
			fdid.PtrData = (wchar_t*) malloc((fdid.PtrLength+1) * sizeof(wchar_t));
			size = SendDlgMessage(hDlg, Msg, Param1, &fdid);
			push_utf8_string(L, size ? fdid.PtrData : L"", size);
			free(fdid.PtrData);
			return 1;
		}

		case DM_GETCONSTTEXTPTR: {
			const wchar_t *ptr = (wchar_t*)PSInfo.SendDlgMessage(hDlg, Msg, Param1, 0);
			push_utf8_string(L, ptr ? ptr:L"", -1);
			return 1;
		}

		case DM_SETTEXT:
		{
			struct FarDialogItemData fdid;
			fdid.PtrLength = 0;
			fdid.PtrData = check_utf8_string(L, pos4, &fdid.PtrLength);
			lua_pushinteger(L, SendDlgMessage(hDlg, Msg, Param1, &fdid));
			return 1;
		}

		case DM_KEY: {
			luaL_checktype(L, pos4, LUA_TTABLE);
			res = lua_objlen(L, pos4);
			if (res) {
				DWORD* arr = (DWORD*)lua_newuserdata(L, res * sizeof(DWORD));
				int i;
				for(i=0; i<res; i++) {
					lua_pushinteger(L,i+1);
					lua_gettable(L,pos4);
					arr[i] = lua_tointeger(L,-1);
					lua_pop(L,1);
				}
				res = PSInfo.SendDlgMessage (hDlg, Msg, res, (LONG_PTR)arr);
			}
			return lua_pushinteger(L, res), 1;
		}

		case DM_LISTADD:
		case DM_LISTSET: {
			luaL_checktype(L, pos4, LUA_TTABLE);
			lua_createtable(L, 1, 0); // "history table"
			lua_insert(L, pos4);
			struct FarList *list = CreateList(L, pos4);
			Param2 = (LONG_PTR)list;
			break;
		}

		case DM_LISTDELETE:
		{
			struct FarListDelete fld;
			if (lua_isnoneornil(L, pos4))
				lua_pushinteger(L, PSInfo.SendDlgMessage(hDlg, Msg, Param1, 0));
			else
			{
				luaL_checktype(L, pos4, LUA_TTABLE);
				fld.StartIndex = GetOptIntFromTable(L, "StartIndex", 1) - 1;
				fld.Count = GetOptIntFromTable(L, "Count", 1);
				lua_pushinteger(L, SendDlgMessage(hDlg, Msg, Param1, &fld));
			}
			return 1;
		}

		case DM_LISTFINDSTRING:
		{
			struct FarListFind flf;
			luaL_checktype(L, pos4, LUA_TTABLE);
			flf.StartIndex = GetOptIntFromTable(L, "StartIndex", 1) - 1;
			lua_getfield(L, pos4, "Pattern");
			flf.Pattern = check_utf8_string(L, -1, NULL);
			lua_getfield(L, pos4, "Flags");
			flf.Flags = GetFlags(L, -1, NULL);
			res = SendDlgMessage(hDlg, Msg, Param1, &flf);
			res < 0 ? lua_pushnil(L) : lua_pushinteger(L, res+1);
			return 1;
		}

		case DM_LISTGETCURPOS:
		{
			struct FarListPos flp;
			SendDlgMessage(hDlg, Msg, Param1, &flp);
			lua_createtable(L,0,2);
			PutIntToTable(L, "SelectPos", flp.SelectPos+1);
			PutIntToTable(L, "TopPos", flp.TopPos+1);
			return 1;
		}

		case DM_LISTGETITEM:
		{
			struct FarListGetItem flgi;
			flgi.ItemIndex = luaL_checkinteger(L, pos4) - 1;
			if (SendDlgMessage(hDlg, Msg, Param1, &flgi))
			{
				lua_createtable(L,0,2);
				PutIntToTable(L, "Flags", flgi.Item.Flags);
				PutWStrToTable(L, "Text", flgi.Item.Text, -1);
				return 1;
			}
			return lua_pushnil(L), 1;
		}

		case DM_LISTGETTITLES:
		{
			struct FarListTitles flt;
			flt.Title = buf;
			flt.Bottom = buf + ARRAYSIZE(buf)/2;
			flt.TitleLen = ARRAYSIZE(buf)/2;
			flt.BottomLen = ARRAYSIZE(buf)/2;
			if (SendDlgMessage(hDlg, Msg, Param1, &flt))
			{
				lua_createtable(L,0,2);
				PutWStrToTable(L, "Title", flt.Title, -1);
				PutWStrToTable(L, "Bottom", flt.Bottom, -1);
				return 1;
			}

			return lua_pushnil(L), 1;
		}

		case DM_LISTSETTITLES:
		{
			struct FarListTitles flt;
			luaL_checktype(L, pos4, LUA_TTABLE);
			lua_getfield(L, pos4, "Title");
			flt.Title = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
			lua_getfield(L, pos4, "Bottom");
			flt.Bottom = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
			lua_pushinteger(L, SendDlgMessage(hDlg, Msg, Param1, &flt));
			return 1;
		}

		case DM_LISTINFO:
		{
			struct FarListInfo fli;
			if (SendDlgMessage(hDlg, Msg, Param1, &fli))
			{
				lua_createtable(L,0,6);
				PutIntToTable(L, "Flags", fli.Flags);
				PutIntToTable(L, "ItemsNumber", fli.ItemsNumber);
				PutIntToTable(L, "SelectPos", fli.SelectPos+1);
				PutIntToTable(L, "TopPos", fli.TopPos+1);
				PutIntToTable(L, "MaxHeight", fli.MaxHeight);
				PutIntToTable(L, "MaxLength", fli.MaxLength);
				return 1;
			}
			return lua_pushnil(L), 1;
		}

		case DM_LISTINSERT:
		{
			struct FarListInsert flins;
			luaL_checktype(L, pos4, LUA_TTABLE);
			flins.Index = GetOptIntFromTable(L, "Index", 1) - 1;
			lua_getfield(L, pos4, "Text");
			flins.Item.Text = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
			lua_getfield(L, pos4, "Flags"); //+1
			flins.Item.Flags = CheckFlags(L, -1);
			res = SendDlgMessage(hDlg, Msg, Param1, &flins);
			res < 0 ? lua_pushnil(L) : lua_pushinteger(L, res);
			return 1;
		}

		case DM_LISTUPDATE:
		{
			struct FarListUpdate flu;
			luaL_checktype(L, pos4, LUA_TTABLE);
			flu.Index = GetOptIntFromTable(L, "Index", 1) - 1;
			lua_getfield(L, pos4, "Text");
			flu.Item.Text = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
			lua_getfield(L, pos4, "Flags"); //+1
			flu.Item.Flags = CheckFlags(L, -1);
			lua_pushboolean(L, SendDlgMessage(hDlg, Msg, Param1, &flu) != 0);
			return 1;
		}

		case DM_LISTSETCURPOS:
		{
			struct FarListPos flp;
			luaL_checktype(L, pos4, LUA_TTABLE);
			flp.SelectPos = GetOptIntFromTable(L, "SelectPos", 1) - 1;
			flp.TopPos = GetOptIntFromTable(L, "TopPos", 1) - 1;
			lua_pushinteger(L, 1 + SendDlgMessage(hDlg, Msg, Param1, &flp));
			return 1;
		}

		case DM_LISTGETDATASIZE:
			Param2 = luaL_checkinteger(L, pos4) - 1;
			break;

		case DM_LISTSETDATA:
		{
			struct FarListItemData flid;
			listdata_t Data, *oldData;
			int Index;
			luaL_checktype(L, pos4, LUA_TTABLE);
			Index = GetOptIntFromTable(L, "Index", 1) - 1;
			lua_getfenv(L, 1);
			lua_getfield(L, pos4, "Data");
			if (lua_isnil(L,-1)) { // nil is not allowed
				lua_pushinteger(L,0);
				return 1;
			}
			oldData = (listdata_t*)PSInfo.SendDlgMessage(hDlg, DM_LISTGETDATA, Param1, Index);
			if (oldData &&
				sizeof(listdata_t) == PSInfo.SendDlgMessage(hDlg, DM_LISTGETDATASIZE, Param1, Index) &&
				oldData->Id == pluginData)
			{
				luaL_unref(L, -2, oldData->Ref);
			}
			Data.Id = pluginData;
			Data.Ref = luaL_ref(L, -2);
			flid.Index = Index;
			flid.Data = &Data;
			flid.DataSize = sizeof(Data);
			lua_pushinteger(L, SendDlgMessage(hDlg, Msg, Param1, &flid));
			return 1;
		}

		case DM_LISTGETDATA: {
			int Index = (int)luaL_checkinteger(L, pos4) - 1;
			listdata_t *Data = (listdata_t*)PSInfo.SendDlgMessage(hDlg, DM_LISTGETDATA, Param1, Index);
			if (Data) {
				if (sizeof(listdata_t) == PSInfo.SendDlgMessage(hDlg, DM_LISTGETDATASIZE, Param1, Index) &&
					Data->Id == pluginData)
				{
					lua_getfenv(L, 1);
					lua_rawgeti(L, -1, Data->Ref);
				}
				else
					lua_pushlightuserdata(L, Data);
			}
			else
				lua_pushnil(L);
			return 1;
		}

		case DM_GETDLGITEM:
			PushDlgItemNum(L, hDlg, Param1, pos4);
			return 1;

		case DM_SETDLGITEM:
			return SetDlgItem(L, hDlg, Param1, pos4);

		case DM_MOVEDIALOG:
		case DM_RESIZEDIALOG:
		case DM_SETCURSORPOS:
		{
			COORD* c;
			luaL_checktype(L, pos4, LUA_TTABLE);
			coord.X = GetOptIntFromTable(L, "X", 0);
			coord.Y = GetOptIntFromTable(L, "Y", 0);

			if(Msg == DM_SETCURSORPOS)
			{
				lua_pushinteger(L, SendDlgMessage(hDlg, Msg, Param1, &coord));
				return 1;
			}

			c = (COORD*) SendDlgMessage(hDlg, Msg, Param1, &coord);
			lua_createtable(L, 0, 2);
			PutIntToTable(L, "X", c->X);
			PutIntToTable(L, "Y", c->Y);
			return 1;
		}

		case DM_SETITEMPOSITION:
			luaL_checktype(L, pos4, LUA_TTABLE);
			small_rect.Left = GetOptIntFromTable(L, "Left", 0);
			small_rect.Top = GetOptIntFromTable(L, "Top", 0);
			small_rect.Right = GetOptIntFromTable(L, "Right", 0);
			small_rect.Bottom = GetOptIntFromTable(L, "Bottom", 0);
			Param2 = (LONG_PTR)&small_rect;
			break;

		case DM_SETCOMBOBOXEVENT:
			Param2 = CheckFlags(L, pos4);
			break;

		case DM_SETEDITPOSITION:
		{
			struct EditorSetPosition esp;
			luaL_checktype(L, pos4, LUA_TTABLE);
			lua_settop(L, pos4);
			FillEditorSetPosition(L, &esp);
			lua_pushinteger(L, SendDlgMessage(hDlg, Msg, Param1, &esp));
			return 1;
		}

		case DM_SETREADONLY:
			Param2 = lua_isnumber(L, pos4) ? lua_tointeger(L, pos4) : lua_toboolean(L, pos4);
			break;
	}
	res = PSInfo.SendDlgMessage (hDlg, Msg, Param1, Param2);
	lua_pushinteger (L, res + res_incr);
	return 1;
}

#define DlgMethod(name,msg,delta) \
static int dlg_##name(lua_State *L) { return DoSendDlgMessage(L,msg,delta); }

static int far_SendDlgMessage(lua_State *L) { return DoSendDlgMessage(L,0,0); }

DlgMethod( AddHistory,             DM_ADDHISTORY, 1)
DlgMethod( Close,                  DM_CLOSE, 1)
DlgMethod( EditUnchangedFlag,      DM_EDITUNCHANGEDFLAG, 1)
DlgMethod( Enable,                 DM_ENABLE, 1)
DlgMethod( EnableRedraw,           DM_ENABLEREDRAW, 1)
DlgMethod( First,                  DM_FIRST, 1)
DlgMethod( GetCheck,               DM_GETCHECK, 1)
DlgMethod( GetColor,               DM_GETCOLOR, 1)
DlgMethod( GetComboboxEvent,       DM_GETCOMBOBOXEVENT, 1)
DlgMethod( GetConstTextPtr,        DM_GETCONSTTEXTPTR, 1)
DlgMethod( GetCursorPos,           DM_GETCURSORPOS, 1)
DlgMethod( GetCursorSize,          DM_GETCURSORSIZE, 1)
DlgMethod( GetDialogInfo,          DM_GETDIALOGINFO, 1)
DlgMethod( GetDlgData,             DM_GETDLGDATA, 1)
DlgMethod( GetDlgItem,             DM_GETDLGITEM, 1)
DlgMethod( GetDlgRect,             DM_GETDLGRECT, 1)
DlgMethod( GetDropdownOpened,      DM_GETDROPDOWNOPENED, 1)
DlgMethod( GetEditPosition,        DM_GETEDITPOSITION, 1)
DlgMethod( GetFocus,               DM_GETFOCUS, 1)
DlgMethod( GetItemData,            DM_GETITEMDATA, 1)
DlgMethod( GetItemPosition,        DM_GETITEMPOSITION, 1)
DlgMethod( GetSelection,           DM_GETSELECTION, 1)
DlgMethod( GetText,                DM_GETTEXT, 1)
DlgMethod( GetTrueColor,           DM_GETTRUECOLOR, 1)
DlgMethod( Key,                    DM_KEY, 1)
DlgMethod( ListAdd,                DM_LISTADD, 1)
DlgMethod( ListAddStr,             DM_LISTADDSTR, 1)
DlgMethod( ListDelete,             DM_LISTDELETE, 1)
DlgMethod( ListFindString,         DM_LISTFINDSTRING, 1)
DlgMethod( ListGetCurPos,          DM_LISTGETCURPOS, 1)
DlgMethod( ListGetData,            DM_LISTGETDATA, 1)
DlgMethod( ListGetDataSize,        DM_LISTGETDATASIZE, 1)
DlgMethod( ListGetItem,            DM_LISTGETITEM, 1)
DlgMethod( ListGetTitles,          DM_LISTGETTITLES, 1)
DlgMethod( ListInfo,               DM_LISTINFO, 1)
DlgMethod( ListInsert,             DM_LISTINSERT, 1)
DlgMethod( ListSet,                DM_LISTSET, 1)
DlgMethod( ListSetCurPos,          DM_LISTSETCURPOS, 1)
DlgMethod( ListSetData,            DM_LISTSETDATA, 1)
DlgMethod( ListSetMouseReaction,   DM_LISTSETMOUSEREACTION, 1)
DlgMethod( ListSetTitles,          DM_LISTSETTITLES, 1)
DlgMethod( ListSort,               DM_LISTSORT, 1)
DlgMethod( ListUpdate,             DM_LISTUPDATE, 1)
DlgMethod( MoveDialog,             DM_MOVEDIALOG, 1)
DlgMethod( Redraw,                 DM_REDRAW, 1)
DlgMethod( ResizeDialog,           DM_RESIZEDIALOG, 1)
DlgMethod( Set3State,              DM_SET3STATE, 1)
DlgMethod( SetCheck,               DM_SETCHECK, 1)
DlgMethod( SetColor,               DM_SETCOLOR, 1)
DlgMethod( SetComboboxEvent,       DM_SETCOMBOBOXEVENT, 1)
DlgMethod( SetCursorPos,           DM_SETCURSORPOS, 1)
DlgMethod( SetCursorSize,          DM_SETCURSORSIZE, 1)
DlgMethod( SetDlgData,             DM_SETDLGDATA, 1)
DlgMethod( SetDlgItem,             DM_SETDLGITEM, 1)
DlgMethod( SetDropdownOpened,      DM_SETDROPDOWNOPENED, 1)
DlgMethod( SetEditPosition,        DM_SETEDITPOSITION, 1)
DlgMethod( SetFocus,               DM_SETFOCUS, 1)
DlgMethod( SetHistory,             DM_SETHISTORY, 1)
DlgMethod( SetItemData,            DM_SETITEMDATA, 1)
DlgMethod( SetItemPosition,        DM_SETITEMPOSITION, 1)
DlgMethod( SetMaxTextLength,       DM_SETMAXTEXTLENGTH, 1)
DlgMethod( SetMouseEventNotify,    DM_SETMOUSEEVENTNOTIFY, 1)
DlgMethod( SetReadOnly,            DM_SETREADONLY, 1)
DlgMethod( SetSelection,           DM_SETSELECTION, 1)
DlgMethod( SetText,                DM_SETTEXT, 1)
DlgMethod( SetTextPtr,             DM_SETTEXTPTR, 1)
DlgMethod( SetTrueColor,           DM_SETTRUECOLOR, 1)
DlgMethod( ShowDialog,             DM_SHOWDIALOG, 1)
DlgMethod( ShowItem,               DM_SHOWITEM, 1)
DlgMethod( User,                   DM_USER, 1)

int PushDNParams (lua_State *L, int Msg, int Param1, LONG_PTR Param2)
{
	// Param1
	switch(Msg)
	{
		case DN_CTLCOLORDIALOG:
		case DN_DRAGGED:
		case DN_DRAWDIALOG:
		case DN_DRAWDIALOGDONE:
		case DN_ENTERIDLE:
		case DN_GETDIALOGINFO:
		case DN_MOUSEEVENT:
		case DN_RESIZECONSOLE:
			break;

		case DN_CLOSE:
		case DN_MOUSECLICK:
		case DN_GOTFOCUS:
		case DN_KILLFOCUS:

		case DN_BTNCLICK:
		case DN_CTLCOLORDLGITEM:
		case DN_CTLCOLORDLGLIST:
		case DN_DRAWDLGITEM:
		case DN_EDITCHANGE:
		case DN_HELP:
		case DN_HOTKEY:
		case DN_INITDIALOG:
		case DN_KEY:
		case DN_LISTCHANGE:
		case DN_LISTHOTKEY:
			if (Param1 >= 0)  // dialog element position
				++Param1;
			break;

		default:
			return FALSE;
	}

	lua_pushinteger(L, Msg);             //+1
	lua_pushinteger(L, Param1);          //+2

	// Param2
	switch(Msg)
	{
		case DN_DRAWDLGITEM:
		case DN_EDITCHANGE:
			PushDlgItem(L, (struct FarDialogItem*)Param2, FALSE);
			break;

		case DN_HELP:
			push_utf8_string(L, Param2 ? (wchar_t*)Param2 : L"", -1);
			break;

		case DN_GETDIALOGINFO: {
			struct DialogInfo* di = (struct DialogInfo*) Param2;
			lua_pushlstring(L, (const char*) &di->Id, 16);
			break;
		}

		case DN_LISTCHANGE:
		case DN_LISTHOTKEY:
			lua_pushinteger(L, Param2+1);  // make list positions 1-based
			break;

		case DN_MOUSECLICK:
		case DN_MOUSEEVENT:
			PutMouseEvent(L, (const MOUSE_EVENT_RECORD*)Param2, FALSE);
			break;

		case DN_RESIZECONSOLE:
		{
			COORD* coord = (COORD*)Param2;
			lua_createtable(L, 0, 2);
			PutIntToTable(L, "X", coord->X);
			PutIntToTable(L, "Y", coord->Y);
			break;
		}

		case DN_CTLCOLORDLGITEM: {
			int i;
			lua_createtable(L, 4, 0);
			for(i=0; i < 4; i++) {
				lua_pushinteger(L, (Param2 >> i*8) & 0xFF);
				lua_rawseti(L, -2, i+1);
			}
			break;
		}

		case DN_CTLCOLORDLGLIST: {
			int i;
			struct FarListColors* flc = (struct FarListColors*) Param2;
			lua_createtable(L, flc->ColorCount, 1);
			PutIntToTable(L, "Flags", flc->Flags);
			for (i=0; i < flc->ColorCount; i++)
				PutIntToArray(L, i+1, flc->Colors[i]);
			break;
		}

		default:
			lua_pushinteger(L, Param2);  //+3
			break;
	}

	return TRUE;
}

int ProcessDNResult(lua_State *L, int Msg, LONG_PTR Param2)
{
	int ret = 0, i;
	switch(Msg)
	{
		case DN_CTLCOLORDLGLIST:
			ret = lua_istable(L,-1);
			if (ret) {
				struct FarListColors* flc = (struct FarListColors*) Param2;
				for (i=0; i < flc->ColorCount; i++)
					flc->Colors[i] = GetIntFromArray(L, i+1);
			}
			break;

		case DN_CTLCOLORDLGITEM:
			ret = Param2;
			if (lua_istable(L,-1))
			{
				ret = 0;
				for(i = 0; i < 4; i++)
				{
					lua_rawgeti(L, -1, i+1);
					ret |= (lua_tointeger(L,-1) & 0xFF) << i*8;
					lua_pop(L, 1);
				}
			}
			break;

		case DN_CTLCOLORDIALOG:
			if(lua_isnumber(L, -1))
				ret = lua_tointeger(L, -1);
			break;

		case DN_HELP:
			ret = (utf8_to_wcstring(L, -1, NULL) != NULL);
			if(ret)
			{
				lua_getfield(L, LUA_REGISTRYINDEX, FAR_DN_STORAGE);
				lua_pushvalue(L, -2);                // keep stack balanced
				lua_setfield(L, -2, "helpstring");   // protect from garbage collector
				lua_pop(L, 1);
			}
			break;

		case DN_KILLFOCUS:
			ret = lua_tointeger(L, -1) - 1;
			break;

		default:
			ret = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : lua_toboolean(L, -1);
			break;
	}
	return ret;
}

static int DN_ConvertParam1(int Msg, int Param1)
{
	switch(Msg) {
		default:
			return Param1;

		case DN_BTNCLICK:
		case DN_CTLCOLORDLGITEM:
		case DN_CTLCOLORDLGLIST:
		case DN_DRAWDLGITEM:
		case DN_EDITCHANGE:
		case DN_HELP:
		case DN_HOTKEY:
		case DN_INITDIALOG:
		case DN_KEY:
		case DN_LISTCHANGE:
		case DN_LISTHOTKEY:
			return Param1 + 1;

		case DN_GOTFOCUS:
		case DN_KILLFOCUS:
		case DN_CLOSE:
		case DN_MOUSECLICK:
			return Param1 < 0 ? Param1 : Param1 + 1;
	}
}

static void RemoveDialogFromRegistry(TDialogData *dd)
{
	luaL_unref(dd->L, LUA_REGISTRYINDEX, dd->dataRef);
	dd->hDlg = INVALID_HANDLE_VALUE;
	lua_pushlightuserdata(dd->L, dd);
	lua_pushnil(dd->L);
	lua_rawset(dd->L, LUA_REGISTRYINDEX);
}

BOOL NonModal(TDialogData *dd)
{
	return dd && !dd->isModal;
}

LONG_PTR LF_DlgProc(lua_State *L, HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	TDialogData *dd = (TDialogData*) PSInfo.SendDlgMessage(hDlg,DM_GETDLGDATA,0,0);
	if (dd->wasError)
		return PSInfo.DefDlgProc(hDlg, Msg, Param1, Param2);

	if (Msg == DN_GETDIALOGINFO)
		 return FALSE;

	L = dd->L; // the dialog may be called from a lua_State other than the main one
	int Param1_mod = DN_ConvertParam1(Msg, Param1);

	lua_pushlightuserdata (L, dd);       //+1   retrieve the table
	lua_rawget (L, LUA_REGISTRYINDEX);   //+1
	lua_rawgeti(L, -1, 2);               //+2   retrieve the procedure
	lua_rawgeti(L, -2, 3);               //+3   retrieve the handle
	lua_pushinteger (L, Msg);            //+4
	lua_pushinteger (L, Param1_mod);     //+5

	switch(Msg) {
		case DN_INITDIALOG:
			lua_rawgeti(L, LUA_REGISTRYINDEX, dd->dataRef);
			if (NonModal(dd))
				dd->hDlg = hDlg;
			break;

		case DN_CTLCOLORDLGITEM:
		case DN_CTLCOLORDLGLIST:
		case DN_DRAWDLGITEM:
		case DN_EDITCHANGE:
		case DN_HELP:
		case DN_LISTCHANGE:
		case DN_LISTHOTKEY:
		case DN_MOUSECLICK:
		case DN_MOUSEEVENT:
		case DN_RESIZECONSOLE:
			lua_pop(L,2);
			PushDNParams(L, Msg, Param1, Param2);
			break;

		default:
			lua_pushinteger (L, Param2); //+6
			break;
	}

	//---------------------------------------------------------------------------
	LONG_PTR ret = pcall_msg (L, 4, 1); //+2
	if (ret) {
		dd->wasError = TRUE;
		PSInfo.SendDlgMessage(hDlg, DM_CLOSE, -1, 0);
		return PSInfo.DefDlgProc(hDlg, Msg, Param1, Param2);
	}
	//---------------------------------------------------------------------------

	if (lua_isnil(L, -1)) {
		lua_pop(L, 2);
		return PSInfo.DefDlgProc(hDlg, Msg, Param1, Param2);
	}

	switch (Msg) {
		case DN_CTLCOLORDLGITEM:
		case DN_CTLCOLORDLGLIST:
		case DN_HELP:
			ret = ProcessDNResult(L, Msg, Param2);
			break;

		case DN_CLOSE:
			ret = lua_isnumber(L,-1) ? lua_tointeger(L,-1) : lua_toboolean(L,-1);
			if (ret && NonModal(dd))
			{
				PSInfo.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, 0);
				RemoveDialogFromRegistry(dd);
			}
			break;

		default:
			ret = lua_isnumber(L,-1) ? lua_tointeger(L,-1) : lua_toboolean(L,-1);
			break;
	}

	lua_pop (L, 2);
	return ret;
}

static int far_DialogInit(lua_State *L)
{
	enum { POS_HISTORIES=1, POS_ITEMS=2 };
	int ItemsNumber, i;
	struct FarDialogItem *Items;
	flags_t Flags;
	TDialogData *dd;
	FARAPIDEFDLGPROC Proc;
	LONG_PTR Param;
	TPluginData *pd = GetPluginData(L);
	GUID Id;
	int X1, Y1, X2, Y2;
	const wchar_t *HelpTopic;

	memset(&Id, 0, sizeof(Id));
	if (lua_type(L,1) == LUA_TSTRING) {
		if (lua_objlen(L,1) >= sizeof(GUID))
			Id = *(const GUID*)lua_tostring(L, 1);
	}
	else if (!lua_isnoneornil(L,1))
		return luaL_typerror(L, 1, "optional string");

	X1 = luaL_checkinteger(L, 2);
	Y1 = luaL_checkinteger(L, 3);
	X2 = luaL_checkinteger(L, 4);
	Y2 = luaL_checkinteger(L, 5);
	HelpTopic = opt_utf8_string(L, 6, NULL);

	luaL_checktype(L, 7, LUA_TTABLE);
	lua_newtable (L); // create a "histories" table, to prevent history strings
										// from being garbage collected too early
	lua_replace (L, POS_HISTORIES);
	ItemsNumber = lua_objlen(L, 7);
	Items = (struct FarDialogItem*)lua_newuserdata (L, ItemsNumber * sizeof(struct FarDialogItem));
	lua_replace (L, POS_ITEMS);

	for(i=0; i < ItemsNumber; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, 7);
		if (lua_type(L, -1) == LUA_TTABLE) {
			SetFarDialogItem(L, Items+i, i, POS_HISTORIES);
			lua_pop(L, 1);
		}
		else
			return luaL_error(L, "Items[%d] is not a table", i+1);
	}

	// 8-th parameter (flags)
	Flags = OptFlags(L,8,0);
	dd = NewDialogData(L, INVALID_HANDLE_VALUE, TRUE);
	dd->isModal = (Flags&FDLG_NONMODAL) == 0;

	// 9-th parameter (DlgProc function)
	Proc = NULL;
	Param = 0;
	if (lua_isfunction(L, 9)) {
		Proc = pd->DlgProc;
		Param = (LONG_PTR)dd;
		if (lua_gettop(L) >= 10) {
			lua_pushvalue(L,10);
			dd->dataRef = luaL_ref(L, LUA_REGISTRYINDEX);
		}
	}

	// Put some values into the registry
	lua_pushlightuserdata(L, dd); // important: index it with dd
	lua_createtable(L, 3, 0);
	lua_pushvalue(L, POS_HISTORIES); // store the "histories" table
	lua_rawseti(L, -2, 1);

	if(lua_isfunction(L, 9))
	{
		lua_pushvalue(L, 9);    // store the procedure
		lua_rawseti(L, -2, 2);
		lua_pushvalue(L, -3);   // store the handle
		lua_rawseti(L, -2, 3);
	}

	lua_rawset (L, LUA_REGISTRYINDEX);

	dd->hDlg = PSInfo.DialogInitV3(pd->ModuleNumber, &Id, X1, Y1, X2, Y2, HelpTopic,
																 Items, ItemsNumber, 0, Flags, Proc, Param);

	if (dd->hDlg == INVALID_HANDLE_VALUE) {
		RemoveDialogFromRegistry(dd);
		lua_pushnil(L);
	}
	return 1;
}

static void free_dialog (TDialogData* dd)
{
	if (dd->isOwned && dd->isModal && dd->hDlg != INVALID_HANDLE_VALUE) {
		PSInfo.DialogFree(dd->hDlg);
		RemoveDialogFromRegistry(dd);
	}
}

static int far_DialogRun (lua_State *L)
{
	TDialogData* dd = CheckValidDialog(L, 1);
	int result = PSInfo.DialogRun(dd->hDlg);
	if (result >= 0) ++result;

	if (dd->wasError) {
		free_dialog(dd);
		luaL_error(L, "error occured in dialog procedure");
	}
	lua_pushinteger(L, result);
	return 1;
}

static int far_DialogFree (lua_State *L)
{
	free_dialog(CheckDialog(L, 1));
	return 0;
}

static int dialog_tostring (lua_State *L)
{
	TDialogData* dd = CheckDialog(L, 1);
	if (dd->hDlg != INVALID_HANDLE_VALUE)
		lua_pushfstring(L, "%s (%p)", FarDialogType, dd->hDlg);
	else
		lua_pushfstring(L, "%s (closed)", FarDialogType);
	return 1;
}

static int dialog_rawhandle(lua_State *L)
{
	TDialogData* dd = CheckDialog(L, 1);
	if(dd->hDlg != INVALID_HANDLE_VALUE)
		lua_pushlightuserdata(L, dd->hDlg);
	else
		lua_pushnil(L);
	return 1;
}

static int far_GetDlgItem(lua_State *L)
{
	HANDLE hDlg = CheckDialogHandle(L,1);
	int numitem = (int)luaL_checkinteger(L,2) - 1;
	PushDlgItemNum(L, hDlg, numitem, 3);
	return 1;
}

static int far_SetDlgItem(lua_State *L)
{
	HANDLE hDlg = CheckDialogHandle(L,1);
	int numitem = (int)luaL_checkinteger(L,2) - 1;
	return SetDlgItem(L, hDlg, numitem, 3);
}

static int far_SubscribeDialogDrawEvents(lua_State *L)
{
	GetPluginData(L)->Flags |= PDF_DIALOGEVENTDRAWENABLE;
	return 0;
}

static int editor_Editor(lua_State *L)
{
	const wchar_t* FileName = check_utf8_string(L, 1, NULL);
	const wchar_t* Title    = opt_utf8_string(L, 2, NULL);
	int X1 = luaL_optinteger(L, 3, 0);
	int Y1 = luaL_optinteger(L, 4, 0);
	int X2 = luaL_optinteger(L, 5, -1);
	int Y2 = luaL_optinteger(L, 6, -1);
	int Flags = CheckFlags(L,7);
	int StartLine = luaL_optinteger(L, 8, -1);
	int StartChar = luaL_optinteger(L, 9, -1);
	int CodePage  = luaL_optinteger(L, 10, CP_AUTODETECT);
	int ret = PSInfo.Editor(FileName, Title, X1, Y1, X2, Y2, Flags, StartLine, StartChar, CodePage);
	lua_pushinteger(L, ret);
	return 1;
}

static int viewer_Viewer(lua_State *L)
{
	const wchar_t* FileName = check_utf8_string(L, 1, NULL);
	const wchar_t* Title    = opt_utf8_string(L, 2, NULL);
	int X1 = luaL_optinteger(L, 3, 0);
	int Y1 = luaL_optinteger(L, 4, 0);
	int X2 = luaL_optinteger(L, 5, -1);
	int Y2 = luaL_optinteger(L, 6, -1);
	int Flags = CheckFlags(L, 7);
	int CodePage = luaL_optinteger(L, 8, CP_AUTODETECT);
	int ret = PSInfo.Viewer(FileName, Title, X1, Y1, X2, Y2, Flags, CodePage);
	lua_pushboolean(L, ret);
	return 1;
}

static int viewer_GetFileName(lua_State *L)
{
	int viewerId = luaL_optinteger(L,1,-1);
	struct ViewerInfo vi;
	vi.StructSize = sizeof(vi);
	if (PSInfo.ViewerControlV2(viewerId, VCTL_GETINFO, &vi))
		push_utf8_string(L, vi.FileName, -1);
	else
		lua_pushnil(L);
	return 1;
}

static int viewer_GetInfo(lua_State *L)
{
	int viewerId = luaL_optinteger(L,1,-1);
	struct ViewerInfo vi;
	vi.StructSize = sizeof(vi);
	if (PSInfo.ViewerControlV2(viewerId, VCTL_GETINFO, &vi)) {
		lua_createtable(L, 0, 10);
		PutNumToTable(L,  "ViewerID",    vi.ViewerID);
		PutWStrToTable(L, "FileName",    vi.FileName, -1);
		PutNumToTable(L,  "FileSize",    vi.FileSize);
		PutNumToTable(L,  "FilePos",     vi.FilePos);
		PutNumToTable(L,  "WindowSizeX", vi.WindowSizeX);
		PutNumToTable(L,  "WindowSizeY", vi.WindowSizeY);
		PutNumToTable(L,  "Options",     vi.Options);
		PutNumToTable(L,  "TabSize",     vi.TabSize);
		PutNumToTable(L,  "LeftPos",     vi.LeftPos + 1);
		lua_createtable(L, 0, 4);
		PutNumToTable (L, "CodePage",    vi.CurMode.CodePage);
		PutBoolToTable(L, "Wrap",        vi.CurMode.Wrap);
		PutNumToTable (L, "WordWrap",    vi.CurMode.WordWrap);
		PutBoolToTable(L, "Hex",         vi.CurMode.Hex);
		PutBoolToTable(L, "Processed",   vi.CurMode.Processed);
		lua_setfield(L, -2, "CurMode");
	}
	else
		lua_pushnil(L);
	return 1;
}

static int viewer_Quit(lua_State *L)
{
	int viewerId = luaL_optinteger(L,1,-1);
	PSInfo.ViewerControlV2(viewerId, VCTL_QUIT, NULL);
	return 0;
}

static int viewer_Redraw(lua_State *L)
{
	int viewerId = luaL_optinteger(L,1,-1);
	PSInfo.ViewerControlV2(viewerId, VCTL_REDRAW, NULL);
	return 0;
}

static int viewer_Select(lua_State *L)
{
	int viewerId = luaL_optinteger(L,1,-1);
	struct ViewerSelect vs;
	vs.BlockStartPos = (long long int)luaL_checknumber(L,2);
	vs.BlockLen = luaL_checkinteger(L,3);
	lua_pushboolean(L, PSInfo.ViewerControlV2(viewerId, VCTL_SELECT, &vs));
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
	int ok;
	int viewerId = luaL_optinteger(L,1,-1);
	struct ViewerSetMode vsm;
	memset(&vsm, 0, sizeof(struct ViewerSetMode));
	luaL_checktype(L, 2, LUA_TTABLE);

	lua_getfield(L, 2, "Type");
	vsm.Type = GetFlags (L, -1, &ok);
	if (!ok)
		return lua_pushboolean(L,0), 1;

	lua_getfield(L, 2, "iParam");
	if (lua_isnumber(L, -1))
		vsm.Param.iParam = lua_tointeger(L, -1);
	else
		return lua_pushboolean(L,0), 1;

	lua_getfield(L, 2, "Flags");
	vsm.Flags = GetFlags (L, -1, &ok);
	if (!ok)
		return lua_pushboolean(L,0), 1;

	lua_pushboolean(L, PSInfo.ViewerControlV2(viewerId, VCTL_SETMODE, &vsm));
	return 1;
}

static int far_ShowHelp(lua_State *L)
{
	const wchar_t *ModuleName = check_utf8_string (L,1,NULL);
	const wchar_t *HelpTopic = opt_utf8_string (L,2,NULL);
	int Flags = CheckFlags(L,3);
	BOOL ret = PSInfo.ShowHelp (ModuleName, HelpTopic, Flags);
	return lua_pushboolean(L, ret), 1;
}

// DestText = far.InputBox(Title,Prompt,HistoryName,SrcText,DestLength,HelpTopic,Flags)
// all arguments are optional
// 1-st argument (GUID) is ignored (kept for compatibility with Far3 scripts)
static int far_InputBox(lua_State *L)
{
	const wchar_t *Title       = opt_utf8_string (L, 2, L"Input Box");
	const wchar_t *Prompt      = opt_utf8_string (L, 3, L"Enter the text:");
	const wchar_t *HistoryName = opt_utf8_string (L, 4, NULL);
	const wchar_t *SrcText     = opt_utf8_string (L, 5, L"");
	int DestLength             = luaL_optinteger (L, 6, 1024);
	const wchar_t *HelpTopic   = opt_utf8_string (L, 7, NULL);
	flags_t Flags = OptFlags (L, 8, FIB_ENABLEEMPTY|FIB_BUTTONS|FIB_NOAMPERSAND);

	if (DestLength < 1) DestLength = 1;
	wchar_t *DestText = (wchar_t*) malloc(sizeof(wchar_t)*DestLength);
	int res = PSInfo.InputBox(Title, Prompt, HistoryName, SrcText, DestText,
													 DestLength, HelpTopic, Flags);

	if (res) push_utf8_string (L, DestText, -1);
	else lua_pushnil(L);

	free(DestText);
	return 1;
}

static int far_GetMsg(lua_State *L)
{
	const wchar_t* Msg = NULL;
	int MsgId = (int)luaL_checkinteger(L, 1);
	DWORD SysId = (DWORD)luaL_optinteger(L, 2, 0);
	if (MsgId >= 0) {
		intptr_t Hnd = SysId ? PSInfo.PluginsControlV3(NULL, PCTL_FINDPLUGIN, PFM_SYSID, &SysId)
			: GetPluginData(L)->ModuleNumber;
		Msg = Hnd ? PSInfo.GetMsg(Hnd, MsgId) : NULL;
	}
	Msg ? push_utf8_string(L, Msg, -1) : lua_pushnil(L);
	return 1;
}

static int far_Text(lua_State *L)
{
	int Color = 0;
	const wchar_t* Str;

	int X = luaL_optinteger(L, 1, 0);
	int Y = luaL_optinteger(L, 2, 0);
	if (!lua_istable(L, 3))
		Color = luaL_optinteger(L, 3, 0x0F);
	Str = opt_utf8_string(L, 4, NULL);

	if (lua_istable(L, 3)) {
		struct FarTrueColorForeAndBack fb;
		memset(&fb, 0, sizeof(fb));
		lua_pushvalue(L, 3);
		FillColor(L, &fb);
		PSInfo.TextV2(X, Y, &fb, Str);
	}
	else
		PSInfo.Text(X, Y, Color, Str);

	return 0;
}

static int far_CopyToClipboard (lua_State *L)
{
	const wchar_t *str = check_utf8_string(L,1,NULL);
	int r = FSF.CopyToClipboard(str);
	return lua_pushboolean(L, r), 1;
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
	if (FSF.FarKeyToName(Key, buf, ARRAYSIZE(buf)-1))
		push_utf8_string(L, buf, -1);
	else
		lua_pushnil(L);
	return 1;
}

static int far_NameToKey (lua_State *L)
{
	const wchar_t* str = check_utf8_string(L,1,NULL);
	FarKey Key = FSF.FarNameToKey(str);
	if (Key == KEY_INVALID)
		lua_pushnil(L);
	else
		lua_pushinteger(L, Key);
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

static int far_LStricmp (lua_State *L)
{
	const wchar_t* s1 = check_utf8_string(L, 1, NULL);
	const wchar_t* s2 = check_utf8_string(L, 2, NULL);
	lua_pushinteger(L, FSF.LStricmp(s1, s2));
	return 1;
}

static int far_LStrnicmp (lua_State *L)
{
	const wchar_t* s1 = check_utf8_string(L, 1, NULL);
	const wchar_t* s2 = check_utf8_string(L, 2, NULL);
	int num = luaL_checkinteger(L, 3);
	if (num < 0) num = 0;
	lua_pushinteger(L, FSF.LStrnicmp(s1, s2, num));
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

	if(Op == PN_CMPNAME || Op == PN_CMPNAMELIST || Op == PN_CHECKMASK) {
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

static int far_GetReparsePointInfo (lua_State *L)
{
	const wchar_t* Src = check_utf8_string(L, 1, NULL);
	int size = FSF.GetReparsePointInfo(Src, NULL, 0);
	if (size <= 0)
		return lua_pushnil(L), 1;
	wchar_t* Dest = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
	FSF.GetReparsePointInfo(Src, Dest, size);
	return push_utf8_string(L, Dest, -1), 1;
}

static int far_LIsAlpha (lua_State *L)
{
	const wchar_t* str = check_utf8_string(L, 1, NULL);
	return lua_pushboolean(L, FSF.LIsAlpha(*str)), 1;
}

static int far_LIsAlphanum (lua_State *L)
{
	const wchar_t* str = check_utf8_string(L, 1, NULL);
	return lua_pushboolean(L, FSF.LIsAlphanum(*str)), 1;
}

static int far_LIsLower (lua_State *L)
{
	const wchar_t* str = check_utf8_string(L, 1, NULL);
	return lua_pushboolean(L, FSF.LIsLower(*str)), 1;
}

static int far_LIsUpper (lua_State *L)
{
	const wchar_t* str = check_utf8_string(L, 1, NULL);
	return lua_pushboolean(L, FSF.LIsUpper(*str)), 1;
}

static int convert_buf (lua_State *L, int command)
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

static int far_LLowerBuf (lua_State *L) {
	return convert_buf(L, 'l');
}

static int far_LUpperBuf (lua_State *L) {
	return convert_buf(L, 'u');
}

static int far_MkTemp (lua_State *L)
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

static int far_MkLink (lua_State *L)
{
	const wchar_t* src = check_utf8_string(L, 1, NULL);
	const wchar_t* dst = check_utf8_string(L, 2, NULL);
	DWORD link_type = OptFlags(L, 3, FLINK_SYMLINK);
	flags_t flags = CheckFlags(L, 4);
	flags = (link_type & 0x0000FFFF) | (flags & 0xFFFF0000);
	lua_pushboolean(L, FSF.MkLink(src, dst, flags));
	return 1;
}

static int truncstring (lua_State *L, int op)
{
	const wchar_t* Src = check_utf8_string(L, 1, NULL);
	int MaxLen = luaL_checkinteger(L, 2);
	int SrcLen = wcslen(Src);
	if (MaxLen < 0) MaxLen = 0;
	else if (MaxLen > SrcLen) MaxLen = SrcLen;
	wchar_t* Trg = (wchar_t*)lua_newuserdata(L, (1 + SrcLen) * sizeof(wchar_t));
	wcscpy(Trg, Src);
	const wchar_t* ptr = (op == 'p') ?
		FSF.TruncPathStr(Trg, MaxLen) : FSF.TruncStr(Trg, MaxLen);
	return push_utf8_string(L, ptr, -1), 1;
}

static int far_TruncPathStr (lua_State *L)
{
	return truncstring(L, 'p');
}

static int far_TruncStr (lua_State *L)
{
	return truncstring(L, 's');
}

typedef struct
{
	lua_State *L;
	int nparams;
	int err;
} FrsData;

static int WINAPI FrsUserFunc (const struct FAR_FIND_DATA *FData, const wchar_t *FullName,
	void *Param)
{
	FrsData *Data = (FrsData*)Param;
	lua_State *L = Data->L;
	int i, nret = lua_gettop(L);

	lua_pushvalue(L, 3); // push the Lua function
	lua_newtable(L);
	PushFarFindData(L, FData);
	push_utf8_string(L, FullName, -1);
	for (i=1; i<=Data->nparams; i++)
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

static int far_RecursiveSearch (lua_State *L)
{
	flags_t Flags;
	FrsData Data = { L,0,0 };
	const wchar_t *InitDir = check_utf8_string(L, 1, NULL);
	wchar_t *Mask = check_utf8_string(L, 2, NULL);

	luaL_checktype(L, 3, LUA_TFUNCTION);
	Flags = CheckFlags(L,4);
	if (lua_gettop(L) == 3)
		lua_pushnil(L);

	Data.nparams = lua_gettop(L) - 4;
	lua_checkstack(L, 256);

	FSF.FarRecursiveSearch(InitDir, Mask, FrsUserFunc, Flags, &Data);

	if(Data.err)
		LF_Error(L, check_utf8_string(L, -1, NULL));
	return Data.err ? 0 : lua_gettop(L) - Data.nparams - 4;
}

static int far_ConvertPath (lua_State *L)
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

static int DoAdvControl (lua_State *L, int Command, int Delta)
{
	int pos2 = 2-Delta, pos3 = 3-Delta;
	TPluginData* pd = GetPluginData(L);
	intptr_t int1;
	wchar_t buf[300];
	COORD coord;

	if (Delta == 0)
		Command = check_env_flag(L, 1);

	switch (Command) {
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
			int1 = PSInfo.AdvControl(pd->ModuleNumber, Command, NULL);
			return lua_pushinteger(L, int1), 1;

		case ACTL_COMMIT:
		case ACTL_REDRAWALL:
			int1 = PSInfo.AdvControl(pd->ModuleNumber, Command, NULL);
			return lua_pushboolean(L, int1), 1;

		case ACTL_QUIT:
			int1 = PSInfo.AdvControl(pd->ModuleNumber, Command, (void*)luaL_optinteger(L,pos2,0));
			return lua_pushinteger(L, int1), 1;

		case ACTL_GETCOLOR:
			int1 = check_env_flag(L, pos2);
			int1 = PSInfo.AdvControl(pd->ModuleNumber, Command, (void*)int1);
			int1 >= 0 ? lua_pushinteger(L, int1) : lua_pushnil(L);
			return 1;

		case ACTL_SYNCHRO: {
			int p = (int)luaL_checkinteger(L, pos2);
			TSynchroData *synchroData = CreateSynchroData(NULL, p);
			lua_pushinteger(L, PSInfo.AdvControl(pd->ModuleNumber, Command, synchroData));
			return 1;
		}

		case ACTL_WAITKEY:
			if (lua_isnumber(L, pos2))
				int1 = lua_tointeger(L, pos2);
			else
				int1 = opt_env_flag(L, pos2, -1);
			if (int1 < -1) //this prevents program freeze
				int1 = -1;
			lua_pushinteger(L, PSInfo.AdvControl(pd->ModuleNumber, Command, (void*)int1));
			return 1;

		case ACTL_SETCURRENTWINDOW:
			int1 = luaL_checkinteger(L, pos2) - 1;
			int1 = PSInfo.AdvControl(pd->ModuleNumber, ACTL_SETCURRENTWINDOW, (void*)int1);
			if (int1 && lua_toboolean(L, pos3))
				PSInfo.AdvControl(pd->ModuleNumber, ACTL_COMMIT, NULL);
			return lua_pushboolean(L, int1), 1;

		case ACTL_GETSYSWORDDIV:
			PSInfo.AdvControl(pd->ModuleNumber, Command, buf);
			return push_utf8_string(L,buf,-1), 1;

		case ACTL_GETARRAYCOLOR: {
			int i;
			int size = PSInfo.AdvControl(pd->ModuleNumber, Command, NULL);
			BYTE *p = (BYTE*) lua_newuserdata(L, size);
			PSInfo.AdvControl(pd->ModuleNumber, Command, p);
			lua_createtable(L, size, 0);
			for (i=0; i < size; i++) {
				lua_pushinteger(L, p[i]);
				lua_rawseti(L, -2, i+1);
			}
			return 1;
		}

		case ACTL_GETFARVERSION: {
			DWORD n = PSInfo.AdvControl(pd->ModuleNumber, Command, 0);
			int v1 = (n >> 16);
			int v2 = n & 0xffff;
			if (lua_toboolean(L, pos2)) {
				lua_pushinteger(L, v1);
				lua_pushinteger(L, v2);
				return 2;
			}
			lua_pushfstring(L, "%d.%d", v1, v2);
			return 1;
		}

		case ACTL_GETWINDOWINFO:
		case ACTL_GETSHORTWINDOWINFO: {
			struct WindowInfo wi;
			memset(&wi, 0, sizeof(wi));
			wi.Pos = luaL_optinteger(L, pos2, 0) - 1;

			if (Command == ACTL_GETWINDOWINFO) {
				int r = PSInfo.AdvControl(pd->ModuleNumber, Command, &wi);
				if (!r)
					return lua_pushnil(L), 1;
				wi.TypeName = (wchar_t*)
					lua_newuserdata(L, (wi.TypeNameSize + wi.NameSize) * sizeof(wchar_t));
				wi.Name = wi.TypeName + wi.TypeNameSize;
			}

			int r = PSInfo.AdvControl(pd->ModuleNumber, Command, &wi);
			if (!r)
				return lua_pushnil(L), 1;
			lua_createtable(L,0,4);
			PutIntToTable(L, "Pos", wi.Pos + 1);
			PutIntToTable(L, "Type", wi.Type);
			PutBoolToTable(L, "Modified", wi.Modified);
			PutBoolToTable(L, "Current", wi.Current);
			if (Command == ACTL_GETWINDOWINFO) {
				PutWStrToTable(L, "TypeName", wi.TypeName, -1);
				PutWStrToTable(L, "Name", wi.Name, -1);
			}
			return 1;
		}

		case ACTL_SETARRAYCOLOR: {
			int i;
			struct FarSetColors fsc;
			luaL_checktype(L, pos2, LUA_TTABLE);
			lua_settop(L, pos2);
			fsc.StartIndex = GetOptIntFromTable(L, "StartIndex", 0);
			lua_getfield(L, pos2, "Flags");
			fsc.Flags = GetFlags(L, -1, NULL);
			fsc.ColorCount = lua_objlen(L, pos2);
			fsc.Colors = (BYTE*)lua_newuserdata(L, fsc.ColorCount);
			for (i=0; i < fsc.ColorCount; i++) {
				lua_pushinteger(L,i+1);
				lua_gettable(L,pos2);
				fsc.Colors[i] = lua_tointeger(L,-1);
				lua_pop(L,1);
			}
			lua_pushboolean(L, PSInfo.AdvControl(pd->ModuleNumber, Command, &fsc));
			return 1;
		}

		case ACTL_GETFARRECT: {
			SMALL_RECT sr;
			if (PSInfo.AdvControl(pd->ModuleNumber, Command, &sr)) {
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
			if (PSInfo.AdvControl(pd->ModuleNumber, Command, &coord)) {
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
			lua_pushboolean(L, PSInfo.AdvControl(pd->ModuleNumber, Command, &coord));
			return 1;

		case ACTL_WINPORTBACKEND:
			PSInfo.AdvControl(pd->ModuleNumber, Command, buf);
			return push_utf8_string(L,buf,-1), 1;

		//case ACTL_KEYMACRO:  //  not supported as it's replaced by separate functions far.MacroXxx
	}
}

#define AdvCommand(name,command,delta) \
static int adv_##name(lua_State *L) { return DoAdvControl(L,command,delta); }

static int far_AdvControl(lua_State *L) { return DoAdvControl(L,0,0); }

AdvCommand( Commit,                 ACTL_COMMIT, 1)
AdvCommand( GetArrayColor,          ACTL_GETARRAYCOLOR, 1)
AdvCommand( GetColor,               ACTL_GETCOLOR, 1)
AdvCommand( GetConfirmations,       ACTL_GETCONFIRMATIONS, 1)
AdvCommand( GetCursorPos,           ACTL_GETCURSORPOS, 1)
AdvCommand( GetDescSettings,        ACTL_GETDESCSETTINGS, 1)
AdvCommand( GetDialogSettings,      ACTL_GETDIALOGSETTINGS, 1)
AdvCommand( GetFarRect,             ACTL_GETFARRECT, 1)
AdvCommand( GetFarVersion,          ACTL_GETFARVERSION, 1)
AdvCommand( GetInterfaceSettings,   ACTL_GETINTERFACESETTINGS, 1)
AdvCommand( GetPanelSettings,       ACTL_GETPANELSETTINGS, 1)
AdvCommand( GetPluginMaxReadData,   ACTL_GETPLUGINMAXREADDATA, 1)
AdvCommand( GetShortWindowInfo,     ACTL_GETSHORTWINDOWINFO, 1)
AdvCommand( GetSystemSettings,      ACTL_GETSYSTEMSETTINGS, 1)
AdvCommand( GetSysWordDiv,          ACTL_GETSYSWORDDIV, 1)
AdvCommand( GetWindowCount,         ACTL_GETWINDOWCOUNT, 1)
AdvCommand( GetWindowInfo,          ACTL_GETWINDOWINFO, 1)
AdvCommand( Quit,                   ACTL_QUIT, 1)
AdvCommand( RedrawAll,              ACTL_REDRAWALL, 1)
AdvCommand( SetArrayColor,          ACTL_SETARRAYCOLOR, 1)
AdvCommand( SetCurrentWindow,       ACTL_SETCURRENTWINDOW, 1)
AdvCommand( SetCursorPos,           ACTL_SETCURSORPOS, 1)
AdvCommand( Synchro,                ACTL_SYNCHRO, 1)
AdvCommand( WaitKey,                ACTL_WAITKEY, 1)
AdvCommand( WinPortBackEnd,         ACTL_WINPORTBACKEND, 1)

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

	if(size)
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

	if(lua_type(L,-1) == LUA_TFUNCTION)
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
	TPluginData *pd = GetPluginData(L);
	MacroAddData *Id;
	int result = FALSE;

	Id = (MacroAddData*)luaL_checkudata(L, 1, AddMacroDataType);
	if (Id->L)
	{
		result = (int)PSInfo.MacroControl(pd->PluginId, MCTL_DELMACRO, 0, Id);
		if(result)
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
		size_t i;
		Data.InCount = top-2;
		Data.InValues = (struct FarMacroValue*)lua_newuserdata(L, Data.InCount*sizeof(struct FarMacroValue));
		memset(Data.InValues, 0, Data.InCount*sizeof(struct FarMacroValue));
		for (i=0; i<Data.InCount; i++)
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

	if(argn > 0)
	{
		int item = 1, i;
		char delim[] = { 226,148,130,0 };        // Unicode char 9474 in UTF-8
		char buf_prefix[64], buf_space[64], buf_format[64];
		int maxno = 0;
		size_t len_prefix;

		for (i=argn; i; maxno++,i/=10) {}
		len_prefix = sprintf(buf_space, "%*s%s ", maxno, "", delim);
		sprintf(buf_format, "%%%dd%%s ", maxno);

		for(i=1; i<=argn; i++)
		{
			size_t j, len_arg;
			const char *start;
			char* str;

			lua_getglobal(L, "tostring");          //+2

			if(i == 1 && lua_type(L,-1) != LUA_TFUNCTION)
				luaL_error(L, "global `tostring' is not function");

			lua_pushvalue(L, i);                   //+3

			if(0 != lua_pcall(L, 1, 1, 0))         //+2 (items,str)
				luaL_error(L, lua_tostring(L, -1));

			if(lua_type(L, -1) != LUA_TSTRING)
				luaL_error(L, "tostring() returned a non-string value");

			sprintf(buf_prefix, buf_format, i, delim);
			start = lua_tolstring(L, -1, &len_arg);
			str = (char*) malloc(len_arg + 1);
			memcpy(str, start, len_arg + 1);

			for (j=0; j<len_arg; j++)
				if(str[j] == '\0') str[j] = ' ';

			for (start=str; start; )
			{
				size_t len_text;
				char *line;
				const char* nl = strchr(start, '\n');

				lua_newtable(L);                     //+3 (items,str,curr_item)
				len_text = nl ? (nl++) - start : (str+len_arg) - start;
				line = (char*) malloc(len_prefix + len_text);
				memcpy(line, buf_prefix, len_prefix);
				memcpy(line + len_prefix, start, len_text);

				lua_pushlstring(L, line, len_prefix + len_text);
				free(line);
				lua_setfield(L, -2, "text");         //+3
				lua_pushvalue(L, i);
				lua_setfield(L, -2, "arg");          //+3
				lua_rawseti(L, -3, item++);          //+2 (items,str)
				strcpy(buf_prefix, buf_space);
				start = nl;
			}

			free(str);
			lua_pop(L, 1);                         //+1 (items)
		}
	}

	return 1;
}

static int far_Show (lua_State *L)
{
	const char* f =
			"local items,n=...\n"
			"local bottom=n==0 and 'No arguments' or n==1 and '1 argument' or n..' arguments'\n"
			"return far.Menu({Title='',Bottom=bottom,Flags='FMENU_SHOWAMPERSAND'},items,"
			"{{BreakKey='SPACE'}})";
	int argn = lua_gettop(L);
	far_MakeMenuItems(L);

	if(luaL_loadstring(L, f) != 0)
		luaL_error(L, lua_tostring(L, -1));

	lua_pushvalue(L, -2);
	lua_pushinteger(L, argn);

	if(lua_pcall(L, 2, LUA_MULTRET, 0) != 0)
		luaL_error(L, lua_tostring(L, -1));

	return lua_gettop(L) - argn - 1;
}

static int far_InputRecordToName(lua_State* L)
{
	char buf[32] = "";
	char uchar[8] = "";
	const char *vk_name;
	DWORD state;
	WORD vk_code;
	int event;

	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 1);

	lua_getfield(L, 1, "EventType");
	event = GetFlags(L, -1, NULL);
	if (! (event==0 || event==KEY_EVENT))
		return lua_pushnil(L), 1;

	lua_getfield(L, 1, "ControlKeyState");
	state = lua_tointeger(L,-1);

	if      (state & RIGHT_CTRL_PRESSED)  strcat(buf, "RCtrl");
	else if (state & LEFT_CTRL_PRESSED )  strcat(buf, "Ctrl");
	if      (state & RIGHT_ALT_PRESSED )  strcat(buf, "RAlt");
	else if (state & LEFT_ALT_PRESSED  )  strcat(buf, "Alt");
	if      (state & SHIFT_PRESSED     )  strcat(buf, "Shift");

	lua_getfield(L, 1, "VirtualKeyCode");
	vk_code = lua_tointeger(L,-1);
	vk_name = (vk_code < ARRAYSIZE(FarKeyStrings)) ? FarKeyStrings[vk_code] : NULL;

	lua_getfield(L, 1, "UnicodeChar");
	if (lua_isstring(L, -1))
		strcpy(uchar, lua_tostring(L,-1));

	lua_getfield(L, 1, "KeyDown");
	if (lua_toboolean(L, -1))
	{
		if (vk_name)
		{
			DWORD Mask_CA = RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED;
			if ((state & Mask_CA) || strlen(vk_name) > 1)  // Alt || Ctrl || virtual key is longer than 1 byte
			{
				strcat(buf, vk_name);
				lua_pushstring(L, buf);
				return 1;
			}
		}
		if (uchar[0])
		{
			lua_pushstring(L, uchar);
			return 1;
		}
	}
	else
	{
		if (!vk_name && *buf && !uchar[0])
		{
			lua_pushstring(L, buf);
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

void NewVirtualKeyTable(lua_State* L, BOOL twoways)
{
	int i;
	lua_createtable(L, 0, twoways ? 360:180);
	for (i=0; i<256; i++) {
		const char* str = VirtualKeyStrings[i];
		if (str != NULL) {
			lua_pushinteger(L, i);
			lua_setfield(L, -2, str);
			if (twoways) {
				lua_pushstring(L, str);
				lua_rawseti(L, -2, i);
			}
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

static int far_CreateFileFilter (lua_State *L)
{
	HANDLE hHandle = (luaL_checkinteger(L,1) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
	int filterType = check_env_flag(L,2);
	HANDLE* pOutHandle = (HANDLE*)lua_newuserdata(L, sizeof(HANDLE));
	if (PSInfo.FileFilterControl(hHandle, FFCTL_CREATEFILEFILTER, filterType,
		(LONG_PTR)pOutHandle))
	{
		luaL_getmetatable(L, FarFileFilterType);
		lua_setmetatable(L, -2);
	}
	else
		lua_pushnil(L);
	return 1;
}

static int filefilter_Free (lua_State *L)
{
	HANDLE *h = CheckFileFilter(L, 1);
	if (*h != INVALID_HANDLE_VALUE) {
		lua_pushboolean(L, PSInfo.FileFilterControl(*h, FFCTL_FREEFILEFILTER, 0, 0));
		*h = INVALID_HANDLE_VALUE;
	}
	else
		lua_pushboolean(L,0);
	return 1;
}

static int filefilter_gc (lua_State *L)
{
	filefilter_Free(L);
	return 0;
}

static int filefilter_tostring (lua_State *L)
{
	HANDLE *h = CheckFileFilter(L, 1);
	if (*h != INVALID_HANDLE_VALUE)
		lua_pushfstring(L, "%s (%p)", FarFileFilterType, h);
	else
		lua_pushfstring(L, "%s (closed)", FarFileFilterType);
	return 1;
}

static int filefilter_OpenMenu (lua_State *L)
{
	HANDLE h = CheckValidFileFilter(L, 1);
	lua_pushboolean(L, PSInfo.FileFilterControl(h, FFCTL_OPENFILTERSMENU, 0, 0));
	return 1;
}

static int filefilter_Starting (lua_State *L)
{
	HANDLE h = CheckValidFileFilter(L, 1);
	lua_pushboolean(L, PSInfo.FileFilterControl(h, FFCTL_STARTINGTOFILTER, 0, 0));
	return 1;
}

static int filefilter_IsFileInFilter (lua_State *L)
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

	if(result) lua_pushlightuserdata(L, (void*)result);
	else lua_pushnil(L);

	return 1;
}

static int far_LoadPlugin(lua_State *L)       { return plugin_load(L, PCTL_LOADPLUGIN); }
static int far_ForcedLoadPlugin(lua_State *L) { return plugin_load(L, PCTL_FORCEDLOADPLUGIN); }

static int far_UnloadPlugin(lua_State *L)
{
	void* Handle = lua_touserdata(L, 1);
	lua_pushboolean(L, Handle ? PSInfo.PluginsControlV3(Handle, PCTL_UNLOADPLUGIN, 0, 0) : 0);
	return 1;
}

static int far_FindPlugin(lua_State *L)
{
	int param1 = check_env_flag(L, 1);
	void *param2 = NULL;
	DWORD SysID;

	if(param1 == PFM_MODULENAME)
	{
		param2 = check_utf8_string(L, 2, NULL);
	}
	else if(param1 == PFM_SYSID)
	{
		SysID = (DWORD)luaL_checkinteger(L, 2);
		param2 = &SysID;
	}

	if(param2)
	{
		intptr_t handle = PSInfo.PluginsControlV3(NULL, PCTL_FINDPLUGIN, param1, param2);

		if(handle)
		{
			lua_pushlightuserdata(L, (void*)handle);
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
		int i;
		for (i=0; i<Count; i++)
			PutWStrToArray(L, i+1, Strings[i], -1);
	}
	lua_setfield(L, -2, "Strings");
	lua_setfield(L, -2, Field);
}

static int far_GetPluginInformation(lua_State *L)
{
	struct FarGetPluginInformation *pi;
	HANDLE Handle;
	size_t size;

	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	Handle = lua_touserdata(L, 1);
	size = PSInfo.PluginsControlV3(Handle, PCTL_GETPLUGININFORMATION, 0, 0);

	if (size == 0) return lua_pushnil(L), 1;

	pi = (struct FarGetPluginInformation *)lua_newuserdata(L, size);
	pi->StructSize = sizeof(*pi);

	if (!PSInfo.PluginsControlV3(Handle, PCTL_GETPLUGININFORMATION, size, pi))
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 4);
	{
		PutWStrToTable(L, "ModuleName", pi->ModuleName, -1);
		PutNumToTable(L, "Flags", pi->Flags);
		lua_createtable(L, 0, 6); // PInfo
		{
			PutNumToTable(L, "StructSize", pi->PInfo->StructSize);
			PutNumToTable(L, "Flags", pi->PInfo->Flags);
			PutNumToTable(L, "SysID", pi->PInfo->SysID);
			PutPluginMenuItemToTable(L, "DiskMenu", pi->PInfo->DiskMenuStrings, pi->PInfo->DiskMenuStringsNumber);
			PutPluginMenuItemToTable(L, "PluginMenu", pi->PInfo->PluginMenuStrings, pi->PInfo->PluginMenuStringsNumber);
			PutPluginMenuItemToTable(L, "PluginConfig", pi->PInfo->PluginConfigStrings, pi->PInfo->PluginConfigStringsNumber);

			if(pi->PInfo->CommandPrefix)
				PutWStrToTable(L, "CommandPrefix", pi->PInfo->CommandPrefix, -1);

			lua_setfield(L, -2, "PInfo");
		}
		lua_createtable(L, 0, 7); // GInfo
		{
			PutNumToTable (L, "StructSize", pi->GInfo->StructSize);
			PutNumToTable (L, "SysID", pi->GInfo->SysID);
			PutWStrToTable(L, "Title", pi->GInfo->Title, -1);
			PutWStrToTable(L, "Description", pi->GInfo->Description, -1);
			PutWStrToTable(L, "Author", pi->GInfo->Author, -1);

			lua_createtable(L, 4, 0);
			PutIntToArray(L, 1, pi->GInfo->Version.Major);
			PutIntToArray(L, 2, pi->GInfo->Version.Minor);
			PutIntToArray(L, 3, pi->GInfo->Version.Revision);
			PutIntToArray(L, 4, pi->GInfo->Version.Build);
			lua_setfield(L, -2, "Version");

			lua_setfield(L, -2, "GInfo");
		}
	}

	return 1;
}

static int far_GetPlugins(lua_State *L)
{
	int count = (int)PSInfo.PluginsControlV3(INVALID_HANDLE_VALUE, PCTL_GETPLUGINS, 0, 0);
	lua_createtable(L, count, 0);

	if(count > 0)
	{
		int i;
		HANDLE *handles = lua_newuserdata(L, count*sizeof(HANDLE));
		count = (int)PSInfo.PluginsControlV3(INVALID_HANDLE_VALUE, PCTL_GETPLUGINS, count, handles);

		for(i=0; i<count; i++)
		{
			lua_pushlightuserdata(L, handles[i]);
			lua_rawseti(L, -3, i+1);
		}

		lua_pop(L, 1);
	}

	return 1;
}

static int far_IsPluginLoaded(lua_State *L)
{
	DWORD SysId = (DWORD)luaL_checkinteger(L, 1);;
	int result = 0;

	intptr_t ret = PSInfo.PluginsControlV3(NULL, PCTL_FINDPLUGIN, PFM_SYSID, &SysId);
	if (ret)
	{
		HANDLE handle = (HANDLE)ret;
		size_t size = PSInfo.PluginsControlV3(handle, PCTL_GETPLUGININFORMATION, 0, 0);
		if (size)
		{
			struct FarGetPluginInformation *pi = (struct FarGetPluginInformation *)malloc(size);
			pi->StructSize = sizeof(*pi);
			if (PSInfo.PluginsControlV3(handle, PCTL_GETPLUGININFORMATION, size, pi))
				result = (pi->Flags & FPF_LOADED) ? 1:0;

			free(pi);
		}
	}
	lua_pushboolean(L, result);
	return 1;
}

static int far_XLat (lua_State *L)
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
	int ExecFlags = CheckFlags(L, 2);
	lua_pushinteger(L, FSF.Execute(CmdStr, ExecFlags));
	return 1;
}

static int far_ExecuteLibrary(lua_State *L)
{
	const wchar_t *Library = check_utf8_string(L, 1, NULL);
	const wchar_t *Symbol  = check_utf8_string(L, 2, NULL);
	const wchar_t *CmdStr  = check_utf8_string(L, 3, NULL);
	int ExecFlags = CheckFlags(L, 4);
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

void ConvertLuaValue (lua_State *L, int pos, struct FarMacroValue *target)
{
	int64_t val64;
	int type = lua_type(L, pos);
	pos = abs_index(L, pos);
	target->Type = FMVT_UNKNOWN;

	if(type == LUA_TNUMBER)
	{
		target->Type = FMVT_DOUBLE;
		target->Value.Double = lua_tonumber(L, pos);
	}
	else if(type == LUA_TSTRING)
	{
		target->Type = FMVT_STRING;
		target->Value.String = check_utf8_string(L, pos, NULL);
	}
	else if(type == LUA_TTABLE)
	{
		lua_rawgeti(L,pos,1);
		if (lua_type(L,-1) == LUA_TSTRING)
		{
			target->Type = FMVT_BINARY;
			target->Value.Binary.Data = (void*)lua_tolstring(L, -1, &target->Value.Binary.Size);
		}
		lua_pop(L,1);
	}
	else if(type == LUA_TBOOLEAN)
	{
		target->Type = FMVT_BOOLEAN;
		target->Value.Boolean = lua_toboolean(L, pos);
	}
	else if(type == LUA_TNIL)
	{
		target->Type = FMVT_NIL;
	}
	else if(type == LUA_TLIGHTUSERDATA)
	{
		target->Type = FMVT_POINTER;
		target->Value.Pointer = lua_touserdata(L, pos);
	}
	else if(bit64_getvalue(L, pos, &val64))
	{
		target->Type = FMVT_INTEGER;
		target->Value.Integer = val64;
	}
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
	struct ColorDialogData Data = { 0, 0, 0x0F, 0 };
	flags_t Flags;

	lua_settop(L, 2);
	if (lua_isnumber (L, 1))
		Data.PaletteColor = (unsigned char)lua_tointeger(L, 1);
	else if (lua_istable(L, 1)) {
		lua_pushvalue(L, 1);
		Data.ForeColor = GetColorFromTable(L, "ForegroundColor", 1);
		Data.BackColor = GetColorFromTable(L, "BackgroundColor", 2);
		Data.PaletteColor = GetColorFromTable(L, "PaletteColor", 3);
		Data.Transparency = GetColorFromTable(L, "Transparency", 4);
	}
	else if (!lua_isnoneornil(L, 1))
		return luaL_argerror(L, 1, "table or integer expected");

	Flags = OptFlags(L, 2, 0);
	if (PSInfo.ColorDialog(pd->ModuleNumber, &Data, Flags)) {
		lua_createtable(L, 0, 4);
		PutIntToTable(L, "ForegroundColor", Data.ForeColor);
		PutIntToTable(L, "BackgroundColor", Data.BackColor);
		PutIntToTable(L, "PaletteColor", Data.PaletteColor);
		PutIntToTable(L, "Transparency", Data.Transparency);
	}
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
	PSInfo.AdvControl(pd->ModuleNumber, ACTL_GETFARRECT, &sr);
	size_t FarWidth = sr.Right - sr.Left + 1;

	for (;;)
	{
		BOOL bResult;
		DWORD nCharsWritten;

		const wchar_t *ptr1 = wcschr(src, L'\n');
		const wchar_t *ptr2 = ptr1 ? ptr1 : src + wcslen(src);
		size_t nCharsToWrite = ptr2 - src;
		int wrap = nCharsToWrite > FarWidth ? 1 : 0;
		if (wrap)
			nCharsToWrite = FarWidth;

		PSInfo.Control(PANEL_ACTIVE, FCTL_GETUSERSCREEN, 0, 0);
		bResult = nCharsToWrite ? WINPORT(WriteConsole)(h_out, src, (DWORD)nCharsToWrite, &nCharsWritten, NULL) : TRUE;
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
	const int MAXARGS = 64;
	int numargs = 0;
	const char *str = luaL_checkstring(L,1), *p=str;
	char *arg;

	lua_settop(L, 1);
	arg = (char*)lua_newuserdata(L, strlen(str)+1);
	lua_checkstack(L, MAXARGS);

	for (const char *q=p; *p && (numargs < MAXARGS); p=q,numargs++)
	{
		int quoted;
		char *trg = arg;
		while (isspace(*q))
			q++;
		if (*q == 0)
			break;
		quoted = *q == '\"';
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
	}
	return numargs;
}

typedef intptr_t WINAPI UDList_Create(unsigned Flags, const wchar_t* Subj);
typedef intptr_t WINAPI UDList_Get(void* udlist, int index);

static int far_DetectCodePage(lua_State *L)
{
	int codepage;
	struct DetectCodePageInfo Info;
	Info.StructSize = sizeof(Info);
	Info.FileName = check_utf8_string(L, 1, NULL);
	codepage = FSF.DetectCodePage(&Info);
	if (codepage)
		lua_pushinteger(L, codepage);
	else
		lua_pushnil(L);
	return 1;
}

#define PAIR(prefix,txt) {#txt, prefix ## _ ## txt}

static const luaL_Reg filefilter_methods[] = {
	{"__gc",             filefilter_gc},
	{"__tostring",       filefilter_tostring},
	{"FreeFileFilter",   filefilter_Free},
	{"OpenFiltersMenu",  filefilter_OpenMenu},
	{"StartingToFilter", filefilter_Starting},
	{"IsFileInFilter",   filefilter_IsFileInFilter},

	{NULL, NULL},
};

static const luaL_Reg dialog_methods[] = {
	{"__gc",                 far_DialogFree},
	{"__tostring",           dialog_tostring},
	{"rawhandle",            dialog_rawhandle},
	{"send",                 far_SendDlgMessage},

	PAIR( dlg, AddHistory),
	PAIR( dlg, Close),
	PAIR( dlg, EditUnchangedFlag),
	PAIR( dlg, Enable),
	PAIR( dlg, EnableRedraw),
	PAIR( dlg, First),
	PAIR( dlg, GetCheck),
	PAIR( dlg, GetColor),
	PAIR( dlg, GetComboboxEvent),
	PAIR( dlg, GetConstTextPtr),
	PAIR( dlg, GetCursorPos),
	PAIR( dlg, GetCursorSize),
	PAIR( dlg, GetDialogInfo),
	PAIR( dlg, GetDlgData),
	PAIR( dlg, GetDlgItem),
	PAIR( dlg, GetDlgRect),
	PAIR( dlg, GetDropdownOpened),
	PAIR( dlg, GetEditPosition),
	PAIR( dlg, GetFocus),
	PAIR( dlg, GetItemData),
	PAIR( dlg, GetItemPosition),
	PAIR( dlg, GetSelection),
	PAIR( dlg, GetText),
	PAIR( dlg, GetTrueColor),
	PAIR( dlg, Key),
	PAIR( dlg, ListAdd),
	PAIR( dlg, ListAddStr),
	PAIR( dlg, ListDelete),
	PAIR( dlg, ListFindString),
	PAIR( dlg, ListGetCurPos),
	PAIR( dlg, ListGetData),
	PAIR( dlg, ListGetDataSize),
	PAIR( dlg, ListGetItem),
	PAIR( dlg, ListGetTitles),
	PAIR( dlg, ListInfo),
	PAIR( dlg, ListInsert),
	PAIR( dlg, ListSet),
	PAIR( dlg, ListSetCurPos),
	PAIR( dlg, ListSetData),
	PAIR( dlg, ListSetMouseReaction),
	PAIR( dlg, ListSetTitles),
	PAIR( dlg, ListSort),
	PAIR( dlg, ListUpdate),
	PAIR( dlg, MoveDialog),
	PAIR( dlg, Redraw),
	PAIR( dlg, ResizeDialog),
	PAIR( dlg, Set3State),
	PAIR( dlg, SetCheck),
	PAIR( dlg, SetColor),
	PAIR( dlg, SetComboboxEvent),
	PAIR( dlg, SetCursorPos),
	PAIR( dlg, SetCursorSize),
	PAIR( dlg, SetDlgData),
	PAIR( dlg, SetDlgItem),
	PAIR( dlg, SetDropdownOpened),
	PAIR( dlg, SetEditPosition),
	PAIR( dlg, SetFocus),
	PAIR( dlg, SetHistory),
	PAIR( dlg, SetItemData),
	PAIR( dlg, SetItemPosition),
	PAIR( dlg, SetMaxTextLength),
	PAIR( dlg, SetMouseEventNotify),
	PAIR( dlg, SetReadOnly),
	PAIR( dlg, SetSelection),
	PAIR( dlg, SetText),
	PAIR( dlg, SetTextPtr),
	PAIR( dlg, SetTrueColor),
	PAIR( dlg, ShowDialog),
	PAIR( dlg, ShowItem),
	PAIR( dlg, User),

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
	PAIR( adv, GetFarRect),
	PAIR( adv, GetFarVersion),
	PAIR( adv, GetInterfaceSettings),
	PAIR( adv, GetPanelSettings),
	PAIR( adv, GetPluginMaxReadData),
	PAIR( adv, GetShortWindowInfo),
	PAIR( adv, GetSystemSettings),
	PAIR( adv, GetSysWordDiv),
	PAIR( adv, GetWindowCount),
	PAIR( adv, GetWindowInfo),
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

static const luaL_Reg editor_funcs[] =
{
	PAIR( editor, AddColor),
	PAIR( editor, AddSessionBookmark),
	PAIR( editor, ClearSessionBookmarks),
	PAIR( editor, DelColor),
	PAIR( editor, DeleteBlock),
	PAIR( editor, DeleteChar),
	PAIR( editor, DeleteSessionBookmark),
	PAIR( editor, DeleteString),
	PAIR( editor, Editor),
	PAIR( editor, ExpandTabs),
	PAIR( editor, GetBookmarks),
	PAIR( editor, GetColor),
	PAIR( editor, GetFileName),
	PAIR( editor, GetInfo),
	PAIR( editor, GetSelection),
	PAIR( editor, GetSessionBookmarks),
	PAIR( editor, GetString),
	PAIR( editor, GetStringW),
	PAIR( editor, GetTitle),
	PAIR( editor, InsertString),
	PAIR( editor, InsertText),
	PAIR( editor, InsertTextW),
	PAIR( editor, NextSessionBookmark),
	PAIR( editor, PrevSessionBookmark),
	PAIR( editor, ProcessInput),
	PAIR( editor, ProcessKey),
	PAIR( editor, Quit),
	PAIR( editor, ReadInput),
	PAIR( editor, RealToTab),
	PAIR( editor, Redraw),
	PAIR( editor, SaveFile),
	PAIR( editor, Select),
	PAIR( editor, SetKeyBar),
	PAIR( editor, SetParam),
	PAIR( editor, SetPosition),
	PAIR( editor, SetString),
	PAIR( editor, SetStringW),
	PAIR( editor, SetTitle),
	PAIR( editor, TabToReal),
	PAIR( editor, TurnOffMarkingBlock),
	PAIR( editor, UndoRedo),

	{NULL, NULL},
};

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

static const luaL_Reg far_funcs[] = {
	PAIR( far, AdvControl),
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
	PAIR( far, DialogFree),
	PAIR( far, DialogInit),
	PAIR( far, DialogRun),
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
	PAIR( far, GetDlgItem),
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
	PAIR( far, SudoClientRegion),
	PAIR( far, SudoCRCall),
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
	PAIR( far, LuafarVersion),
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
	PAIR( far, SendDlgMessage),
	PAIR( far, SetDlgItem),
	PAIR( far, Show),
	PAIR( far, ShowHelp),
	PAIR( far, SplitCmdLine),
	PAIR( far, SubscribeDialogDrawEvents),
	PAIR( far, Text),
	PAIR( far, TruncPathStr),
	PAIR( far, TruncStr),
	PAIR( far, UnloadPlugin),
	PAIR( far, WriteConsole),
	PAIR( far, XLat),

	{NULL, NULL}
};

const char far_Dialog[] =
"function far.Dialog (Guid,X1,Y1,X2,Y2,HelpTopic,Items,Flags,DlgProc,Param)\n\
	local hDlg = far.DialogInit(Guid,X1,Y1,X2,Y2,HelpTopic,Items,Flags,DlgProc,Param)\n\
	if hDlg == nil then return nil end\n\
\n\
	local ret = far.DialogRun(hDlg)\n\
	for i, item in ipairs(Items) do\n\
		local newitem = hDlg:GetDlgItem(i)\n\
		if type(item[6]) == 'table' then\n\
			item[6].SelectIndex = newitem[6].SelectIndex\n\
		else\n\
			item[6] = newitem[6]\n\
		end\n\
		item[10] = newitem[10]\n\
	end\n\
\n\
	far.DialogFree(hDlg)\n\
	return ret\n\
end";

static int luaopen_far (lua_State *L)
{
	TPluginData* pd = GetPluginData(L);

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, FAR_DN_STORAGE);

	NewVirtualKeyTable(L, FALSE);
	lua_setfield(L, LUA_REGISTRYINDEX, FAR_VIRTUALKEYS);

	lua_createtable(L, 0, 1600);
	lua_pushvalue(L, -1);
	lua_replace (L, LUA_ENVIRONINDEX);

	luaL_register(L, "far", far_funcs);
	lua_insert(L, -2);
	add_flags(L);
	lua_setfield(L, -2, "Flags");

	luaopen_far_host(L);
	lua_setfield(L, -2, "Host");

	if (pd->Private)
	{
		lua_pushcfunction(L, far_MacroCallFar);
		lua_setfield(L, -2, "MacroCallFar");
		lua_pushcfunction(L, far_MacroCallToLua);
		lua_setfield(L, -2, "MacroCallToLua");
	}

	lua_newtable(L);
	lua_setglobal(L, "export");

	luaopen_regex(L);
	luaL_register(L, "editor", editor_funcs);
	luaL_register(L, "viewer", viewer_funcs);
	luaL_register(L, "panel",  panel_funcs);
	luaL_register(L, "actl",   actl_funcs);

	luaL_newmetatable(L, FarFileFilterType);
	lua_pushvalue(L,-1);
	lua_setfield(L, -2, "__index");
	luaL_register(L, NULL, filefilter_methods);

#if !defined(__FreeBSD__) && !defined(__DragonFly__)
	lua_getglobal(L, "far");
	lua_pushcfunction(L, luaopen_timer);
	lua_call(L, 0, 1);
	lua_setfield(L, -2, "Timer");
#endif

	lua_pushcfunction(L, luaopen_usercontrol);
	lua_call(L, 0, 0);

	luaL_newmetatable(L, FarDialogType);
	lua_pushvalue(L,-1);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, DialogHandleEqual);
	lua_setfield(L, -2, "__eq");
	luaL_register(L, NULL, dialog_methods);

	(void) luaL_dostring(L, far_Dialog);

	luaL_newmetatable(L, AddMacroDataType);
	lua_pushcfunction(L, AddMacroData_gc);
	lua_setfield(L, -2, "__gc");

	luaL_newmetatable(L, SavedScreenType);
	lua_pushcfunction(L, far_FreeScreen);
	lua_setfield(L, -2, "__gc");

	return 0;
}

static void InitLuaState (lua_State *L, TPluginData *aPlugData, lua_CFunction aOpenLibs)
{
	int idx, pos;

	lua_CFunction func_arr[] = {
		luaopen_far,
		luaopen_bit64,
		luaopen_unicode,
		luaopen_utf8,
		luaopen_win,
		luaopen_sysutils,
	};

	// open Lua libraries
	luaL_openlibs(L);

	if (aOpenLibs) {
		lua_pushcfunction(L, aOpenLibs);
		lua_call(L, 0, 0);
	}

	// open more libraries
	for (idx=0; idx < ARRAYSIZE(func_arr); idx++) {
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
	lua_setfield(L, -2, "__index");	            //+4
	lua_pop(L, 4);                              //+0

	{ // modify package.path
		const char *farhome;
		lua_getglobal  (L, "package");            //+1
		pos = lua_gettop(L);
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
	}

	// create Lua State
	lua_State *L = lua_open();
	if (L) {
		// place pointer to plugin data in the L's registry -
		aPlugData->MainLuaState = L;
		lua_pushlightuserdata(L, aPlugData);
		lua_setfield(L, LUA_REGISTRYINDEX, FAR_KEYINFO);

		// Evaluate the path where the scripts are (ShareDir)
		// It may (or may not) be the same as ModuleDir.
		const char *s1=  "/lib/far2m/Plugins/luafar/";
		const char *s2="/share/far2m/Plugins/luafar/";
		push_utf8_string(L, aPlugData->ModuleName, -1);                  //+1
		aPlugData->ShareDir = (char*) malloc(lua_objlen(L,-1) + 8);
		strcpy(aPlugData->ShareDir, luaL_gsub(L, lua_tostring(L,-1), s1, s2)); //+2
		strrchr(aPlugData->ShareDir,'/')[0] = '\0';

		DIR* dir = opendir(aPlugData->ShareDir); // a "patch" for PPA installations
		if (dir)
			closedir(dir);
		else {
			strcpy(aPlugData->ShareDir, lua_tostring(L,-2));
			strrchr(aPlugData->ShareDir,'/')[0] = '\0';
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
		lua_setfield(L, LUA_REGISTRYINDEX, FAR_KEYINFO);
		memcpy(pd, PluginData, sizeof(TPluginData));
		pd->MainLuaState = L;
		InitLuaState(L, pd, aOpenLibs);
	}
	return 0;
}
