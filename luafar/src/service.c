//coding: utf-8
//---------------------------------------------------------------------------

#include <windows.h>
#include <dlfcn.h> //dlopen
#include <ctype.h>
#include <math.h>
#include "luafar.h"
#include "util.h"
#include "ustring.h"
#include "version.h"

extern int  luaopen_bit64 (lua_State *L);
extern int  luaopen_unicode (lua_State *L);
extern int  luaopen_utf8 (lua_State *L);
extern int  luaopen_timer (lua_State *L);
extern int  luaopen_usercontrol (lua_State *L);
extern int  luaopen_regex (lua_State*);
extern int  pcall_msg (lua_State* L, int narg, int nret);
extern void add_flags (lua_State *L);
extern void add_colors (lua_State *L);
extern void add_keys (lua_State *L);
extern void PushPluginTable(lua_State* L, HANDLE hPlugin);
extern int  far_MacroCallFar(lua_State *L);
extern int  far_FarMacroCallToLua(lua_State *L);
extern int  bit64_push(lua_State *L, INT64 v);
extern int  bit64_getvalue(lua_State *L, int pos, INT64 *target);
extern void PackMacroValues(lua_State* L, size_t Count, const struct FarMacroValue* Values);

#ifndef ARRAYSIZE
#  define ARRAYSIZE(buff) (sizeof(buff)/sizeof(buff[0]))
#endif

const char FarFileFilterType[] = "FarFileFilter";
const char FarDialogType[]     = "FarDialog";
const char AddMacroDataType[]  = "FarAddMacroData";
const char FAR_KEYINFO[]       = "far.info";
const char FAR_VIRTUALKEYS[]   = "far.virtualkeys";
const char FAR_DN_STORAGE[]    = "FAR_DN_STORAGE";

const char* VirtualKeyStrings[256] = {
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

const char far_Guids[] = "far.Guids = {"
//  "AdvancedConfigId                 = 'A204FF09-07FA-478C-98C9-E56F61377BDE';"
    "ApplyCommandId                   = '044EF83E-8146-41B2-97F0-404C2F4C7B69';"
    "AskInsertMenuOrCommandId         = '57209AD5-51F6-4257-BAB6-837462BBCE74';"
//  "BadEditorCodePageId              = '4811039D-03A3-4F15-8D7A-8EBC4BCC97F9';"
//  "CannotRecycleFileId              = '52CEB5A5-06FA-43DD-B37C-239C02652C99';"
//  "CannotRecycleFolderId            = 'BBD9B7AE-9F6B-4444-89BF-C6124A5A83A4';"
//  "ChangeDiskMenuId                 = '252CE4A3-C415-4B19-956B-83E2FDD85960';"
//  "ChangeDriveCannotReadDiskErrorId = 'F3D46DC3-380B-4264-8BF8-10B05B897A5E';"
    "ChangeDriveModeId                = 'F87F9351-6A80-4872-BEEE-96EF80C809FB';"
    "CodePagesMenuId                  = '78A4A4E3-C2F0-40BD-9AA7-EAAC11836631';"
    "CopyCurrentOnlyFileId            = '502D00DF-EE31-41CF-9028-442D2E352990';"
    "CopyFilesId                      = 'FCEF11C4-5490-451D-8B4A-62FA03F52759';"
    "CopyOverwriteId                  = '9FBCB7E1-ACA2-475D-B40D-0F7365B632FF';"
    "CopyReadOnlyId                   = '879A8DE6-3108-4BEB-80DE-6F264991CE98';"
//  "DeleteAskDeleteROId              = '8D4E84B3-08F6-47DF-8C40-7130CD31D0E6';"
//  "DeleteAskWipeROId                = '6792A975-57C5-4110-8129-2D8045120964';"
//  "DeleteFileFolderId               = '6EF09401-6FE1-495A-8539-61B0F761408E';"
//  "DeleteFolderId                   = '4E714029-11BF-476F-9B17-9E47AA0DA8EA';"
//  "DeleteFolderRecycleId            = 'A318CBDC-DBA9-49E9-A248-E6A9FF8EC849';"
//  "DeleteLinkId                     = 'B1099BC3-14BD-4B22-87AC-44770D4189A3';"
//  "DeleteRecycleId                  = '85A5F779-A881-4B0B-ACEE-6D05653AE0EB';"
//  "DeleteWipeId                     = '9C054039-5C7E-4B04-96CD-3585228C916F';"
    "DescribeFileId                   = 'D8AF7A38-8357-44A5-A44B-A595CF707549';"
//  "DisconnectDriveId                = 'A1BDBEB1-2911-41FF-BC08-EEBC44040B50';"
    "EditAskSaveExtId                 = '40A699F1-BBDD-4E21-A137-97FFF798B0C8';"
    "EditAskSaveId                    = 'F776FEC0-50F7-4E7E-BDA6-2A63F84A957B';"
    "EditorAskOverwriteId             = '4109C8B3-760D-4011-B1D5-14C36763B23E';"
    "EditorCanNotEditDirectoryId      = 'CCA2C4D0-8705-4FA1-9B10-C9E3C8F37A65';"
    "EditorFileGetSizeErrorId         = '6AD4B317-C1ED-44C8-A76A-9146CA8AF984';"
    "EditorFileLongId                 = 'E3AFCD2D-BDE5-4E92-82B6-87C6A7B78FB6';"
//  "EditorFindAllListId              = '9BD3E306-EFB8-4113-8405-E7BADE8F0A59';"
    "EditorOpenRSHId                  = 'D8AA706F-DA7E-4BBF-AB78-6B7BDB49E006';"
    "EditorReloadId                   = 'AFDAD388-494C-41E8-BAC6-BBE9115E1CC0';"
//  "EditorReloadModalId              = 'D6F557E8-7E89-4895-BD75-4D3F2C30E382';"
    "EditorReplaceId                  = '8BCCDFFD-3B34-49F8-87CD-F4D885B75873';"
    "EditorSavedROId                  = '3F9311F5-3CA3-4169-A41C-89C76B3A8C1D';"
    "EditorSaveExitDeletedId          = '2D71DCCE-F0B8-4E29-A3A9-1F6D8C1128C2';"
    "EditorSaveF6DeletedId            = '85532BD5-1583-456D-A810-41AB345995A9';"
    "EditorSearchId                   = '5D3CBA90-F32D-433C-B016-9BB4AF96FACC';"
//  "EditorSwitchUnicodeCPDisabledId  = '15568DC5-4D6B-4C60-B43D-2040EE39871A';"
    "EditUserMenuId                   = '73BC6E3E-4CC3-4FE3-8709-545FF72B49B4';"
//  "EjectHotPlugMediaErrorId         = 'D6DC3621-877E-4BE2-80CC-BDB2864CE038';"
    "FarAskQuitId                     = '72E6E6D8-0BC6-4265-B9C4-C8DB712136AF';"
    "FileAssocMenuId                  = 'F6D2437C-FEDC-4075-AA56-275666FC8979';"
    "FileAssocModifyId                = '6F245B1A-47D9-41A6-AF3F-FA2C8DBEEBD0';"
    "FileAttrDlgId                    = '80695D20-1085-44D6-8061-F3C41AB5569C';"
    "FileOpenCreateId                 = '1D07CEE2-8F4F-480A-BE93-069B4FF59A2B';"
    "FileSaveAsId                     = '9162F965-78B8-4476-98AC-D699E5B6AFE7';"
    "FiltersConfigId                  = 'EDDB9286-3B08-4593-8F7F-E5925A3A0FF8';"
    "FiltersMenuId                    = '5B87B32E-494A-4982-AF55-DAFFCD251383';"
    "FindFileId                       = '8C9EAD29-910F-4B24-A669-EDAFBA6ED964';"
    "FindFileResultId                 = '536754EB-C2D1-4626-933F-A25D1E1D110A';"
    "FolderShortcutsDlgId             = 'DC8D98AC-475C-4F37-AB1D-45765EF06269';"
    "FolderShortcutsId                = '4CD742BC-295F-4AFA-A158-7AA05A16BEA1';"
//  "FolderShortcutsMoreId            = '601DD149-92FA-4601-B489-74C981BC8E38';"
    "GetNameAndPasswordId             = 'CD2AC546-9E4F-4445-A258-AB5F7A7800E0';"
    "HardSymLinkId                    = '5EB266F4-980D-46AF-B3D2-2C50E64BCA81';"
    "HelpSearchId                     = 'F63B558F-9185-46BA-8701-D143B8F62658';"
    "HighlightConfigId                = '51B6E342-B499-464D-978C-029F18ECCE59';"
    "HighlightMenuId                  = 'D0422DF0-AAF5-46E0-B98B-1776B427E70D';"
    "HistoryCmdId                     = '880968A6-6258-43E0-9BDC-F2B8678EC278';"
    "HistoryEditViewId                = 'E770E044-23A8-4F4D-B268-0E602B98CCF9';"
    "HistoryFolderId                  = 'FC3384A8-6608-4C9B-8D6B-EE105F4C5A54';"
    "MakeFolderId                     = 'FAD00DBE-3FFF-4095-9232-E1CC70C67737';"
    "MoveCurrentOnlyFileId            = '89664EF4-BB8C-4932-A8C0-59CAFD937ABA';"
    "MoveFilesId                      = '431A2F37-AC01-4ECD-BB6F-8CDE584E5A03';"
    "PanelViewModesEditId             = '98B75500-4A97-4299-BFAD-C3E349BF3674';"
    "PanelViewModesId                 = 'B56D5C08-0336-418B-A2A7-CF0C80F93ACC';"
//  "PluginInformationId              = 'FC4FD19A-43D2-4987-AC31-0F7A94901692';"
    "PluginsConfigMenuId              = 'B4C242E7-AA8E-4449-B0C3-BD8D9FA11AED';"
    "PluginsMenuId                    = '937F0B1C-7690-4F85-8469-AA935517F202';"
//  "RecycleFolderConfirmDeleteLinkId = '26A7AB9F-51F5-40F7-9061-1AE6E2FBD00A';"
//  "RemoteDisconnectDriveError1Id    = 'C9439386-9544-49BF-954B-6BEEDE7F1BD0';"
//  "RemoteDisconnectDriveError2Id    = 'F06953B8-25AA-4FC0-9899-422FC1D49F7A';"
    "ScreensSwitchId                  = '72EB948A-5F1D-4481-9A91-A4BFD869D127';"
    "SelectAssocMenuId                = 'D2BCB5A5-6B82-4EB5-B321-1AE7607A6236';"
    "SelectDialogId                   = '29C03C36-9C50-4F78-AB99-F5DC1A9C67CD';"
//  "SelectFromEditHistoryId          = '4406C688-209F-4378-8B7B-465BF16205FF';"
    "SelectSortModeId                 = 'B8B6E1DA-4221-47D2-AB2E-9EC67D0DC1E3';"
//  "SUBSTDisconnectDriveError1Id     = 'FF18299E-1881-42FA-AF7E-AC05D99F269C';"
//  "SUBSTDisconnectDriveError2Id     = '43B0FFC2-70BE-4289-91E6-FE9A3D54311B';"
//  "SUBSTDisconnectDriveId           = '75554EEB-A3A7-45FD-9795-4A85887A75A0';"
    "UnSelectDialogId                 = '34614DDB-2A22-4EA9-BD4A-2DC075643F1B';"
    "UserMenuUserInputId              = 'D2750B57-D3E6-42F4-8137-231C50DDC6E4';"
//  "VHDDisconnectDriveErrorId        = 'B890E6B0-05A9-4ED8-A4C3-BBC4D29DA3BE';"
//  "VHDDisconnectDriveId             = '629A8CA6-25C6-498C-B3DD-0E18D1CC0BCD';"
    "ViewerSearchId                   = '03B6C098-A3D6-4DFB-AED4-EB32D711D9AA';"
//  "WipeFolderId                     = 'E23BB390-036E-4A30-A9E6-DC621617C7F5';"
//  "WipeHardLinkId                   = '5297DDFE-0A37-4465-85EF-CBF9006D65C6';"
"}";

HANDLE OptHandlePos(lua_State *L, int pos)
{
  switch(lua_type(L,pos))
  {
    case LUA_TNUMBER:
    {
      lua_Integer whatPanel = lua_tointeger(L,pos);
      HANDLE hh = (HANDLE)whatPanel;
      return (hh==PANEL_PASSIVE || hh==PANEL_ACTIVE) ? hh : whatPanel%2 ? PANEL_ACTIVE:PANEL_PASSIVE;
    }
    case LUA_TLIGHTUSERDATA:
      return lua_touserdata(L,pos);
    default:
      luaL_typerror(L, pos, "integer or light userdata");
      return NULL;
  }
}

HANDLE OptHandle(lua_State *L)
{
  return OptHandlePos(L,1);
}

BOOL get_env_flag (lua_State *L, int stack_pos, int *trg)
{
  *trg = 0;
  int type = lua_type (L, stack_pos);
  if (type == LUA_TNUMBER) {
    *trg = lua_tointeger (L, stack_pos);
    return TRUE;
  }
  else if (type == LUA_TNONE || type == LUA_TNIL)
    return TRUE;
  if (type == LUA_TSTRING) {
    lua_getfield (L, LUA_ENVIRONINDEX, lua_tostring(L, stack_pos));
    if (lua_isnumber(L, -1)) {
      *trg = lua_tointeger (L, -1);
      lua_pop (L, 1);
      return TRUE;
    }
    lua_pop (L, 1);
  }
  return FALSE;
}

int check_env_flag (lua_State *L, int stack_pos)
{
  int trg;
  if (lua_isnoneornil(L, stack_pos) || !get_env_flag (L, stack_pos, &trg))
    luaL_argerror(L, stack_pos, "invalid flag");
  return trg;
}

int opt_env_flag (lua_State *L, int stack_pos, int dflt)
{
  int trg = dflt;
  if (!lua_isnoneornil(L, stack_pos) && !get_env_flag (L, stack_pos, &trg))
    luaL_argerror(L, stack_pos, "invalid flag");
  return trg;
}

BOOL GetFlagCombination (lua_State *L, int stack_pos, int *trg)
{
  *trg = 0;
  int type = lua_type (L, stack_pos);
  if (type == LUA_TNUMBER) {
    *trg = lua_tointeger (L, stack_pos);
    return TRUE;
  }
  if (type == LUA_TNONE || type == LUA_TNIL)
    return TRUE;
  if (type == LUA_TSTRING)
    return get_env_flag (L, stack_pos, trg);
  if (type == LUA_TTABLE) {
    stack_pos = abs_index (L, stack_pos);
    lua_pushnil(L);
    while (lua_next(L, stack_pos)) {
      if (lua_type(L,-2)==LUA_TSTRING && lua_toboolean(L,-1)) {
        int flag;
        if (get_env_flag (L, -2, &flag))
          *trg |= flag;
        else
          { lua_pop(L,2); return FALSE; }
      }
      lua_pop(L, 1);
    }
    return TRUE;
  }
  return FALSE;
}

int CheckFlags(lua_State* L, int stackpos)
{
  int Flags;
  if (!GetFlagCombination (L, stackpos, &Flags))
    luaL_error(L, "invalid flag combination");
  return Flags;
}

int OptFlags(lua_State* L, int pos, int dflt)
{
  return lua_isnoneornil(L, pos) ? dflt : CheckFlags(L, pos);
}

int CheckFlagsFromTable(lua_State *L, int pos, const char* key)
{
  int f = 0;
  lua_getfield(L, pos, key);
  if (!lua_isnil(L, -1))
    f = CheckFlags(L, -1);
  lua_pop(L, 1);
  return f;
}

int GetFlagsFromTable(lua_State *L, int pos, const char* key)
{
  int f;
  lua_getfield(L, pos, key);
  GetFlagCombination(L, -1, &f);
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

PSInfo* GetPluginStartupInfo(lua_State* L)
{
  TPluginData* pd = GetPluginData(L);
  return pd->Info;
}

struct FarStandardFunctions* GetFSF(lua_State* L)
{
  TPluginData* pd = GetPluginData(L);
  return pd->Info->FSF;
}

void uuid_to_guid(const char *uuid, GUID *guid)
{
  //copy field-wise because uuid_t is always 16 bytes while GUID may be more than that
  memcpy(&guid->Data1, uuid+0, 4);
  memcpy(&guid->Data2, uuid+4, 2);
  memcpy(&guid->Data3, uuid+6, 2);
  memcpy( guid->Data4, uuid+8, 8);
}

void guid_to_uuid(const GUID *guid, char *uuid)
{
  //copy field-wise because uuid_t is always 16 bytes while GUID may be more than that
  memcpy(uuid+0, &guid->Data1, 4);
  memcpy(uuid+4, &guid->Data2, 2);
  memcpy(uuid+6, &guid->Data3, 2);
  memcpy(uuid+8,  guid->Data4, 8);
}

int far_GetFileOwner (lua_State *L)
{
  wchar_t Owner[512];
  const wchar_t *Computer = opt_utf8_string (L, 1, NULL);
  const wchar_t *Name = check_utf8_string (L, 2, NULL);
  if (GetFSF(L)->GetFileOwner (Computer, Name, Owner, ARRAYSIZE(Owner))) {
    push_utf8_string(L, Owner, -1);
    return 1;
  }
  return 0;
}

int far_GetNumberOfLinks (lua_State *L)
{
  const wchar_t *Name = check_utf8_string (L, 1, NULL);
  int num = GetFSF(L)->GetNumberOfLinks (Name);
  return lua_pushinteger (L, num), 1;
}

int far_GetFileFormat (lua_State *L)
{
  int codepage;
  struct FarStandardFunctions* FSF = GetFSF(L);
  if (FSF->StructSize <= offsetof(struct FarStandardFunctions, GetFileFormat))
    luaL_error(L, "This version of FAR doesn't support FSF.GetFileFormat()");
  codepage = FSF->GetFileFormat(check_utf8_string(L,1,NULL));
  if (codepage)
    lua_pushinteger(L, codepage);
  else
    lua_pushnil(L);
  return 1;
}

int far_LuafarVersion (lua_State *L)
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

void GetMouseEvent(lua_State *L, MOUSE_EVENT_RECORD* rec)
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
void PushFarFindData(lua_State *L, const struct FAR_FIND_DATA *wfd)
{
  PutAttrToTable     (L,                       wfd->dwFileAttributes);
  PutNumToTable      (L, "FileSize",           (double)wfd->nFileSize);
  PutFileTimeToTable (L, "LastWriteTime",      wfd->ftLastWriteTime);
  PutFileTimeToTable (L, "LastAccessTime",     wfd->ftLastAccessTime);
  PutFileTimeToTable (L, "CreationTime",       wfd->ftCreationTime);
  PutWStrToTable     (L, "FileName",           wfd->lpwszFileName, -1);
}

// on entry : the table's on the stack top
// on exit  : 2 strings added to the stack top (don't pop them!)
void GetFarFindData(lua_State *L, struct FAR_FIND_DATA *wfd)
{
  memset(wfd, 0, sizeof(*wfd));

  wfd->dwFileAttributes = GetAttrFromTable(L);
  wfd->nFileSize = GetFileSizeFromTable(L, "FileSize");
  wfd->ftLastWriteTime  = GetFileTimeFromTable(L, "LastWriteTime");
  wfd->ftLastAccessTime = GetFileTimeFromTable(L, "LastAccessTime");
  wfd->ftCreationTime   = GetFileTimeFromTable(L, "CreationTime");

  lua_getfield(L, -1, "FileName"); // +1
  wfd->lpwszFileName = opt_utf8_string(L, -1, L""); // +1
}
//---------------------------------------------------------------------------

void PushWinFindData (lua_State *L, const WIN32_FIND_DATAW *FData)
{
  lua_createtable(L, 0, 7);
  PutAttrToTable(L,                          FData->dwFileAttributes);
  PutNumToTable(L, "FileSize",               FData->nFileSize);
  PutFileTimeToTable(L, "LastWriteTime",     FData->ftLastWriteTime);
  PutFileTimeToTable(L, "LastAccessTime",    FData->ftLastAccessTime);
  PutFileTimeToTable(L, "CreationTime",      FData->ftCreationTime);
  PutWStrToTable(L, "FileName",              FData->cFileName, -1);
}

void PushOptPluginTable(lua_State *L, HANDLE handle, PSInfo *Info)
{
  HANDLE plug_handle = handle;
  if (handle == PANEL_ACTIVE || handle == PANEL_PASSIVE)
    Info->Control(handle, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)&plug_handle);
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
  if (PanelItem->Description)
    PutWStrToTable(L, "Description",  PanelItem->Description, -1);
  if (PanelItem->Owner)
    PutWStrToTable(L, "Owner",  PanelItem->Owner, -1);

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
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_createtable(L, ItemsNumber, 0);    //+1 "PanelItems"
  PushOptPluginTable(L, handle, Info);   //+2
  for(i=0; i < ItemsNumber; i++) {
    PushPanelItem (L, PanelItems + i);
    lua_rawseti(L, -3, i+1);
  }
  lua_pop(L, 1);                         //+1
}
//---------------------------------------------------------------------------

int far_PluginStartupInfo(lua_State *L)
{
  const wchar_t *slash;
  TPluginData *pd = GetPluginData(L);
  lua_createtable(L, 0, 3);
  PutWStrToTable(L, "ModuleName", pd->Info->ModuleName, -1);

  slash = wcsrchr(pd->Info->ModuleName, L'/');
  if (slash)
    PutWStrToTable(L, "ModuleDir", pd->Info->ModuleName, slash + 1 - pd->Info->ModuleName);

  lua_pushlightuserdata(L, (void*)pd->Info->ModuleNumber);
  lua_setfield(L, -2, "ModuleNumber");

  PutWStrToTable(L, "RootKey", pd->Info->RootKey, -1);

  lua_pushinteger(L, pd->PluginId);
  lua_setfield(L, -2, "PluginId");

  PutStrToTable(L, "ShareDir", pd->ShareDir);
  return 1;
}

int far_GetPluginId(lua_State *L)
{
  lua_pushinteger(L, GetPluginData(L)->PluginId);
  return 1;
}

int far_GetCurrentDirectory (lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int size = Info->FSF->GetCurrentDirectory(0, NULL);
  wchar_t* buf = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
  Info->FSF->GetCurrentDirectory(size, buf);
  push_utf8_string(L, buf, -1);
  return 1;
}

int push_editor_filename(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int size = Info->EditorControl(ECTL_GETFILENAME, 0);
  if (!size) return 0;

  wchar_t* fname = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
  if (Info->EditorControl(ECTL_GETFILENAME, fname)) {
    push_utf8_string(L, fname, -1);
    lua_remove(L, -2);
    return 1;
  }
  lua_pop(L,1);
  return 0;
}

int editor_GetFileName(lua_State *L) {
  if (!push_editor_filename(L)) lua_pushnil(L);
  return 1;
}

int editor_GetTitle(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int size = Info->EditorControl(ECTL_GETTITLE, NULL);
  lua_pushstring(L, "");
  if (size) {
    void* str = lua_newuserdata(L, size * sizeof(wchar_t));
    if (Info->EditorControl(ECTL_GETTITLE, str))
      push_utf8_string(L, (wchar_t*)str, -1);
  }
  return 1;
}

int editor_GetInfo(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct EditorInfo ei;
  if (!Info->EditorControl(ECTL_GETINFO, &ei))
    return lua_pushnil(L), 1;
  lua_createtable(L, 0, 18);
  PutNumToTable(L, "EditorID", ei.EditorID);

  if (push_editor_filename(L))
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
  PutNumToTable(L, "BookMarkCount", ei.BookMarkCount);
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
BOOL FastGetString(int string_num, struct EditorGetString *egs, PSInfo *Info)
{
  struct EditorSetPosition esp;
  esp.CurLine   = string_num;
  esp.CurPos    = -1;
  esp.CurTabPos = -1;
  esp.TopScreenLine = -1;
  esp.LeftPos   = -1;
  esp.Overtype  = -1;

  if(!Info->EditorControl(ECTL_SETPOSITION, &esp))
    return FALSE;

  egs->StringNumber = string_num;
  return Info->EditorControl(ECTL_GETSTRING, egs) != 0;
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
  PSInfo *Info = GetPluginData(L)->Info;
  intptr_t line_num = luaL_optinteger(L, 1, 0) - 1;
  intptr_t mode = luaL_optinteger(L, 2, 0);
  BOOL res = 0;
  struct EditorGetString egs;

  if(mode == 0 || mode == 3)
  {
    egs.StringNumber = line_num;
    res = Info->EditorControl(ECTL_GETSTRING, &egs) != 0;
  }
  else if(mode == 1 || mode == 2)
    res = FastGetString(line_num, &egs, Info);

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
  PSInfo *Info = GetPluginStartupInfo(L);
  struct EditorSetString ess;
  size_t len;
  ess.StringNumber = luaL_optinteger(L, 1, 0) - 1;

  if(is_wide)
  {
    ess.StringText = check_wcstring(L, 2, &len);
    ess.StringEOL = opt_wcstring(L, 3, NULL);

    if(ess.StringEOL)
    {
      lua_pushvalue(L, 3);
      lua_pushliteral(L, "\0\0\0\0");
      lua_concat(L, 2);
      ess.StringEOL = (wchar_t*) lua_tostring(L, -1);
    }
  }
  else
  {
    ess.StringText = check_utf8_string(L, 2, &len);
    ess.StringEOL = opt_utf8_string(L, 3, NULL);
  }

  ess.StringLength = len;
  lua_pushboolean(L, Info->EditorControl(ECTL_SETSTRING, &ess) != 0);
  return 1;
}

static int editor_SetString(lua_State *L) { return _EditorSetString(L, 0); }
static int editor_SetStringW(lua_State *L) { return _EditorSetString(L, 1); }

int editor_InsertString(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int indent = lua_toboolean(L, 1);
  lua_pushboolean(L, Info->EditorControl(ECTL_INSERTSTRING, &indent));
  return 1;
}

int editor_DeleteString(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushboolean(L, Info->EditorControl(ECTL_DELETESTRING, NULL));
  return 1;
}

int editor_InsertText(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  wchar_t* text = check_utf8_string(L,1,NULL);
  int res = Info->EditorControl(ECTL_INSERTTEXT, text);
  if (res && lua_toboolean(L,2))
    Info->EditorControl(ECTL_REDRAW, NULL);
  lua_pushboolean(L, res);
  return 1;
}

int editor_InsertTextW(lua_State *L)
{
  int res, redraw;
  PSInfo *Info = GetPluginStartupInfo(L);
  luaL_checkstring(L,1);
  redraw = lua_toboolean(L,2);
  lua_pushvalue(L,1);
  lua_pushlstring(L, "\0\0\0\0", 4);
  lua_concat(L,2);
  res = Info->EditorControl(ECTL_INSERTTEXT, (void*)lua_tostring(L,-1));
  if (res && redraw)
    Info->EditorControl(ECTL_REDRAW, NULL);
  lua_pushboolean(L, res);
  return 1;
}

int editor_DeleteChar(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushboolean(L, Info->EditorControl(ECTL_DELETECHAR, NULL));
  return 1;
}

int editor_DeleteBlock(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushboolean(L, Info->EditorControl(ECTL_DELETEBLOCK, NULL));
  return 1;
}

int editor_UndoRedo(lua_State *L)
{
  struct EditorUndoRedo eur;
  memset(&eur, 0, sizeof(eur));
  eur.Command = check_env_flag(L, 1);
  PSInfo *Info = GetPluginStartupInfo(L);
  return lua_pushboolean (L, Info->EditorControl(ECTL_UNDOREDO, &eur)), 1;
}

int SetKeyBar(lua_State *L, BOOL editor)
{
  void* param;
  struct KeyBarTitles kbt;
  PSInfo *Info = GetPluginStartupInfo(L);

  enum { REDRAW=-1, RESTORE=0 }; // corresponds to FAR API
  BOOL argfail = FALSE;
  if (lua_isstring(L,1)) {
    const char* p = lua_tostring(L,1);
    if (0 == strcmp("redraw", p)) param = (void*)REDRAW;
    else if (0 == strcmp("restore", p)) param = (void*)RESTORE;
    else argfail = TRUE;
  }
  else if (lua_istable(L,1)) {
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
    lua_settop(L, 1);
    lua_newtable(L);
    int store = 0;
    size_t i;
    int j;
    for (i=0; i < sizeof(pairs)/sizeof(pairs[0]); i++) {
      lua_getfield (L, 1, pairs[i].key);
      if (lua_istable (L, -1)) {
        for (j=0; j<12; j++) {
          lua_pushinteger(L,j+1);
          lua_gettable(L,-2);
          if (lua_isstring(L,-1))
            pairs[i].trg[j] = (wchar_t*)StoreTempString(L, 2, &store);
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
    return luaL_argerror(L, 1, "must be 'redraw', 'restore', or table");

  int result = editor ? Info->EditorControl(ECTL_SETKEYBAR, param) :
                        Info->ViewerControl(VCTL_SETKEYBAR, param);
  lua_pushboolean(L, result);
  return 1;
}

int editor_SetKeyBar(lua_State *L)
{
  return SetKeyBar(L, TRUE);
}

int viewer_SetKeyBar(lua_State *L)
{
  return SetKeyBar(L, FALSE);
}

int editor_SetParam(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct EditorSetParameter esp;
  memset(&esp, 0, sizeof(esp));
  wchar_t buf[256];
  esp.Type = check_env_flag(L,1);
  //-----------------------------------------------------
  int tp = lua_type(L,2);
  if (tp == LUA_TNUMBER)
    esp.Param.iParam = lua_tointeger(L,2);
  else if (tp == LUA_TBOOLEAN)
    esp.Param.iParam = lua_toboolean(L,2);
  else if (tp == LUA_TSTRING)
    esp.Param.wszParam = (wchar_t*)check_utf8_string(L,2,NULL);
  //-----------------------------------------------------
  if(esp.Type == ESPT_GETWORDDIV) {
    esp.Param.wszParam = buf;
    esp.Size = ARRAYSIZE(buf);
  }
  //-----------------------------------------------------
  int f;
  GetFlagCombination (L, 3, &f);
  esp.Flags = f;
  //-----------------------------------------------------
  int result = Info->EditorControl(ECTL_SETPARAM, &esp);
  lua_pushboolean(L, result);
  if(result && esp.Type == ESPT_GETWORDDIV) {
    push_utf8_string(L,buf,-1); return 2;
  }
  return 1;
}

int editor_SetPosition(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct EditorSetPosition esp;
  if (lua_istable(L, 1)) {
    lua_settop(L, 1);
    FillEditorSetPosition(L, &esp);
  }
  else {
    esp.CurLine   = luaL_optinteger(L, 1, 0) - 1;
    esp.CurPos    = luaL_optinteger(L, 2, 0) - 1;
    esp.CurTabPos = luaL_optinteger(L, 3, 0) - 1;
    esp.TopScreenLine = luaL_optinteger(L, 4, 0) - 1;
    esp.LeftPos   = luaL_optinteger(L, 5, 0) - 1;
    esp.Overtype  = luaL_optinteger(L, 6, -1);
  }
  lua_pushboolean(L, Info->EditorControl(ECTL_SETPOSITION, &esp));
  return 1;
}

int editor_Redraw(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushboolean(L, Info->EditorControl(ECTL_REDRAW, NULL));
  return 1;
}

int editor_ExpandTabs(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int line_num = luaL_optinteger(L, 1, 0) - 1;
  lua_pushboolean(L, Info->EditorControl(ECTL_EXPANDTABS, &line_num));
  return 1;
}

int PushBookmarks(lua_State *L, int count, int command)
{
  if (count > 0) {
    struct EditorBookMarks ebm;
    ebm.Line = (long*)lua_newuserdata(L, 4 * count * sizeof(long));
    ebm.Cursor     = ebm.Line + count;
    ebm.ScreenLine = ebm.Cursor + count;
    ebm.LeftPos    = ebm.ScreenLine + count;
    PSInfo *Info = GetPluginStartupInfo(L);
    if (Info->EditorControl(command, &ebm)) {
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

int editor_GetBookmarks(lua_State *L)
{
  struct EditorInfo ei;
  PSInfo *Info = GetPluginStartupInfo(L);
  if (!Info->EditorControl(ECTL_GETINFO, &ei))
    return 0;
  return PushBookmarks(L, ei.BookMarkCount, ECTL_GETBOOKMARKS);
}

int editor_GetStackBookmarks(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int count = Info->EditorControl(ECTL_GETSTACKBOOKMARKS, NULL);
  return PushBookmarks(L, count, ECTL_GETSTACKBOOKMARKS);
}

int editor_AddStackBookmark(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushboolean(L, Info->EditorControl(ECTL_ADDSTACKBOOKMARK, NULL));
  return 1;
}

int editor_ClearStackBookmarks(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushinteger(L, Info->EditorControl(ECTL_CLEARSTACKBOOKMARKS, NULL));
  return 1;
}

int editor_DeleteStackBookmark(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  INT_PTR num = luaL_optinteger(L, 1, 0) - 1;
  lua_pushboolean(L, Info->EditorControl(ECTL_DELETESTACKBOOKMARK, (void*)num));
  return 1;
}

int editor_NextStackBookmark(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushboolean(L, Info->EditorControl(ECTL_NEXTSTACKBOOKMARK, NULL));
  return 1;
}

int editor_PrevStackBookmark(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushboolean(L, Info->EditorControl(ECTL_PREVSTACKBOOKMARK, NULL));
  return 1;
}

int editor_SetTitle(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  const wchar_t* text = opt_utf8_string(L, 1, NULL);
  lua_pushboolean(L, Info->EditorControl(ECTL_SETTITLE, (wchar_t*)text));
  return 1;
}

int editor_Quit(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushboolean(L, Info->EditorControl(ECTL_QUIT, NULL));
  return 1;
}

int SetEditorSelect(lua_State *L, int pos_table, struct EditorSelect *es)
{
  lua_getfield(L, pos_table, "BlockType");
  if (!get_env_flag(L, -1, &es->BlockType)) {
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

int editor_Select(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct EditorSelect es;
  int result;
  if (lua_istable(L, 1))
    result = SetEditorSelect(L, 1, &es);
  else {
    result = get_env_flag(L, 1, &es.BlockType);
    if (result) {
      es.BlockStartLine = luaL_optinteger(L, 2, 0) - 1;
      es.BlockStartPos  = luaL_optinteger(L, 3, 0) - 1;
      es.BlockWidth     = luaL_optinteger(L, 4, -1);
      es.BlockHeight    = luaL_optinteger(L, 5, -1);
    }
  }
  result = result && Info->EditorControl(ECTL_SELECT, &es);
  return lua_pushboolean(L, result), 1;
}

// This function is that long because FAR API does not supply needed
// information directly.
int editor_GetSelection(lua_State *L)
{
  int BlockStartPos, h, from, to;
  PSInfo *Info = GetPluginStartupInfo(L);
  struct EditorInfo EI;
  struct EditorGetString egs;
  struct EditorSetPosition esp;
  Info->EditorControl(ECTL_GETINFO, &EI);

  if(EI.BlockType == BTYPE_NONE || !FastGetString(EI.BlockStartLine, &egs, Info))
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
    if(!FastGetString(to, &egs, Info))
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

    if(!FastGetString(curr, &egs, Info))
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

  if(!FastGetString(from, &egs, Info))
    return lua_pushnil(L), 1;

  PutIntToTable(L, "EndLine", from+1);
  PutIntToTable(L, "EndPos", egs.SelEnd);
  // restore current position, since FastGetString() changed it
  esp.CurLine       = EI.CurLine;
  esp.CurPos        = EI.CurPos;
  esp.CurTabPos     = EI.CurTabPos;
  esp.TopScreenLine = EI.TopScreenLine;
  esp.LeftPos       = EI.LeftPos;
  esp.Overtype      = EI.Overtype;
  Info->EditorControl(ECTL_SETPOSITION, &esp);
  return 1;
}

int _EditorTabConvert(lua_State *L, int Operation)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct EditorConvertPos ecp;
  ecp.StringNumber = luaL_optinteger(L, 1, 0) - 1;
  ecp.SrcPos = luaL_checkinteger(L, 2) - 1;
  if (Info->EditorControl(Operation, &ecp))
    lua_pushinteger(L, ecp.DestPos+1);
  else
    lua_pushnil(L);
  return 1;
}

int editor_TabToReal(lua_State *L)
{
  return _EditorTabConvert(L, ECTL_TABTOREAL);
}

int editor_RealToTab(lua_State *L)
{
  return _EditorTabConvert(L, ECTL_REALTOTAB);
}

int editor_TurnOffMarkingBlock(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  Info->EditorControl(ECTL_TURNOFFMARKINGBLOCK, NULL);
  return 0;
}

int editor_AddColor(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct EditorColor ec;
  int Flags;
  ec.StringNumber = luaL_optinteger  (L, 1, 0) - 1;
  ec.StartPos     = luaL_checkinteger(L, 2) - 1;
  ec.EndPos       = luaL_checkinteger(L, 3) - 1;
  Flags           = CheckFlags       (L, 4);
  ec.Color        = (luaL_checkinteger(L, 5) & 0xFFFF) | (Flags & 0xFFFF0000);
  ec.ColorItem    = 0;
  lua_pushboolean(L, Info->EditorControl(ECTL_ADDCOLOR, &ec));
  return 1;
}

int editor_DelColor(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct EditorColor ec;
  memset(&ec, 0, sizeof(ec)); // set ec.Color = 0
  ec.StringNumber = luaL_optinteger  (L, 1, 0) - 1;
  ec.StartPos     = luaL_optinteger  (L, 2, 0) - 1;
  lua_pushboolean(L, Info->EditorControl(ECTL_ADDCOLOR, &ec)); // ECTL_ADDCOLOR (sic)
  return 1;
}

int editor_GetColor(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct EditorColor ec;
  memset(&ec, 0, sizeof(ec));
  ec.StringNumber = luaL_optinteger(L, 1, 0) - 1;
  ec.ColorItem    = luaL_checkinteger(L, 2) - 1;
  if (Info->EditorControl(ECTL_GETCOLOR, &ec))
  {
    lua_createtable(L, 0, 3);
    PutNumToTable(L, "StartPos", ec.StartPos+1);
    PutNumToTable(L, "EndPos", ec.EndPos+1);
    PutNumToTable(L, "Color", ec.Color);
  }
  else
    lua_pushnil(L);
  return 1;
}

int editor_SaveFile(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct EditorSaveFile esf;
  esf.FileName = opt_utf8_string(L, 1, L"");
  esf.FileEOL = opt_utf8_string(L, 2, NULL);
  esf.CodePage = luaL_optinteger(L, 3, 0);
  if (esf.CodePage == 0) {
    struct EditorInfo ei;
    if (Info->EditorControl(ECTL_GETINFO, &ei))
      esf.CodePage = ei.CodePage;
  }
  lua_pushboolean(L, Info->EditorControl(ECTL_SAVEFILE, &esf));
  return 1;
}

int editor_ReadInput(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  INPUT_RECORD ir;
  lua_pushnil(L); // prepare to return nil
  if (!Info->EditorControl(ECTL_READINPUT, &ir))
    return 1;
  lua_newtable(L);
  switch(ir.EventType) {
    case KEY_EVENT:
      PutStrToTable(L, "EventType", "KEY_EVENT");
      PutBoolToTable(L,"KeyDown", ir.Event.KeyEvent.bKeyDown);
      PutNumToTable(L, "RepeatCount", ir.Event.KeyEvent.wRepeatCount);
      PutNumToTable(L, "VirtualKeyCode", ir.Event.KeyEvent.wVirtualKeyCode);
      PutNumToTable(L, "VirtualScanCode", ir.Event.KeyEvent.wVirtualScanCode);
      PutWStrToTable(L, "UnicodeChar", &ir.Event.KeyEvent.uChar.UnicodeChar, 1);
      PutNumToTable(L, "AsciiChar", ir.Event.KeyEvent.uChar.AsciiChar);
      PutNumToTable(L, "ControlKeyState", ir.Event.KeyEvent.dwControlKeyState);
      break;

    case MOUSE_EVENT:
      PutStrToTable(L, "EventType", "MOUSE_EVENT");
      PutMouseEvent(L, &ir.Event.MouseEvent, TRUE);
      break;

    case WINDOW_BUFFER_SIZE_EVENT:
      PutStrToTable(L, "EventType", "WINDOW_BUFFER_SIZE_EVENT");
      PutNumToTable(L, "SizeX", ir.Event.WindowBufferSizeEvent.dwSize.X);
      PutNumToTable(L, "SizeY", ir.Event.WindowBufferSizeEvent.dwSize.Y);
      break;

    case MENU_EVENT:
      PutStrToTable(L, "EventType", "MENU_EVENT");
      PutNumToTable(L, "CommandId", ir.Event.MenuEvent.dwCommandId);
      break;

    case FOCUS_EVENT:
      PutStrToTable(L, "EventType", "FOCUS_EVENT");
      PutBoolToTable(L,"SetFocus", ir.Event.FocusEvent.bSetFocus);
      break;

    default:
      lua_pushnil(L);
  }
  return 1;
}

void FillInputRecord(lua_State *L, int pos, INPUT_RECORD *ir)
{
  int temp;
  size_t size;

  pos = abs_index(L, pos);
  luaL_checktype(L, pos, LUA_TTABLE);
  memset(ir, 0, sizeof(INPUT_RECORD));

  // determine event type
  lua_getfield(L, pos, "EventType");
  if(!get_env_flag(L, -1, &temp))
    luaL_argerror(L, pos, "EventType field is missing or invalid");
  lua_pop(L, 1);

  lua_pushvalue(L, pos);
  ir->EventType = temp;
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

int editor_ProcessInput(lua_State *L)
{
  if (!lua_istable(L, 1))
    return 0;
  PSInfo *Info = GetPluginStartupInfo(L);
  INPUT_RECORD ir;
  FillInputRecord(L, 1, &ir);
  if (Info->EditorControl(ECTL_PROCESSINPUT, &ir))
    return lua_pushboolean(L, 1), 1;
  return 0;
}

int editor_ProcessKey(lua_State *L)
{
  INT_PTR key = luaL_checkinteger(L,1);
  PSInfo *Info = GetPluginStartupInfo(L);
  Info->EditorControl(ECTL_PROCESSKEY, (void*)key);
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
int far_Menu(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int X = -1, Y = -1, MaxHeight = 0;
  int Flags;
  const wchar_t *Title = L"Menu", *Bottom = NULL, *HelpTopic = NULL;

  lua_settop (L, 3);    // cut unneeded parameters; make stack predictable
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checktype(L, 2, LUA_TTABLE);
  if (!lua_isnil(L,3) && !lua_istable(L,3))
    return luaL_argerror(L, 3, "must be table or nil");

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
  int NumBreakCodes = lua_istable(L,3) ? lua_objlen(L,3) : 0;
  if (NumBreakCodes) {
    int* BreakKeys = (int*)lua_newuserdata(L, (1+NumBreakCodes)*sizeof(int));
    // get virtualkeys table from the registry; push it on top
    lua_pushstring(L, FAR_VIRTUALKEYS);
    lua_rawget(L, LUA_REGISTRYINDEX);
    // push breakkeys table on top
    lua_pushvalue(L, 3);              // vk=-2; bk=-1;
    char buf[32];
    int ind, out; // used outside the following loop
    for(ind=0,out=0; ind < NumBreakCodes; ind++) {
      // get next break key (optional modifier plus virtual key)
      lua_pushinteger(L,ind+1);       // vk=-3; bk=-2;
      lua_gettable(L,-2);             // vk=-3; bk=-2;
      if(!lua_istable(L,-1))  { lua_pop(L,1); continue; }
      lua_getfield(L, -1, "BreakKey");// vk=-4; bk=-3;
      if(!lua_isstring(L,-1)) { lua_pop(L,2); continue; }
      // separate modifier and virtual key strings
      int mod = 0;
      const char* s = lua_tostring(L,-1);
      if(strlen(s) >= sizeof(buf)) { lua_pop(L,2); continue; }
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
      BreakKeys[out++] = lua_tointeger(L,-1) | mod;
      lua_pop(L,2);                   // vk=-2; bk=-1;
    }
    BreakKeys[out] = 0; // required by FAR API
    pBreakKeys = BreakKeys;
    pBreakCode = &BreakCode;
  }

  int ret = Info->Menu(
    Info->ModuleNumber, X, Y, MaxHeight, Flags|FMENU_USEEXT,
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
int LF_Message(PSInfo *Info,
               const wchar_t* aMsg,      // if multiline, then lines must be separated by '\n'
               const wchar_t* aTitle,
               const wchar_t* aButtons,  // if multiple, then captions must be separated by ';'
               const char*    aFlags,
               const wchar_t* aHelpTopic)
{
  const wchar_t **items, **pItems;
  wchar_t** allocLines;
  int nAlloc;
  wchar_t *lastDelim, *MsgCopy, *start, *pos;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  int ret = WINPORT(GetConsoleScreenBufferInfo)(NULL, &csbi);//GetStdHandle(STD_OUTPUT_HANDLE)
  const int max_len   = ret ? csbi.srWindow.Right - csbi.srWindow.Left+1-14 : 66;
  const int max_lines = ret ? csbi.srWindow.Bottom - csbi.srWindow.Top+1-5 : 20;
  int num_lines = 0, num_buttons = 0;
  UINT64 Flags = 0;
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

  ret = Info->Message(Info->ModuleNumber, Flags, aHelpTopic, items, 1+num_lines+num_buttons, num_buttons);
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
  PSInfo *Info = GetPluginStartupInfo(L);
  if (!aMsg) aMsg = L"<non-string error message>";
  lua_pushlstring(L, (void*)Info->ModuleName, sizeof(wchar_t) * wcslen(Info->ModuleName));
  lua_pushlstring(L, (void*)L":\n", sizeof(wchar_t) * 2);
  LF_Gsub(L, aMsg, L"\n\t", L"\n   ");
  lua_concat(L, 3);
  LF_Message(Info, (void*)lua_tostring(L,-1), L"Error", L"OK", "w", NULL);
  lua_pop(L, 1);
}

int SplitToTable(lua_State *L, const wchar_t *Text, wchar_t Delim, int StartIndex)
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
int far_Message(lua_State *L)
{
  luaL_checkany(L,1);
  lua_settop(L,5);
  const wchar_t *Msg = NULL;
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
  const wchar_t *Title   = opt_utf8_string(L, 2, L"Message");
  const wchar_t *Buttons = opt_utf8_string(L, 3, L";OK");
  const char    *Flags   = luaL_optstring(L, 4, "");
  const wchar_t *HelpTopic = opt_utf8_string(L, 5, NULL);

  PSInfo *Info = GetPluginStartupInfo(L);
  int ret = LF_Message(Info, Msg, Title, Buttons, Flags, HelpTopic);
  lua_pushinteger(L, ret<0 ? ret : ret+1);
  return 1;
}

int panel_CheckPanelsExist(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushboolean(L, (int)Info->Control(PANEL_ACTIVE, FCTL_CHECKPANELSEXIST, 0, 0));
  return 1;
}

int panel_ClosePlugin(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE handle = OptHandle(L);
  const wchar_t *dir = opt_utf8_string(L, 2, NULL);
  lua_pushboolean(L, Info->Control(handle, FCTL_CLOSEPLUGIN, 0, (LONG_PTR)dir));
  return 1;

}

int panel_GetPanelInfo(lua_State *L /*, BOOL ShortInfo*/)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE handle = OptHandle(L);
  HANDLE plug_handle;
  struct PanelInfo pi;
  if (!Info->Control(handle, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi))
    return lua_pushnil(L), 1;

  lua_createtable(L, 0, 14);
  //-------------------------------------------------------------------------
  Info->Control(handle, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)&plug_handle);
  if (plug_handle != INVALID_HANDLE_VALUE) {
    lua_pushlightuserdata(L, plug_handle);
    lua_setfield(L, -2, "PluginHandle");
  }
  //-------------------------------------------------------------------------
  PutIntToTable(L, "PanelType", pi.PanelType);
  PutBoolToTable(L, "Plugin", pi.Plugin != 0);
  //-------------------------------------------------------------------------
  lua_createtable(L, 0, 4); // "PanelRect"
  PutIntToTable(L, "left", pi.PanelRect.left);
  PutIntToTable(L, "top", pi.PanelRect.top);
  PutIntToTable(L, "right", pi.PanelRect.right);
  PutIntToTable(L, "bottom", pi.PanelRect.bottom);
  lua_setfield(L, -2, "PanelRect");
  //-------------------------------------------------------------------------
  PutIntToTable(L, "ItemsNumber", pi.ItemsNumber);
  PutIntToTable(L, "SelectedItemsNumber", pi.SelectedItemsNumber);
  //-------------------------------------------------------------------------
  PutIntToTable(L, "CurrentItem", pi.CurrentItem + 1);
  PutIntToTable(L, "TopPanelItem", pi.TopPanelItem + 1);
  PutBoolToTable(L, "Visible", pi.Visible);
  PutBoolToTable(L, "Focus", pi.Focus);
  PutIntToTable(L, "ViewMode", pi.ViewMode);
  PutIntToTable(L, "SortMode", pi.SortMode);
  PutIntToTable(L, "Flags", pi.Flags);
  //-------------------------------------------------------------------------
  return 1;
}

int get_panel_item(lua_State *L, int command)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE handle = OptHandle(L);
  int index = luaL_optinteger(L,2,1) - 1;
  if(index >= 0 || command == FCTL_GETCURRENTPANELITEM)
  {
    int size = Info->Control(handle, command, index, 0);
    if (size) {
      struct PluginPanelItem* item = (struct PluginPanelItem*)lua_newuserdata(L, size);
      if (Info->Control(handle, command, index, (LONG_PTR)item)) {
        PushOptPluginTable(L, handle, Info);
        PushPanelItem(L, item);
        return 1;
      }
    }
  }
  return lua_pushnil(L), 1;
}

int panel_GetPanelItem(lua_State *L) {
  return get_panel_item(L, FCTL_GETPANELITEM);
}

int panel_GetSelectedPanelItem(lua_State *L) {
  return get_panel_item(L, FCTL_GETSELECTEDPANELITEM);
}

int panel_GetCurrentPanelItem(lua_State *L) {
  return get_panel_item(L, FCTL_GETCURRENTPANELITEM);
}

int get_string_info(lua_State *L, int command)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE handle = OptHandle(L);
  int size = Info->Control(handle, command, 0, 0);
  if (size) {
    wchar_t *buf = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
    if (Info->Control(handle, command, size, (LONG_PTR)buf)) {
      push_utf8_string(L, buf, -1);
      return 1;
    }
  }
  return lua_pushnil(L), 1;
}

int panel_GetPanelDirectory(lua_State *L) {
  return get_string_info(L, FCTL_GETPANELDIR);
}

int panel_GetPanelFormat(lua_State *L) {
  return get_string_info(L, FCTL_GETPANELFORMAT);
}

int panel_GetPanelHostFile(lua_State *L) {
  return get_string_info(L, FCTL_GETPANELHOSTFILE);
}

int panel_GetColumnTypes(lua_State *L) {
  return get_string_info(L, FCTL_GETCOLUMNTYPES);
}

int panel_GetColumnWidths(lua_State *L) {
  return get_string_info(L, FCTL_GETCOLUMNWIDTHS);
}

int panel_GetPanelPrefix(lua_State *L) {
  return get_string_info(L, FCTL_GETPANELPREFIX);
}

int panel_RedrawPanel(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE handle = OptHandle(L);
  LONG_PTR param2 = 0;
  struct PanelRedrawInfo pri;
  if (lua_istable(L, 2)) {
    param2 = (LONG_PTR)&pri;
    lua_getfield(L, 2, "CurrentItem");
    pri.CurrentItem = lua_tointeger(L, -1) - 1;
    lua_getfield(L, 2, "TopPanelItem");
    pri.TopPanelItem = lua_tointeger(L, -1) - 1;
  }
  lua_pushboolean(L, Info->Control(handle, FCTL_REDRAWPANEL, 0, param2));
  return 1;
}

int SetPanelBooleanProperty(lua_State *L, int command)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE handle = OptHandle(L);
  int param1 = lua_toboolean(L,2);
  lua_pushboolean(L, Info->Control(handle, command, param1, 0));
  return 1;
}

int SetPanelIntegerProperty(lua_State *L, int command)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE handle = OptHandle(L);
  int param1 = check_env_flag(L,2);
  lua_pushboolean(L, Info->Control(handle, command, param1, 0));
  return 1;
}

int panel_SetCaseSensitiveSort(lua_State *L) {
  return SetPanelBooleanProperty(L, FCTL_SETCASESENSITIVESORT);
}

int panel_SetNumericSort(lua_State *L) {
  return SetPanelBooleanProperty(L, FCTL_SETNUMERICSORT);
}

int panel_SetSortOrder(lua_State *L) {
  return SetPanelBooleanProperty(L, FCTL_SETSORTORDER);
}

int panel_SetDirectoriesFirst(lua_State *L)
{
  return SetPanelBooleanProperty(L, FCTL_SETDIRECTORIESFIRST);
}

int panel_UpdatePanel(lua_State *L) {
  return SetPanelBooleanProperty(L, FCTL_UPDATEPANEL);
}

int panel_SetSortMode(lua_State *L) {
  return SetPanelIntegerProperty(L, FCTL_SETSORTMODE);
}

int panel_SetViewMode(lua_State *L) {
  return SetPanelIntegerProperty(L, FCTL_SETVIEWMODE);
}

int panel_SetPanelDirectory(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE handle = OptHandle(L);
  LONG_PTR param2 = 0;
  if (lua_isstring(L, 2)) {
    const wchar_t* dir = check_utf8_string(L, 2, NULL);
    param2 = (LONG_PTR)dir;
  }
  lua_pushboolean(L, Info->Control(handle, FCTL_SETPANELDIR, 0, param2));
  return 1;
}

int panel_GetCmdLine(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int size = Info->Control(PANEL_ACTIVE, FCTL_GETCMDLINE, 0, 0);
  wchar_t *buf = (wchar_t*) malloc(size*sizeof(wchar_t));
  Info->Control(PANEL_ACTIVE, FCTL_GETCMDLINE, size, (LONG_PTR)buf);
  push_utf8_string(L, buf, -1);
  free(buf);
  return 1;
}

int panel_SetCmdLine(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  const wchar_t* str = check_utf8_string(L, 1, NULL);
  lua_pushboolean(L, Info->Control(PANEL_ACTIVE, FCTL_SETCMDLINE, 0, (LONG_PTR)str));
  return 1;
}

int panel_GetCmdLinePos(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int pos;
  Info->Control(PANEL_ACTIVE, FCTL_GETCMDLINEPOS, 0, (LONG_PTR)&pos) ?
    lua_pushinteger(L, pos+1) : lua_pushnil(L);
  return 1;
}

int panel_SetCmdLinePos(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int pos = luaL_checkinteger(L, 1) - 1;
  int ret = Info->Control(PANEL_ACTIVE, FCTL_SETCMDLINEPOS, pos, 0);
  return lua_pushboolean(L, ret), 1;
}

int panel_InsertCmdLine(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  const wchar_t* str = check_utf8_string(L, 1, NULL);
  lua_pushboolean(L, Info->Control(PANEL_ACTIVE, FCTL_INSERTCMDLINE, 0, (LONG_PTR)str));
  return 1;
}

int panel_GetCmdLineSelection(lua_State *L)
{
  struct CmdLineSelect cms;
  PSInfo *Info = GetPluginStartupInfo(L);
  if (Info->Control(PANEL_ACTIVE, FCTL_GETCMDLINESELECTION, 0, (LONG_PTR)&cms)) {
    if (cms.SelStart < 0) cms.SelStart = 0;
    if (cms.SelEnd < 0) cms.SelEnd = 0;
    lua_pushinteger(L, cms.SelStart + 1);
    lua_pushinteger(L, cms.SelEnd);
    return 2;
  }
  return lua_pushnil(L), 1;
}

int panel_SetCmdLineSelection(lua_State *L)
{
  struct CmdLineSelect cms;
  PSInfo *Info = GetPluginStartupInfo(L);
  cms.SelStart = luaL_checkinteger(L, 1) - 1;
  cms.SelEnd = luaL_checkinteger(L, 2);
  if (cms.SelStart < -1) cms.SelStart = -1;
  if (cms.SelEnd < -1) cms.SelEnd = -1;
  int ret = Info->Control(PANEL_ACTIVE, FCTL_SETCMDLINESELECTION, 0, (LONG_PTR)&cms);
  return lua_pushboolean(L, ret), 1;
}

// CtrlSetSelection   (handle, items, selection)
// CtrlClearSelection (handle, items)
//   handle:       handle
//   items:        either number of an item, or a list of item numbers
//   selection:    boolean
int ChangePanelSelection(lua_State *L, BOOL op_set)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE handle = OptHandle(L);
  int itemindex = -1;
  if (lua_isnumber(L,2)) {
    itemindex = lua_tointeger(L,2) - 1;
    if (itemindex < 0) return luaL_argerror(L, 2, "non-positive index");
  }
  else if (!lua_istable(L,2))
    return luaL_typerror(L, 2, "number or table");
  int state = op_set ? lua_toboolean(L,3) : 0;

  // get panel info
  struct PanelInfo pi;
  if (!Info->Control(handle, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi) ||
     (pi.PanelType != PTYPE_FILEPANEL))
    return lua_pushboolean(L,0), 1;
  //---------------------------------------------------------------------------
  int numItems = op_set ? pi.ItemsNumber : pi.SelectedItemsNumber;
  int command  = op_set ? FCTL_SETSELECTION : FCTL_CLEARSELECTION;
  if (itemindex >= 0 && itemindex < numItems)
    Info->Control(handle, command, itemindex, state);
  else {
    int i, len = lua_objlen(L,2);
    for (i=1; i<=len; i++) {
      lua_pushinteger(L, i);
      lua_gettable(L,2);
      if (lua_isnumber(L,-1)) {
        itemindex = lua_tointeger(L,-1) - 1;
        if (itemindex >= 0 && itemindex < numItems)
          Info->Control(handle, command, itemindex, state);
      }
      lua_pop(L,1);
    }
  }
  //---------------------------------------------------------------------------
  return lua_pushboolean(L,1), 1;
}

int panel_SetSelection(lua_State *L) {
  return ChangePanelSelection(L, TRUE);
}

int panel_ClearSelection(lua_State *L) {
  return ChangePanelSelection(L, FALSE);
}

int panel_BeginSelection(lua_State *L)
{
  int res = GetPluginData(L)->Info->Control(OptHandle(L), FCTL_BEGINSELECTION, 0, 0);
  return lua_pushboolean(L, res), 1;
}

int panel_EndSelection(lua_State *L)
{
  int res = GetPluginData(L)->Info->Control(OptHandle(L), FCTL_ENDSELECTION, 0, 0);
  return lua_pushboolean(L, res), 1;
}

int panel_SetUserScreen(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int ret = Info->Control(PANEL_ACTIVE, FCTL_SETUSERSCREEN, 0, 0);
  return lua_pushboolean(L, ret), 1;
}

int panel_GetUserScreen(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int ret = Info->Control(PANEL_ACTIVE, FCTL_GETUSERSCREEN, 0, 0);
  return lua_pushboolean(L, ret), 1;
}

int panel_IsActivePanel(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE handle = OptHandle(L);
  return lua_pushboolean(L, Info->Control(handle, FCTL_ISACTIVEPANEL, 0, 0)), 1;
}

int panel_SetActivePanel(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE handle = OptHandle(L);
  return lua_pushboolean(L, Info->Control(handle, FCTL_SETACTIVEPANEL, 0, 0)), 1;
}

int panel_GetPanelPluginHandle(lua_State *L)
{
  HANDLE plug_handle;
  PSInfo *Info = GetPluginStartupInfo(L);
  Info->Control(OptHandle(L), FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)&plug_handle);
  if (plug_handle == INVALID_HANDLE_VALUE)
    lua_pushnil(L);
  else
    lua_pushlightuserdata(L, plug_handle);
  return 1;
}

// GetDirList (Dir)
//   Dir:     Name of the directory to scan (full pathname).
int far_GetDirList (lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  const wchar_t *Dir = check_utf8_string (L, 1, NULL);
  struct FAR_FIND_DATA *PanelItems;
  int ItemsNumber;
  int ret = Info->GetDirList (Dir, &PanelItems, &ItemsNumber);
  if(ret) {
    int i;
    lua_createtable(L, ItemsNumber, 0); // "PanelItems"
    for(i=0; i < ItemsNumber; i++) {
      lua_newtable(L);
      PushFarFindData (L, PanelItems + i);
      lua_rawseti(L, -2, i+1);
    }
    Info->FreeDirList (PanelItems, ItemsNumber);
    return 1;
  }
  return lua_pushnil(L), 1;
}

// GetPluginDirList (PluginNumber, hPlugin, Dir)
//   PluginNumber:    Number of plugin module.
//   hPlugin:         Current plugin instance handle.
//   Dir:             Name of the directory to scan (full pathname).
int far_GetPluginDirList (lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int PluginNumber = luaL_checkinteger (L, 1);
  HANDLE handle = OptHandlePos(L, 2);
  const wchar_t *Dir = check_utf8_string (L, 3, NULL);
  struct PluginPanelItem *PanelItems;
  int ItemsNumber;
  int ret = Info->GetPluginDirList (PluginNumber, handle, Dir, &PanelItems, &ItemsNumber);
  if(ret) {
    PushPanelItems (L, handle, PanelItems, ItemsNumber);
    Info->FreePluginDirList (PanelItems, ItemsNumber);
    return 1;
  }
  return lua_pushnil(L), 1;
}

// RestoreScreen (handle)
//   handle:    handle of saved screen.
int far_RestoreScreen (lua_State *L)
{
  int res = 0;
  if (lua_type(L,1) == LUA_TLIGHTUSERDATA) {
    PSInfo *Info = GetPluginStartupInfo(L);
    Info->RestoreScreen ((HANDLE)lua_touserdata (L, 1));
    res = 1;
  }
  return lua_pushboolean(L,res), 1;
}

// handle = SaveScreen (X1,Y1,X2,Y2)
//   handle:    handle of saved screen, [lightuserdata]
int far_SaveScreen (lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int X1 = luaL_optinteger(L,1,0);
  int Y1 = luaL_optinteger(L,2,0);
  int X2 = luaL_optinteger(L,3,-1);
  int Y2 = luaL_optinteger(L,4,-1);
  lua_pushlightuserdata(L, Info->SaveScreen(X1,Y1,X2,Y2));
  return 1;
}

int GetDialogItemType(lua_State* L, int key, int item)
{
  lua_pushinteger(L, key);
  lua_gettable(L, -2);
  int iType;
  if(!get_env_flag(L, -1, &iType)) {
    const char* sType = lua_tostring(L, -1);
    return luaL_error(L, "%s - unsupported type in dialog item %d", sType, item);
  }
  lua_pop(L, 1);
  return iType;
}

// the table is on lua stack top
int GetItemFlags(lua_State* L, int flag_index, int item_index)
{
  int flags;
  lua_pushinteger(L, flag_index);
  lua_gettable(L, -2);
  if (!GetFlagCombination (L, -1, &flags))
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
void SetFarDialogItem(lua_State *L, struct FarDialogItem* Item, int itemindex,
  int historyindex)
{
  memset(Item, 0, sizeof(struct FarDialogItem));
  Item->Type  = GetDialogItemType (L, 1, itemindex+1);
  Item->X1    = GetIntFromArray   (L, 2);
  Item->Y1    = GetIntFromArray   (L, 3);
  Item->X2    = GetIntFromArray   (L, 4);
  Item->Y2    = GetIntFromArray   (L, 5);
  Item->Focus = GetIntFromArray   (L, 6);
  Item->Flags = GetItemFlags      (L, 8, itemindex+1);
  if (Item->Type==DI_LISTBOX || Item->Type==DI_COMBOBOX) {
    lua_pushinteger(L, 7);   // +1
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
    lua_rawgeti(L, -1, 7);
    if (lua_type(L,-1) == LUA_TUSERDATA)
    {
      TFarUserControl* fuc = CheckFarUserControl(L, -1);
      Item->VBuf = fuc->VBuf;
    }
    lua_pop(L,1);
  }
  else if (Item->Type == DI_CHECKBOX || Item->Type == DI_RADIOBUTTON) {
    lua_pushinteger(L, 7);
    lua_gettable(L, -2);
    if (lua_isnumber(L,-1))
      Item->Selected = lua_tointeger(L,-1);
    else
      Item->Selected = lua_toboolean(L,-1) ? BSTATE_CHECKED : BSTATE_UNCHECKED;
    lua_pop(L, 1);
  }
  else if (Item->Type == DI_EDIT || Item->Type == DI_FIXEDIT) {
    if ((Item->Flags & DIF_HISTORY) ||
        (Item->Type == DI_FIXEDIT && (Item->Flags & DIF_MASKEDIT)))
    {
      lua_pushinteger(L, 7);   // +1
      lua_gettable(L, -2);     // +1
      if (!lua_isstring(L,-1))
        luaLF_SlotError (L, 7, "string");
      Item->History = check_utf8_string (L, -1, NULL); // +1 --> Item->History and Item->Mask are aliases (union members)
      size_t len = lua_objlen(L, historyindex);
      lua_rawseti (L, historyindex, len+1); // +0; put into "histories" table to avoid being gc'ed
    }
  }

  Item->DefaultButton = GetIntFromArray(L, 9);

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

void PushCheckbox (lua_State *L, int value)
{
  switch (value) {
    case BSTATE_3STATE:
      lua_pushinteger(L,2); break;
    case BSTATE_UNCHECKED:
      lua_pushboolean(L,0); break;
    default:
    case BSTATE_CHECKED:
      lua_pushboolean(L,1); break;
  }
}

void PushDlgItem (lua_State *L, const struct FarDialogItem* pItem, BOOL table_exist)
{
  if (! table_exist) {
    lua_createtable(L, 11, 0);
    if (pItem->Type == DI_LISTBOX || pItem->Type == DI_COMBOBOX) {
      lua_createtable(L, 0, 1);
      lua_rawseti(L, -2, 7);
    }
  }
  PutIntToArray  (L, 1, pItem->Type);
  PutIntToArray  (L, 2, pItem->X1);
  PutIntToArray  (L, 3, pItem->Y1);
  PutIntToArray  (L, 4, pItem->X2);
  PutIntToArray  (L, 5, pItem->Y2);
  PutIntToArray  (L, 6, pItem->Focus);

  if (pItem->Type == DI_LISTBOX || pItem->Type == DI_COMBOBOX) {
    lua_rawgeti(L, -1, 7);
    lua_pushinteger(L, pItem->ListPos+1);
    lua_setfield(L, -2, "SelectIndex");
    lua_pop(L,1);
  }
  else if (pItem->Type == DI_USERCONTROL)
  {
    lua_pushlightuserdata(L, pItem->VBuf);
    lua_rawseti(L, -2, 7);
  }
  else if (pItem->Type == DI_CHECKBOX || pItem->Type == DI_RADIOBUTTON)
  {
    PushCheckbox(L, pItem->Selected);
    lua_rawseti(L, -2, 7);
  }
  else
    PutIntToArray(L, 7, pItem->Selected);

  PutIntToArray  (L, 8, pItem->Flags);
  PutIntToArray  (L, 9, pItem->DefaultButton);
  lua_pushinteger(L, 10);
  push_utf8_string(L, pItem->PtrData, -1);
  lua_settable(L, -3);
  PutIntToArray  (L, 11, pItem->MaxLen);
}

int SetDlgItem (lua_State *L, HANDLE hDlg, int numitem, int pos_table, PSInfo *Info)
{
  struct FarDialogItem DialogItem;
  lua_newtable(L);
  lua_replace(L,1);
  luaL_checktype(L, pos_table, LUA_TTABLE);
  lua_pushvalue(L, pos_table);
  SetFarDialogItem(L, &DialogItem, numitem, 1);
  lua_pushboolean(L, Info->SendDlgMessage(hDlg, DM_SETDLGITEM, numitem, (LONG_PTR)&DialogItem));
  return 1;
}

TDialogData* NewDialogData(lua_State* L, PSInfo *Info, HANDLE hDlg, BOOL isOwned)
{
  TDialogData *dd = (TDialogData*) lua_newuserdata(L, sizeof(TDialogData));
  dd->L        = GetPluginData(L)->MainLuaState;
  dd->Info     = Info;
  dd->hDlg     = hDlg;
  dd->isOwned  = isOwned;
  dd->wasError = FALSE;
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

int DialogHandleEqual(lua_State* L)
{
  TDialogData* dd1 = CheckDialog(L, 1);
  TDialogData* dd2 = CheckDialog(L, 2);
  lua_pushboolean(L, dd1->hDlg == dd2->hDlg);
  return 1;
}

int Is_DM_DialogItem(int Msg)
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
    case DM_GETTEXTLENGTH:
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
    case DM_SHOWITEM:
      return 1;
  }
  return 0;
}

int PushDMParams (lua_State *L, int Msg, int Param1)
{
  if (! ((Msg>DM_FIRST && Msg<=DM_SETCOLOR) || Msg==DM_USER))
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

int DoSendDlgMessage (lua_State *L, int Msg, int delta)
{
  typedef struct { void *Id; int Ref; } listdata_t;
  PSInfo *Info = GetPluginStartupInfo(L);
  int Param1, res, res_incr=0, tmpint;
  LONG_PTR Param2=0;
  wchar_t buf[512];
  int pos2 = 2-delta, pos3 = 3-delta, pos4 = 4-delta;
  //---------------------------------------------------------------------------
  DWORD                      dword;
  COORD                      coord;
  struct DialogInfo          dlg_info;
  struct EditorSelect        es;
  struct EditorSetPosition   esp;
  struct FarDialogItemData   fdid;
  struct FarListDelete       fld;
  struct FarListFind         flf;
  struct FarListGetItem      flgi;
  struct FarListInfo         fli;
  struct FarListInsert       flins;
  struct FarListPos          flp;
  struct FarListTitles       flt;
  struct FarListUpdate       flu;
  struct FarListItemData     flid;
  SMALL_RECT                 small_rect;
  //---------------------------------------------------------------------------
  lua_settop(L, pos4); //many cases below rely on top==pos4
  HANDLE hDlg = CheckDialogHandle(L, 1);
  if (delta == 0)
    Msg = check_env_flag (L, 2);
  if (Msg == DM_CLOSE) {
    Param1 = luaL_optinteger(L,pos3,-1);
    if (Param1>0) --Param1;
  }
  else
    Param1 = Is_DM_DialogItem(Msg) ? luaL_optinteger(L,pos3,1)-1 : luaL_optinteger(L,pos3,0);

  //Param2 and the rest
  switch(Msg) {
    default:
    case DM_GETDLGDATA: //Not supported as used internally by LuaFAR
    case DM_SETDLGDATA: //+++
      luaL_argerror(L, pos2, "operation not implemented");
      break;

    case DM_GETFOCUS:
      res_incr = 1; // fall through
    case DM_CLOSE:
    case DM_EDITUNCHANGEDFLAG:
    case DM_GETCOMBOBOXEVENT:
    case DM_GETCURSORSIZE:
    case DM_GETDROPDOWNOPENED:
    case DM_GETITEMDATA:
    case DM_GETTEXTLENGTH:
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
      Param2 = GetEnableFromLua(L, pos4);
      break;

    case DM_ENABLE:
      Param2 = GetEnableFromLua(L, pos4);
      lua_pushboolean(L, Info->SendDlgMessage(hDlg, Msg, Param1, Param2));
      return 1;

    case DM_GETCHECK:
      PushCheckbox(L, Info->SendDlgMessage(hDlg, Msg, Param1, 0));
      return 1;

    case DM_SETCHECK:
      if (lua_isnumber(L,pos4))
        Param2 = lua_tointeger(L,pos4);
      else
        Param2 = lua_toboolean(L,pos4) ? BSTATE_CHECKED : BSTATE_UNCHECKED;
      break;

    case DM_GETCOLOR:
      Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&dword);
      lua_pushinteger (L, dword);
      return 1;

    case DM_SETCOLOR:
      Param2 = luaL_checkinteger(L, pos4);
      break;

    case DM_LISTADDSTR:
      res_incr=1;
    case DM_ADDHISTORY:
    case DM_SETTEXTPTR:
      Param2 = (LONG_PTR) check_utf8_string(L, pos4, NULL);
      break;

    case DM_SETHISTORY:
      Param2 = (LONG_PTR) opt_utf8_string(L, pos4, NULL);
      break;

    case DM_LISTSETMOUSEREACTION:
      get_env_flag (L, pos4, &tmpint);
      Param2 = tmpint;
      break;

    case DM_GETCURSORPOS:
      if (Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&coord)) {
        lua_createtable(L,0,2);
        PutNumToTable(L, "X", coord.X + 1);
        PutNumToTable(L, "Y", coord.Y + 1);
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_GETDIALOGINFO:
      dlg_info.StructSize = sizeof(dlg_info);
      if (Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&dlg_info)) {
        char uuid[16];
        guid_to_uuid(&dlg_info.Id, uuid);
        lua_createtable(L,0,1);
        PutLStrToTable(L, "Id", uuid, 16);
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_GETDLGRECT:
    case DM_GETITEMPOSITION:
      if (Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&small_rect)) {
        lua_createtable(L,0,4);
        PutNumToTable(L, "Left", small_rect.Left);
        PutNumToTable(L, "Top", small_rect.Top);
        PutNumToTable(L, "Right", small_rect.Right);
        PutNumToTable(L, "Bottom", small_rect.Bottom);
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_GETEDITPOSITION:
      if (Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&esp))
        return PushEditorSetPosition(L, &esp), 1;
      return lua_pushnil(L), 1;

    case DM_GETSELECTION:
      if (Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&es)) {
        lua_createtable(L,0,5);
        PutNumToTable(L, "BlockType", es.BlockType);
        PutNumToTable(L, "BlockStartLine", es.BlockStartLine+1);
        PutNumToTable(L, "BlockStartPos", es.BlockStartPos+1);
        PutNumToTable(L, "BlockWidth", es.BlockWidth);
        PutNumToTable(L, "BlockHeight", es.BlockHeight);
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_SETSELECTION:
      luaL_checktype(L, pos4, LUA_TTABLE);
      if (SetEditorSelect(L, pos4, &es)) {
        Param2 = (LONG_PTR)&es;
        break;
      }
      return lua_pushinteger(L,0), 1;

    case DM_GETTEXT: {
      size_t size;
      fdid.PtrLength = (size_t) Info->SendDlgMessage(hDlg, Msg, Param1, 0);
      fdid.PtrData = (wchar_t*) malloc((fdid.PtrLength+1) * sizeof(wchar_t));
      size = Info->SendDlgMessage(hDlg, Msg, Param1, (LONG_PTR)&fdid);
      push_utf8_string(L, size ? fdid.PtrData : L"", size);
      free(fdid.PtrData);
      return 1;
    }

    case DM_GETCONSTTEXTPTR: {
      const wchar_t *ptr = (wchar_t*)Info->SendDlgMessage(hDlg, Msg, Param1, 0);
      push_utf8_string(L, ptr ? ptr:L"", -1);
      return 1;
    }

    case DM_SETTEXT:
      fdid.PtrData = check_utf8_string(L, pos4, NULL);
      fdid.PtrLength = 0; // wcslen(fdid.PtrData);
      Param2 = (LONG_PTR)&fdid;
      break;

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
        res = Info->SendDlgMessage (hDlg, Msg, res, (LONG_PTR)arr);
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
      if (lua_isnoneornil(L, pos4))
        Param2 = 0;
      else {
        luaL_checktype(L, pos4, LUA_TTABLE);
        fld.StartIndex = GetOptIntFromTable(L, "StartIndex", 1) - 1;
        fld.Count = GetOptIntFromTable(L, "Count", 1);
        Param2 = (LONG_PTR)&fld;
      }
      break;

    case DM_LISTFINDSTRING:
      luaL_checktype(L, pos4, LUA_TTABLE);
      flf.StartIndex = GetOptIntFromTable(L, "StartIndex", 1) - 1;
      lua_getfield(L, pos4, "Pattern");
      flf.Pattern = check_utf8_string(L, -1, NULL);
      lua_getfield(L, pos4, "Flags");
      get_env_flag(L, -1, (int*)&flf.Flags);
      res = Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&flf);
      res < 0 ? lua_pushnil(L) : lua_pushinteger (L, res+1);
      return 1;

    case DM_LISTGETCURPOS:
      Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&flp);
      lua_createtable(L,0,2);
      PutIntToTable(L, "SelectPos", flp.SelectPos+1);
      PutIntToTable(L, "TopPos", flp.TopPos+1);
      return 1;

    case DM_LISTGETITEM:
      flgi.ItemIndex = luaL_checkinteger(L, pos4) - 1;
      res = Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&flgi);
      if (res) {
        lua_createtable(L,0,2);
        PutIntToTable(L, "Flags", flgi.Item.Flags);
        PutWStrToTable(L, "Text", flgi.Item.Text, -1);
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_LISTGETTITLES:
      flt.Title = buf;
      flt.Bottom = buf + ARRAYSIZE(buf)/2;
      flt.TitleLen = ARRAYSIZE(buf)/2;
      flt.BottomLen = ARRAYSIZE(buf)/2;
      res = Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&flt);
      if (res) {
        lua_createtable(L,0,2);
        PutWStrToTable(L, "Title", flt.Title, -1);
        PutWStrToTable(L, "Bottom", flt.Bottom, -1);
        return 1;
      }
      return lua_pushnil(L), 1;

    case DM_LISTSETTITLES:
      luaL_checktype(L, pos4, LUA_TTABLE);
      lua_getfield(L, pos4, "Title");
      flt.Title = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
      lua_getfield(L, pos4, "Bottom");
      flt.Bottom = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
      Param2 = (LONG_PTR)&flt;
      break;

    case DM_LISTINFO:
      res = Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&fli);
      if (res) {
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

    case DM_LISTINSERT:
      luaL_checktype(L, pos4, LUA_TTABLE);
      flins.Index = GetOptIntFromTable(L, "Index", 1) - 1;
      lua_getfield(L, pos4, "Text");
      flins.Item.Text = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
      lua_getfield(L, pos4, "Flags"); //+1
      flins.Item.Flags = CheckFlags(L, -1);
      res = Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&flins);
      res < 0 ? lua_pushnil(L) : lua_pushinteger (L, res);
      return 1;

    case DM_LISTUPDATE:
      luaL_checktype(L, pos4, LUA_TTABLE);
      flu.Index = GetOptIntFromTable(L, "Index", 1) - 1;
      lua_getfield(L, pos4, "Text");
      flu.Item.Text = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
      lua_getfield(L, pos4, "Flags"); //+1
      flu.Item.Flags = CheckFlags(L, -1);
      lua_pushboolean(L, Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&flu));
      return 1;

    case DM_LISTSETCURPOS:
      res_incr = 1;
      luaL_checktype(L, pos4, LUA_TTABLE);
      flp.SelectPos = GetOptIntFromTable(L, "SelectPos", 1) - 1;
      flp.TopPos = GetOptIntFromTable(L, "TopPos", 1) - 1;
      Param2 = (LONG_PTR)&flp;
      break;

    case DM_LISTGETDATASIZE:
      Param2 = luaL_checkinteger(L, pos4) - 1;
      break;

    case DM_LISTSETDATA: {
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
      oldData = (listdata_t*)Info->SendDlgMessage(hDlg, DM_LISTGETDATA, Param1, Index);
      if (oldData &&
        sizeof(listdata_t) == Info->SendDlgMessage(hDlg, DM_LISTGETDATASIZE, Param1, Index) &&
        oldData->Id == Info)
      {
        luaL_unref(L, -2, oldData->Ref);
      }
      Data.Id = Info;
      Data.Ref = luaL_ref(L, -2);
      flid.Index = Index;
      flid.Data = &Data;
      flid.DataSize = sizeof(Data);
      lua_pushinteger(L, Info->SendDlgMessage(hDlg, Msg, Param1, (LONG_PTR)&flid));
      return 1;
    }

    case DM_LISTGETDATA: {
      int Index = (int)luaL_checkinteger(L, pos4) - 1;
      listdata_t *Data = (listdata_t*)Info->SendDlgMessage(hDlg, DM_LISTGETDATA, Param1, Index);
      if (Data) {
        if (sizeof(listdata_t) == Info->SendDlgMessage(hDlg, DM_LISTGETDATASIZE, Param1, Index) &&
          Data->Id == Info)
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

    case DM_GETDLGITEM: {
      int size = Info->SendDlgMessage(hDlg, DM_GETDLGITEM, Param1, 0);
      if (size > 0) {
        BOOL table_exist = lua_istable(L, pos4);
        struct FarDialogItem* pItem = (struct FarDialogItem*) lua_newuserdata(L, size);
        Info->SendDlgMessage(hDlg, DM_GETDLGITEM, Param1, (LONG_PTR)pItem);
        if (table_exist)
          lua_pushvalue(L, pos4);
        PushDlgItem(L, pItem, table_exist);
      }
      else
        lua_pushnil(L);
      return 1;
    }

    case DM_SETDLGITEM:
      return SetDlgItem(L, hDlg, Param1, pos4, Info);

    case DM_MOVEDIALOG:
    case DM_RESIZEDIALOG: {
      COORD* c;
      luaL_checktype(L, pos4, LUA_TTABLE);
      coord.X = GetOptIntFromTable(L, "X", 0);
      coord.Y = GetOptIntFromTable(L, "Y", 0);
      c = (COORD*) Info->SendDlgMessage (hDlg, Msg, Param1, (LONG_PTR)&coord);
      lua_createtable(L, 0, 2);
      PutIntToTable(L, "X", c->X);
      PutIntToTable(L, "Y", c->Y);
      return 1;
    }

    case DM_SETCURSORPOS:
      luaL_checktype(L, pos4, LUA_TTABLE);
      coord.X = GetOptIntFromTable(L, "X", 1) - 1;
      coord.Y = GetOptIntFromTable(L, "Y", 1) - 1;
      Param2 = (LONG_PTR)&coord;
      lua_pushboolean(L, Info->SendDlgMessage (hDlg, Msg, Param1, Param2));
      return 1;

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
      luaL_checktype(L, pos4, LUA_TTABLE);
      lua_settop(L, pos4);
      FillEditorSetPosition(L, &esp);
      Param2 = (LONG_PTR)&esp;
      break;

    //~ case DM_GETTEXTPTR:
  }
  res = Info->SendDlgMessage (hDlg, Msg, Param1, Param2);
  lua_pushinteger (L, res + res_incr);
  return 1;
}

#define DlgMethod(name,msg,delta) \
int dlg_##name(lua_State *L) { return DoSendDlgMessage(L,msg,delta); }

int far_SendDlgMessage(lua_State *L) { return DoSendDlgMessage(L,0,0); }

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
DlgMethod( GetDlgItem,             DM_GETDLGITEM, 1)
DlgMethod( GetDlgRect,             DM_GETDLGRECT, 1)
DlgMethod( GetDropdownOpened,      DM_GETDROPDOWNOPENED, 1)
DlgMethod( GetEditPosition,        DM_GETEDITPOSITION, 1)
DlgMethod( GetFocus,               DM_GETFOCUS, 1)
DlgMethod( GetItemData,            DM_GETITEMDATA, 1)
DlgMethod( GetItemPosition,        DM_GETITEMPOSITION, 1)
DlgMethod( GetSelection,           DM_GETSELECTION, 1)
DlgMethod( GetText,                DM_GETTEXT, 1)
DlgMethod( GetTextLength,          DM_GETTEXTLENGTH, 1)
DlgMethod( GetTextPtr,             DM_GETTEXTPTR, 1)
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
DlgMethod( SetDlgItem,             DM_SETDLGITEM, 1)
DlgMethod( SetDropdownOpened,      DM_SETDROPDOWNOPENED, 1)
DlgMethod( SetEditPosition,        DM_SETEDITPOSITION, 1)
DlgMethod( SetFocus,               DM_SETFOCUS, 1)
DlgMethod( SetHistory,             DM_SETHISTORY, 1)
DlgMethod( SetItemData,            DM_SETITEMDATA, 1)
DlgMethod( SetItemPosition,        DM_SETITEMPOSITION, 1)
DlgMethod( SetMaxTextLength,       DM_SETMAXTEXTLENGTH, 1)
DlgMethod( SetMouseEventNotify,    DM_SETMOUSEEVENTNOTIFY, 1)
DlgMethod( SetSelection,           DM_SETSELECTION, 1)
DlgMethod( SetText,                DM_SETTEXT, 1)
DlgMethod( SetTextPtr,             DM_SETTEXTPTR, 1)
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

    case DN_BTNCLICK:
    case DN_CLOSE:
    case DN_CTLCOLORDLGITEM:
    case DN_CTLCOLORDLGLIST:
    case DN_DRAWDLGITEM:
    case DN_EDITCHANGE:
    case DN_GOTFOCUS:
    case DN_HELP:
    case DN_HOTKEY:
    case DN_INITDIALOG:
    case DN_KEY:
    case DN_KILLFOCUS:
    case DN_LISTCHANGE:
    case DN_LISTHOTKEY:
    case DN_MOUSECLICK:
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
      char uuid[16];
      struct DialogInfo* di = (struct DialogInfo*) Param2;
      guid_to_uuid(&di->Id, uuid);
      lua_pushlstring(L, uuid, 16);
      break;
    }

    case DN_LISTCHANGE:
    case DN_LISTHOTKEY:
      lua_pushinteger(L, Param2+1);  // make list positions 1-based
      break;

    case DN_MOUSECLICK:
    case DN_MOUSEEVENT:
      PutMouseEvent(L, (MOUSE_EVENT_RECORD*)Param2, FALSE);
      break;

    case DN_RESIZECONSOLE:
    {
      COORD* coord = (COORD*)Param2;
      lua_createtable(L, 0, 2);
      PutIntToTable(L, "X", coord->X);
      PutIntToTable(L, "Y", coord->Y);
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
  int ret = 0;
  switch(Msg)
  {
    case DN_CTLCOLORDLGLIST:
      if((ret = lua_istable(L,-1)) != 0)
      {
        struct FarListColors* flc = (struct FarListColors*) Param2;
        int i;
        size_t len = lua_objlen(L, -1);

        if(len > flc->ColorCount) len = flc->ColorCount;

        for(i = 0; i < (int)len; i++)
        {
          lua_rawgeti(L, -1, i+1);
          flc->Colors[i] = lua_tointeger(L, -1);
          lua_pop(L, 1);
        }
      }
      break;

    case DN_CTLCOLORDLGITEM:
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

int DN_ConvertParam1(int Msg, int Param1)
{
  switch(Msg) {
    default:
      return Param1;

    case DN_BTNCLICK:
    case DN_CTLCOLORDLGITEM:
    case DN_CTLCOLORDLGLIST:
    case DN_DRAWDLGITEM:
    case DN_EDITCHANGE:
    case DN_GOTFOCUS:
    case DN_HELP:
    case DN_HOTKEY:
    case DN_INITDIALOG:
    case DN_KEY:
    case DN_KILLFOCUS:
    case DN_LISTCHANGE:
    case DN_LISTHOTKEY:
      return Param1 + 1;

    case DN_CLOSE:
    case DN_MOUSECLICK:
      return Param1 < 0 ? Param1 : Param1 + 1;
  }
}

LONG_PTR LF_DlgProc(lua_State *L, HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
  TPluginData *pd = GetPluginData(L);
  TDialogData *dd = (TDialogData*) pd->Info->SendDlgMessage(hDlg,DM_GETDLGDATA,0,0);
  if (dd->wasError)
    return dd->Info->DefDlgProc(hDlg, Msg, Param1, Param2);

  L = dd->L; // the dialog may be called from a lua_State other than the main one
  PSInfo *Info = dd->Info;
  int Param1_mod = DN_ConvertParam1(Msg, Param1);

  lua_pushlightuserdata (L, dd);       //+1   retrieve the table
  lua_rawget (L, LUA_REGISTRYINDEX);   //+1
  lua_rawgeti(L, -1, 2);               //+2   retrieve the procedure
  lua_rawgeti(L, -2, 3);               //+3   retrieve the handle
  lua_pushinteger (L, Msg);            //+4
  lua_pushinteger (L, Param1_mod);     //+5

  if (Msg == DN_CTLCOLORDLGLIST) {
    struct FarListColors* flc = (struct FarListColors*) Param2;
    lua_createtable(L, flc->ColorCount, 1);
    PutIntToTable(L, "Flags", flc->Flags);
    int i;
    for (i=0; i < flc->ColorCount; i++)
      PutIntToArray(L, i+1, flc->Colors[i]);
  }

  else if (Msg == DN_DRAWDLGITEM)
    PushDlgItem (L, (struct FarDialogItem*)Param2, FALSE);

  else if (Msg == DN_EDITCHANGE)
    PushDlgItem (L, (struct FarDialogItem*)Param2, FALSE);

  else if (Msg == DN_HELP)
    push_utf8_string (L, Param2 ? (wchar_t*)Param2 : L"", -1);

  else if (Msg == DN_INITDIALOG)
    lua_pushnil(L);

  else if (Msg == DN_LISTCHANGE || Msg == DN_LISTHOTKEY)
    lua_pushinteger (L, Param2+1); // make list positions 1-based

  else if (Msg == DN_MOUSECLICK || Msg == DN_MOUSEEVENT)
    PutMouseEvent (L, (const MOUSE_EVENT_RECORD*)Param2, FALSE);

  else if (Msg == DN_RESIZECONSOLE) {
    COORD* coord = (COORD*)Param2;
    lua_createtable(L, 0, 2);
    PutIntToTable(L, "X", coord->X);
    PutIntToTable(L, "Y", coord->Y);
  }

  else
    lua_pushinteger (L, Param2); //+6

  //---------------------------------------------------------------------------
  LONG_PTR ret = pcall_msg (L, 4, 1); //+2
  if (ret) {
    lua_pop(L, 1);
    dd->wasError = TRUE;
    Info->SendDlgMessage(hDlg, DM_CLOSE, -1, 0);
    return Info->DefDlgProc(hDlg, Msg, Param1, Param2);
  }
  //---------------------------------------------------------------------------

  if (lua_isnil(L, -1))
    ret = Info->DefDlgProc(hDlg, Msg, Param1, Param2);

  else if (Msg == DN_CTLCOLORDLGLIST) {
    struct FarListColors* flc = (struct FarListColors*) Param2;
    if ((ret = lua_istable(L,-1)) != 0) {
      int i;
      for (i=0; i < flc->ColorCount; i++)
        flc->Colors[i] = GetIntFromArray(L, i+1);
    }
  }

  else if (Msg == DN_GETDIALOGINFO) {
    ret = lua_isstring(L,-1) && lua_objlen(L,-1) >= 16;
    if (ret) {
      struct DialogInfo* di = (struct DialogInfo*) Param2;
      uuid_to_guid(lua_tostring(L,-1), &di->Id);
    }
  }

  else if (Msg == DN_HELP) {
    if ((ret = (LONG_PTR)utf8_to_wcstring(L, -1, NULL)) != 0) {
      lua_pushvalue(L, -1);                // keep stack balanced
      lua_setfield(L, -3, "helpstring");   // protect from garbage collector
    }
  }

  else if (lua_isnumber(L, -1))
    ret = lua_tointeger (L, -1);
  else
    ret = lua_toboolean(L, -1);

  lua_pop (L, 2);
  return ret;
}

void RemoveDialogFromRegistry(TDialogData *dd)
{
  dd->hDlg = INVALID_HANDLE_VALUE;
  lua_pushlightuserdata(dd->L, dd);
  lua_pushnil(dd->L);
  lua_rawset(dd->L, LUA_REGISTRYINDEX);
}

int far_DialogInit(lua_State *L)
{
  TPluginData *pd = GetPluginData(L);
  PSInfo *Info = pd->Info;

  int X1 = luaL_checkinteger(L, 1);
  int Y1 = luaL_checkinteger(L, 2);
  int X2 = luaL_checkinteger(L, 3);
  int Y2 = luaL_checkinteger(L, 4);
  const wchar_t *HelpTopic = opt_utf8_string(L, 5, NULL);
  luaL_checktype(L, 6, LUA_TTABLE);

  lua_newtable (L); // create a "histories" table, to prevent history strings
                    // from being garbage collected too early
  lua_replace (L, 1);

  int ItemsNumber = lua_objlen(L, 6);
  struct FarDialogItem* Items = (struct FarDialogItem*)
    lua_newuserdata (L, ItemsNumber * sizeof(struct FarDialogItem));
  lua_replace (L, 2);
  int i;
  for(i=0; i < ItemsNumber; i++) {
    lua_pushinteger(L, i+1);
    lua_gettable(L, 6);
    int type = lua_type(L, -1);
    if (type == LUA_TTABLE) {
      SetFarDialogItem(L, Items+i, i, 1);
    }
    lua_pop(L, 1);
    if(type == LUA_TNIL)
      break;
    if(type != LUA_TTABLE)
      return luaL_error(L, "Items[%d] is not a table", i+1);
  }

  // 7-th parameter (flags)
  int Flags = CheckFlags(L,7);

  TDialogData* dd = NewDialogData(L, Info, INVALID_HANDLE_VALUE, TRUE);

  // 8-th parameter (DlgProc function)
  FARAPIDEFDLGPROC Proc = NULL;
  LONG_PTR Param = 0;
  if (lua_isfunction(L, 8)) {
    Proc = pd->DlgProc;
    Param = (LONG_PTR)dd;
  }

  dd->hDlg = Info->DialogInit(Info->ModuleNumber, X1, Y1, X2, Y2, HelpTopic,
                              Items, ItemsNumber, 0, Flags, Proc, Param);

  if (dd->hDlg != INVALID_HANDLE_VALUE) {
    // Put some values into the registry
    lua_pushlightuserdata(L, dd); // important: index it with dd
    lua_createtable(L, 3, 0);
    lua_pushvalue (L, 1);     // store the "histories" table
    lua_rawseti(L, -2, 1);
    if (lua_isfunction(L, 8)) {
      lua_pushvalue (L, 8);   // store the procedure
      lua_rawseti(L, -2, 2);
      lua_pushvalue (L, -3);  // store the handle
      lua_rawseti(L, -2, 3);
    }
    lua_rawset (L, LUA_REGISTRYINDEX);
  }
  else {
    RemoveDialogFromRegistry(dd);
    lua_pushnil(L);
  }
  return 1;
}

void free_dialog (TDialogData* dd)
{
  if (dd->isOwned && dd->hDlg != INVALID_HANDLE_VALUE) {
    dd->Info->DialogFree(dd->hDlg);
    RemoveDialogFromRegistry(dd);
  }
}

int far_DialogRun (lua_State *L)
{
  TDialogData* dd = CheckValidDialog(L, 1);
  int result = dd->Info->DialogRun(dd->hDlg);
  if (result >= 0) ++result;

  if (dd->wasError) {
    free_dialog(dd);
    luaL_error(L, "error occured in dialog procedure");
  }
  lua_pushinteger(L, result);
  return 1;
}

int far_DialogFree (lua_State *L)
{
  free_dialog(CheckDialog(L, 1));
  return 0;
}

int dialog_tostring (lua_State *L)
{
  TDialogData* dd = CheckDialog(L, 1);
  if (dd->hDlg != INVALID_HANDLE_VALUE)
    lua_pushfstring(L, "%s (%p)", FarDialogType, dd->hDlg);
  else
    lua_pushfstring(L, "%s (closed)", FarDialogType);
  return 1;
}

int dialog_rawhandle(lua_State *L)
{
  TDialogData* dd = CheckDialog(L, 1);
  if(dd->hDlg != INVALID_HANDLE_VALUE)
    lua_pushlightuserdata(L, dd->hDlg);
  else
    lua_pushnil(L);
  return 1;
}

int far_DefDlgProc(lua_State *L)
{
  int Msg, Param1, Param2;

  luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
  HANDLE hDlg = lua_touserdata(L, 1);
  get_env_flag(L, 2, &Msg);
  Param1 = luaL_checkinteger(L, 3);
  Param2 = luaL_checkinteger(L, 4);

  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushinteger(L, Info->DefDlgProc(hDlg, Msg, Param1, Param2));
  return 1;
}

int editor_Editor(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
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
  int ret = Info->Editor(FileName, Title, X1, Y1, X2, Y2, Flags,
                         StartLine, StartChar, CodePage);
  lua_pushinteger(L, ret);
  return 1;
}

int viewer_Viewer(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  const wchar_t* FileName = check_utf8_string(L, 1, NULL);
  const wchar_t* Title    = opt_utf8_string(L, 2, NULL);
  int X1 = luaL_optinteger(L, 3, 0);
  int Y1 = luaL_optinteger(L, 4, 0);
  int X2 = luaL_optinteger(L, 5, -1);
  int Y2 = luaL_optinteger(L, 6, -1);
  int Flags = CheckFlags(L, 7);
  int CodePage = luaL_optinteger(L, 8, CP_AUTODETECT);
  int ret = Info->Viewer(FileName, Title, X1, Y1, X2, Y2, Flags, CodePage);
  lua_pushboolean(L, ret);
  return 1;
}

int viewer_GetInfo(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct ViewerInfo vi;
  vi.StructSize = sizeof(vi);
  if (Info->ViewerControl(VCTL_GETINFO, &vi)) {
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
    lua_setfield(L, -2, "CurMode");
  }
  else
    lua_pushnil(L);
  return 1;
}

int viewer_GetFileName(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct ViewerInfo vi;
  vi.StructSize = sizeof(vi);
  if (Info->ViewerControl(VCTL_GETINFO, &vi))
    push_utf8_string(L, vi.FileName, -1);
  else
    lua_pushnil(L);
  return 1;
}

int viewer_Quit(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  Info->ViewerControl(VCTL_QUIT, NULL);
  return 0;
}

int viewer_Redraw(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  Info->ViewerControl(VCTL_REDRAW, NULL);
  return 0;
}

int viewer_Select(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct ViewerSelect vs;
  vs.BlockStartPos = (long long int)luaL_checknumber(L,1);
  vs.BlockLen = luaL_checkinteger(L,2);
  lua_pushboolean(L, Info->ViewerControl(VCTL_SELECT, &vs));
  return 1;
}

int viewer_SetPosition(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  struct ViewerSetPosition vsp;
  if (lua_istable(L, 1)) {
    lua_settop(L, 1);
    vsp.StartPos = (int64_t)GetOptNumFromTable(L, "StartPos", 0);
    vsp.LeftPos = (int64_t)GetOptNumFromTable(L, "LeftPos", 1) - 1;
    vsp.Flags   = GetOptIntFromTable(L, "Flags", 0);
  }
  else {
    vsp.StartPos = (int64_t)luaL_optnumber(L,1,0);
    vsp.LeftPos = (int64_t)luaL_optnumber(L,2,1) - 1;
    vsp.Flags = luaL_optinteger(L,3,0);
  }
  if (Info->ViewerControl(VCTL_SETPOSITION, &vsp))
    lua_pushnumber(L, (double)vsp.StartPos);
  else
    lua_pushnil(L);
  return 1;
}

int viewer_SetMode(lua_State *L)
{
  struct ViewerSetMode vsm;
  memset(&vsm, 0, sizeof(struct ViewerSetMode));
  luaL_checktype(L, 1, LUA_TTABLE);

  lua_getfield(L, 1, "Type");
  if (!get_env_flag (L, -1, &vsm.Type))
    return lua_pushboolean(L,0), 1;

  lua_getfield(L, 1, "iParam");
  if (lua_isnumber(L, -1))
    vsm.Param.iParam = lua_tointeger(L, -1);
  else
    return lua_pushboolean(L,0), 1;

  int flags;
  lua_getfield(L, 1, "Flags");
  if (!get_env_flag (L, -1, &flags))
    return lua_pushboolean(L,0), 1;
  vsm.Flags = flags;

  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushboolean(L, Info->ViewerControl(VCTL_SETMODE, &vsm));
  return 1;
}

int far_ShowHelp(lua_State *L)
{
  const wchar_t *ModuleName = check_utf8_string (L,1,NULL);
  const wchar_t *HelpTopic = opt_utf8_string (L,2,NULL);
  int Flags = CheckFlags(L,3);
  PSInfo *Info = GetPluginStartupInfo(L);
  BOOL ret = Info->ShowHelp (ModuleName, HelpTopic, Flags);
  return lua_pushboolean(L, ret), 1;
}

// DestText = far.InputBox(Title,Prompt,HistoryName,SrcText,DestLength,HelpTopic,Flags)
// all arguments are optional
int far_InputBox(lua_State *L)
{
  const wchar_t *Title       = opt_utf8_string (L, 1, L"Input Box");
  const wchar_t *Prompt      = opt_utf8_string (L, 2, L"Enter the text:");
  const wchar_t *HistoryName = opt_utf8_string (L, 3, NULL);
  const wchar_t *SrcText     = opt_utf8_string (L, 4, L"");
  int DestLength             = luaL_optinteger (L, 5, 1024);
  const wchar_t *HelpTopic   = opt_utf8_string (L, 6, NULL);
  DWORD Flags = luaL_optinteger (L, 7, FIB_ENABLEEMPTY|FIB_BUTTONS|FIB_NOAMPERSAND);

  if (DestLength < 1) DestLength = 1;
  wchar_t *DestText = (wchar_t*) malloc(sizeof(wchar_t)*DestLength);
  PSInfo *Info = GetPluginStartupInfo(L);
  int res = Info->InputBox(Title, Prompt, HistoryName, SrcText, DestText,
                           DestLength, HelpTopic, Flags);

  if (res) push_utf8_string (L, DestText, -1);
  else lua_pushnil(L);

  free(DestText);
  return 1;
}

int far_GetMsg(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int MsgId = luaL_checkinteger(L, 1);
  const wchar_t* msg = (MsgId < 0) ? NULL : Info->GetMsg(Info->ModuleNumber, MsgId);
  msg ? push_utf8_string(L,msg,-1) : lua_pushnil(L);
  return 1;
}

int far_Text(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int X = luaL_optinteger(L, 1, 0);
  int Y = luaL_optinteger(L, 2, 0);
  int Color = luaL_optinteger(L, 3, 0x0F);
  const wchar_t* Str = opt_utf8_string(L, 4, NULL);
  Info->Text(X, Y, Color, Str);
  return 0;
}

// Based on "CheckForEsc" function, by Ivan Sintyurin (spinoza@mail.ru)
WORD ExtractKey()
{
  INPUT_RECORD rec;
  DWORD ReadCount;
  HANDLE hConInp = NULL; //GetStdHandle(STD_INPUT_HANDLE);
  while (WINPORT(PeekConsoleInput)(hConInp,&rec,1,&ReadCount), ReadCount) {
    WINPORT(ReadConsoleInput)(hConInp,&rec,1,&ReadCount);
    if (rec.EventType==KEY_EVENT && rec.Event.KeyEvent.bKeyDown)
      return rec.Event.KeyEvent.wVirtualKeyCode;
  }
  return 0;
}

// result = ExtractKey()
// -- general purpose function; not FAR dependent
int win_ExtractKey(lua_State *L)
{
  WORD vKey = ExtractKey() & 0xff;
  if (vKey && VirtualKeyStrings[vKey])
    lua_pushstring(L, VirtualKeyStrings[vKey]);
  else
    lua_pushnil(L);
  return 1;
}

int far_CopyToClipboard (lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  const wchar_t *str = check_utf8_string(L,1,NULL);
  int r = Info->FSF->CopyToClipboard(str);
  return lua_pushboolean(L, r), 1;
}

int far_PasteFromClipboard (lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  wchar_t* str = Info->FSF->PasteFromClipboard();
  if (str) {
    push_utf8_string(L, str, -1);
    Info->FSF->DeleteBuffer(str);
  }
  else lua_pushnil(L);
  return 1;
}

int far_KeyToName (lua_State *L)
{
  wchar_t buf[256];
  int Key = luaL_checkinteger(L,1);
  BOOL result = GetFSF(L)->FarKeyToName(Key, buf, ARRAYSIZE(buf)-1);
  if (result) push_utf8_string(L, buf, -1);
  else lua_pushnil(L);
  return 1;
}

int far_NameToKey (lua_State *L)
{
  const wchar_t* str = check_utf8_string(L,1,NULL);
  int Key = GetFSF(L)->FarNameToKey(str);
  if (Key == -1) lua_pushnil(L);
  else lua_pushinteger(L, Key);
  return 1;
}

int far_InputRecordToKey (lua_State *L)
{
  INPUT_RECORD ir;
  FillInputRecord(L, 1, &ir);
  lua_pushinteger(L, GetFSF(L)->FarInputRecordToKey(&ir));
  return 1;
}

int far_LStricmp (lua_State *L)
{
  const wchar_t* s1 = check_utf8_string(L, 1, NULL);
  const wchar_t* s2 = check_utf8_string(L, 2, NULL);
  lua_pushinteger(L, GetFSF(L)->LStricmp(s1, s2));
  return 1;
}

int far_LStrnicmp (lua_State *L)
{
  const wchar_t* s1 = check_utf8_string(L, 1, NULL);
  const wchar_t* s2 = check_utf8_string(L, 2, NULL);
  int num = luaL_checkinteger(L, 3);
  if (num < 0) num = 0;
  lua_pushinteger(L, GetFSF(L)->LStrnicmp(s1, s2, num));
  return 1;
}

int _ProcessName (lua_State *L, int Op)
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
  struct FarStandardFunctions* FSF = GetFSF(L);

  if(Op == PN_CMPNAME || Op == PN_CMPNAMELIST || Op == PN_CHECKMASK) {
    int result = FSF->ProcessName(Mask, (wchar_t*)Name, 0, Flags);
    lua_pushboolean(L, result);
  }
  else if (Op == PN_GENERATENAME) {
    const int BUFSIZE = 1024;
    wchar_t* buf = (wchar_t*)lua_newuserdata(L, BUFSIZE * sizeof(wchar_t));
    wcsncpy(buf, Mask, BUFSIZE-1);
    buf[BUFSIZE-1] = 0;

    int result = FSF->ProcessName(Name, buf, BUFSIZE, Flags);
    if (result)
      push_utf8_string(L, buf, -1);
    else
      lua_pushboolean(L, result);
  }
  else
    luaL_argerror(L, 1, "command not supported");

  return 1;
}

int far_ProcessName  (lua_State *L) { return _ProcessName(L, -1);              }
int far_CmpName      (lua_State *L) { return _ProcessName(L, PN_CMPNAME);      }
int far_CmpNameList  (lua_State *L) { return _ProcessName(L, PN_CMPNAMELIST);  }
int far_CheckMask    (lua_State *L) { return _ProcessName(L, PN_CHECKMASK);    }
int far_GenerateName (lua_State *L) { return _ProcessName(L, PN_GENERATENAME); }

int far_GetReparsePointInfo (lua_State *L)
{
  struct FarStandardFunctions* FSF = GetFSF(L);
  const wchar_t* Src = check_utf8_string(L, 1, NULL);
  int size = FSF->GetReparsePointInfo(Src, NULL, 0);
  if (size <= 0)
    return lua_pushnil(L), 1;
  wchar_t* Dest = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
  FSF->GetReparsePointInfo(Src, Dest, size);
  return push_utf8_string(L, Dest, -1), 1;
}

int far_LIsAlpha (lua_State *L)
{
  const wchar_t* str = check_utf8_string(L, 1, NULL);
  return lua_pushboolean(L, GetFSF(L)->LIsAlpha(*str)), 1;
}

int far_LIsAlphanum (lua_State *L)
{
  const wchar_t* str = check_utf8_string(L, 1, NULL);
  return lua_pushboolean(L, GetFSF(L)->LIsAlphanum(*str)), 1;
}

int far_LIsLower (lua_State *L)
{
  const wchar_t* str = check_utf8_string(L, 1, NULL);
  return lua_pushboolean(L, GetFSF(L)->LIsLower(*str)), 1;
}

int far_LIsUpper (lua_State *L)
{
  const wchar_t* str = check_utf8_string(L, 1, NULL);
  return lua_pushboolean(L, GetFSF(L)->LIsUpper(*str)), 1;
}

int convert_buf (lua_State *L, int command)
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
    GetFSF(L)->LLowerBuf(dest,len);
  else
    GetFSF(L)->LUpperBuf(dest,len);
  return push_utf8_string(L, dest, -1), 1;
}

int far_LLowerBuf (lua_State *L) {
  return convert_buf(L, 'l');
}

int far_LUpperBuf (lua_State *L) {
  return convert_buf(L, 'u');
}

int far_MkTemp (lua_State *L)
{
  const wchar_t* prefix = opt_utf8_string(L, 1, NULL);
  const int dim = 4096;
  wchar_t* dest = (wchar_t*)lua_newuserdata(L, dim * sizeof(wchar_t));
  if (GetFSF(L)->MkTemp(dest, dim, prefix))
    push_utf8_string(L, dest, -1);
  else
    lua_pushnil(L);
  return 1;
}

int far_MkLink (lua_State *L)
{
  const wchar_t* src = check_utf8_string(L, 1, NULL);
  const wchar_t* dst = check_utf8_string(L, 2, NULL);
  DWORD flags = CheckFlags(L, 3);
  return lua_pushboolean(L, GetFSF(L)->MkLink(src, dst, flags)), 1;
}

int far_GetPathRoot (lua_State *L)
{
  const wchar_t* Path = check_utf8_string(L, 1, NULL);
  wchar_t* Root = (wchar_t*)lua_newuserdata(L, 4096 * sizeof(wchar_t));
  *Root = L'\0';
  GetFSF(L)->GetPathRoot(Path, Root, 4096);
  return push_utf8_string(L, Root, -1), 1;
}

int truncstring (lua_State *L, int op)
{
  const wchar_t* Src = check_utf8_string(L, 1, NULL);
  int MaxLen = luaL_checkinteger(L, 2);
  int SrcLen = wcslen(Src);
  if (MaxLen < 0) MaxLen = 0;
  else if (MaxLen > SrcLen) MaxLen = SrcLen;
  wchar_t* Trg = (wchar_t*)lua_newuserdata(L, (1 + SrcLen) * sizeof(wchar_t));
  wcscpy(Trg, Src);
  const wchar_t* ptr = (op == 'p') ?
    GetFSF(L)->TruncPathStr(Trg, MaxLen) : GetFSF(L)->TruncStr(Trg, MaxLen);
  return push_utf8_string(L, ptr, -1), 1;
}

int far_TruncPathStr (lua_State *L)
{
  return truncstring(L, 'p');
}

int far_TruncStr (lua_State *L)
{
  return truncstring(L, 's');
}

typedef struct
{
  lua_State *L;
  int nparams;
  int err;
} FrsData;

int WINAPI FrsUserFunc (const struct FAR_FIND_DATA *FData, const wchar_t *FullName,
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

int far_RecursiveSearch (lua_State *L)
{
  DWORD Flags;
  FrsData Data = { L,0,0 };
  const wchar_t *InitDir = check_utf8_string(L, 1, NULL);
  wchar_t *Mask = check_utf8_string(L, 2, NULL);

  luaL_checktype(L, 3, LUA_TFUNCTION);
  Flags = CheckFlags(L,4);
  if (lua_gettop(L) == 3)
    lua_pushnil(L);

  Data.nparams = lua_gettop(L) - 4;
  lua_checkstack(L, 256);

  GetPluginStartupInfo(L)->FSF->FarRecursiveSearch(InitDir, Mask, FrsUserFunc, Flags, &Data);

  if(Data.err)
    LF_Error(L, check_utf8_string(L, -1, NULL));
  return Data.err ? 0 : lua_gettop(L) - Data.nparams - 4;
}

int far_ConvertPath (lua_State *L)
{
  const wchar_t *Src = check_utf8_string(L, 1, NULL);
  enum CONVERTPATHMODES Mode = lua_isnoneornil(L,2) ?
    CPM_FULL : (enum CONVERTPATHMODES)check_env_flag(L,2);
  PSInfo *Info = GetPluginStartupInfo(L);
  size_t Size = Info->FSF->ConvertPath(Mode, Src, NULL, 0);
  wchar_t* Target = (wchar_t*)lua_newuserdata(L, Size*sizeof(wchar_t));
  Info->FSF->ConvertPath(Mode, Src, Target, Size);
  push_utf8_string(L, Target, -1);
  return 1;
}

int win_GetFileInfo (lua_State *L)
{
  WIN32_FIND_DATAW fd;
  const wchar_t *fname = check_utf8_string(L, 1, NULL);
  HANDLE h = WINPORT(FindFirstFile)(fname, &fd);
  if (h == INVALID_HANDLE_VALUE)
    lua_pushnil(L);
  else {
    PushWinFindData(L, &fd);
    WINPORT(FindClose)(h);
  }
  return 1;
}

// os.getenv does not always work correctly, hence the following.
int win_GetEnv (lua_State *L)
{
  const char* name = luaL_checkstring(L, 1);
  const char* val = getenv(name);
  if (val) lua_pushstring(L, val);
  else lua_pushnil(L);
  return 1;
}

int win_SetEnv (lua_State *L)
{
  const char* name = luaL_checkstring(L, 1);
  const char* value = luaL_optstring(L, 2, NULL);
  int res = value ? setenv(name, value, 1) : unsetenv(name);
  lua_pushboolean (L, res == 0);
  return 1;
}

int DoAdvControl (lua_State *L, int Command, int Delta)
{
  int pos2 = 2-Delta, pos3 = 3-Delta;
  PSInfo *Info = GetPluginStartupInfo(L);
  intptr_t int1;
  wchar_t buf[300];
  COORD coord;

  if (Delta == 0)
    Command = check_env_flag(L, 1);

  switch (Command) {
    default:
      return luaL_argerror(L, 1, "command not supported");

    case ACTL_GETFARHWND:
      int1 = Info->AdvControl(Info->ModuleNumber, Command, NULL);
      return lua_pushlightuserdata(L, (void*)int1), 1;

    case ACTL_GETCONFIRMATIONS:
    case ACTL_GETDESCSETTINGS:
    case ACTL_GETDIALOGSETTINGS:
    case ACTL_GETINTERFACESETTINGS:
    case ACTL_GETPANELSETTINGS:
    case ACTL_GETPLUGINMAXREADDATA:
    case ACTL_GETSYSTEMSETTINGS:
    case ACTL_GETWINDOWCOUNT:
      int1 = Info->AdvControl(Info->ModuleNumber, Command, NULL);
      return lua_pushinteger(L, int1), 1;

    case ACTL_COMMIT:
    case ACTL_PROGRESSNOTIFY:
    case ACTL_QUIT:
    case ACTL_REDRAWALL:
      int1 = Info->AdvControl(Info->ModuleNumber, Command, NULL);
      return lua_pushboolean(L, int1), 1;

    case ACTL_GETCOLOR:
      int1 = check_env_flag(L, pos2);
      int1 = Info->AdvControl(Info->ModuleNumber, Command, (void*)int1);
      int1 >= 0 ? lua_pushinteger(L, int1) : lua_pushnil(L);
      return 1;

    case ACTL_WAITKEY:
      int1 = opt_env_flag(L, pos2, -1);
      if (int1 < -1) //this prevents program freeze
        int1 = -1;
      lua_pushinteger(L, Info->AdvControl(Info->ModuleNumber, Command, (void*)int1));
      return 1;

    case ACTL_SETCURRENTWINDOW:
      int1 = luaL_checkinteger(L, pos2) - 1;
      int1 = Info->AdvControl(Info->ModuleNumber, ACTL_SETCURRENTWINDOW, (void*)int1);
      if (int1 && lua_toboolean(L, pos3))
        int1 = Info->AdvControl(Info->ModuleNumber, ACTL_COMMIT, NULL);
      return lua_pushboolean(L, int1), 1;

    case ACTL_SETPROGRESSSTATE:
      int1 = check_env_flag(L, pos2);
      int1 = Info->AdvControl(Info->ModuleNumber, Command, (void*)int1);
      return lua_pushboolean(L, int1), 1;

    case ACTL_SETPROGRESSVALUE: {
      struct PROGRESSVALUE pv;
      luaL_checktype(L, pos2, LUA_TTABLE);
      lua_settop(L, pos2);
      pv.Completed = GetOptNumFromTable(L, "Completed", 0.0);
      pv.Total = GetOptNumFromTable(L, "Total", 100.0);
      lua_pushboolean(L, Info->AdvControl(Info->ModuleNumber, Command, &pv));
      return 1;
    }

    case ACTL_GETSYSWORDDIV:
      Info->AdvControl(Info->ModuleNumber, Command, buf);
      return push_utf8_string(L,buf,-1), 1;

    case ACTL_EJECTMEDIA: {
      struct ActlEjectMedia em;
      luaL_checktype(L, pos2, LUA_TTABLE);
      lua_getfield(L, pos2, "Letter");
      em.Letter = lua_isstring(L,-1) ? lua_tostring(L,-1)[0] : '\0';
      lua_getfield(L, pos2, "Flags");
      em.Flags = CheckFlags(L,-1);
      lua_pushboolean(L, Info->AdvControl(Info->ModuleNumber, Command, &em));
      return 1;
    }

    case ACTL_GETARRAYCOLOR: {
      int i;
      int size = Info->AdvControl(Info->ModuleNumber, Command, NULL);
      void *p = lua_newuserdata(L, size);
      Info->AdvControl(Info->ModuleNumber, Command, p);
      lua_createtable(L, size, 0);
      for (i=0; i < size; i++) {
        lua_pushinteger(L, i+1);
        lua_pushinteger(L, ((BYTE*)p)[i]);
        lua_rawset(L,-3);
      }
      return 1;
    }

    case ACTL_GETFARVERSION: {
      DWORD n = Info->AdvControl(Info->ModuleNumber, Command, 0);
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
        int r = Info->AdvControl(Info->ModuleNumber, Command, &wi);
        if (!r)
          return lua_pushnil(L), 1;
        wi.TypeName = (wchar_t*)
          lua_newuserdata(L, (wi.TypeNameSize + wi.NameSize) * sizeof(wchar_t));
        wi.Name = wi.TypeName + wi.TypeNameSize;
      }

      int r = Info->AdvControl(Info->ModuleNumber, Command, &wi);
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
      GetFlagCombination(L, -1, (int*)&fsc.Flags);
      fsc.ColorCount = lua_objlen(L, pos2);
      fsc.Colors = (BYTE*)lua_newuserdata(L, fsc.ColorCount);
      for (i=0; i < fsc.ColorCount; i++) {
        lua_pushinteger(L,i+1);
        lua_gettable(L,pos2);
        fsc.Colors[i] = lua_tointeger(L,-1);
        lua_pop(L,1);
      }
      lua_pushboolean(L, Info->AdvControl(Info->ModuleNumber, Command, &fsc));
      return 1;
    }

    case ACTL_GETFARRECT: {
      SMALL_RECT sr;
      if (Info->AdvControl(Info->ModuleNumber, Command, &sr)) {
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
      if (Info->AdvControl(Info->ModuleNumber, Command, &coord)) {
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
      lua_pushboolean(L, Info->AdvControl(Info->ModuleNumber, Command, &coord));
      return 1;

    //case ACTL_SYNCHRO:   //  not supported as it is used in far.Timer
    //case ACTL_KEYMACRO:  //  not supported as it's replaced by separate functions far.MacroXxx
  }
}

#define AdvCommand(name,command,delta) \
int adv_##name(lua_State *L) { return DoAdvControl(L,command,delta); }

int far_AdvControl(lua_State *L) { return DoAdvControl(L,0,0); }

AdvCommand( Commit,                 ACTL_COMMIT, 1)
AdvCommand( EjectMedia,             ACTL_EJECTMEDIA, 1)
AdvCommand( GetArrayColor,          ACTL_GETARRAYCOLOR, 1)
AdvCommand( GetColor,               ACTL_GETCOLOR, 1)
AdvCommand( GetConfirmations,       ACTL_GETCONFIRMATIONS, 1)
AdvCommand( GetCursorPos,           ACTL_GETCURSORPOS, 1)
AdvCommand( GetDescSettings,        ACTL_GETDESCSETTINGS, 1)
AdvCommand( GetDialogSettings,      ACTL_GETDIALOGSETTINGS, 1)
AdvCommand( GetFarHwnd,             ACTL_GETFARHWND, 1)
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
AdvCommand( ProgressNotify,         ACTL_PROGRESSNOTIFY, 1)
AdvCommand( Quit,                   ACTL_QUIT, 1)
AdvCommand( RedrawAll,              ACTL_REDRAWALL, 1)
AdvCommand( SetArrayColor,          ACTL_SETARRAYCOLOR, 1)
AdvCommand( SetCurrentWindow,       ACTL_SETCURRENTWINDOW, 1)
AdvCommand( SetCursorPos,           ACTL_SETCURSORPOS, 1)
AdvCommand( SetProgressState,       ACTL_SETPROGRESSSTATE, 1)
AdvCommand( SetProgressValue,       ACTL_SETPROGRESSVALUE, 1)
AdvCommand( WaitKey,                ACTL_WAITKEY, 1)

int far_CPluginStartupInfo(lua_State *L)
{
  return lua_pushlightuserdata(L, (void*)GetPluginStartupInfo(L)), 1;
}

#if 0
int win_GetTimeZoneInformation (lua_State *L)
{
  TIME_ZONE_INFORMATION tzi;
  DWORD res = GetTimeZoneInformation(&tzi);
  if (res == 0xFFFFFFFF)
    return lua_pushnil(L), 1;

  lua_createtable(L, 0, 5);
  PutNumToTable(L, "Bias", tzi.Bias);
  PutNumToTable(L, "StandardBias", tzi.StandardBias);
  PutNumToTable(L, "DaylightBias", tzi.DaylightBias);
  PutLStrToTable(L, "StandardName", tzi.StandardName, sizeof(WCHAR)*wcslen(tzi.StandardName));
  PutLStrToTable(L, "DaylightName", tzi.DaylightName, sizeof(WCHAR)*wcslen(tzi.DaylightName));

  lua_pushnumber(L, res);
  return 2;
}
#endif

void pushSystemTime (lua_State *L, const SYSTEMTIME *st)
{
  lua_createtable(L, 0, 8);
  PutIntToTable(L, "wYear", st->wYear);
  PutIntToTable(L, "wMonth", st->wMonth);
  PutIntToTable(L, "wDayOfWeek", st->wDayOfWeek);
  PutIntToTable(L, "wDay", st->wDay);
  PutIntToTable(L, "wHour", st->wHour);
  PutIntToTable(L, "wMinute", st->wMinute);
  PutIntToTable(L, "wSecond", st->wSecond);
  PutIntToTable(L, "wMilliseconds", st->wMilliseconds);
}

void pushFileTime (lua_State *L, const FILETIME *ft)
{
  long long llFileTime = ft->dwLowDateTime + 0x100000000ll * ft->dwHighDateTime;
  llFileTime /= 10000;
  lua_pushnumber(L, (double)llFileTime);
}

int win_GetSystemTimeAsFileTime (lua_State *L)
{
  FILETIME ft;
  WINPORT(GetSystemTimeAsFileTime)(&ft);
  pushFileTime(L, &ft);
  return 1;
}

int win_FileTimeToSystemTime (lua_State *L)
{
  FILETIME ft;
  SYSTEMTIME st;
  long long llFileTime = 10000 * (long long) luaL_checknumber(L, 1);
  ft.dwLowDateTime = llFileTime & 0xFFFFFFFF;
  ft.dwHighDateTime = llFileTime >> 32;
  if (! WINPORT(FileTimeToSystemTime)(&ft, &st))
    return lua_pushnil(L), 1;
  pushSystemTime(L, &st);
  return 1;
}

int win_SystemTimeToFileTime (lua_State *L)
{
  FILETIME ft;
  SYSTEMTIME st;
  memset(&st, 0, sizeof(st));
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_settop(L, 1);
  st.wYear         = GetOptIntFromTable(L, "wYear", 0);
  st.wMonth        = GetOptIntFromTable(L, "wMonth", 0);
  st.wDayOfWeek    = GetOptIntFromTable(L, "wDayOfWeek", 0);
  st.wDay          = GetOptIntFromTable(L, "wDay", 0);
  st.wHour         = GetOptIntFromTable(L, "wHour", 0);
  st.wMinute       = GetOptIntFromTable(L, "wMinute", 0);
  st.wSecond       = GetOptIntFromTable(L, "wSecond", 0);
  st.wMilliseconds = GetOptIntFromTable(L, "wMilliseconds", 0);
  if (! WINPORT(SystemTimeToFileTime)(&st, &ft))
    return lua_pushnil(L), 1;
  pushFileTime(L, &ft);
  return 1;
}

int win_FileTimeToLocalFileTime(lua_State *L)
{
  FILETIME ft, local_ft;
  long long llFileTime = (long long) luaL_checknumber(L, 1);
  llFileTime *= 10000; // convert from milliseconds to 1e-7

  ft.dwLowDateTime = llFileTime & 0xFFFFFFFF;
  ft.dwHighDateTime = llFileTime >> 32;

  if(WINPORT(FileTimeToLocalFileTime)(&ft, &local_ft))
    pushFileTime(L, &local_ft);
  else
    return SysErrorReturn(L);

  return 1;
}

int win_GetSystemTime(lua_State *L)
{
  SYSTEMTIME st;
  WINPORT(GetSystemTime)(&st);
  pushSystemTime(L, &st);
  return 1;
}

int win_CompareString (lua_State *L)
{
  size_t len1, len2;
  const wchar_t *ws1  = check_utf8_string(L, 1, &len1);
  const wchar_t *ws2  = check_utf8_string(L, 2, &len2);
  const char *sLocale = luaL_optstring(L, 3, "");
  const char *sFlags  = luaL_optstring(L, 4, "");

  LCID Locale = LOCALE_USER_DEFAULT;
  if      (!strcmp(sLocale, "s")) Locale = LOCALE_SYSTEM_DEFAULT;
  else if (!strcmp(sLocale, "n")) Locale = 0x0000; // LOCALE_NEUTRAL;

  DWORD dwFlags = 0;
  if (strchr(sFlags, 'c')) dwFlags |= NORM_IGNORECASE;
  if (strchr(sFlags, 'k')) dwFlags |= NORM_IGNOREKANATYPE;
  if (strchr(sFlags, 'n')) dwFlags |= NORM_IGNORENONSPACE;
  if (strchr(sFlags, 's')) dwFlags |= NORM_IGNORESYMBOLS;
  if (strchr(sFlags, 'w')) dwFlags |= NORM_IGNOREWIDTH;
  if (strchr(sFlags, 'S')) dwFlags |= SORT_STRINGSORT;

  int result = WINPORT(CompareString)(Locale, dwFlags, ws1, len1, ws2, len2) - 2;
  (result == -2) ? lua_pushnil(L) : lua_pushinteger(L, result);
  return 1;
}

int win_wcscmp (lua_State *L)
{
  const wchar_t *ws1  = check_utf8_string(L, 1, NULL);
  const wchar_t *ws2  = check_utf8_string(L, 2, NULL);
  int insens = lua_toboolean(L, 3);
  lua_pushinteger(L, (insens ? wcscasecmp : wcscmp)(ws1, ws2));
  return 1;
}

int far_MakeMenuItems (lua_State *L)
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

int far_Show (lua_State *L)
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

int far_InputRecordToName(lua_State* L)
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
  get_env_flag(L, -1, &event);
  if (! (event==0 || event==KEY_EVENT || event==FARMACRO_KEY_EVENT))
    return lua_pushnil(L), 1;

  lua_getfield(L, 1, "ControlKeyState");
  state = lua_tointeger(L,-1);
  if (state & 0x1F)
  {
    if      (state & 0x04) strcat(buf, "RCtrl");
    else if (state & 0x08) strcat(buf, "Ctrl");
    if      (state & 0x01) strcat(buf, "RAlt");
    else if (state & 0x02) strcat(buf, "Alt");
    if      (state & 0x10) strcat(buf, "Shift");
  }

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
      if ((state & 0x0F) || strlen(vk_name) > 1)  // Alt || Ctrl || virtual key is longer than 1 byte
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
    if (!vk_name && (state & 0x1F) && !uchar[0])
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

int win_GetVirtualKeys (lua_State *L)
{
  NewVirtualKeyTable(L, TRUE);
  return 1;
}

int win_Sleep (lua_State *L)
{
  unsigned usec = (unsigned) luaL_checknumber(L,1) * 1000; // msec -> mcsec
  usleep(usec);
  return 0;
}

int win_Clock (lua_State *L)
{
  struct timespec ts;
  if (0 != clock_gettime(CLOCK_MONOTONIC, &ts))
    luaL_error(L, "clock_gettime failed");
  lua_pushnumber(L, ts.tv_sec + (double)ts.tv_nsec/1e9);
  return 1;
}

int win_GetCurrentDir (lua_State *L)
{
  char *buf = (char*)lua_newuserdata(L, PATH_MAX*2);
  char *dir = getcwd(buf, PATH_MAX*2);
  if (dir) lua_pushstring(L,dir); else lua_pushnil(L);
  return 1;
}

int win_SetCurrentDir (lua_State *L)
{
  const char *dir = luaL_checkstring(L,1);
  lua_pushboolean(L, chdir(dir) == 0);
  return 1;
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

int far_CreateFileFilter (lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE hHandle = (luaL_checkinteger(L,1) % 2) ? PANEL_ACTIVE:PANEL_PASSIVE;
  int filterType = check_env_flag(L,2);
  HANDLE* pOutHandle = (HANDLE*)lua_newuserdata(L, sizeof(HANDLE));
  if (Info->FileFilterControl(hHandle, FFCTL_CREATEFILEFILTER, filterType,
    (LONG_PTR)pOutHandle))
  {
    luaL_getmetatable(L, FarFileFilterType);
    lua_setmetatable(L, -2);
  }
  else
    lua_pushnil(L);
  return 1;
}

int filefilter_Free (lua_State *L)
{
  HANDLE *h = CheckFileFilter(L, 1);
  if (*h != INVALID_HANDLE_VALUE) {
    PSInfo *Info = GetPluginStartupInfo(L);
    lua_pushboolean(L, Info->FileFilterControl(*h, FFCTL_FREEFILEFILTER, 0, 0));
    *h = INVALID_HANDLE_VALUE;
  }
  else
    lua_pushboolean(L,0);
  return 1;
}

int filefilter_gc (lua_State *L)
{
  filefilter_Free(L);
  return 0;
}

int filefilter_tostring (lua_State *L)
{
  HANDLE *h = CheckFileFilter(L, 1);
  if (*h != INVALID_HANDLE_VALUE)
    lua_pushfstring(L, "%s (%p)", FarFileFilterType, h);
  else
    lua_pushfstring(L, "%s (closed)", FarFileFilterType);
  return 1;
}

int filefilter_OpenMenu (lua_State *L)
{
  HANDLE h = CheckValidFileFilter(L, 1);
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushboolean(L, Info->FileFilterControl(h, FFCTL_OPENFILTERSMENU, 0, 0));
  return 1;
}

int filefilter_Starting (lua_State *L)
{
  HANDLE h = CheckValidFileFilter(L, 1);
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushboolean(L, Info->FileFilterControl(h, FFCTL_STARTINGTOFILTER, 0, 0));
  return 1;
}

int filefilter_IsFileInFilter (lua_State *L)
{
  struct FAR_FIND_DATA ffd;
  PSInfo *Info = GetPluginStartupInfo(L);
  HANDLE h = CheckValidFileFilter(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);
  lua_settop(L, 2);         // +2
  GetFarFindData(L, &ffd);  // +4
  lua_pushboolean(L, Info->FileFilterControl(h, FFCTL_ISFILEINFILTER, 0, (LONG_PTR)&ffd));
  return 1;
}

int plugin_load(lua_State *L, enum FAR_PLUGINS_CONTROL_COMMANDS command)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  int param1 = check_env_flag(L, 1);
  LONG_PTR param2 = (LONG_PTR)check_utf8_string(L, 2, NULL);
  int result = Info->PluginsControl(INVALID_HANDLE_VALUE, command, param1, param2);
  lua_pushboolean(L, result);
  return 1;
}

int far_LoadPlugin(lua_State *L)       { return plugin_load(L, PCTL_LOADPLUGIN); }
int far_ForcedLoadPlugin(lua_State *L) { return plugin_load(L, PCTL_FORCEDLOADPLUGIN); }
int far_UnloadPlugin(lua_State *L)     { return plugin_load(L, PCTL_UNLOADPLUGIN); }

int far_XLat (lua_State *L)
{
  size_t size;
  wchar_t *Line = check_utf8_string(L, 1, &size), *str;
  intptr_t StartPos = luaL_optinteger(L, 2, 1) - 1;
  intptr_t EndPos = luaL_optinteger(L, 3, size);
  int Flags = OptFlags(L, 4, 0);
  StartPos < 0 ? StartPos = 0 : StartPos > (intptr_t)size ? StartPos = size : 0;
  EndPos < StartPos ? EndPos = StartPos : EndPos > (intptr_t)size ? EndPos = size : 0;
  str = GetPluginStartupInfo(L)->FSF->XLat(Line, StartPos, EndPos, Flags);
  str ? push_utf8_string(L, str, -1) : lua_pushnil(L);
  return 1;
}

int far_Execute(lua_State *L)
{
  const wchar_t *CmdStr = check_utf8_string(L, 1, NULL);
  int ExecFlags = CheckFlags(L, 2);
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushinteger(L, Info->FSF->Execute(CmdStr, ExecFlags));
  return 1;
}

int far_ExecuteLibrary(lua_State *L)
{
  const wchar_t *Library = check_utf8_string(L, 1, NULL);
  const wchar_t *Symbol  = check_utf8_string(L, 2, NULL);
  const wchar_t *CmdStr  = check_utf8_string(L, 3, NULL);
  int ExecFlags = CheckFlags(L, 4);
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushinteger(L, Info->FSF->ExecuteLibrary(Library, Symbol, CmdStr, ExecFlags));
  return 1;
}

int far_DisplayNotification(lua_State *L)
{
  const wchar_t *action = check_utf8_string(L, 1, NULL);
  const wchar_t *object  = check_utf8_string(L, 2, NULL);
  PSInfo *Info = GetPluginStartupInfo(L);
  Info->FSF->DisplayNotification(action, object);
  return 0;
}

int far_DispatchInterThreadCalls(lua_State *L)
{
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushinteger(L, Info->FSF->DispatchInterThreadCalls());
  return 1;
}

int far_BackgroundTask(lua_State *L)
{
  const wchar_t *Info = check_utf8_string(L, 1, NULL);
  BOOL Started = lua_toboolean(L, 2);
  PSInfo *psInfo = GetPluginStartupInfo(L);
  psInfo->FSF->BackgroundTask(Info, Started);
  return 0;
}

void ConvertLuaValue (lua_State *L, int pos, struct FarMacroValue *target)
{
  INT64 val64;
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

int far_MacroLoadAll(lua_State* L)
{
  TPluginData *pd = GetPluginData(L);
  struct FarMacroLoad Data;
  Data.StructSize = sizeof(Data);
  Data.Path = opt_utf8_string(L, 1, NULL);
  Data.Flags = OptFlags(L, 2, 0);
  lua_pushboolean(L, pd->Info->MacroControl(pd->PluginId, MCTL_LOADALL, 0, &Data) != 0);
  return 1;
}

int far_MacroSaveAll(lua_State* L)
{
  TPluginData *pd = GetPluginData(L);
  lua_pushboolean(L, pd->Info->MacroControl(pd->PluginId, MCTL_SAVEALL, 0, 0) != 0);
  return 1;
}

int far_MacroGetState(lua_State* L)
{
  TPluginData *pd = GetPluginData(L);
  lua_pushinteger(L, pd->Info->MacroControl(pd->PluginId, MCTL_GETSTATE, 0, 0));
  return 1;
}

int far_MacroGetArea(lua_State* L)
{
  TPluginData *pd = GetPluginData(L);
  lua_pushinteger(L, pd->Info->MacroControl(pd->PluginId, MCTL_GETAREA, 0, 0));
  return 1;
}

int MacroSendString(lua_State* L, int Param1)
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
      (DWORD)pd->Info->FSF->FarNameToKey(check_utf8_string(L,3,NULL)) :
      (DWORD)luaL_optinteger(L,3,0);
  }

  lua_pushboolean(L, pd->Info->MacroControl(pd->PluginId, MCTL_SENDSTRING, Param1, &smt) != 0);
  return 1;
}

int far_MacroPost(lua_State* L)
{
  return MacroSendString(L, MSSC_POST);
}

int far_MacroCheck(lua_State* L)
{
  return MacroSendString(L, MSSC_CHECK);
}

int far_MacroGetLastError(lua_State* L)
{
  TPluginData *pd = GetPluginData(L);
  intptr_t size = pd->Info->MacroControl(pd->PluginId, MCTL_GETLASTERROR, 0, NULL);

  if(size)
  {
    struct MacroParseResult *mpr = (struct MacroParseResult*)lua_newuserdata(L, size);
    mpr->StructSize = sizeof(*mpr);
    pd->Info->MacroControl(pd->PluginId, MCTL_GETLASTERROR, size, mpr);
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

intptr_t WINAPI MacroAddCallback (void* Id, FARADDKEYMACROFLAGS Flags)
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

  if (pd->Info->MacroControl(pd->PluginId, MCTL_ADDMACRO, 0, &data))
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
    result = (int)pd->Info->MacroControl(pd->PluginId, MCTL_DELMACRO, 0, Id);
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

int far_MacroExecute(lua_State* L)
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

  if (pd->Info->MacroControl(pd->PluginId, MCTL_EXECSTRING, 0, &Data))
    PackMacroValues(L, Data.OutCount, Data.OutValues);
  else
    lua_pushnil(L);

  return 1;
}

int far_Log(lua_State *L)
{
  const char* txt = luaL_optstring(L, 1, "log message");
  Log(txt);
  return 0;
}

int far_ColorDialog(lua_State *L)
{
  PSInfo *info = GetPluginStartupInfo(L);
  WORD Color = (WORD)luaL_optinteger(L,1,0x0F);
  int Transparent = lua_toboolean(L,2);
  if (info->ColorDialog(info->ModuleNumber, &Color, Transparent))
    lua_pushinteger(L, Color);
  else
    lua_pushnil(L);
  return 1;
}

int far_GetConfigDir(lua_State *L)
{
  const char* dir = getenv("FARSETTINGS");
  if (dir) {
    lua_pushstring(L, dir);
    lua_pushstring(L, "/.config");
  }
  else {
    dir = getenv("HOME");
    if (!dir) luaL_error(L, "$HOME not found");
    lua_pushstring(L, dir);
    lua_pushstring(L, "/.config/far2l");
  }
  lua_concat(L,2);
  return 1;
}

int win_GetConsoleScreenBufferInfo (lua_State* L)
{
  CONSOLE_SCREEN_BUFFER_INFO info;
  HANDLE h = NULL; // GetStdHandle(STD_OUTPUT_HANDLE); //TODO: probably incorrect
  if (!WINPORT(GetConsoleScreenBufferInfo)(h, &info))
    return lua_pushnil(L), 1;
  lua_createtable(L, 0, 11);
  PutIntToTable(L, "SizeX",              info.dwSize.X);
  PutIntToTable(L, "SizeY",              info.dwSize.Y);
  PutIntToTable(L, "CursorPositionX",    info.dwCursorPosition.X);
  PutIntToTable(L, "CursorPositionY",    info.dwCursorPosition.Y);
  PutIntToTable(L, "Attributes",         info.wAttributes);
  PutIntToTable(L, "WindowLeft",         info.srWindow.Left);
  PutIntToTable(L, "WindowTop",          info.srWindow.Top);
  PutIntToTable(L, "WindowRight",        info.srWindow.Right);
  PutIntToTable(L, "WindowBottom",       info.srWindow.Bottom);
  PutIntToTable(L, "MaximumWindowSizeX", info.dwMaximumWindowSize.X);
  PutIntToTable(L, "MaximumWindowSizeY", info.dwMaximumWindowSize.Y);
  return 1;
}

int win_CopyFile (lua_State *L)
{
  FILE *inp, *out;
  int err;
  char buf[0x2000]; // 8 KiB
  const char* src = luaL_checkstring(L, 1);
  const char* trg = luaL_checkstring(L, 2);

  // a primitive (not sufficient) check but better than nothing
  if (!strcmp(src, trg)) {
    lua_pushnil(L);
    lua_pushstring(L, "input and output files are the same");
    return 2;
  }

  if(lua_gettop(L) > 2) {
    int fail_if_exists = lua_toboolean(L,3);
    if (fail_if_exists && (out=fopen(trg,"r"))) {
      fclose(out);
      lua_pushnil(L);
      lua_pushstring(L, "output file already exists");
      return 2;
    }
  }

  if (!(inp = fopen(src, "rb"))) {
    lua_pushnil(L);
    lua_pushstring(L, "cannot open input file");
    return 2;
  }

  if (!(out = fopen(trg, "wb"))) {
    fclose(inp);
    lua_pushnil(L);
    lua_pushstring(L, "cannot open output file");
    return 2;
  }

  while(1) {
    size_t rd, wr;
    rd = fread(buf, 1, sizeof(buf), inp);
    if (rd && (wr = fwrite(buf, 1, rd, out)) < rd)
      break;
    if (rd < sizeof(buf))
      break;
  }

  err = ferror(inp) || ferror(out);
  fclose(out);
  fclose(inp);
  if (!err) {
    lua_pushboolean(L,1);
    return 1;
  }
  lua_pushnil(L);
  lua_pushstring(L, "some error occured");
  return 2;
}

int win_MoveFile (lua_State *L)
{
  const wchar_t* src = check_utf8_string(L, 1, NULL);
  const wchar_t* trg = check_utf8_string(L, 2, NULL);
  const char* sFlags = luaL_optstring(L, 3, NULL);
  int flags = 0;
  if (sFlags) {
    if (strchr(sFlags, 'c')) flags |= MOVEFILE_COPY_ALLOWED;
    if (strchr(sFlags, 'd')) flags |= MOVEFILE_DELAY_UNTIL_REBOOT;
    if (strchr(sFlags, 'r')) flags |= MOVEFILE_REPLACE_EXISTING;
    if (strchr(sFlags, 'w')) flags |= MOVEFILE_WRITE_THROUGH;
  }
  if (WINPORT(MoveFileEx)(src, trg, flags))
    return lua_pushboolean(L, 1), 1;
  return SysErrorReturn(L);
}

int win_DeleteFile (lua_State *L)
{
  if (WINPORT(DeleteFile)(check_utf8_string(L, 1, NULL)))
    return lua_pushboolean(L, 1), 1;
  return SysErrorReturn(L);
}

BOOL dir_exist(const wchar_t* path)
{
  DWORD attr = WINPORT(GetFileAttributes)(path);
  return (attr != 0xFFFFFFFF) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

BOOL makedir (const wchar_t* path)
{
  BOOL result = FALSE;
  const wchar_t* src = path;
  wchar_t *p = wcsdup(path), *trg = p;
  while (*src) {
    if (*src == L'/') {
      *trg++ = L'/';
      do src++; while (*src == L'/');
    }
    else *trg++ = *src++;
  }
  if (trg > p && trg[-1] == '/') trg--;
  *trg = 0;

  wchar_t* q;
  for (q=p; *q; *q++=L'/') {
    q = wcschr(q, L'/');
    if (q != NULL)  *q = 0;
    if (q != p && !dir_exist(p) && !WINPORT(CreateDirectory)(p, NULL)) break;
    if (q == NULL) { result=TRUE; break; }
  }
  free(p);
  return result;
}

int win_CreateDir (lua_State *L)
{
  const wchar_t* path = check_utf8_string(L, 1, NULL);
  BOOL tolerant = lua_toboolean(L, 2);
  if (dir_exist(path)) {
    if (tolerant) return lua_pushboolean(L,1), 1;
    return lua_pushnil(L), lua_pushliteral(L, "directory already exists"), 2;
  }
  if (makedir(path))
    return lua_pushboolean(L, 1), 1;
  return SysErrorReturn(L);
}

int win_RemoveDir (lua_State *L)
{
  if (WINPORT(RemoveDirectory)(check_utf8_string(L, 1, NULL)))
    return lua_pushboolean(L, 1), 1;
  return SysErrorReturn(L);
}

int win_IsProcess64bit(lua_State *L)
{
  lua_pushboolean(L, sizeof(void*) == 8);
  return 1;
}

int ustring_sub(lua_State *L)
{
  size_t len;
  intptr_t from, to;
  const char* s = luaL_checklstring(L, 1, &len);
  len /= sizeof(wchar_t);
  from = luaL_optinteger(L, 2, 1);

  if(from < 0) from += len+1;

  if(--from < 0) from = 0;
  else if((size_t)from > len) from = len;

  to = luaL_optinteger(L, 3, -1);

  if(to < 0) to += len+1;

  if(to < from) to = from;
  else if((size_t)to > len) to = len;

  lua_pushlstring(L, s + from*sizeof(wchar_t), (to-from)*sizeof(wchar_t));
  return 1;
}

int ustring_len(lua_State *L)
{
  size_t len;
  (void) luaL_checklstring(L, 1, &len);
  lua_pushinteger(L, len / sizeof(wchar_t));
  return 1;
}

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

  {"AddHistory",           dlg_AddHistory},
  {"Close",                dlg_Close},
  {"EditUnchangedFlag",    dlg_EditUnchangedFlag},
  {"Enable",               dlg_Enable},
  {"EnableRedraw",         dlg_EnableRedraw},
  {"First",                dlg_First},
  {"GetCheck",             dlg_GetCheck},
  {"GetColor",             dlg_GetColor},
  {"GetComboboxEvent",     dlg_GetComboboxEvent},
  {"GetConstTextPtr",      dlg_GetConstTextPtr},
  {"GetCursorPos",         dlg_GetCursorPos},
  {"GetCursorSize",        dlg_GetCursorSize},
  {"GetDialogInfo",        dlg_GetDialogInfo},
  {"GetDlgItem",           dlg_GetDlgItem},
  {"GetDlgRect",           dlg_GetDlgRect},
  {"GetDropdownOpened",    dlg_GetDropdownOpened},
  {"GetEditPosition",      dlg_GetEditPosition},
  {"GetFocus",             dlg_GetFocus},
  {"GetItemData",          dlg_GetItemData},
  {"GetItemPosition",      dlg_GetItemPosition},
  {"GetSelection",         dlg_GetSelection},
  {"GetText",              dlg_GetText},
  {"GetTextLength",        dlg_GetTextLength},
  {"GetTextPtr",           dlg_GetTextPtr},
  {"Key",                  dlg_Key},
  {"ListAdd",              dlg_ListAdd},
  {"ListAddStr",           dlg_ListAddStr},
  {"ListDelete",           dlg_ListDelete},
  {"ListFindString",       dlg_ListFindString},
  {"ListGetCurPos",        dlg_ListGetCurPos},
  {"ListGetData",          dlg_ListGetData},
  {"ListGetDataSize",      dlg_ListGetDataSize},
  {"ListGetItem",          dlg_ListGetItem},
  {"ListGetTitles",        dlg_ListGetTitles},
  {"ListInfo",             dlg_ListInfo},
  {"ListInsert",           dlg_ListInsert},
  {"ListSet",              dlg_ListSet},
  {"ListSetCurPos",        dlg_ListSetCurPos},
  {"ListSetData",          dlg_ListSetData},
  {"ListSetMouseReaction", dlg_ListSetMouseReaction},
  {"ListSetTitles",        dlg_ListSetTitles},
  {"ListSort",             dlg_ListSort},
  {"ListUpdate",           dlg_ListUpdate},
  {"MoveDialog",           dlg_MoveDialog},
  {"Redraw",               dlg_Redraw},
  {"ResizeDialog",         dlg_ResizeDialog},
  {"Set3State",            dlg_Set3State},
  {"SetCheck",             dlg_SetCheck},
  {"SetColor",             dlg_SetColor},
  {"SetComboboxEvent",     dlg_SetComboboxEvent},
  {"SetCursorPos",         dlg_SetCursorPos},
  {"SetCursorSize",        dlg_SetCursorSize},
  {"SetDlgItem",           dlg_SetDlgItem},
  {"SetDropdownOpened",    dlg_SetDropdownOpened},
  {"SetEditPosition",      dlg_SetEditPosition},
  {"SetFocus",             dlg_SetFocus},
  {"SetHistory",           dlg_SetHistory},
  {"SetItemData",          dlg_SetItemData},
  {"SetItemPosition",      dlg_SetItemPosition},
  {"SetMaxTextLength",     dlg_SetMaxTextLength},
  {"SetMouseEventNotify",  dlg_SetMouseEventNotify},
  {"SetSelection",         dlg_SetSelection},
  {"SetText",              dlg_SetText},
  {"SetTextPtr",           dlg_SetTextPtr},
  {"ShowDialog",           dlg_ShowDialog},
  {"ShowItem",             dlg_ShowItem},
  {"User",                 dlg_User},
  {NULL, NULL},
};

static const luaL_Reg actl_funcs[] =
{
  {"Commit",                adv_Commit},
  {"EjectMedia",            adv_EjectMedia},
  {"GetArrayColor",         adv_GetArrayColor},
  {"GetColor",              adv_GetColor},
  {"GetConfirmations",      adv_GetConfirmations},
  {"GetCursorPos",          adv_GetCursorPos},
  {"GetDescSettings",       adv_GetDescSettings},
  {"GetDialogSettings",     adv_GetDialogSettings},
  {"GetFarHwnd",            adv_GetFarHwnd},
  {"GetFarRect",            adv_GetFarRect},
  {"GetFarVersion",         adv_GetFarVersion},
  {"GetInterfaceSettings",  adv_GetInterfaceSettings},
  {"GetPanelSettings",      adv_GetPanelSettings},
  {"GetPluginMaxReadData",  adv_GetPluginMaxReadData},
  {"GetShortWindowInfo",    adv_GetShortWindowInfo},
  {"GetSystemSettings",     adv_GetSystemSettings},
  {"GetSysWordDiv",         adv_GetSysWordDiv},
  {"GetWindowCount",        adv_GetWindowCount},
  {"GetWindowInfo",         adv_GetWindowInfo},
  {"ProgressNotify",        adv_ProgressNotify},
  {"Quit",                  adv_Quit},
  {"RedrawAll",             adv_RedrawAll},
  {"SetArrayColor",         adv_SetArrayColor},
  {"SetCurrentWindow",      adv_SetCurrentWindow},
  {"SetCursorPos",          adv_SetCursorPos},
  {"SetProgressState",      adv_SetProgressState},
  {"SetProgressValue",      adv_SetProgressValue},
  {"WaitKey",               adv_WaitKey},
  {NULL, NULL},
};

static const luaL_Reg viewer_funcs[] =
{
  {"Viewer",        viewer_Viewer},
  {"GetFileName",   viewer_GetFileName},
  {"GetInfo",       viewer_GetInfo},
  {"Quit",          viewer_Quit},
  {"Redraw",        viewer_Redraw},
  {"Select",        viewer_Select},
  {"SetKeyBar",     viewer_SetKeyBar},
  {"SetPosition",   viewer_SetPosition},
  {"SetMode",       viewer_SetMode},
  {NULL, NULL},
};

static const luaL_Reg editor_funcs[] =
{
  {"AddColor",            editor_AddColor},
  {"AddStackBookmark",    editor_AddStackBookmark},
  {"ClearStackBookmarks", editor_ClearStackBookmarks},
  {"DelColor",            editor_DelColor},
  {"DeleteBlock",         editor_DeleteBlock},
  {"DeleteChar",          editor_DeleteChar},
  {"DeleteStackBookmark", editor_DeleteStackBookmark},
  {"DeleteString",        editor_DeleteString},
  {"Editor",              editor_Editor},
  {"ExpandTabs",          editor_ExpandTabs},
  {"GetBookmarks",        editor_GetBookmarks},
  {"GetColor",            editor_GetColor},
  {"GetFileName",         editor_GetFileName},
  {"GetInfo",             editor_GetInfo},
  {"GetSelection",        editor_GetSelection},
  {"GetStackBookmarks",   editor_GetStackBookmarks},
  {"GetString",           editor_GetString},
  {"GetStringW",          editor_GetStringW},
  {"GetTitle",            editor_GetTitle},
  {"InsertString",        editor_InsertString},
  {"InsertText",          editor_InsertText},
  {"InsertTextW",         editor_InsertTextW},
  {"NextStackBookmark",   editor_NextStackBookmark},
  {"PrevStackBookmark",   editor_PrevStackBookmark},
  {"ProcessInput",        editor_ProcessInput},
  {"ProcessKey",          editor_ProcessKey},
  {"Quit",                editor_Quit},
  {"ReadInput",           editor_ReadInput},
  {"RealToTab",           editor_RealToTab},
  {"Redraw",              editor_Redraw},
  {"SaveFile",            editor_SaveFile},
  {"Select",              editor_Select},
  {"SetKeyBar",           editor_SetKeyBar},
  {"SetParam",            editor_SetParam},
  {"SetPosition",         editor_SetPosition},
  {"SetString",           editor_SetString},
  {"SetStringW",          editor_SetStringW},
  {"SetTitle",            editor_SetTitle},
  {"TabToReal",           editor_TabToReal},
  {"TurnOffMarkingBlock", editor_TurnOffMarkingBlock},
  {"UndoRedo",            editor_UndoRedo},
  {NULL, NULL},
};

static const luaL_Reg panel_funcs[] =
{
  {"BeginSelection",          panel_BeginSelection},
  {"CheckPanelsExist",        panel_CheckPanelsExist},
  {"ClearSelection",          panel_ClearSelection},
  {"ClosePlugin",             panel_ClosePlugin},
  {"EndSelection",            panel_EndSelection},
  {"GetCmdLine",              panel_GetCmdLine},
  {"GetCmdLinePos",           panel_GetCmdLinePos},
  {"GetCmdLineSelection",     panel_GetCmdLineSelection},
  {"GetColumnTypes",          panel_GetColumnTypes},
  {"GetColumnWidths",         panel_GetColumnWidths},
  {"GetCurrentPanelItem",     panel_GetCurrentPanelItem},
  {"GetPanelDirectory",       panel_GetPanelDirectory},
  {"GetPanelFormat",          panel_GetPanelFormat},
  {"GetPanelHostFile",        panel_GetPanelHostFile},
  {"GetPanelInfo",            panel_GetPanelInfo},
  {"GetPanelItem",            panel_GetPanelItem},
  {"GetPanelPluginHandle",    panel_GetPanelPluginHandle},
  {"GetPanelPrefix",          panel_GetPanelPrefix},
  {"GetSelectedPanelItem",    panel_GetSelectedPanelItem},
  {"GetUserScreen",           panel_GetUserScreen},
  {"InsertCmdLine",           panel_InsertCmdLine},
  {"IsActivePanel",           panel_IsActivePanel},
  {"RedrawPanel",             panel_RedrawPanel},
  {"SetActivePanel",          panel_SetActivePanel},
  {"SetCaseSensitiveSort",    panel_SetCaseSensitiveSort},
  {"SetCmdLine",              panel_SetCmdLine},
  {"SetCmdLinePos",           panel_SetCmdLinePos},
  {"SetCmdLineSelection",     panel_SetCmdLineSelection},
  {"SetDirectoriesFirst",     panel_SetDirectoriesFirst},
  {"SetNumericSort",          panel_SetNumericSort},
  {"SetPanelDirectory",       panel_SetPanelDirectory},
  {"SetSelection",            panel_SetSelection},
  {"SetSortMode",             panel_SetSortMode},
  {"SetSortOrder",            panel_SetSortOrder},
  {"SetUserScreen",           panel_SetUserScreen},
  {"SetViewMode",             panel_SetViewMode},
  {"UpdatePanel",             panel_UpdatePanel},
  {NULL, NULL},
};

static const luaL_Reg win_funcs[] = {
  {"GetConsoleScreenBufferInfo", win_GetConsoleScreenBufferInfo},
  {"CopyFile",                   win_CopyFile},
  {"DeleteFile",                 win_DeleteFile},
  {"MoveFile",                   win_MoveFile},
  {"RenameFile",                 win_MoveFile}, // alias
  {"CreateDir",                  win_CreateDir},
  {"RemoveDir",                  win_RemoveDir},

  {"GetEnv",                     win_GetEnv},
  {"SetEnv",                     win_SetEnv},
//$  {"GetTimeZoneInformation",  win_GetTimeZoneInformation},
  {"GetFileInfo",                win_GetFileInfo},
  {"FileTimeToLocalFileTime",    win_FileTimeToLocalFileTime},
  {"FileTimeToSystemTime",       win_FileTimeToSystemTime},
  {"SystemTimeToFileTime",       win_SystemTimeToFileTime},
  {"GetSystemTimeAsFileTime",    win_GetSystemTimeAsFileTime},
  {"GetSystemTime",              win_GetSystemTime},
  {"CompareString",              win_CompareString},
  {"wcscmp",                     win_wcscmp},
  {"ExtractKey",                 win_ExtractKey},
  {"GetVirtualKeys",             win_GetVirtualKeys},
  {"Sleep",                      win_Sleep},
  {"Clock",                      win_Clock},
  {"GetCurrentDir",              win_GetCurrentDir},
  {"SetCurrentDir",              win_SetCurrentDir},
  {"IsProcess64bit",             win_IsProcess64bit},

  {"EnumSystemCodePages",        ustring_EnumSystemCodePages },
  {"GetACP",                     ustring_GetACP},
  {"GetCPInfo",                  ustring_GetCPInfo},
  {"GetDriveType",               ustring_GetDriveType},
  {"GetOEMCP",                   ustring_GetOEMCP},
  {"MultiByteToWideChar",        ustring_MultiByteToWideChar },
  {"WideCharToMultiByte",        ustring_WideCharToMultiByte },
  {"OemToUtf8",                  ustring_OemToUtf8},
  {"Utf32ToUtf8",                ustring_Utf32ToUtf8},
  {"Utf8ToOem",                  ustring_Utf8ToOem},
  {"Utf8ToUtf32",                ustring_Utf8ToUtf32},
  {"lenW",                       ustring_len},
  {"subW",                       ustring_sub},
  {"Uuid",                       ustring_Uuid},
  {"GetFileAttr",                ustring_GetFileAttr},
  {"SetFileAttr",                ustring_SetFileAttr},
  {NULL, NULL},
};

static const luaL_Reg far_funcs[] = {
  {"PluginStartupInfo",   far_PluginStartupInfo},
  {"GetPluginId",         far_GetPluginId},

  {"CheckMask",           far_CheckMask},
  {"CmpName",             far_CmpName},
  {"CmpNameList",         far_CmpNameList},
  {"GenerateName",        far_GenerateName},
  {"DialogInit",          far_DialogInit},
  {"DialogRun",           far_DialogRun},
  {"DialogFree",          far_DialogFree},
  {"SendDlgMessage",      far_SendDlgMessage},
  {"GetDirList",          far_GetDirList},
  {"GetMsg",              far_GetMsg},
  {"GetPluginDirList",    far_GetPluginDirList},
  {"Menu",                far_Menu},
  {"Message",             far_Message},
  {"RestoreScreen",       far_RestoreScreen},
  {"SaveScreen",          far_SaveScreen},
  {"Text",                far_Text},
  {"ShowHelp",            far_ShowHelp},
  {"InputBox",            far_InputBox},
  {"AdvControl",          far_AdvControl},
  {"DefDlgProc",          far_DefDlgProc},
  {"CreateFileFilter",    far_CreateFileFilter},
  {"LoadPlugin",          far_LoadPlugin},
  {"ForcedLoadPlugin",    far_ForcedLoadPlugin},
  {"UnloadPlugin",        far_UnloadPlugin},

  {"CopyToClipboard",     far_CopyToClipboard},
  {"PasteFromClipboard",  far_PasteFromClipboard},
  {"KeyToName",           far_KeyToName},
  {"NameToKey",           far_NameToKey},
  {"InputRecordToKey",    far_InputRecordToKey},
  {"InputRecordToName",   far_InputRecordToName},
  {"LStricmp",            far_LStricmp},
  {"LStrnicmp",           far_LStrnicmp},
  {"ProcessName",         far_ProcessName},
  {"GetPathRoot",         far_GetPathRoot},
  {"GetReparsePointInfo", far_GetReparsePointInfo},
  {"LIsAlpha",            far_LIsAlpha},
  {"LIsAlphanum",         far_LIsAlphanum},
  {"LIsLower",            far_LIsLower},
  {"LIsUpper",            far_LIsUpper},
  {"LLowerBuf",           far_LLowerBuf},
  {"LUpperBuf",           far_LUpperBuf},
  {"MkTemp",              far_MkTemp},
  {"MkLink",              far_MkLink},
  {"TruncPathStr",        far_TruncPathStr},
  {"TruncStr",            far_TruncStr},
  {"RecursiveSearch",     far_RecursiveSearch},
  {"ConvertPath",         far_ConvertPath},
  {"XLat",                far_XLat},
  {"Execute",             far_Execute},
  {"ExecuteLibrary",      far_ExecuteLibrary},
  {"DisplayNotification", far_DisplayNotification},
  {"DispatchInterThreadCalls", far_DispatchInterThreadCalls},
  {"BackgroundTask",      far_BackgroundTask},

  {"ColorDialog",         far_ColorDialog},
  {"CPluginStartupInfo",  far_CPluginStartupInfo},
  {"GetCurrentDirectory", far_GetCurrentDirectory},
  {"GetFileFormat",       far_GetFileFormat},
  {"GetFileOwner",        far_GetFileOwner},
  {"GetNumberOfLinks",    far_GetNumberOfLinks},
  {"LuafarVersion",       far_LuafarVersion},
  {"MakeMenuItems",       far_MakeMenuItems},
  {"Show",                far_Show},
  {"MacroAdd",            far_MacroAdd},
  {"MacroDelete",         far_MacroDelete},
  {"MacroExecute",        far_MacroExecute},
  {"MacroGetArea",        far_MacroGetArea},
  {"MacroGetLastError",   far_MacroGetLastError},
  {"MacroGetState",       far_MacroGetState},
  {"MacroLoadAll",        far_MacroLoadAll},
  {"MacroSaveAll",        far_MacroSaveAll},
  {"MacroCheck",          far_MacroCheck},
  {"MacroPost",           far_MacroPost},
  {"Log",                 far_Log},
  {"GetConfigDir",        far_GetConfigDir},

  {NULL, NULL}
};

const char far_Dialog[] =
"function far.Dialog (X1,Y1,X2,Y2,HelpTopic,Items,Flags,DlgProc)\n\
  local hDlg = far.DialogInit(X1,Y1,X2,Y2,HelpTopic,Items,Flags,DlgProc)\n\
  if hDlg == nil then return nil end\n\
\n\
  local ret = far.DialogRun(hDlg)\n\
  for i, item in ipairs(Items) do\n\
    local newitem = hDlg:GetDlgItem(i)\n\
    if type(item[7]) == 'table' then\n\
      item[7].SelectIndex = newitem[7].SelectIndex\n\
    else\n\
      item[7] = newitem[7]\n\
    end\n\
    item[10] = newitem[10]\n\
  end\n\
\n\
  far.DialogFree(hDlg)\n\
  return ret\n\
end";

int luaopen_far (lua_State *L)
{
  PSInfo* Info = GetPluginStartupInfo(L);

  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, FAR_DN_STORAGE);

  NewVirtualKeyTable(L, FALSE);
  lua_setfield(L, LUA_REGISTRYINDEX, FAR_VIRTUALKEYS);

  lua_createtable(L, 0, 1500);
  add_flags(L);
  add_colors(L);
  add_keys(L);
  lua_pushvalue(L, -1);
  lua_replace (L, LUA_ENVIRONINDEX);

  luaL_register(L, "far", far_funcs);
  lua_insert(L, -2);
  lua_setfield(L, -2, "Flags");

  if (Info->StructSize > offsetof(PSInfo, Private) && Info->Private)
  {
    lua_pushcfunction(L, far_MacroCallFar);
    lua_setfield(L, -2, "MacroCallFar");
    lua_pushcfunction(L, far_FarMacroCallToLua);
    lua_setfield(L, -2, "FarMacroCallToLua");
  }

  (void)luaL_dostring(L, far_Guids);

  lua_newtable(L);
  lua_setglobal(L, "export");

  luaopen_regex(L);
  luaL_register(L, "editor", editor_funcs);
  luaL_register(L, "viewer", viewer_funcs);
  luaL_register(L, "panel",  panel_funcs);
  luaL_register(L, "win",    win_funcs);
  luaL_register(L, "actl",   actl_funcs);

  luaL_newmetatable(L, FarFileFilterType);
  lua_pushvalue(L,-1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, filefilter_methods);

  lua_getglobal(L, "far");
  lua_pushcfunction(L, luaopen_timer);
  lua_call(L, 0, 1);
  lua_setfield(L, -2, "Timer");

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
  lua_pop(L, 1);

  return 0;
}

// Run default script
BOOL LF_RunDefaultScript(lua_State* L)
{
  int pos = lua_gettop (L);

  // First: try to load the default script embedded into the plugin
  lua_getglobal(L, "require");
  lua_pushliteral(L, "<boot");
  int status = lua_pcall(L,1,1,0);
  if (status == 0) {
    status = pcall_msg(L,0,0);
    lua_settop (L, pos);
    return (status == 0);
  }

  // Second: try to load the default script from a disk file
  TPluginData* pd = GetPluginData(L);
  lua_pushstring(L, pd->ShareDir);
  push_utf8_string(L, wcsrchr(pd->Info->ModuleName,L'/')+1, -1);
  lua_concat(L,2);

  char* defscript = (char*)lua_newuserdata (L, lua_objlen(L,-1) + 8);
  strcpy(defscript, lua_tostring(L, -2));

  FILE *fp = NULL;
  const char delims[] = ".-";
  int i;
  for (i=0; delims[i]; i++) {
    char *end = strrchr(defscript, delims[i]);
    if (end) {
      strcpy(end, ".lua");
      if ((fp = fopen(defscript, "r")) != NULL)
        break;
    }
  }
  if (fp) {
    fclose(fp);
    status = luaL_loadfile(L, defscript);
    if (status == 0)
      status = pcall_msg(L,0,0);
    else
      LF_Error(L, utf8_to_wcstring (L, -1, NULL));
  }
  else
    LF_Error(L, L"Default script not found");

  lua_settop (L, pos);
  return (status == 0);
}

void LF_InitLuaState (lua_State *L, TPluginData *aPlugData, lua_CFunction aOpenLibs)
{
  int idx;
  lua_CFunction func_arr[] = { luaopen_far, luaopen_bit64, luaopen_unicode, luaopen_utf8 };

  // open Lua libraries
  luaL_openlibs(L);
  if (aOpenLibs) aOpenLibs(L);

  // open "far", "bit64", "unicode" and "utf8" libraries
  for (idx=0; idx < ARRAYSIZE(func_arr); idx++) {
    lua_pushcfunction(L, func_arr[idx]);
    lua_call(L, 0, 0);
  }

  // getmetatable("").__index = utf8
  lua_pushliteral(L, "");
  lua_getmetatable(L, -1);
  lua_getglobal(L, "utf8");
  lua_setfield(L, -2, "__index");
  lua_pop(L, 2);

  // Run "luafar_init.lua" residing in the Plugins/luafar directory (if any).
  // Absence of that file is not error.
  int top = lua_gettop(L);
  lua_pushstring(L, aPlugData->ShareDir);             //+1
  lua_pushliteral(L, "../../luafar_init.lua");        //+2
  lua_concat(L, 2);                                   //+1
  FILE *fp = fopen(lua_tostring(L,-1), "r");
  if (fp) {
    fclose(fp);
    if (luaL_loadfile(L,lua_tostring(L,-1)) || lua_pcall(L,0,0,0))
      LF_Error(L, utf8_to_wcstring(L,-1,NULL));
  }
  lua_settop(L,top);
}

// Initialize the interpreter
int LF_LuaOpen (TPluginData* aPlugData, lua_CFunction aOpenLibs)
{
  void *handle;
  lua_State *L;

  // without dlopen() all attempts to require() a binary Lua module would fail, e.g.
  // require "lfs" --> undefined symbol: lua_gettop
  handle = dlopen("libluajit-5.1.so", RTLD_NOW | RTLD_GLOBAL);
  if (handle == NULL)
    handle = dlopen("liblua5.1.so", RTLD_NOW | RTLD_GLOBAL);
  if (handle == NULL)
    return 0;

  // create Lua State
  L = lua_open();
  if (L) {
    // place pointer to plugin data in the L's registry -
    aPlugData->MainLuaState = L;
    aPlugData->dlopen_handle = handle;
    lua_pushlightuserdata(L, aPlugData);
    lua_setfield(L, LUA_REGISTRYINDEX, FAR_KEYINFO);

    // Evaluate the path where the scripts are (ShareDir)
    // It may (or may not) be the same as ModuleDir.
    const char *s1=  "/lib/far2l/Plugins/luafar/";
    const char *s2="/share/far2l/Plugins/luafar/";
    push_utf8_string(L, aPlugData->Info->ModuleName, -1);                  //+1
    aPlugData->ShareDir = (char*) malloc(lua_objlen(L,-1) + 8);
    strcpy(aPlugData->ShareDir, luaL_gsub(L, lua_tostring(L,-1), s1, s2)); //+2
    lua_pop(L,2);                                                          //+0
    strrchr(aPlugData->ShareDir,'/')[1] = '\0';

    LF_InitLuaState(L, aPlugData, aOpenLibs);
    return 1;
  }
  dlclose(handle);
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
    pd->dlopen_handle = NULL;
    LF_InitLuaState(L, pd, aOpenLibs);
  }
  return 0;
}
