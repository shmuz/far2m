#include <dlfcn.h> //dlopen

#include <windows.h>
#include <farplug-mb.h>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>

#include "util.h"
#include "ustring.h"
#include "service.h"

	void FillPluginPanelItem(lua_State *L, struct PluginPanelItem *pi, int CollectorPos);
}


const char* GetStrFromTable(lua_State *L, const char *Key)
{
	lua_getfield(L, -1, Key);
	const char *val = lua_tostring(L,-1);
	lua_pop(L,1);
	return val;
}

// - input table is on stack top (-1)
// - no collector is required
void FillAnsiPluginPanelItem (lua_State *L, oldfar::PluginPanelItem *pi)
{
	memset(pi, 0, sizeof(*pi));
	pi->FindData.dwFileAttributes = GetAttrFromTable(L);
	pi->FindData.ftCreationTime   = GetFileTimeFromTable(L, "CreationTime");
	pi->FindData.ftLastAccessTime = GetFileTimeFromTable(L, "LastAccessTime");
	pi->FindData.ftLastWriteTime  = GetFileTimeFromTable(L, "LastWriteTime");
	pi->FindData.nFileSize        = GetFileSizeFromTable(L, "FileSize");
	pi->FindData.nPhysicalSize    = GetFileSizeFromTable(L, "PhysicalSize");
	pi->FindData.dwUnixMode       = GetOptIntFromTable  (L, "UnixMode", 0);
	pi->NumberOfLinks             = GetOptIntFromTable  (L, "NumberOfLinks", 0);

	const char* FileName          = GetStrFromTable  (L, "FileName");
	if (FileName)
		strncpy(pi->FindData.cFileName, FileName, MAX_NAME-1);
	pi->Description        = (char*)GetStrFromTable  (L, "Description");
	pi->Owner              = (char*)GetStrFromTable  (L, "Owner");
	pi->Group              = (char*)GetStrFromTable  (L, "Group");

	// custom column data
	lua_getfield(L, -1, "CustomColumnData");
	if (lua_istable(L,-1)) {
		int i;
		pi->CustomColumnNumber = lua_objlen(L,-1);
		pi->CustomColumnData = (char**) malloc(pi->CustomColumnNumber * sizeof(char*));
		for (i=0; i < pi->CustomColumnNumber; i++) {
			lua_rawgeti(L, -1, i+1);
			pi->CustomColumnData[i] = (char*)lua_tostring(L, -1);
			lua_pop(L,1);
		}
	}
	lua_pop(L,1);

	// Flags
	pi->Flags = GetOptIntFromTable(L, "Flags", 0);

	// UserData
	lua_getfield(L, -1, "UserData");
	if (pi->Flags & oldfar::PPIF_USERDATA) {
		if (lua_isuserdata(L,-1))
			pi->UserData = (DWORD_PTR) lua_touserdata(L,-1);
	}
	else
		pi->UserData = (DWORD_PTR) lua_tointeger(L,-1);
	lua_pop(L,1);
}

//a table expected on Lua stack top
void PushAnsiFarFindData(lua_State *L, const oldfar::FAR_FIND_DATA *wfd)
{
	PutAttrToTable     (L,                       wfd->dwFileAttributes);
	PutNumToTable      (L, "FileSize",           (double)wfd->nFileSize);
	PutNumToTable      (L, "PhysicalSize",       (double)wfd->nPhysicalSize);
	PutFileTimeToTable (L, "LastWriteTime",      wfd->ftLastWriteTime);
	PutFileTimeToTable (L, "LastAccessTime",     wfd->ftLastAccessTime);
	PutFileTimeToTable (L, "CreationTime",       wfd->ftCreationTime);
	PutStrToTable      (L, "FileName",           wfd->cFileName);
	PutNumToTable      (L, "UnixMode",           wfd->dwUnixMode);
}

void PushAnsiPanelItems(lua_State *L, oldfar::PluginPanelItem *Items, int ItemsNumber)
{
	lua_createtable(L, ItemsNumber, 0);
	for (int i=0; i<ItemsNumber; i++)
	{
		lua_createtable(L, 16, 0);
		{
			auto *PanelItem = Items+i;
			PushAnsiFarFindData(L, &PanelItem->FindData);

			PutNumToTable(L, "Flags", PanelItem->Flags);
			PutNumToTable(L, "NumberOfLinks", PanelItem->NumberOfLinks);

			if (PanelItem->Description)  PutStrToTable(L, "Description",  PanelItem->Description);
			if (PanelItem->Owner)        PutStrToTable(L, "Owner",  PanelItem->Owner);
			if (PanelItem->Group)        PutStrToTable(L, "Group",  PanelItem->Group);

			if (PanelItem->CustomColumnNumber > 0) {
				lua_createtable (L, PanelItem->CustomColumnNumber, 0);
				for (int j=0; j < PanelItem->CustomColumnNumber; j++)
					PutStrToArray(L, j+1, PanelItem->CustomColumnData[j]);
				lua_setfield(L, -2, "CustomColumnData");
			}

			if (PanelItem->Flags & oldfar::PPIF_USERDATA) {
				DWORD *structsize = (DWORD*)PanelItem->UserData;
				void *udata = lua_newuserdata(L, *structsize);
				memcpy(udata, structsize, *structsize);
				lua_setfield(L, -2, "UserData");
			}
			else
				PutNumToTable(L, "UserData", PanelItem->UserData);
		}
		lua_rawseti(L, -2, i+1);
	}
}

struct CommonParams
{
	HANDLE hPanel;
	HMODULE hModule;
	bool Ansi;
};

struct ModuleGuard
{
	HMODULE hModule;
	ModuleGuard(HMODULE hm) : hModule(hm) {}
	~ModuleGuard() { dlclose(hModule); }
};

bool GetCommonParams(lua_State *L, CommonParams *Params)
{
	HANDLE panHandle = OptHandle(L);   //1-st argument
	if (panHandle == NULL)
		return false;

	// get plugin handle (for obtaining dll name)
	PanelInfo panInfo;
	if (! (PSInfo.Control(panHandle,FCTL_GETPANELINFO,0,(LONG_PTR)&panInfo) && panInfo.OwnerHandle) )
		return false;

	// get panel handle (a parameter to dll function)
	PSInfo.Control(panHandle, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)&Params->hPanel);
	if (Params->hPanel == INVALID_HANDLE_VALUE)
		return false;

	size_t size = PSInfo.PluginsControlV3(panInfo.OwnerHandle, PCTL_GETPLUGININFORMATION, 0, NULL);
	if (size == 0)
		return false;

	FarGetPluginInformation *piInfo = (FarGetPluginInformation *) malloc(size);
	if (piInfo == NULL)
		return false;

	piInfo->StructSize = sizeof(*piInfo);
	if (0 == PSInfo.PluginsControlV3(panInfo.OwnerHandle, PCTL_GETPLUGININFORMATION, size, piInfo))
	{
		free(piInfo);
		return false;
	}

	Params->Ansi = piInfo->Flags & FPF_ANSI;
	push_utf8_string(L, piInfo->ModuleName, -1);
	Params->hModule = dlopen(lua_tostring(L,-1), RTLD_LAZY | RTLD_GLOBAL);
	lua_pop(L, 1);
	free(piInfo);

	return (Params->hModule != NULL);
}


// result = far.Host.GetFiles (handle, Items, Move, DestPath [, OpMode])
int _GetFilesW(lua_State *L, HANDLE hPanel, HMODULE hModule)
{
	typedef int (WINAPI * T_GetFilesW)(HANDLE, struct PluginPanelItem*, int, int, const wchar_t**, int);
	ModuleGuard mg(hModule);

	luaL_checktype(L, 2, LUA_TTABLE);  //2-nd argument
	size_t numLines = lua_objlen(L, 2);
	int Move = lua_toboolean(L, 3);    //3-rd argument
	const wchar_t* DestPath = check_utf8_string(L, 4, NULL); //4-th argument
	int OpMode = luaL_optinteger(L, 5, OPM_FIND|OPM_SILENT); //5-th argument

	T_GetFilesW getfilesW;
	if (NULL == (getfilesW = (T_GetFilesW) dlsym(hModule, "GetFilesW")))
		return FALSE;

	PluginPanelItem *ppi, *ppi_curr;
	ppi = ppi_curr = (PluginPanelItem*)malloc(sizeof(PluginPanelItem) * numLines);
	if (ppi == NULL)
		return FALSE;

	lua_newtable(L); //collector
	for (size_t i=1; i<=numLines; i++)
	{
		lua_pushinteger(L, i);
		lua_gettable(L, 2);
		if(lua_istable(L,-1))
			FillPluginPanelItem(L, ppi_curr++, 0);
		else
			i = numLines + 1; //break the loop
		lua_pop(L,1);
	}

	int ret = getfilesW(hPanel, ppi, ppi_curr-ppi, Move, &DestPath, OpMode);
	for (auto curr=ppi; curr<ppi_curr; curr++)
		free((void*)curr->CustomColumnData);
	free(ppi);
	return ret;
}


int _GetFilesA(lua_State *L, HANDLE hPanel, HMODULE hModule)
{
	typedef int (WINAPI * T_GetFilesA)(HANDLE, oldfar::PluginPanelItem*, int, int, char*, int);
	ModuleGuard mg(hModule);

	luaL_checktype(L, 2, LUA_TTABLE);  //2-nd argument
	size_t numLines = lua_objlen(L, 2);
	int Move = lua_toboolean(L, 3);    //3-rd argument

	const char* DestPathIn = luaL_checkstring(L, 4); //4-th argument
	char DestPathOut[MAX_PATH];
	strncpy(DestPathOut, DestPathIn, MAX_PATH-1);
	DestPathOut[MAX_PATH-1] = 0;

	int OpMode = luaL_optinteger(L, 5, oldfar::OPM_FIND|oldfar::OPM_SILENT); //5-th argument

	T_GetFilesA getfilesA;
	if (NULL == (getfilesA = (T_GetFilesA) dlsym(hModule, "GetFiles")))
		return FALSE;

	oldfar::PluginPanelItem *ppi, *ppi_curr;
	ppi = ppi_curr = (oldfar::PluginPanelItem*)malloc(sizeof(oldfar::PluginPanelItem) * numLines);
	if (ppi == NULL)
		return FALSE;

	// collector table not needed for the ANSI variant
	for (size_t i=1; i<=numLines; i++)
	{
		lua_pushinteger(L, i);
		lua_gettable(L, 2);
		if(lua_istable(L,-1))
			FillAnsiPluginPanelItem(L, ppi_curr++);
		else
			i = numLines + 1; //break the loop
		lua_pop(L,1);
	}

	int ret = getfilesA(hPanel, ppi, ppi_curr-ppi, Move, DestPathOut, OpMode);
	for (auto curr=ppi; curr<ppi_curr; curr++)
		free((void*)curr->CustomColumnData);
	free(ppi);
	return ret;
}

int _SetDirectoryW(lua_State *L, HANDLE hPanel, HMODULE hModule)
{
	typedef int (WINAPI * T_SetDirectoryW)(HANDLE, const wchar_t*, int);
	ModuleGuard mg(hModule);

	const wchar_t *dir_name = check_utf8_string(L, 2, NULL); //2-nd argument
	int OpMode = luaL_optinteger(L, 3, OPM_FIND | OPM_SILENT); //3-rd argument

	T_SetDirectoryW setDirectoryW;
	if (NULL == (setDirectoryW = (T_SetDirectoryW) dlsym(hModule, "SetDirectoryW")))
		return FALSE;

	return setDirectoryW(hPanel, dir_name, OpMode);
}

int _SetDirectoryA(lua_State *L, HANDLE hPanel, HMODULE hModule)
{
	typedef int (WINAPI * T_SetDirectoryA)(HANDLE, const char*, int);
	ModuleGuard mg(hModule);

	const char *dir_name = luaL_checkstring(L, 2); //2-nd argument
	int OpMode = luaL_optinteger(L, 3, oldfar::OPM_FIND | oldfar::OPM_SILENT); //3-rd argument

	T_SetDirectoryA setDirectoryA;
	if (NULL == (setDirectoryA = (T_SetDirectoryA) dlsym(hModule, "SetDirectory")))
		return FALSE;

	return setDirectoryA(hPanel, dir_name, OpMode);
}

int _GetFindDataW(lua_State *L, HANDLE hPanel, HMODULE hModule)
{
	typedef int  (WINAPI * T_GetFindDataW) (HANDLE, PluginPanelItem**, int*, int);
	typedef void (WINAPI * T_FreeFindDataW)(HANDLE, PluginPanelItem*, int);
	ModuleGuard mg(hModule);

	T_GetFindDataW getFindDataW;
	if (NULL == (getFindDataW = (T_GetFindDataW) dlsym(hModule, "GetFindDataW")))
		return FALSE;

	PluginPanelItem *PanelItems;
	int ItemsNumber;
	int OpMode = luaL_optinteger(L, 2, OPM_FIND | OPM_SILENT); //2-nd argument

	if (0 == getFindDataW(hPanel, &PanelItems, &ItemsNumber, OpMode))
		return FALSE;

	PushPanelItems(L, INVALID_HANDLE_VALUE, PanelItems, ItemsNumber);

	T_FreeFindDataW freeFindDataW = (T_FreeFindDataW) dlsym(hModule, "FreeFindDataW");
	if (freeFindDataW)
		freeFindDataW(hPanel, PanelItems, ItemsNumber);

	return TRUE;
}

int _GetFindDataA(lua_State *L, HANDLE hPanel, HMODULE hModule)
{
	typedef int  (WINAPI * T_GetFindDataA) (HANDLE, oldfar::PluginPanelItem**, int*, int);
	typedef void (WINAPI * T_FreeFindDataA)(HANDLE, oldfar::PluginPanelItem*, int);
	ModuleGuard mg(hModule);

	T_GetFindDataA getFindDataA;
	if (NULL == (getFindDataA = (T_GetFindDataA) dlsym(hModule, "GetFindData")))
		return FALSE;

	oldfar::PluginPanelItem *PanelItems;
	int ItemsNumber;
	int OpMode = luaL_optinteger(L, 2, oldfar::OPM_FIND | oldfar::OPM_SILENT); //2-nd argument

	if (0 == getFindDataA(hPanel, &PanelItems, &ItemsNumber, OpMode))
		return FALSE;

	PushAnsiPanelItems(L, PanelItems, ItemsNumber);

	T_FreeFindDataA freeFindDataA = (T_FreeFindDataA) dlsym(hModule, "FreeFindData");
	if (freeFindDataA)
		freeFindDataA(hPanel, PanelItems, ItemsNumber);

	return TRUE;
}

extern "C" int far_host_GetFiles(lua_State *L)
{
	CommonParams CP;
	if (GetCommonParams(L, &CP))
		lua_pushinteger(L, (CP.Ansi ? _GetFilesA : _GetFilesW)(L, CP.hPanel, CP.hModule));
	else
		lua_pushinteger(L,0);

	return 1;
}

extern "C" int far_host_SetDirectory(lua_State *L)
{
	CommonParams CP;
	if (GetCommonParams(L, &CP))
		lua_pushboolean(L, (CP.Ansi ? _SetDirectoryA : _SetDirectoryW)(L, CP.hPanel, CP.hModule));
	else
		lua_pushboolean(L,0);

	return 1;
}

extern "C" int far_host_GetFindData(lua_State *L)
{
	CommonParams CP;
	if (GetCommonParams(L, &CP)) {
		if ((CP.Ansi ? _GetFindDataA : _GetFindDataW)(L, CP.hPanel, CP.hModule))
			return 1;
	}
	lua_pushnil(L);
	return 1;
}

const luaL_Reg far_host_funcs[] =
{
	{"GetFiles",      far_host_GetFiles},
//  {"PutFiles",      far_host_PutFiles},
	{"GetFindData",   far_host_GetFindData},
	{"SetDirectory",  far_host_SetDirectory},

	{NULL, NULL}
};

extern "C" int luaopen_far_host(lua_State *L)
{
	lua_newtable(L);
	luaL_register(L, NULL, far_host_funcs);
	return 1;
}
