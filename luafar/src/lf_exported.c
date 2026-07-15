//---------------------------------------------------------------------------
#include <windows.h>
#include <dlfcn.h> //dlclose

#include <lua.h>
#include <lauxlib.h>

#include "far3parts.h"
#include <farkeys.h>

#include "lf_util.h"
#include "lf_string.h"
#include "lf_bit64.h"
#include "lf_flags.h"
#include "lf_service.h"

extern HANDLE Open_Luamacro (lua_State* L, INT_PTR Item);

void PackMacroValues(lua_State* L, size_t Count, const struct FarMacroValue* Values); // forward declaration

// "Collector" is a Lua table referenced from the Plugin Object table by name.
// Collector contains an array of lightuserdata which are pointers to malloc'ed
// memory regions.
const char COLLECTOR_OPI[] = "Collector_OpenPanelInfo";
const char COLLECTOR_PI[]  = "Collector_PluginInfo";
const char KEY_OBJECT[]    = "Panel_Object";

// taken from lua.c v5.1.2
int traceback(lua_State *L)
{
	lua_getglobal(L, "debug");

	if (!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		return 1;
	}

	lua_getfield(L, -1, "traceback");

	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 2);
		return 1;
	}

	lua_pushvalue(L, 1);  /* pass error message */
	lua_pushinteger(L, 2);  /* skip this function and traceback */
	lua_call(L, 2, 1);  /* call debug.traceback */
	return 1;
}

// taken from lua.c v5.1.2 (modified)
int docall(lua_State *L, int narg, int nret)
{
	int status;
	int base = lua_gettop(L) - narg;  /* function index */
	lua_pushcfunction(L, traceback);  /* push traceback function */
	lua_insert(L, base);  /* put it under chunk and args */
	status = lua_pcall(L, narg, nret, base);
	lua_remove(L, base);  /* remove traceback function */
	/* force a complete garbage collection in case of errors */
	if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);

	return status;
}

// if the function is successfully retrieved, it's on the stack top; 1 is returned
// else 0 returned (and the stack is unchanged)
int GetExportFunction(lua_State* L, const char* FuncName)
{
	lua_getglobal(L, "export");
	if (lua_istable(L,-1))
	{
		lua_getfield(L, -1, FuncName);
		if (lua_isfunction(L,-1))
			return lua_remove(L,-2), 1;

		lua_pop(L,1);
	}
	return lua_pop(L,1), 0;
}

int pcall_msg(lua_State* L, int narg, int nret)
{
	// int status = lua_pcall(L, narg, nret, 0);
	int status = docall(L, narg, nret);

	if (status != 0)
	{
		int status2 = 1;
		DWORD *Flags = &GetPluginData(L)->Flags;

		*Flags |= PDF_PROCESSINGERROR;

		if (GetExportFunction(L, "OnError"))
		{
			lua_insert(L,-2);
			status2 = lua_pcall(L,1,0,0);
		}

		if (status2 != 0) {
			switch (safe__tostring_meta(L, -1))
			{
				case TOSTRING_NOMETA:
					if (lua_isstring(L, -1))
						lua_pushvalue(L, -1);
					else
						lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, -1));
					break;
				case TOSTRING_SUCCESS:
					break;
				case TOSTRING_ERROR:
					lua_pop(L, 1);
					lua_pushstring(L, "error in error handling");
					break;
			}
			LF_Error (L, check_utf8_string(L, -1, NULL));
			lua_pop (L, 2);
		}

		*Flags &= ~PDF_PROCESSINGERROR;
	}

	return status;
}

void PushPluginTable(lua_State* L, HANDLE hPlugin)
{
	lua_pushlightuserdata(L, hPlugin);
	lua_rawget(L, LUA_REGISTRYINDEX);
}

void PushPluginObject(lua_State* L, HANDLE hPlugin)
{
	PushPluginTable(L, hPlugin);
	if (lua_istable(L, -1))
		lua_getfield(L, -1, KEY_OBJECT);
	else
		lua_pushnil(L);
	lua_remove(L, -2);
}

void PushPluginPair(lua_State* L, HANDLE hPlugin)
{
	PushPluginObject(L, hPlugin);
	lua_pushlightuserdata(L, hPlugin);
}

void ReplacePluginInfoCollector(lua_State* L, const char* Key)
{
	lua_newtable(L);
	lua_pushvalue(L, -1);
	lua_setfield(L, LUA_REGISTRYINDEX, Key);
}

// the value is on stack top (-1)
// collector table is under the index 'pos' (this index cannot be a pseudo-index)
// the function pops the value off the stack;
const wchar_t* _AddStringToCollector(lua_State *L, int pos)
{
	if (lua_isstring(L,-1))
	{
		const wchar_t* s = check_utf8_string(L, -1, NULL);
		lua_rawseti(L, pos, (int)lua_objlen(L, pos) + 1);
		return s;
	}

	lua_pop(L,1);
	return L"";
}

// input table is on stack top (-1)
// collector table is under the index 'pos' (this index cannot be a pseudo-index)
const wchar_t* AddStringToCollectorField(lua_State *L, int pos, const char* key)
{
	lua_getfield(L, -1, key);
	if (pos < 0) --pos;
	return _AddStringToCollector(L, pos);
}

// input table is on stack top (-1)
// collector table is under the index 'pos' (this index cannot be a pseudo-index)
const wchar_t* AddStringToCollectorSlot(lua_State *L, int pos, int key)
{
	lua_pushinteger(L, key);
	lua_gettable(L, -2);
	if (pos < 0) --pos;
	return _AddStringToCollector(L, pos);
}

// collector table is under the index 'pos' (this index cannot be a pseudo-index)
void* AddBufToCollector(lua_State *L, int pos, size_t size)
{
	void *t = lua_newuserdata(L, size);
	if (pos < 0) --pos;
	memset(t, 0, size);
	lua_rawseti(L, pos, (int)lua_objlen(L, pos) + 1);
	return t;
}

// -- a table is on stack top
// -- its field 'field' is an array of strings
// -- 'cpos' - collector stack position
const wchar_t** CreateStringsArray(lua_State* L, int cpos, const char* field, int *numstrings)
{
	const wchar_t **buf = NULL;
	if (numstrings) *numstrings = 0;
	lua_getfield(L, -1, field);     //+1
	if (cpos < 0) --cpos;

	if (lua_istable(L, -1))
	{
		size_t n = lua_objlen(L, -1);

		if (numstrings) *numstrings = n;

		if (n > 0)
		{
			buf = (const wchar_t**)AddBufToCollector(L, cpos, (n+1) * sizeof(wchar_t*));

			for(size_t i=0; i < n; i++)
				buf[i] = AddStringToCollectorSlot(L, cpos, (int)i+1);

			buf[n] = NULL;
		}
	}

	lua_pop(L, 1);  //+0
	return buf;
}

// input table is on stack top (-1)
// collector table is one under the top (-2)
// userdata table is two under the top (-3)
void FillPluginPanelItem (lua_State *L, struct PluginPanelItem *pi, int index)
{
	int Collector = lua_gettop(L) - 1;
	memset(pi, 0, sizeof(*pi));
	pi->FindData.dwFileAttributes = GetAttrFromTable(L);
	pi->FindData.ftCreationTime   = GetFileTimeFromTable(L, "CreationTime");
	pi->FindData.ftLastAccessTime = GetFileTimeFromTable(L, "LastAccessTime");
	pi->FindData.ftLastWriteTime  = GetFileTimeFromTable(L, "LastWriteTime");
	pi->FindData.nFileSize        = GetFileSizeFromTable(L, "FileSize");
	pi->FindData.nPhysicalSize    = GetFileSizeFromTable(L, "PhysicalSize");
	pi->FindData.dwUnixMode       = GetOptIntFromTable  (L, "UnixMode", 0);
	pi->NumberOfLinks             = GetOptIntFromTable  (L, "NumberOfLinks", 0);

	pi->FindData.lpwszFileName = AddStringToCollectorField(L, Collector, "FileName");
	pi->Description            = AddStringToCollectorField(L, Collector, "Description");
	pi->Owner                  = AddStringToCollectorField(L, Collector, "Owner");
	pi->Group                  = AddStringToCollectorField(L, Collector, "Group");

	// custom column data
	lua_getfield(L, -1, "CustomColumnData");
	if (lua_istable(L,-1)) {
		pi->CustomColumnNumber = lua_objlen(L,-1);
		pi->CustomColumnData = malloc(pi->CustomColumnNumber * sizeof(wchar_t*));
		for (int i=0; i < pi->CustomColumnNumber; i++) {
			lua_rawgeti(L, -1, i+1);
			*(wchar_t**)(pi->CustomColumnData+i) = (wchar_t*)_AddStringToCollector(L, Collector);
		}
	}
	lua_pop(L,1);

	// prevent Far from treating UserData as pointer and copying data from it
	pi->Flags = GetOptIntFromTable(L, "Flags", 0) & ~PPIF_USERDATA;
	if (index) {
		lua_getfield(L, -1, "UserData");
		if (!lua_isnil(L, -1)) {
			pi->UserData = index;
			lua_rawseti(L, Collector-1, index);
		}
		else {
			pi->UserData = 0;
			lua_pop(L, 1);
		}
	}
}

// FindData table is on the stack top. It is popped off the stack on return.
void FillFindData(lua_State* L, HANDLE hPanel, struct PluginPanelItem **pPanelItems, int *pItemsNumber)
{
	// allocate an extra item to avoid implementation defined malloc(0);
	size_t numLines = lua_objlen(L, -1);
	struct PluginPanelItem *ppi = (struct PluginPanelItem *) malloc(sizeof(*ppi) * (numLines + 1));

	if (!ppi)
	{
		lua_pop(L, 1);                           //+0
		*pItemsNumber = 0;
		*pPanelItems = NULL;
		return;
	}

	PushPluginTable(L, hPanel);          //+2: FindData,PTbl
	lua_insert(L, -2);                   //+2: PTbl,FindData

	// PTbl[COLLECTOR_UD] = {} -- UData
	lua_newtable(L);                     //+3  PTbl,FindData,UData
	lua_pushvalue(L,-1);                 //+4: PTbl,FindData,UData,UData
	lua_setfield(L, -4, COLLECTOR_UD);   //+3: PTbl,FindData,UData

	// PTbl[ppi] = Coll
	lua_newtable(L);                     //+4  PTbl,FindData,UData,Coll
	lua_pushlightuserdata(L, ppi);       //+5  PTbl,FindData,UData,Coll,ppi
	lua_pushvalue(L,-2);                 //+6: PTbl,FindData,UData,Coll,ppi,Coll
	lua_rawset(L, -6);                   //+4: PTbl,FindData,UData,Coll

	size_t num = 0;
	for(size_t i = 1; i <= numLines; i++)
	{
		lua_pushinteger(L, i);             //+5  Ptbl,FindData,Udata,Coll,i
		lua_gettable(L, -4);               //+5: Ptbl,FindData,Udata,Coll,FindData[i]

		if (lua_istable(L, -1))
		{
			FillPluginPanelItem(L, ppi+num, num+1);
			++num;
			lua_pop(L,1);                    //+4
		}
		else
		{
			lua_pop(L,1);                    //+4
			break;
		}
	}

	lua_pop(L,4);                        //+0
	*pItemsNumber = num;
	*pPanelItems = ppi;
}

int LF_GetFindData(lua_State* L, HANDLE hPanel, struct PluginPanelItem **pPanelItem,
									 int *pItemsNumber, DWORD OpMode)
{
	if (GetExportFunction(L, "GetFindData"))      //+1: Func
	{
		PushPluginPair(L, hPanel);                 //+3: Func,Pair
		lua_pushinteger(L, OpMode);                //+4: Func,Pair,OpMode

		if (!pcall_msg(L, 3, 1))                    //+1: FindData
		{
			if (lua_istable(L, -1))
			{
				FillFindData(L, hPanel, pPanelItem, pItemsNumber); //+0
				lua_gc(L, LUA_GCCOLLECT, 0);         //free memory taken by FindData
				return TRUE;
			}
			lua_pop(L,1);
		}
	}
	return FALSE;
}

void LF_FreeFindData(lua_State* L, HANDLE hPanel, struct PluginPanelItem *PanelItems, int ItemsNumber)
{
	if (PanelItems)
	{
		for (int i = 0; i < ItemsNumber; i++) {
			free((void*)PanelItems[i].CustomColumnData);
		}
		PushPluginTable(L, hPanel);
		lua_pushlightuserdata(L, PanelItems);
		lua_pushnil(L);
		lua_rawset(L, -3); //free the collector
		lua_pop(L, 1);
		lua_gc(L, LUA_GCCOLLECT, 0); //free memory taken by Collector
		free(PanelItems);
	}
}
//---------------------------------------------------------------------------

// PanelItems table should be on Lua stack top
void UpdateFileSelection(lua_State* L, struct PluginPanelItem *PanelItems, size_t ItemsNumber)
{
	for (int i=0; i < (int)ItemsNumber; i++)
	{
		lua_rawgeti(L, -1, i+1);           //+1

		if (lua_istable(L,-1))
		{
			lua_getfield(L,-1,"Flags");      //+2

			int success = 0;
			DWORD Flags = GetFlagCombination(L,-1,&success);
			if (success && ((Flags & PPIF_SELECTED) == 0))
				PanelItems[i].Flags &= ~PPIF_SELECTED;

			lua_pop(L,1);         //+1
		}

		lua_pop(L,1);           //+0
	}
}
//---------------------------------------------------------------------------

int LF_GetFiles(lua_State* L, HANDLE hPanel, struct PluginPanelItem *PanelItem,
	int ItemsNumber, int Move, const wchar_t **DestPath, DWORD OpMode)
{
	int ret = 0;

	if (GetExportFunction(L, "GetFiles"))         //+1: Func
	{
		PushPanelItems(L, hPanel, PanelItem, ItemsNumber); //+2: Func,Item
		lua_insert(L,-2);                  //+2: Item,Func
		PushPluginPair(L, hPanel);         //+4: Item,Func,Pair
		lua_pushvalue(L,-4);               //+5: Item,Func,Pair,Item
		lua_pushboolean(L, Move);
		push_utf8_string(L, *DestPath, -1);
		lua_pushinteger(L, OpMode);        //+8: Item,Func,Pair,Item,Move,Dest,OpMode

		if (!pcall_msg(L, 6, 2))           //+3: Item,Res,Dest
		{
			if (lua_isstring(L,-1))
			{
				*DestPath = check_utf8_string(L,-1,NULL);
				lua_setfield(L, LUA_REGISTRYINDEX, "GetFiles.DestPath"); // protect from GC
			}
			else
				lua_pop(L,1);                  //+2: Item,Res

			ret = lua_tointeger(L,-1);
			lua_pop(L,1);                    //+1: Item (required for UpdateFileSelection)
			UpdateFileSelection(L, PanelItem, ItemsNumber);
		}
		lua_pop(L,1); //+0
	}
	return ret;
}
//---------------------------------------------------------------------------

BOOL RunDefaultScript(lua_State* L, int ForFirstTime)
{
	int pos = lua_gettop(L);
	// First: try to load the default script embedded into the plugin
	lua_getglobal(L, "require");
	lua_pushliteral(L, "<boot");
	int status = lua_pcall(L,1,1,0);
	if (status == 0)
	{
		lua_pushboolean(L, ForFirstTime);
		status = pcall_msg(L,1,0);
		lua_settop (L, pos);
		return (status == 0);
	}

	// Second: try to load the default script from a disk file
	TPluginData* pd = GetPluginData(L);
	lua_pushstring(L, pd->ShareDir);
	lua_pushstring(L, "/");
	push_utf8_string(L, wcsrchr(pd->ModuleName, GOOD_SLASH) + 1, -1);
	lua_concat(L,3);

	char* defscript = (char*)lua_newuserdata (L, lua_objlen(L,-1) + 8);
	strcpy(defscript, lua_tostring(L, -2));

	FILE *fp = NULL;
	const char delims[] = ".-";
	for (int i=0; delims[i]; i++)
	{
		char *end = strrchr(defscript, delims[i]);

		if (end)
		{
			strcpy(end, ".lua");
			if ((fp = fopen(defscript, "r")) != NULL)
				break;
		}
	}

	if (fp)
	{
		fclose(fp);
		status = luaL_loadfile(L, defscript);
		if (status == 0)
		{
			lua_pushboolean(L, ForFirstTime);
			status = pcall_msg(L,1,0);
		}
		else
			LF_Error(L, utf8_to_wcstring (L, -1, NULL));
	}
	else
		LF_Error(L, L"Default script not found");

	lua_settop(L, pos);
	return (status == 0);
}

BOOL LF_RunDefaultScript(lua_State* L)
{
	return RunDefaultScript(L, 1);
}

// return FALSE only if error occurred
static BOOL CheckReloadDefaultScript(lua_State *L)
{
	// reload default script?
	int reload = 0;
	lua_getglobal(L, "far");
	if (lua_istable(L, -1))
	{
		lua_getfield(L, -1, "ReloadDefaultScript");
		reload = lua_toboolean(L, -1);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return !reload || RunDefaultScript(L, 0);
}

// -- an object (any non-nil value) is on stack top;
// -- a new table is created, the object is put into it under the key KEY_OBJECT;
// -- the table is put into the registry, and reference to it is obtained;
// -- the function pops the object and returns the reference;
static HANDLE RegisterObject(lua_State* L)
{
	void *ptr;
	lua_newtable(L);                  //+2: Obj,Tbl
	lua_pushvalue(L,-2);              //+3: Obj,Tbl,Obj
	lua_setfield(L,-2,KEY_OBJECT);    //+2: Obj,Tbl
	ptr = (void*)lua_topointer(L,-1); //+2
	lua_pushlightuserdata(L, ptr);    //+3
	lua_pushvalue(L,-2);              //+4
	lua_rawset(L, LUA_REGISTRYINDEX); //+2
	lua_pop(L,2);                     //+0
	return ptr;
}

static void PushAnalyseInfo(lua_State* L, const struct AnalyseInfo *Info)
{
	lua_createtable(L, 0, 4);
	PutIntToTable(L,  "StructSize", Info->StructSize);
	PutWStrToTable(L, "FileName", Info->FileName, -1);
	PutLStrToTable(L, "Buffer", Info->Buffer, Info->BufferSize);
	PutFlagsToTable(L, "OpMode", Info->OpMode);
}

HANDLE LF_Analyse(lua_State* L, const struct AnalyseInfo *Info)
{
	HANDLE result = INVALID_HANDLE_VALUE;

	if (GetExportFunction(L, "Analyse"))    //+1
	{
		PushAnalyseInfo(L, Info);            //+2

		if (!pcall_msg(L, 1, 1))              //+1
		{
			if (lua_toboolean(L, -1))
			{
				const intptr_t Unfit = (intptr_t)INVALID_HANDLE_VALUE;
				intptr_t ref = luaL_ref(L, LUA_REGISTRYINDEX);   //+0

				if (ref == Unfit)
				{
					lua_rawgeti(L, LUA_REGISTRYINDEX, Unfit);      //+1
					ref = luaL_ref(L, LUA_REGISTRYINDEX);          //+0
					luaL_unref(L, LUA_REGISTRYINDEX, Unfit);
				}

				result = (HANDLE)ref;
			}
			else
				lua_pop(L, 1);                   //+0
		}
	}

	return result;
}

void LF_CloseAnalyse(lua_State* L, const struct CloseAnalyseInfo *Info)
{
	luaL_unref(L, LUA_REGISTRYINDEX, (int)(intptr_t)Info->Handle);
}
//---------------------------------------------------------------------------

// the KeyBar table is on stack top
static void OPI_FillInfoLines(lua_State *L, struct OpenPluginInfo *Info, int cpos)
{
	lua_getfield(L, -1, "InfoLines");
	lua_getfield(L, -2, "InfoLinesNumber");

	if (lua_istable(L,-2) && lua_isnumber(L,-1))
	{
		int InfoLinesNumber = lua_tointeger(L, -1);
		lua_pop(L,1);                         //+5: Info,Tbl,Coll,Info,Lines

		if (InfoLinesNumber > 0)
		{
			struct InfoPanelLine *pl = (struct InfoPanelLine*)
				AddBufToCollector(L, cpos, InfoLinesNumber * sizeof(struct InfoPanelLine));
			Info->InfoLines = pl;
			Info->InfoLinesNumber = InfoLinesNumber;

			for(int i=0; i<InfoLinesNumber; ++i,++pl,lua_pop(L,1))
			{
				lua_pushinteger(L, i+1);
				lua_gettable(L, -2);

				if (lua_istable(L, -1))            //+6: Info,Tbl,Coll,Info,Lines,Line
				{
					pl->Text = AddStringToCollectorField(L, cpos, "Text");
					pl->Data = AddStringToCollectorField(L, cpos, "Data");
					flags_t Flags = GetFlagsFromTable(L, -1, "Flags");
					pl->Separator = (Flags & IPLFLAGS_SEPARATOR) ? 1 : 0;
				}
			}
		}

		lua_pop(L,1);
	}
	else lua_pop(L, 2);
}

// the KeyBar table is on stack top
static void OPI_FillPanelModes(lua_State *L, struct OpenPluginInfo *Info, int cpos)
{
	lua_getfield(L, -1, "PanelModesArray");
	lua_getfield(L, -2, "PanelModesNumber");

	if (lua_istable(L,-2) && lua_isnumber(L,-1))
	{
		int PanelModesNumber = lua_tointeger(L, -1);
		lua_pop(L,1);                               //+5: Info,Tbl,Coll,Info,Modes

		if (PanelModesNumber > 0)
		{
			struct PanelMode *pm = (struct PanelMode*)
				AddBufToCollector(L, cpos, PanelModesNumber * sizeof(struct PanelMode));
			Info->PanelModesArray = pm;
			Info->PanelModesNumber = PanelModesNumber;

			for(int i=0; i<PanelModesNumber; ++i,++pm,lua_pop(L,1))
			{
				lua_pushinteger(L, i+1);
				lua_gettable(L, -2);

				if (lua_istable(L, -1))                  //+6: Info,Tbl,Coll,Info,Modes,Mode
				{
					pm->ColumnTypes  = AddStringToCollectorField(L, cpos, "ColumnTypes");
					pm->ColumnWidths = AddStringToCollectorField(L, cpos, "ColumnWidths");
					pm->StatusColumnTypes  = AddStringToCollectorField(L, cpos, "StatusColumnTypes");
					pm->StatusColumnWidths = AddStringToCollectorField(L, cpos, "StatusColumnWidths");
					pm->ColumnTitles = (const wchar_t* const*)CreateStringsArray(L, cpos, "ColumnTitles", NULL);
					flags_t Flags = GetFlagsFromTable (L, -1, "Flags");
					pm->FullScreen      = (Flags & PMFLAGS_FULLSCREEN)      ? 1 : 0;
					pm->DetailedStatus  = (Flags & PMFLAGS_DETAILEDSTATUS)  ? 1 : 0;
					pm->AlignExtensions = (Flags & PMFLAGS_ALIGNEXTENSIONS) ? 1 : 0;
					pm->CaseConversion  = (Flags & PMFLAGS_CASECONVERSION)  ? 1 : 0;
				}
			}
		}

		lua_pop(L,1);
	}
	else lua_pop(L, 2);
}

// the KeyBar table is on stack top
static void OPI_FillKeyBarTitles(lua_State *L, struct OpenPluginInfo *Info, int cpos)
{
	lua_getfield(L, -1, "KeyBar");

	if (lua_istable(L, -1))
	{
		struct KeyBarTitles *kbt = (struct KeyBarTitles*)
			AddBufToCollector(L, cpos, sizeof(struct KeyBarTitles));
		Info->KeyBar = kbt;
		size_t Count = lua_objlen(L, -1);

		for(size_t i=0; i < Count; i++)
		{
			lua_rawgeti(L, -1, i+1);

			if (!lua_istable(L, -1))
			{
				lua_pop(L, 1);
				break;
			}

			WORD VirtualKeyCode = GetOptIntFromTable(L, "VirtualKeyCode", 0);
			if (VirtualKeyCode >= VK_F1 && VirtualKeyCode <= VK_F12)
			{
				lua_getfield(L, -1, "ControlKeyState");
				DWORD State = (DWORD)GetFlagCombination(L, -1, NULL);
				lua_pop(L, 1);

				wchar_t **target;
				if ((State & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) && (State & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)))
					target = kbt->CtrlAltTitles;
				else if ((State & SHIFT_PRESSED) && (State & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)))
					target = kbt->AltShiftTitles;
				else if ((State & SHIFT_PRESSED) && (State & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)))
					target = kbt->CtrlShiftTitles;
				else if (State & SHIFT_PRESSED)
					target = kbt->ShiftTitles;
				else if (State & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
					target = kbt->AltTitles;
				else if (State & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
					target = kbt->CtrlTitles;
				else
					target = kbt->Titles;

				wchar_t *Text = (wchar_t*)AddStringToCollectorField(L, cpos, "Text");
				wchar_t *LongText = (wchar_t*)AddStringToCollectorField(L, cpos, "LongText");
				target[VirtualKeyCode - VK_F1] = *LongText ? LongText : Text;
			}

			lua_pop(L, 1); // pop KeyBarLabel table
		}
	}

	lua_pop(L, 1); // pop KeyBar table
}

void LF_GetOpenPanelInfo(lua_State* L, HANDLE hPanel, struct OpenPluginInfo *aInfo)
{
	int stack_top = lua_gettop(L);

	aInfo->StructSize = sizeof (struct OpenPluginInfo);
	if (!GetExportFunction(L, "GetOpenPanelInfo"))     //+1
		return;

	PushPluginPair(L, hPanel);                         //+3

	if (pcall_msg(L, 2, 1) != 0)
		return;

	if (!lua_istable(L, -1))                            //+1: Info
	{
		lua_pop(L, 1);
		return;
	}

	PushPluginTable(L, hPanel);                        //+2: Info,Tbl
	lua_newtable(L);                                   //+3: Info,Tbl,Coll
	int cpos = lua_gettop (L);  // collector stack position
	lua_pushvalue(L,-1);                               //+4: Info,Tbl,Coll,Coll
	lua_setfield(L, -3, COLLECTOR_OPI);                //+3: Info,Tbl,Coll
	lua_pushvalue(L,-3);                               //+4: Info,Tbl,Coll,Info
	//---------------------------------------------------------------------------
	// First element in the collector; can be retrieved on later calls;
	struct OpenPluginInfo *Info =
		(struct OpenPluginInfo*) AddBufToCollector(L, cpos, sizeof(struct OpenPluginInfo));
	//---------------------------------------------------------------------------
	Info->StructSize = sizeof (struct OpenPluginInfo);
	Info->HostFile   = AddStringToCollectorField(L, cpos, "HostFile");
	Info->CurDir     = AddStringToCollectorField(L, cpos, "CurDir");
	Info->Format     = AddStringToCollectorField(L, cpos, "Format");
	Info->PanelTitle = AddStringToCollectorField(L, cpos, "PanelTitle");
	Info->CurURL     = AddStringToCollectorField(L, cpos, "CurURL");
	//---------------------------------------------------------------------------
	// Convert 3 far3 API flags to far2 API
	flags_t Flags = GetFlagsFromTable(L, -1, "Flags");
	Flags |= (OPIF_USEFILTER | OPIF_USEHIGHLIGHTING | OPIF_USESORTGROUPS);
	if (Flags & OPIF_DISABLEFILTER)        Flags &= ~OPIF_USEFILTER;
	if (Flags & OPIF_DISABLEHIGHLIGHTING)  Flags &= ~OPIF_USEHIGHLIGHTING;
	if (Flags & OPIF_DISABLESORTGROUPS)    Flags &= ~OPIF_USESORTGROUPS;
	Info->Flags = Flags & 0xFFFFFFFF;
	//---------------------------------------------------------------------------
	OPI_FillInfoLines(L, Info, cpos);
	OPI_FillPanelModes(L, Info, cpos);
	OPI_FillKeyBarTitles(L, Info, cpos);
	//---------------------------------------------------------------------------
	Info->DescrFiles = CreateStringsArray(L, cpos, "DescrFiles", &Info->DescrFilesNumber);
	//---------------------------------------------------------------------------
	Info->StartPanelMode = GetOptIntFromTable(L, "StartPanelMode", 0);
	Info->StartSortMode  = GetFlagsFromTable (L, -1, "StartSortMode");
	Info->StartSortOrder = GetOptIntFromTable(L, "StartSortOrder", 0);
	//---------------------------------------------------------------------------
	Info->ShortcutData = AddStringToCollectorField (L, cpos, "ShortcutData");
	//---------------------------------------------------------------------------
	lua_settop(L, stack_top);
	*aInfo = *Info;
}
//---------------------------------------------------------------------------

static void WINAPI FillFarMacroCall_Callback (void *CallbackData, struct FarMacroValue *Values, size_t Count)
{
	struct FarMacroCall *fmc = (struct FarMacroCall*)CallbackData;
	(void)Values; // not used
	(void)Count;  // not used
	for(size_t i=0; i<fmc->Count; i++)
	{
		struct FarMacroValue *v = fmc->Values + i;
		if (v->Type == FMVT_STRING)
			free((void*)v->Value.String);
		else if (v->Type == FMVT_BINARY && v->Value.Binary.Size)
			free((void*)v->Value.Binary.Data);
	}
	free(CallbackData);
}

static struct FarMacroCall* CreateFarMacroCall(size_t narg)
{
	size_t offset = aligned_size(sizeof(struct FarMacroCall), _Alignof(struct FarMacroValue));
	struct FarMacroCall *fmc = (struct FarMacroCall*)
			malloc(offset + narg*sizeof(struct FarMacroValue));
	fmc->StructSize = sizeof(*fmc);
	fmc->Count = narg;
	fmc->Values = (struct FarMacroValue*)((char*)fmc + offset);
	fmc->Callback = FillFarMacroCall_Callback;
	fmc->CallbackData = fmc;
	return fmc;
}

static HANDLE FillFarMacroCall (lua_State* L, int narg)
{
	struct FarMacroCall *fmc = CreateFarMacroCall(narg);

	for (int i=0; i<narg; i++)
	{
		int64_t val64;
		int pos = i - narg;
		int type = lua_type(L, pos);
		fmc->Values[i].Type = FMVT_NIL;

		if (type == LUA_TNUMBER)
		{
			fmc->Values[i].Type = FMVT_DOUBLE;
			fmc->Values[i].Value.Double = lua_tonumber(L, pos);
		}
		else if (type == LUA_TBOOLEAN)
		{
			fmc->Values[i].Type = FMVT_BOOLEAN;
			fmc->Values[i].Value.Boolean = lua_toboolean(L, pos);
		}
		else if (type == LUA_TSTRING)
		{
			fmc->Values[i].Type = FMVT_STRING;
			fmc->Values[i].Value.String = wcsdup(check_utf8_string(L, pos, NULL));
		}
		else if (type == LUA_TLIGHTUSERDATA)
		{
			fmc->Values[i].Type = FMVT_POINTER;
			fmc->Values[i].Value.Pointer = lua_touserdata(L, pos);
		}
		else if (type == LUA_TTABLE)
		{
			lua_getfield(L, pos, TKEY_BINARY);
			if (lua_type(L,-1) == LUA_TSTRING)
			{
				size_t len;
				const char *str = lua_tolstring(L, -1, &len);
				void* arr = malloc(len);
				memcpy(arr, str, len);
				fmc->Values[i].Value.Binary.Data = arr;
				fmc->Values[i].Value.Binary.Size = len;
				fmc->Values[i].Type = FMVT_BINARY;
			}
			lua_pop(L,1);
		}
		else if (bit64_getvalue(L, pos, &val64))
		{
			fmc->Values[i].Type = FMVT_INTEGER;
			fmc->Values[i].Value.Integer = val64;
		}
	}

	return (HANDLE)fmc;
}

static void push_guid(lua_State *L, const void *pGuid)
{
	lua_pushlstring(L, (const char*)pGuid, sizeof(GUID));
}

HANDLE LF_Open (lua_State* L, int OpenFrom, INT_PTR Item)
{
	int entry_top = lua_gettop(L);
	HANDLE Result = INVALID_HANDLE_VALUE;

	if (!CheckReloadDefaultScript(L) || !GetExportFunction(L, "Open"))
		return Result;

	if (OpenFrom == OPEN_LUAMACRO)
		return Open_Luamacro(L, Item);

	lua_pushinteger(L, OpenFrom); // 1-st argument
	lua_pushnil(L);               // 2-nd argument (dummy menuitem Id)

	// 3-rd argument

	if (OpenFrom == OPEN_FROMMACRO)
	{
		struct OpenMacroInfo* data = (struct OpenMacroInfo*)Item;
		PackMacroValues(L, data->Count, data->Values);
	}
	else if (OpenFrom == OPEN_SHORTCUT)
	{
		push_utf8_string(L, (wchar_t*)Item, -1);
	}
	else if (OpenFrom == OPEN_COMMANDLINE)
	{
		push_utf8_string(L, (wchar_t*)Item, -1);
	}
	else if (OpenFrom == OPEN_DIALOG)
	{
		struct OpenDlgPluginData *data = (struct OpenDlgPluginData*)Item;
		lua_pop(L, 1);       // pop dummy menuitem Id
		push_guid(L, &data->ItemGuid);
		lua_createtable(L, 0, 1);
		NewDialogData(L, data->hDlg, FALSE);
		lua_setfield(L, -2, "hDlg");
	}
	else if (OpenFrom == OPEN_ANALYSE)
	{
		struct OpenAnalyseInfo* oai = (struct OpenAnalyseInfo*)Item;
		PushAnalyseInfo(L, oai->Info);
		lua_rawgeti(L, LUA_REGISTRYINDEX, (int)(intptr_t)oai->Handle);
		lua_setfield(L, -2, "Handle");
		luaL_unref(L, LUA_REGISTRYINDEX, (int)(intptr_t)oai->Handle);
	}
	else if (OpenFrom == OPEN_LEFTDISKMENU || OpenFrom == OPEN_RIGHTDISKMENU ||
			OpenFrom == OPEN_PLUGINSMENU || OpenFrom == OPEN_EDITOR || OpenFrom == OPEN_VIEWER)
	{
		lua_pop(L, 1);       // pop dummy menuitem Id
		push_guid(L, (void*)Item);
		lua_pushnil(L);      // dummy Data
	}
	else
	{
		lua_pushnil(L);
	}

	// Call export.Open()

	if (OpenFrom == OPEN_FROMMACRO)
	{
		int top = lua_gettop(L);
		if (pcall_msg(L, 3, LUA_MULTRET) == 0)
		{
			int nret = lua_gettop(L) - top + 4; // nret
			if (nret > 0 && lua_istable(L, -nret))
			{
				lua_getfield(L, -nret, "type"); // nret+1
				if (lua_type(L,-1)==LUA_TSTRING && lua_objlen(L,-1)==5 && !strcmp("panel",lua_tostring(L,-1)))
				{
					lua_pop(L,1); // nret
					lua_rawgeti(L,-nret,1); // nret+1
					if (lua_toboolean(L, -1))
					{
						struct FarMacroCall* fmc = CreateFarMacroCall(1);
						fmc->Values[0].Type = FMVT_PANEL;
						fmc->Values[0].Value.Pointer = RegisterObject(L); // nret
						Result = fmc;
					}
					goto Exit;
				}
				lua_pop(L,1); // nret
			}
			if (nret)
			{
				Result = FillFarMacroCall(L,nret);
			}
		}
	}
	else if (OpenFrom == OPEN_ANALYSE)
	{
		if (pcall_msg(L, 3, 1) == 0)
		{
			if (lua_type(L,-1) == LUA_TNUMBER && lua_tointeger(L,-1) == (intptr_t)PANEL_STOP)
				Result = PANEL_STOP;
			else if (lua_toboolean(L, -1))
				Result = RegisterObject(L);
		}
	}
  else
  {
		if (pcall_msg(L, 3, 1) == 0)
		{
			if (lua_toboolean(L, -1))
				Result = RegisterObject(L);
		}
	}

Exit:
	lua_settop(L, entry_top);
	return Result;
}

void LF_ClosePanel(lua_State* L, HANDLE hPanel)
{
	if (GetExportFunction(L, "ClosePanel"))    //+1: Func
	{
		PushPluginPair(L, hPanel);               //+3: Func,Pair
		pcall_msg(L, 2, 0);
	}
	lua_pushlightuserdata(L, hPanel);
	lua_pushnil(L);
	lua_rawset(L, LUA_REGISTRYINDEX);
}

int LF_Compare(lua_State* L, HANDLE hPanel, const struct PluginPanelItem *Item1,
							 const struct PluginPanelItem *Item2, unsigned int Mode)
{
	int res = -2; // default FAR compare function should be used
	if (GetExportFunction(L, "Compare"))    //+1: Func
	{
		PushPluginPair(L, hPanel);           //+3: Func,Pair
		PushPanelItem(L, Item1);             //+4
		PushPanelItem(L, Item2);             //+5
		lua_pushinteger(L, Mode);            //+6

		if (0 == pcall_msg(L, 5, 1))          //+1
		{
			res = lua_tointeger(L,-1);
			lua_pop(L,1);
		}
	}

	return res;
}

int LF_Configure(lua_State* L, const struct ConfigureInfo *Info)
{
	int res = FALSE;

	if (GetExportFunction(L, "Configure"))    //+1: Func
	{
		push_guid(L, Info->Guid);

		if (0 == pcall_msg(L, 1, 1))          //+1
		{
			res = lua_toboolean(L,-1);
			lua_pop(L,1);
		}
	}

	return res;
}

int LF_DeleteFiles(lua_State* L, HANDLE hPanel, struct PluginPanelItem *PanelItem,
	int ItemsNumber, DWORD OpMode)
{
	int res = FALSE;

	if (GetExportFunction(L, "DeleteFiles"))      //+1: Func
	{
		PushPluginPair(L, hPanel);                 //+3: Func,Pair
		PushPanelItems(L, hPanel, PanelItem, ItemsNumber); //+4
		lua_pushinteger(L, OpMode);                //+5

		if (0 == pcall_msg(L, 4, 1))                //+1
		{
			res = lua_toboolean(L,-1);
			lua_pop(L,1);
		}
	}

	return res;
}

// far.MakeDirectory returns 2 values:
//    a) status (an integer; in accordance to FAR API), and
//    b) new directory name (a string; optional)
int LF_MakeDirectory (lua_State* L, HANDLE hPanel, const wchar_t **Name, DWORD OpMode)
{
	int res = 0;

	if (GetExportFunction(L, "MakeDirectory"))    //+1: Func
	{
		PushPluginPair(L, hPanel);                 //+3: Func,Pair
		push_utf8_string(L, *Name, -1);            //+4
		lua_pushinteger(L, OpMode);                //+5

		if (0 == pcall_msg(L, 4, 2))                //+2
		{
			res = lua_tointeger(L,-2);
			if (res == 1 && lua_isstring(L,-1))
			{
				*Name = check_utf8_string(L,-1,NULL);
				lua_pushvalue(L, -1);
				lua_setfield(L, LUA_REGISTRYINDEX, "MakeDirectory.Name"); // protect from GC
			}
			else if (res != -1)
				res = 0;
			lua_pop(L,2);
		}
	}

	return res;
}

int LF_ProcessPanelEvent(lua_State* L, HANDLE hPanel, int Event, void *Param)
{
	int res = FALSE;

	if (!(GetPluginData(L)->Flags & PDF_PROCESSINGERROR) &&
			GetExportFunction(L, "ProcessPanelEvent"))     //+1: Func
	{
		PushPluginPair(L, hPanel);         //+3
		lua_pushinteger(L, Event);         //+4
		if (Event == FE_CHANGEVIEWMODE || Event == FE_COMMAND)
			push_utf8_string(L, (const wchar_t*)Param, -1); //+5
		else
			lua_pushnil(L);                  //+5
		if (0 == pcall_msg(L, 4, 1))  {     //+1
			res = lua_toboolean(L,-1);
			lua_pop(L,1);                    //+0
		}
	}
	return res;
}

int LF_ProcessHostFile(lua_State* L, HANDLE hPanel, struct PluginPanelItem *PanelItem,
	int ItemsNumber, DWORD OpMode)
{
	int ret = 0;

	if (GetExportFunction(L, "ProcessHostFile"))      //+1: Func
	{
		PushPanelItems(L, hPanel, PanelItem, ItemsNumber); //+2: Func,Item
		lua_insert(L,-2);                  //+2: Item,Func
		PushPluginPair(L, hPanel);         //+4: Item,Func,Pair
		lua_pushvalue(L,-4);               //+5: Item,Func,Pair,Item
		lua_pushinteger(L, OpMode);        //+6: Item,Func,Pair,Item,OpMode

		if (!pcall_msg(L, 4, 1))           //+2: Item,Res
		{
			ret = lua_toboolean(L,-1);
			lua_pop(L,1);                    //+1: Item (required for UpdateFileSelection)
			UpdateFileSelection(L, PanelItem, ItemsNumber);
		}
		lua_pop(L,1); //+0
	}
	return ret;
}

int LF_ProcessKey(lua_State* L, HANDLE hPanel, int Key, unsigned int ControlState)
{
	if ((Key & ~PKF_PREPROCESS) == KEY_NONE)
		return FALSE; //ignore garbage

	if (GetExportFunction(L, "ProcessKey")) {   //+1: Func
		PushPluginPair(L, hPanel);         //+3: Func,Pair
		lua_pushinteger(L, Key);           //+4
		lua_pushinteger(L, ControlState);  //+5
		if (pcall_msg(L, 4, 1) == 0)    {  //+1: Res
			int ret = lua_toboolean(L,-1);
			return lua_pop(L,1), ret;
		}
	}
	return FALSE;
}

int LF_PutFiles(lua_State* L, HANDLE hPanel, struct PluginPanelItem *PanelItems,
	int ItemsNumber, int Move, const wchar_t *SrcPath, DWORD OpMode)
{
	int ret = 0;

	if (GetExportFunction(L, "PutFiles"))       //+1: Func
	{
		PushPanelItems(L, hPanel, PanelItems, ItemsNumber); //+2: Func,Items
		lua_insert(L,-2);                  //+2: Items,Func
		PushPluginPair(L, hPanel);         //+4: Items,Func,Pair
		lua_pushvalue(L,-4);               //+5: Items,Func,Pair,Item
		lua_pushboolean(L, Move);          //+6: Items,Func,Pair,Item,Move
		push_utf8_string(L, SrcPath, -1);  //+7: Items,Func,Pair,Item,Move,SrcPath
		lua_pushinteger(L, OpMode);        //+8: Items,Func,Pair,Item,Move,SrcPath,OpMode
		if (!pcall_msg(L, 6, 1))           //+2: Items,Res
		{
			ret = lua_tointeger(L,-1);
			lua_pop(L,1);                    //+1: Items (required for UpdateFileSelection)
			UpdateFileSelection(L, PanelItems, ItemsNumber);
		}
		lua_pop(L,1); //+0
	}
	return ret;
}

int LF_SetDirectory(lua_State* L, HANDLE hPanel, const wchar_t *Dir, DWORD OpMode)
{
	int ret = 0;

	if (GetExportFunction(L, "SetDirectory"))      //+1: Func
	{
		PushPluginPair(L, hPanel);         //+3: Func,Pair
		push_utf8_string(L, Dir, -1);      //+4: Func,Pair,Dir
		lua_pushinteger(L, OpMode);        //+5: Func,Pair,Dir,OpMode

		if (!pcall_msg(L, 4, 1))           //+1: Res
		{
			ret = lua_toboolean(L,-1);
			lua_pop(L,1);
		}
	}
	return ret;
}

int LF_SetFindList(lua_State* L, HANDLE hPanel, const struct PluginPanelItem *PanelItems,
	int ItemsNumber)
{
	int ret = 0;

	if (GetExportFunction(L, "SetFindList"))       //+1: Func
	{
		PushPluginPair(L, hPanel);                  //+3: Func,Pair
		PushPanelItems(L, hPanel, PanelItems, ItemsNumber); //+4: Func,Pair,Items

		if (!pcall_msg(L, 3, 1))                    //+1: Res
		{
			ret = lua_toboolean(L,-1);
			lua_pop(L,1);
		}
	}

	return ret;
}

void LF_LuaClose(TPluginData* aPlugData)
{
	lua_close(aPlugData->MainLuaState);
	free(aPlugData->ShareDir);
}

void LF_ExitFAR(lua_State* L)
{
	if (GetExportFunction(L, "ExitFAR"))   //+1: Func
		pcall_msg(L, 0, 0);                  //+0
}

int LF_MayExitFAR(lua_State* L)
{
	int ret = 1;
	if (GetExportFunction(L, "MayExitFAR"))  { //+1: Func
		if (!pcall_msg(L, 0, 1)) {               //+1
			ret = lua_toboolean(L,-1);
			lua_pop(L,1);                          //+0
		}
	}
	return ret;
}

static void SetMenuGuids(lua_State *L, const char *Field, size_t Count, const GUID** Target, int CPos)
{
	if (Count) {
		lua_getfield(L, -1, Field);
		if (lua_type(L,-1) == LUA_TSTRING) {
			if (lua_objlen(L,-1) >= Count*sizeof(GUID)) {
				*Target = (const GUID*)lua_tostring(L,-1);
				lua_rawseti(L, CPos, lua_objlen(L, CPos) + 1);
				return;
			}
		}
		lua_pop(L, 1);
	}
}

void LF_GetPluginInfo(lua_State* L, struct PluginInfo *aPI)
{
	aPI->StructSize = sizeof (struct PluginInfo);
	if (!GetExportFunction(L, "GetPluginInfo"))     //+1
		return;

	if (pcall_msg(L, 0, 1) != 0)
		return;

	if (!lua_istable(L, -1))
	{
		lua_pop(L,1);
		return;
	}

	ReplacePluginInfoCollector(L, COLLECTOR_PI);       //+2: Info,Coll
	int cpos = lua_gettop(L);  // collector position
	lua_pushvalue(L, -2);                              //+3: Info,Coll,Info
	//--------------------------------------------------------------------------
	struct PluginInfo *PI = (struct PluginInfo*)
		AddBufToCollector (L, cpos, sizeof(struct PluginInfo));
	PI->StructSize = sizeof (struct PluginInfo);
	//--------------------------------------------------------------------------
	PI->Flags = GetFlagsFromTable (L, -1, "Flags");
	//--------------------------------------------------------------------------
	PI->DiskMenuStrings = CreateStringsArray (L, cpos, "DiskMenuStrings", &PI->DiskMenuStringsNumber);
	PI->PluginMenuStrings = CreateStringsArray (L, cpos, "PluginMenuStrings", &PI->PluginMenuStringsNumber);
	PI->PluginConfigStrings = CreateStringsArray (L, cpos, "PluginConfigStrings", &PI->PluginConfigStringsNumber);
	PI->CommandPrefix = AddStringToCollectorField(L, cpos, "CommandPrefix");
	//--------------------------------------------------------------------------
	SetMenuGuids(L, "DiskMenuGuids", PI->DiskMenuStringsNumber, &PI->DiskMenuGuids, cpos);
	SetMenuGuids(L, "PluginMenuGuids", PI->PluginMenuStringsNumber, &PI->PluginMenuGuids, cpos);
	SetMenuGuids(L, "PluginConfigGuids", PI->PluginConfigStringsNumber, &PI->PluginConfigGuids, cpos);
	//--------------------------------------------------------------------------
	lua_pop(L, 3);
	*aPI = *PI;
}

int LF_ProcessEditorInput (lua_State* L, const INPUT_RECORD *Rec)
{
	int ret = 0;

	if (!GetExportFunction(L, "ProcessEditorInput"))    //+1: Func
		return 0;

	PushInputRecord(L, Rec);

	if (pcall_msg(L, 1, 1) == 0)        //+1: Res
	{
		ret = lua_toboolean(L,-1);
		lua_pop(L,1);
	}
	return ret;
}

int LF_ProcessEditorEvent (lua_State* L, const struct ProcessEditorEventInfo *Info)
{
	int ret = 0;
	int Top = lua_gettop(L);

	if (!(GetPluginData(L)->Flags & PDF_PROCESSINGERROR) &&
			GetExportFunction(L, "ProcessEditorEvent"))     //+1: Func
	{
		lua_pushinteger(L, Info->EditorID);  //+2
		lua_pushinteger(L, Info->Event);     //+3;

		// 3-rd parameter
		switch (Info->Event) {
			case EE_READ:
			case EE_CLOSE:
				lua_pushinteger(L, 0);  //dummy
				break;

			case EE_SAVE: {
				struct EditorSaveFile *esf = (struct EditorSaveFile*)Info->Param;
				lua_createtable(L, 0, 3);
				PutWStrToTable(L, "FileName", esf->FileName, -1);
				PutWStrToTable(L, "FileEOL", esf->FileEOL, -1);
				PutIntToTable(L, "CodePage", esf->CodePage);
				break;
			}

			case EE_GOTFOCUS:
			case EE_KILLFOCUS:
				lua_pushinteger(L, *(int*)Info->Param);
				break;

			case EE_REDRAW:
				lua_pushinteger(L, (INT_PTR)Info->Param);
				break;

			default: // ignore an unknown event type
				lua_settop(L, Top);
				return 0;
		}

		if (pcall_msg(L, 3, 1) == 0) {    //+1
			if (lua_isnumber(L,-1))
				ret = lua_tointeger(L,-1);
			lua_pop(L,1);
		}
	}
	return ret;
}

int LF_ProcessViewerEvent (lua_State* L, int Event, void* Param)
{
	int ret = 0;

	if (!(GetPluginData(L)->Flags & PDF_PROCESSINGERROR) &&
			GetExportFunction(L, "ProcessViewerEvent"))     //+1: Func
	{
		int ViewerID = Param ? *(int*)Param : -1;

		lua_pushinteger(L, ViewerID);
		lua_pushinteger(L, Event);
		lua_pushnil(L);

		if (pcall_msg(L, 3, 1) == 0)         //+1
		{
			if (lua_isnumber(L,-1)) ret = lua_tointeger(L,-1);

			lua_pop(L,1);
		}
	}

	return ret;
}

int LF_ProcessDialogEvent (lua_State* L, int Event, void *Param)
{
	int ret = 0;
	struct FarDialogEvent *fde = (struct FarDialogEvent*) Param;
	DWORD *Flags = &GetPluginData(L)->Flags;
	BOOL PushDN = FALSE;

	if (*Flags & PDF_PROCESSINGERROR)
		return 0;

	if (Event == DE_DLGPROCINIT && fde->Msg == DN_INITDIALOG)
	{
		*Flags &= ~PDF_DIALOGEVENTDRAWENABLE;
	}
	else if (!(*Flags & PDF_DIALOGEVENTDRAWENABLE) && (
		fde->Msg == DN_CTLCOLORDIALOG  ||
		fde->Msg == DN_CTLCOLORDLGITEM ||
		fde->Msg == DN_CTLCOLORDLGLIST ||
		fde->Msg == DN_DRAWDIALOG      ||
		fde->Msg == DN_DRAWDIALOGDONE  ||
		fde->Msg == DN_DRAWDLGITEM))
	{
		return 0;
	}

	if (!GetExportFunction(L, "ProcessDialogEvent")) //+1: Func
		return 0;

	lua_pushinteger(L, Event);       //+2
	lua_createtable(L, 0, 5);        //+3
	NewDialogData(L, fde->hDlg, FALSE);
	lua_setfield(L, -2, "hDlg");     //+3

	if (PushDNParams(L, fde->Msg, fde->Param1, fde->Param2)) //+6
	{
		PushDN = TRUE;
		lua_setfield(L, -4, "Param2"); //+5
		lua_setfield(L, -3, "Param1"); //+4
		lua_setfield(L, -2, "Msg");    //+3
	}
	else if (PushDMParams(L, fde->Msg, fde->Param1)) //+5
	{
		lua_setfield(L, -3, "Param1"); //+4
		lua_setfield(L, -2, "Msg");    //+3
		PutIntToTable(L, "Param2", fde->Param2); //FIXME: temporary solution
	}
	else
	{
		PutIntToTable(L, "Msg", fde->Msg);
		PutIntToTable(L, "Param1", fde->Param1);
		PutIntToTable(L, "Param2", fde->Param2);
	}

	if (pcall_msg(L, 2, 1) == 0)      //+1
	{
		if ((ret=lua_toboolean(L,-1)) != 0)
		{
			fde->Result = PushDN ? ProcessDNResult(L, fde->Msg, fde->Param2) : lua_tointeger(L,-1);
		}

		lua_pop(L,1);
	}

	return ret;
}

static int Common_ProcessSynchroEvent(lua_State* L, int Event, int Data)
{
	int ret = 0;
	if (GetExportFunction(L, "ProcessSynchroEvent"))     //+1: Func
	{
		lua_pushinteger(L, Event);     //+2
		lua_pushinteger(L, Data);      //+3
		if (pcall_msg(L, 2, 1) == 0)   //+1
		{
			if (lua_isnumber(L,-1))
				ret = (int)lua_tointeger(L,-1);
			lua_pop(L,1);
		}
	}
	return ret;
}

int LF_ProcessSynchroEvent (lua_State* L, int Event, void *Param)
{
	if (Event == SE_COMMONSYNCHRO)
	{
		TSynchroData sd = *(TSynchroData*)Param; // copy
		free(Param);

		switch(sd.type)
		{
			case SYNCHRO_TIMER:
			{
#if !defined(__DragonFly__) && !defined(__ANDROID__)
				int narg, index, posTab;
				TTimerData *td = sd.timerData;
				switch (td->closeStage)
				{
					case 0:
						lua_rawgeti(L, LUA_REGISTRYINDEX, td->tabRef);  //+1: Table
						posTab = lua_gettop(L);
						lua_getfield(L, -1, "n");                       //+2
						narg = (int)lua_tointeger(L, -1);
						for (index=1; index <= 2+narg; index++)         //+2+2+narg
							lua_rawgeti(L, posTab, index);
						lua_remove(L, posTab);
						lua_remove(L, posTab);                          //+2+narg
						pcall_msg(L, 1+narg, 0);                        //+0
						break;

					case 1:
						break;

					case 2:
						luaL_unref(L, LUA_REGISTRYINDEX, td->tabRef);
						break;
				}
#endif
				break;
			}

			case SYNCHRO_COMMON:
				Common_ProcessSynchroEvent(L, Event, sd.data);
				break;

			case SYNCHRO_FUNCTION:
				lua_rawgeti(L, LUA_REGISTRYINDEX, sd.ref);
				luaL_unref(L, LUA_REGISTRYINDEX, sd.ref);
				if (lua_istable(L,-1) && lua_checkstack(L, sd.narg)) {
					for (int i=1; i <= sd.narg; i++) {
						lua_rawgeti(L, -i, i);
					}
					pcall_msg(L, sd.narg - 1, 0);
				}
				lua_pop(L, 1);
				break;
		}
	}
	else if (Event == SE_FOLDERCHANGED)
	{
		Common_ProcessSynchroEvent(L, Event, 0);
	}

	return 0;
}

int LF_GetCustomData(lua_State* L, const wchar_t *FilePath, wchar_t **CustomData)
{
	if (GetExportFunction(L, "GetCustomData"))  { //+1: Func
		push_utf8_string(L, FilePath, -1);  //+2
		if (pcall_msg(L, 1, 1) == 0) {  //+1
			if (lua_isstring(L, -1)) {
				const wchar_t* p = utf8_to_wcstring(L, -1, NULL);
				if (p) {
					*CustomData = wcsdup(p);
					lua_pop(L, 1);
					return TRUE;
				}
			}
			lua_pop(L, 1);
		}
	}
	return FALSE;
}

void LF_FreeCustomData(lua_State* L, wchar_t *CustomData)
{
	(void) L;
	if (CustomData) free(CustomData);
}

int LF_ProcessConsoleInput(lua_State* L, INPUT_RECORD *Rec)
{
	int ret = 0;

	if (!(GetPluginData(L)->Flags & PDF_PROCESSINGERROR) &&
			GetExportFunction(L, "ProcessConsoleInput"))    //+1: Func
	{
		PushInputRecord(L, Rec);                         //+2

		if (pcall_msg(L, 1, 1) == 0)                      //+1: Res
		{
			if (lua_type(L,-1) == LUA_TNUMBER && lua_tonumber(L,-1) == 0)
				ret = 0;
			else if (lua_type(L,-1) == LUA_TTABLE)
			{
				FillInputRecord(L, -1, Rec);
				ret = 2;
			}
			else
				ret = lua_toboolean(L,-1);

			lua_pop(L,1);
		}
	}

	return ret;
}

int LF_GetLinkTarget(
	lua_State *L,
	HANDLE hPanel,
	struct PluginPanelItem *PanelItem,
	wchar_t *Target,
	size_t TargetSize,
	DWORD OpMode)
{
	if (GetExportFunction(L, "GetLinkTarget"))  //+1: Func
	{
		PushPluginPair(L, hPanel);                //+3: Func,Pair
		PushPanelItem(L, PanelItem);              //+4: Func,Pair,Item
		lua_pushinteger(L, OpMode);               //+5  Func,Pair,Item,OpMode
		if (!pcall_msg(L, 4, 1))                  //+1: Res
		{
			if (lua_type(L,-1) == LUA_TSTRING)
			{
				size_t size = 0;
				const wchar_t* ptr = utf8_to_wcstring(L,-1,&size); //+1 (conversion in place)
				if (Target && TargetSize)
				{
					wcsncpy(Target, ptr, TargetSize);
				}
				lua_pop(L, 1);                        //+0
				return (int)size + 1;
			}
			lua_pop(L, 1);                          //+0
		}
	}
	return 0;
}
