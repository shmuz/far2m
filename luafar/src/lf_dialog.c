#include <lua.h>
#include <lauxlib.h>

#include "lf_luafar.h"
#include "lf_flags.h"
#include "lf_service.h"
#include "lf_bit64.h"
#include "lf_string.h"
#include "lf_util.h"

const char FarDialogType[]   = "FarDialog";
const char FAR_DN_STORAGE[]  = "FAR_DN_STORAGE";

static LONG_PTR GetEnableFromLua (lua_State *L, int pos)
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

int Dialog_getvalue(lua_State *L, int pos, HANDLE *target)
{
	if (lua_type(L, pos) == LUA_TUSERDATA)
	{
		int equal;
		lua_getmetatable(L, pos);
		luaL_getmetatable(L, FarDialogType);
		equal = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		if (equal && target)
		{
			*target = ((TDialogData*)lua_touserdata(L, pos))->hDlg;
			equal = *target != INVALID_HANDLE_VALUE;
		}
		return equal;
	}
	return 0;
}

// the table is on lua stack top
static flags_t GetItemFlags(lua_State* L, int flag_index, int item_index)
{
	int success;
	lua_pushinteger(L, flag_index);
	lua_gettable(L, -2);
	flags_t flags = GetFlagCombination(L, -1, &success);

	if (!success)
		return luaL_error(L, "unsupported flag in dialog item %d", item_index);

	lua_pop(L, 1);
	return flags;
}

static int GetDialogItemType(lua_State* L, int key, int item)
{
	int ok;
	lua_pushinteger(L, key);
	lua_gettable(L, -2);
	int iType = get_env_flag(L, -1, &ok);
	if (!ok) {
		const char* sType = lua_tostring(L, -1);
		return luaL_error(L, "%s - unsupported type in dialog item %d", sType, item);
	}
	lua_pop(L, 1);
	return iType;
}

// list table is on Lua stack top
static struct FarList* CreateList(lua_State *L, int historyindex)
{
	int n = (int)lua_objlen(L,-1);
	struct FarList* list = (struct FarList*)lua_newuserdata(L,
	                       sizeof(struct FarList) + n*sizeof(struct FarListItem)); // +2
	int len = (int)lua_objlen(L, historyindex);
	lua_rawseti(L, historyindex, ++len);  // +1; put into "histories" table to avoid being gc'ed
	list->ItemsNumber = n;
	list->Items = (struct FarListItem*)(list+1);
	for(int i=0; i<n; i++)
	{
		struct FarListItem *p = list->Items + i;
		lua_pushinteger(L, i+1); // +2
		lua_gettable(L,-2);      // +2
		if (lua_type(L,-1) != LUA_TTABLE)
			luaL_error(L, "value at index %d is not a table", i+1);
		p->Text = NULL;
		lua_getfield(L, -1, "Text"); // +3
		if (lua_isstring(L,-1))
		{
			lua_pushvalue(L,-1);                     // +4
			p->Text = check_utf8_string(L,-1,NULL);  // +4
			lua_rawseti(L, historyindex, ++len);     // +3
		}
		lua_pop(L, 1);                 // +2
		p->Flags = CheckFlagsFromTable(L, -1, "Flags");
		lua_pop(L, 1);                 // +1
	}
	return list;
}

// - This function, among other things, makes "conversion" from far3 to far2 API.
// - Item table is on Lua stack top.
static void SetFarDialogItem(lua_State *L, struct FarDialogItem* Item, int itemindex, int historyindex)
{
	++itemindex;
	flags_t Flags = GetItemFlags(L, 9, itemindex);
	memset(Item, 0, sizeof(*Item));

	// positions 1-5
	Item->Type  = GetDialogItemType (L, 1, itemindex);
	Item->X1    = GetIntFromArray   (L, 2);
	Item->Y1    = GetIntFromArray   (L, 3);
	Item->X2    = GetIntFromArray   (L, 4);
	Item->Y2    = GetIntFromArray   (L, 5);

	// position 6
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
		Item->VBuf = INVALID_HANDLE_VALUE; // Cope with dialog.cpp change taken from far2l on 2025-11-11
		                                   // (and consider that to be a temporary solution).
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

	// position 7
	if (Item->Type == DI_EDIT) {
		if (Flags & DIF_HISTORY) {
			lua_rawgeti(L, -1, 7);      // +1
			Item->History = opt_utf8_string (L, -1, NULL); // +1 --> Item->History and Item->Mask are aliases (union members)
			size_t len = lua_objlen(L, historyindex);
			lua_rawseti (L, historyindex, len+1); // +0; put into "histories" table to avoid being gc'ed
		}
	}

	// position 8
	if (Item->Type == DI_FIXEDIT) {
		if (Flags & DIF_MASKEDIT) {
			lua_rawgeti(L, -1, 8);      // +1
			Item->Mask = opt_utf8_string (L, -1, NULL); // +1 --> Item->History and Item->Mask are aliases (union members)
			size_t len = lua_objlen(L, historyindex);
			lua_rawseti (L, historyindex, len+1); // +0; put into "histories" table to avoid being gc'ed
		}
	}

	// position 9
	Item->Focus = (Flags & DIF_FOCUS) ? 1:0;
	Item->DefaultButton = (Flags & DIF_DEFAULTBUTTON) ? 1:0;
	Item->Flags = Flags & 0xFFFFFFFF;

	// position 10
	lua_pushinteger(L, 10); // +1
	lua_gettable(L, -2);    // +1
	if (lua_isstring(L, -1)) {
		Item->PtrData = check_utf8_string (L, -1, NULL); // +1
		size_t len = lua_objlen(L, historyindex);
		lua_rawseti (L, historyindex, len+1); // +0; put into "histories" table to avoid being gc'ed
	}
	else
		lua_pop(L, 1);

	// position 11
	Item->MaxLen = GetOptIntFromArray(L, 11, 0);
}

static void PushList (lua_State *L, const struct FarList *list)
{
	lua_createtable(L, list->ItemsNumber, 0);
	for (int i=0; i < list->ItemsNumber; i++)
	{
		lua_createtable(L, 0, 2);
		PutFlagsToTable(L, "Flags", list->Items[i].Flags);
		PutWStrToTable(L, "Text", list->Items[i].Text, -1);
		lua_rawseti(L, -2, i + 1);
		if (list->Items[i].Flags & LIF_SELECTED)
			PutIntToTable(L, "SelectIndex", i + 1);
	}
}

// This function, among other things, makes "conversion" from far2 to far3 API
static void PushDlgItem (lua_State *L, const struct FarDialogItem* pItem, BOOL table_exist)
{
	if (! table_exist)
		lua_createtable(L, 11, 0);

	// position 1-5
	PutIntToArray  (L, 1, pItem->Type);
	PutIntToArray  (L, 2, pItem->X1);
	PutIntToArray  (L, 3, pItem->Y1);
	PutIntToArray  (L, 4, pItem->X2);
	PutIntToArray  (L, 5, pItem->Y2);

	// position 6
	if ((pItem->Type == DI_LISTBOX || pItem->Type == DI_COMBOBOX) && pItem->ListItems)
	{
		PushList(L, pItem->ListItems);
		lua_rawseti(L, -2, 6);
	}
	else if (pItem->Type == DI_USERCONTROL)
	{
		//-- "bad light userdata pointer" error with old LuaJIT builds.
		//-- See: https://github.com/shmuz/far2m/issues/102.
		// lua_pushlightuserdata(L, pItem->VBuf);
		lua_pushinteger(L, 0); // dummy; to avoid a hole in the array
		lua_rawseti(L, -2, 6);
	}
	else
	{
		int sel = (pItem->Type == DI_CHECKBOX || pItem->Type == DI_RADIOBUTTON) ? pItem->Selected : 0;
		PutIntToArray(L, 6, sel);
	}

	// position 7
	const wchar_t *str;
	str = (pItem->Type == DI_EDIT && (pItem->Flags & DIF_HISTORY)) ? pItem->History : L"";
	PutWStrToArray(L, 7, str, -1);

	// position 8
	str = (pItem->Type == DI_FIXEDIT && (pItem->Flags & DIF_MASKEDIT)) ? pItem->Mask : L"";
	PutWStrToArray(L, 8, str, -1);

	// position 9
	flags_t Flags = pItem->Flags
			| (pItem->Focus ? DIF_FOCUS : 0) | (pItem->DefaultButton ? DIF_DEFAULTBUTTON : 0);
	PutFlagsToArray(L, 9, Flags);

	// position 10-11
	PutWStrToArray(L, 10, pItem->PtrData, -1);
	PutIntToArray(L, 11, pItem->MaxLen);
}

LONG_PTR SendDlgMessage(HANDLE hDlg, int Msg, int Param1, void *Param2)
{
	return PSInfo.SendDlgMessage(hDlg, Msg, Param1, (LONG_PTR)Param2);
}

static void PushDlgItemNum(lua_State *L, HANDLE hDlg, int numitem, int pos_table)
{
	int size = SendDlgMessage(hDlg, DM_GETDLGITEM, numitem, NULL);
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

	if (isOwned)
	{
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
		case DM_GETCOMBOBOXEVENT:
		case DM_GETCONSTTEXTPTR:
		case DM_GETCURSORPOS:
		case DM_GETCURSORSIZE:
		case DM_GETDEFAULTCOLOR:
		case DM_GETDLGITEM:
		case DM_GETEDITPOSITION:
		case DM_GETITEMDATA:
		case DM_GETITEMPOSITION:
		case DM_GETMEMOEDITID:
		case DM_GETSELECTION:
		case DM_GETTEXT:
		case DM_GETCOLOR:  //same as DM_GETTRUECOLOR
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
		case DM_SETTEXTPTRSILENT:
		case DM_SETCOLOR:  //same as DM_SETTRUECOLOR
		case DM_SHOWITEM:
		case DM_SETREADONLY:
		// DN_*
		case DN_BTNCLICK:
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
		case DM_SETMOUSEEVENTNOTIFY:
		case DM_SHOWDIALOG:
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

	// Param2 and the rest
	switch(Msg)
	{
		default:
			luaL_argerror(L, pos2, "operation not implemented");
			break;

		case DM_CLOSE:
		case DM_EDITUNCHANGEDFLAG:
		case DM_GETCHECK:
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
		case DM_USER:
		// DN_*
		case DN_BTNCLICK:
		case DN_DROPDOWNOPENED:
			Param2 = luaL_optlong(L, pos4, 0);
			break;

		case DM_ENABLEREDRAW:
			break;

		case DM_ENABLE:
		case DM_SHOWITEM:
			Param2 = GetEnableFromLua(L, pos4);
			break;

		case DM_LISTGETDATASIZE:
			Param2 = luaL_checkinteger(L, pos4) - 1;
			break;

		case DM_LISTADDSTR:
		case DM_ADDHISTORY:
		case DM_SETTEXTPTR:
		case DM_SETTEXTPTRSILENT:
			Param2 = (LONG_PTR) check_utf8_string(L, pos4, NULL);
			break;

		case DM_SETHISTORY:
			Param2 = (LONG_PTR) opt_utf8_string(L, pos4, NULL);
			break;

		case DM_SETCHECK:
			Param2 = lua_isboolean(L, pos4)
				? (lua_toboolean(L, pos4) ? BSTATE_CHECKED : BSTATE_UNCHECKED)
				: check_env_flag(L, pos4);
			break;

		case DM_GETCURSORPOS:
			if (SendDlgMessage(hDlg, Msg, Param1, &coord))
			{
				lua_createtable(L,0,2);
				PutNumToTable(L, "X", coord.X);
				PutNumToTable(L, "Y", coord.Y);
				return 1;
			}
			return lua_pushnil(L), 1;

		case DM_GETDIALOGINFO:
		{
			struct DialogInfo dlg_info = { sizeof(dlg_info) };
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
			TDialogData *dd = (TDialogData*) SendDlgMessage(hDlg,Msg,0,NULL);
			lua_rawgeti(L, LUA_REGISTRYINDEX, dd->dataRef);
			return 1;
		}

		case DM_SETDLGDATA: {
			TDialogData *dd = (TDialogData*) SendDlgMessage(hDlg,DM_GETDLGDATA,0,NULL);
			lua_rawgeti(L, LUA_REGISTRYINDEX, dd->dataRef);
			luaL_unref(L, LUA_REGISTRYINDEX, dd->dataRef);
			lua_pushvalue(L, pos3);
			dd->dataRef = luaL_ref(L, LUA_REGISTRYINDEX);
			return 1;
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
				PutNumToTable(L, "BlockType", es.BlockType);
				PutNumToTable(L, "BlockStartLine", es.BlockStartLine+1);
				PutNumToTable(L, "BlockStartPos", es.BlockStartPos+1);
				PutNumToTable(L, "BlockWidth", es.BlockWidth);
				PutNumToTable(L, "BlockHeight", es.BlockHeight);
				return 1;
			}
			return lua_pushnil(L), 1;
		}

		case DM_SETSELECTION:
		{
			struct EditorSelect es;
			luaL_checktype(L, pos4, LUA_TTABLE);

			if (FillEditorSelect(L, pos4, &es))
				lua_pushinteger(L, SendDlgMessage(hDlg, Msg, Param1, &es));
			else
				lua_pushinteger(L,0);

			return 1;
		}

		case DM_GETTEXT:
		case DM_GETDIALOGTITLE:
		{
			struct FarDialogItemData fdid;
			fdid.PtrLength = (size_t) SendDlgMessage(hDlg, Msg, Param1, NULL);
			fdid.PtrData = (wchar_t*) malloc((fdid.PtrLength+1) * sizeof(wchar_t));
			size_t size = SendDlgMessage(hDlg, Msg, Param1, &fdid);
			push_utf8_string(L, size ? fdid.PtrData : L"", size);
			free(fdid.PtrData);
			return 1;
		}

		case DM_GETCONSTTEXTPTR: {
			const wchar_t *ptr = (wchar_t*)SendDlgMessage(hDlg, Msg, Param1, NULL);
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

		case DM_KEY:
		{
			luaL_checktype(L, pos4, LUA_TTABLE);
			res = lua_objlen(L, pos4);
			if (res) {
				DWORD* arr = (DWORD*)lua_newuserdata(L, res * sizeof(DWORD));
				for(int i=0; i<res; i++) {
					lua_pushinteger(L,i+1);
					lua_gettable(L,pos4);
					arr[i] = lua_tointeger(L,-1);
					lua_pop(L,1);
				}
				res = SendDlgMessage (hDlg, Msg, res, arr);
			}
			return lua_pushinteger(L, res), 1;
		}

		case DM_LISTADD:
		case DM_LISTSET:
		{
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
				lua_pushinteger(L, SendDlgMessage(hDlg, Msg, Param1, NULL));
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
			struct FarListFind flf = {};
			luaL_checktype(L, pos4, LUA_TTABLE);
			flf.StartIndex = GetOptIntFromTable(L, "StartIndex", 1) - 1;
			lua_getfield(L, pos4, "Pattern");
			flf.Pattern = check_utf8_string(L, -1, NULL);
			lua_getfield(L, pos4, "Flags");
			flf.Flags = GetFlagCombination(L, -1, NULL);
			res = SendDlgMessage(hDlg, Msg, Param1, &flf);
			res < 0 ? lua_pushnil(L) : lua_pushinteger(L, res+1);
			return 1;
		}

		case DM_LISTGETCURPOS:
		{
			struct FarListPos flp = {};
			SendDlgMessage(hDlg, Msg, Param1, &flp);
			lua_createtable(L,0,2);
			PutIntToTable(L, "SelectPos", flp.SelectPos+1);
			PutIntToTable(L, "TopPos", flp.TopPos+1);
			return 1;
		}

		case DM_LISTGETITEM:
		{
			struct FarListGetItem flgi = {};
			flgi.ItemIndex = luaL_checkinteger(L, pos4) - 1;
			if (SendDlgMessage(hDlg, Msg, Param1, &flgi))
			{
				lua_createtable(L,0,2);
				PutFlagsToTable(L, "Flags", flgi.Item.Flags);
				PutWStrToTable(L, "Text", flgi.Item.Text, -1);
				return 1;
			}

			return lua_pushnil(L), 1;
		}

		case DM_LISTGETTITLES:
		{
			struct FarListTitles flt = {};
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
			struct FarListTitles flt = {};
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
			struct FarListInfo fli = {};
			if (SendDlgMessage(hDlg, Msg, Param1, &fli))
			{
				lua_createtable(L,0,6);
				PutFlagsToTable(L, "Flags", fli.Flags);
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
			struct FarListInsert flins = {};
			luaL_checktype(L, pos4, LUA_TTABLE);
			flins.Index = GetOptIntFromTable(L, "Index", 1) - 1;
			lua_getfield(L, pos4, "Text");
			flins.Item.Text = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
			lua_getfield(L, pos4, "Flags"); //+1
			flins.Item.Flags = OptFlags(L, -1, 0);
			res = SendDlgMessage(hDlg, Msg, Param1, &flins);
			res < 0 ? lua_pushnil(L) : lua_pushinteger(L, res);
			return 1;
		}

		case DM_LISTUPDATE:
		{
			struct FarListUpdate flu = {};
			luaL_checktype(L, pos4, LUA_TTABLE);
			flu.Index = GetOptIntFromTable(L, "Index", 1) - 1;
			lua_getfield(L, pos4, "Text");
			flu.Item.Text = lua_isstring(L,-1) ? check_utf8_string(L,-1,NULL) : NULL;
			flu.Item.Flags = CheckFlagsFromTable(L, pos4, "Flags");
			lua_pushboolean(L, SendDlgMessage(hDlg, Msg, Param1, &flu) != 0);
			return 1;
		}

		case DM_LISTSETCURPOS:
		{
			struct FarListPos flp = {};
			luaL_checktype(L, pos4, LUA_TTABLE);
			flp.SelectPos = GetOptIntFromTable(L, "SelectPos", 1) - 1;
			flp.TopPos = GetOptIntFromTable(L, "TopPos", 1) - 1;
			lua_pushinteger(L, 1 + SendDlgMessage(hDlg, Msg, Param1, &flp));
			return 1;
		}

		case DM_LISTSETDATA:
		{
			struct FarListItemData flid = {};
			luaL_checktype(L, pos4, LUA_TTABLE);
			int Index = GetOptIntFromTable(L, "Index", 1) - 1;
			lua_getfenv(L, 1);
			lua_getfield(L, pos4, "Data");
			if (lua_isnil(L,-1)) // nil is not allowed
			{
				lua_pushinteger(L,0);
				return 1;
			}

			listdata_t *oldData = (listdata_t*)PSInfo.SendDlgMessage(hDlg, DM_LISTGETDATA, Param1, Index);
			if (oldData &&
				sizeof(listdata_t) == PSInfo.SendDlgMessage(hDlg, DM_LISTGETDATASIZE, Param1, Index) &&
				oldData->Id == pluginData)
			{
				luaL_unref(L, -2, oldData->Ref);
			}
			listdata_t Data;
			Data.Id = pluginData;
			Data.Ref = luaL_ref(L, -2);
			flid.Index = Index;
			flid.Data = &Data;
			flid.DataSize = sizeof(Data);
			lua_pushinteger(L, SendDlgMessage(hDlg, Msg, Param1, &flid));
			return 1;
		}

		case DM_LISTGETDATA:
		{
			int Index = (int)luaL_checkinteger(L, pos4) - 1;
			listdata_t *Data = (listdata_t*)PSInfo.SendDlgMessage(hDlg, DM_LISTGETDATA, Param1, Index);
			if (Data)
			{
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
			luaL_checktype(L, pos4, LUA_TTABLE);
			coord.X = GetOptIntFromTable(L, "X", 0);
			coord.Y = GetOptIntFromTable(L, "Y", 0);

			if (Msg == DM_SETCURSORPOS)
			{
				lua_pushinteger(L, SendDlgMessage(hDlg, Msg, Param1, &coord));
				return 1;
			}

			COORD *c = (COORD*) SendDlgMessage(hDlg, Msg, Param1, &coord);
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
			Param2 = OptFlags(L, pos4, 0);
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

		case DM_GETMEMOEDITID:
		{
			int MemoId;
			if (SendDlgMessage(hDlg, Msg, Param1, &MemoId))
				lua_pushinteger(L, MemoId);
			else
				lua_pushboolean(L, 0);
			return 1;
		}

		case DM_GETDEFAULTCOLOR:
		case DM_GETCOLOR: //same as DM_GETTRUECOLOR
		{
			const int MAXCOLORS = DLG_ITEM_MAX_CUST_COLORS;
			uint64_t Colors[MAXCOLORS];
			SendDlgMessage(hDlg, Msg, Param1, Colors);
			lua_createtable(L, MAXCOLORS, 0);
			for (int i=0; i < MAXCOLORS; i++) {
				PushFarColor(L, Colors[i]);
				lua_rawseti(L, -2, i+1);
			}
			return 1;
		}

		case DM_SETCOLOR: {  //same as DM_SETTRUECOLOR
			const int MAXCOLORS = DLG_ITEM_MAX_CUST_COLORS;
			uint64_t Colors[MAXCOLORS];
			luaL_argcheck(L, lua_istable(L,pos4), pos4, "table expected");
			memset(Colors, 0, sizeof(Colors));
			for (int i=0; i < MAXCOLORS; i++) {
				lua_rawgeti(L, pos4, i+1);
				if (!lua_isnil(L, -1)) {
					Colors[i] = GetFarColor64(L, -1);
				}
				lua_pop(L,1);
			}
			lua_pushinteger (L, SendDlgMessage(hDlg, Msg, Param1, Colors));
			return 1;
		}

		case DM_LISTSETMOUSEREACTION:
			Param2 = get_env_flag(L, pos4, NULL);
			break;
	}

	res = PSInfo.SendDlgMessage(hDlg, Msg, Param1, Param2);
	lua_pushinteger(L, res + res_incr);
	return 1;
}

#define DlgMethod(name,msg) \
static int dlg_##name(lua_State *L) { return DoSendDlgMessage(L,msg,1); }

static int far_SendDlgMessage(lua_State *L) { return DoSendDlgMessage(L,0,0); }

DlgMethod( AddHistory,             DM_ADDHISTORY)
DlgMethod( Close,                  DM_CLOSE)
DlgMethod( EditUnchangedFlag,      DM_EDITUNCHANGEDFLAG)
DlgMethod( Enable,                 DM_ENABLE)
DlgMethod( EnableRedraw,           DM_ENABLEREDRAW)
DlgMethod( GetCheck,               DM_GETCHECK)
DlgMethod( GetColor,               DM_GETCOLOR)
DlgMethod( GetComboboxEvent,       DM_GETCOMBOBOXEVENT)
DlgMethod( GetConstTextPtr,        DM_GETCONSTTEXTPTR)
DlgMethod( GetCursorPos,           DM_GETCURSORPOS)
DlgMethod( GetCursorSize,          DM_GETCURSORSIZE)
DlgMethod( GetDefaultColor,        DM_GETDEFAULTCOLOR)
DlgMethod( GetDialogInfo,          DM_GETDIALOGINFO)
DlgMethod( GetDialogTitle,         DM_GETDIALOGTITLE)
DlgMethod( GetDlgData,             DM_GETDLGDATA)
DlgMethod( GetDlgItem,             DM_GETDLGITEM)
DlgMethod( GetDlgRect,             DM_GETDLGRECT)
DlgMethod( GetDropdownOpened,      DM_GETDROPDOWNOPENED)
DlgMethod( GetEditPosition,        DM_GETEDITPOSITION)
DlgMethod( GetFocus,               DM_GETFOCUS)
DlgMethod( GetItemData,            DM_GETITEMDATA)
DlgMethod( GetItemPosition,        DM_GETITEMPOSITION)
DlgMethod( GetMemoEditId,          DM_GETMEMOEDITID)
DlgMethod( GetSelection,           DM_GETSELECTION)
DlgMethod( GetText,                DM_GETTEXT)
DlgMethod( Key,                    DM_KEY)
DlgMethod( ListAdd,                DM_LISTADD)
DlgMethod( ListAddStr,             DM_LISTADDSTR)
DlgMethod( ListDelete,             DM_LISTDELETE)
DlgMethod( ListFindString,         DM_LISTFINDSTRING)
DlgMethod( ListGetCurPos,          DM_LISTGETCURPOS)
DlgMethod( ListGetData,            DM_LISTGETDATA)
DlgMethod( ListGetDataSize,        DM_LISTGETDATASIZE)
DlgMethod( ListGetItem,            DM_LISTGETITEM)
DlgMethod( ListGetTitles,          DM_LISTGETTITLES)
DlgMethod( ListInfo,               DM_LISTINFO)
DlgMethod( ListInsert,             DM_LISTINSERT)
DlgMethod( ListSet,                DM_LISTSET)
DlgMethod( ListSetCurPos,          DM_LISTSETCURPOS)
DlgMethod( ListSetData,            DM_LISTSETDATA)
DlgMethod( ListSetMouseReaction,   DM_LISTSETMOUSEREACTION)
DlgMethod( ListSetTitles,          DM_LISTSETTITLES)
DlgMethod( ListSort,               DM_LISTSORT)
DlgMethod( ListUpdate,             DM_LISTUPDATE)
DlgMethod( MoveDialog,             DM_MOVEDIALOG)
DlgMethod( Redraw,                 DM_REDRAW)
DlgMethod( ResizeDialog,           DM_RESIZEDIALOG)
DlgMethod( Set3State,              DM_SET3STATE)
DlgMethod( SetCheck,               DM_SETCHECK)
DlgMethod( SetColor,               DM_SETCOLOR)
DlgMethod( SetComboboxEvent,       DM_SETCOMBOBOXEVENT)
DlgMethod( SetCursorPos,           DM_SETCURSORPOS)
DlgMethod( SetCursorSize,          DM_SETCURSORSIZE)
DlgMethod( SetDlgData,             DM_SETDLGDATA)
DlgMethod( SetDlgItem,             DM_SETDLGITEM)
DlgMethod( SetDropdownOpened,      DM_SETDROPDOWNOPENED)
DlgMethod( SetEditPosition,        DM_SETEDITPOSITION)
DlgMethod( SetFocus,               DM_SETFOCUS)
DlgMethod( SetHistory,             DM_SETHISTORY)
DlgMethod( SetItemData,            DM_SETITEMDATA)
DlgMethod( SetItemPosition,        DM_SETITEMPOSITION)
DlgMethod( SetMaxTextLength,       DM_SETMAXTEXTLENGTH)
DlgMethod( SetMouseEventNotify,    DM_SETMOUSEEVENTNOTIFY)
DlgMethod( SetReadOnly,            DM_SETREADONLY)
DlgMethod( SetSelection,           DM_SETSELECTION)
DlgMethod( SetText,                DM_SETTEXT)
DlgMethod( SetTextPtr,             DM_SETTEXTPTR)
DlgMethod( SetTextPtrSilent,       DM_SETTEXTPTRSILENT)
DlgMethod( ShowDialog,             DM_SHOWDIALOG)
DlgMethod( ShowItem,               DM_SHOWITEM)
DlgMethod( User,                   DM_USER)

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
		case DN_DROPDOWNOPENED:
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

	lua_pushinteger(L, Msg);       //+1
	lua_pushinteger(L, Param1);    //+2

	// Param2
	switch(Msg)
	{
		case DN_CTLCOLORDIALOG:
			PushFarColor(L, *(uint64_t*) Param2);
			break;

		case DN_CTLCOLORDLGITEM:
		{
			uint64_t *ItemColor = (uint64_t*) Param2;
			lua_createtable(L, DLG_ITEM_MAX_CUST_COLORS, 0);
			for(int i=0; i < DLG_ITEM_MAX_CUST_COLORS; i++) {
				PushFarColor(L, ItemColor[i]);
				lua_rawseti(L, -2, i+1);
			}
			break;
		}

		case DN_CTLCOLORDLGLIST:
		{
			struct FarListColors* flc = (struct FarListColors*) Param2;
			lua_createtable(L, flc->ColorCount, 1);
			PutFlagsToTable(L, "Flags", flc->Flags);
			for(int i=0; i < flc->ColorCount; i++)
			{
				PushFarColor(L, flc->Colors[i]);
				lua_rawseti(L, -2, i+1);
			}
			break;
		}

		case DN_DRAWDLGITEM:
		case DN_EDITCHANGE:
		{
			struct FarDialogItem fdi = *(struct FarDialogItem*)Param2;
			fdi.History = NULL; // clear possible garbage value sent by Far
			PushDlgItem(L, &fdi, FALSE);
			break;
		}

		case DN_HELP:
			push_utf8_string(L, Param2 ? (wchar_t*)Param2 : L"", -1);
			break;

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

		case DN_GETDIALOGINFO:
		{
			struct DialogInfo* di = (struct DialogInfo*) Param2;
			lua_pushlstring(L, (const char*) &di->Id, 16);
			break;
		}

		default:
			lua_pushinteger(L, Param2);  //+3
			break;
	}

	return TRUE;
}

LONG_PTR ProcessDNResult(lua_State *L, int Msg, LONG_PTR Param2)
{
	LONG_PTR ret = 0;
	switch(Msg)
	{
		case DN_CTLCOLORDLGLIST:
			ret = lua_istable(L,-1);
			if (ret) {
				struct FarListColors* flc = (struct FarListColors*) Param2;
				for (int i=0; i < flc->ColorCount; i++) {
					lua_rawgeti(L, -1, i+1);
					flc->Colors[i] = GetFarColor64(L, -1);
					lua_pop(L, 1);
				}
			}
			break;

		case DN_CTLCOLORDLGITEM:
			if (lua_istable(L,-1))
			{
				uint64_t *ItemColor = (uint64_t*) Param2;
				for(int i = 0; i < 4; i++)
				{
					lua_rawgeti(L, -1, i+1);
					if (!lua_isnil(L, -1)) {
						ItemColor[i] = GetFarColor64(L, -1);
					}
					lua_pop(L, 1);
				}
				ret = 1;
			}
			break;

		case DN_CTLCOLORDIALOG:
			if (lua_istable(L, -1) || lua_isnumber(L, -1)) {
				uint64_t *Color = (uint64_t*) Param2;
				Color[0] = GetFarColor64(L, -1);
				ret = 1;
			}
			break;

		case DN_HELP:
			if (lua_type(L, -1) == LUA_TSTRING) {
				ret = (LONG_PTR) utf8_to_wcstring(L, -1, NULL);
				if (ret)
				{
					lua_getfield(L, LUA_REGISTRYINDEX, FAR_DN_STORAGE);
					lua_pushvalue(L, -2);                // keep stack balanced
					lua_setfield(L, -2, "helpstring");   // protect from garbage collector
					lua_pop(L, 1);
				}
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

static BOOL NonModal(TDialogData *dd)
{
	return dd && !dd->isModal;
}

static LONG_PTR WINAPI DlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	TDialogData *dd = (TDialogData*) PSInfo.SendDlgMessage(hDlg,DM_GETDLGDATA,0,0);
	if (dd->wasError)
		return PSInfo.DefDlgProc(hDlg, Msg, Param1, Param2);

	if (Msg == DN_GETDIALOGINFO)
		 return FALSE;

	lua_State *L = dd->L; // the dialog may be called from a lua_State other than the main one
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

		case DN_CTLCOLORDIALOG:
		case DN_CTLCOLORDLGITEM:
		case DN_CTLCOLORDLGLIST:
		case DN_DRAWDLGITEM:
		case DN_DROPDOWNOPENED:
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
		case DN_CTLCOLORDIALOG:
		case DN_CTLCOLORDLGITEM:
		case DN_CTLCOLORDLGLIST:
		case DN_HELP:
		case DN_KILLFOCUS:
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
	TPluginData *pd = GetPluginData(L);

	GUID Id = OptGuid(L, 1);
	int X1 = luaL_checkinteger(L, 2);
	int Y1 = luaL_checkinteger(L, 3);
	int X2 = luaL_checkinteger(L, 4);
	int Y2 = luaL_checkinteger(L, 5);
	const wchar_t *HelpTopic = opt_utf8_string(L, 6, NULL);

	luaL_checktype(L, 7, LUA_TTABLE);
	lua_newtable (L); // create a "histories" table, to prevent history strings
										// from being garbage collected too early
	lua_replace (L, POS_HISTORIES);
	int ItemsNumber = lua_objlen(L, 7);
	struct FarDialogItem *Items = (struct FarDialogItem*)
			lua_newuserdata(L, ItemsNumber * sizeof(struct FarDialogItem));
	lua_replace (L, POS_ITEMS);

	for(int i=0; i < ItemsNumber; i++) {
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
	flags_t Flags = OptFlags(L,8,0);
	TDialogData *dd = NewDialogData(L, INVALID_HANDLE_VALUE, TRUE);
	dd->isModal = (Flags&FDLG_NONMODAL) == 0;

	// 9-th parameter (DlgProc function)
	FARAPIDEFDLGPROC Proc = NULL;
	LONG_PTR Param = 0;
	if (lua_isfunction(L, 9)) {
		Proc = DlgProc;
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

	if (lua_isfunction(L, 9))
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

	if (dd->wasError)
	{
		free_dialog(dd);
		// sometimes parts of the closed dialog are left on the screen; ACTL_COMMIT clears them
		PSInfo.AdvControl(GetPluginData(L)->ModuleNumber, ACTL_COMMIT, NULL, NULL);
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
	if (dd->hDlg != INVALID_HANDLE_VALUE)
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

static const luaL_Reg dialog_methods[] =
{
	{"__gc",                 far_DialogFree},
	{"__tostring",           dialog_tostring},
	{"rawhandle",            dialog_rawhandle},
	{"send",                 far_SendDlgMessage},

	PAIR( dlg, AddHistory),
	PAIR( dlg, Close),
	PAIR( dlg, EditUnchangedFlag),
	PAIR( dlg, Enable),
	PAIR( dlg, EnableRedraw),
	PAIR( dlg, GetCheck),
	PAIR( dlg, GetColor),
	PAIR( dlg, GetComboboxEvent),
	PAIR( dlg, GetConstTextPtr),
	PAIR( dlg, GetCursorPos),
	PAIR( dlg, GetCursorSize),
	PAIR( dlg, GetDefaultColor),
	PAIR( dlg, GetDialogInfo),
	PAIR( dlg, GetDialogTitle),
	PAIR( dlg, GetDlgData),
	PAIR( dlg, GetDlgItem),
	PAIR( dlg, GetDlgRect),
	PAIR( dlg, GetDropdownOpened),
	PAIR( dlg, GetEditPosition),
	PAIR( dlg, GetFocus),
	PAIR( dlg, GetItemData),
	PAIR( dlg, GetItemPosition),
	PAIR( dlg, GetMemoEditId),
	PAIR( dlg, GetSelection),
	PAIR( dlg, GetText),
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
	PAIR( dlg, SetTextPtrSilent),
	PAIR( dlg, ShowDialog),
	PAIR( dlg, ShowItem),
	PAIR( dlg, User),

	{NULL, NULL},
};

static const luaL_Reg dialog_funcs[] =
{
	PAIR( far, DialogFree),
	PAIR( far, DialogInit),
	PAIR( far, DialogRun),
	PAIR( far, GetDlgItem),
	PAIR( far, SendDlgMessage),
	PAIR( far, SetDlgItem),
	PAIR( far, SubscribeDialogDrawEvents),
};

static const char far_Dialog[] =
"function far.Dialog (Id,X1,Y1,X2,Y2,HelpTopic,Items,Flags,DlgProc,Param)\n"
  "local hDlg = far.DialogInit(Id,X1,Y1,X2,Y2,HelpTopic,Items,Flags,DlgProc,Param)\n"
  "if hDlg == nil then return nil end\n"

  "local ret = far.DialogRun(hDlg)\n"
  "for i, item in ipairs(Items) do\n"
    "local newitem = far.GetDlgItem(hDlg, i)\n"
    "if type(item[6]) == 'table' then\n"
      "item[6].SelectIndex = newitem[6].SelectIndex\n"
    "else\n"
      "item[6] = newitem[6]\n"
    "end\n"
    "item[10] = newitem[10]\n"
  "end\n"

  "far.DialogFree(hDlg)\n"
  "return ret\n"
"end";

// the "far" table must be on Lua stack top
int luaopen_dialog(lua_State *L)
{
	int top = lua_gettop(L);
	luaL_register(L, NULL, dialog_funcs);

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, FAR_DN_STORAGE);

	luaL_newmetatable(L, FarDialogType);
	lua_pushvalue(L,-1);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, DialogHandleEqual);
	lua_setfield(L, -2, "__eq");
	luaL_register(L, NULL, dialog_methods);

	(void) luaL_dostring(L, far_Dialog);

	lua_settop(L, top);
	return 0;
}
