#include <windows.h>

#include <lua.h>
#include <lauxlib.h>

#include "far3parts.h"
#include "lf_flags.h"
#include "lf_service.h"
#include "lf_string.h"

static int push_editor_filename(lua_State *L, int Id)
{
	int size = PSInfo.EditorControlV2(Id, ECTL_GETFILENAME, NULL);
	if (!size) return 0;

	wchar_t* fname = (wchar_t*)lua_newuserdata(L, size * sizeof(wchar_t));
	size = PSInfo.EditorControlV2(Id, ECTL_GETFILENAME, fname);

	if (size)
	{
		push_utf8_string(L, fname, -1);
		lua_remove(L, -2);
		return 1;
	}

	lua_pop(L,1);
	return 0;
}

static int editor_GetFileName(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);

	if (!push_editor_filename(L, EditorId)) lua_pushnil(L);

	return 1;
}

static int editor_GetInfo(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	struct EditorInfo ei = { sizeof(ei) };
	if (!PSInfo.EditorControlV2(EditorId, ECTL_GETINFO, &ei))
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 22);
	PutNumToTable(L, "EditorID", ei.EditorID);

	if (push_editor_filename(L, EditorId))
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
	PutRECTToTable(L, "WindowArea", ei.WindowArea);
	PutRECTToTable(L, "ClientArea", ei.ClientArea);
	PutBoolToTable(L, "IsMemoEdit", ei.IsMemoEdit);
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
static BOOL FastGetString(int EditorId, int string_num, struct EditorGetString *egs)
{
	struct EditorSetPosition esp = {};
	esp.CurLine   = string_num;
	esp.CurPos    = -1;
	esp.CurTabPos = -1;
	esp.TopScreenLine = -1;
	esp.LeftPos   = -1;
	esp.Overtype  = -1;

	if (!PSInfo.EditorControlV2(EditorId, ECTL_SETPOSITION, &esp))
		return FALSE;

	egs->StringNumber = string_num;
	return PSInfo.EditorControlV2(EditorId, ECTL_GETSTRING, egs) != 0;
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
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	intptr_t line_num = luaL_optinteger(L, 2, 0) - 1;
	intptr_t mode = luaL_optinteger(L, 3, 0);
	BOOL res = 0;
	struct EditorGetString egs;

	if (mode == 0 || mode == 3 || mode == 4)
	{
		egs.StringNumber = line_num;
		res = PSInfo.EditorControlV2(EditorId, ECTL_GETSTRING, &egs) != 0;
	}
	else if (mode == 1 || mode == 2)
		res = FastGetString(EditorId, line_num, &egs);

	if (res)
	{
		if (mode == 2 || mode == 3)
		{
			if (is_wide)
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
		else if (mode == 4)
		{
			lua_pushinteger(L, egs.SelStart+1);
			lua_pushinteger(L, egs.SelEnd);
			lua_pushinteger(L, egs.StringLength);
			return 3;
		}
		else
		{
			lua_createtable(L, 0, 6);
			PutNumToTable(L, "StringNumber", (double)egs.StringNumber+1);
			PutNumToTable(L, "StringLength", (double)egs.StringLength);
			PutNumToTable(L, "SelStart", (double)egs.SelStart+1);
			PutNumToTable(L, "SelEnd", (double)egs.SelEnd);

			if (is_wide)
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
	struct EditorSetString ess = {};
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	size_t len;
	ess.StringNumber = luaL_optinteger(L, 2, 0) - 1;

	if (is_wide)
	{
		ess.StringText = check_wcstring(L, 3, &len);
		ess.StringEOL = opt_wcstring(L, 4, NULL);

		if (ess.StringEOL)
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
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_SETSTRING, &ess) != 0);
	return 1;
}

static int editor_SetString(lua_State *L) { return _EditorSetString(L, 0); }
static int editor_SetStringW(lua_State *L) { return _EditorSetString(L, 1); }

static int editor_InsertString(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	int indent = lua_toboolean(L, 2);
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_INSERTSTRING, &indent));
	return 1;
}

static int editor_DeleteString(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_DELETESTRING, NULL));
	return 1;
}

static int editor_InsertText(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	wchar_t *text = check_utf8_string(L,2,NULL);
	int redraw = lua_toboolean(L,3);
	int res = PSInfo.EditorControlV2(EditorId, ECTL_INSERTTEXT_V2, text);
	if (res && redraw)
		PSInfo.EditorControlV2(EditorId, ECTL_REDRAW, NULL);
	lua_pushboolean(L, res);
	return 1;
}

static int editor_InsertTextW(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	(void)luaL_checkstring(L,2);
	int redraw = lua_toboolean(L,3);
	lua_pushvalue(L,2);
	lua_pushlstring(L, "\0\0\0\0", sizeof(wchar_t));
	lua_concat(L,2);
	int res = PSInfo.EditorControlV2(EditorId, ECTL_INSERTTEXT_V2, (void*)lua_tostring(L,-1));
	if (res && redraw)
		PSInfo.EditorControlV2(EditorId, ECTL_REDRAW, NULL);
	lua_pushboolean(L, res);
	return 1;
}

static int editor_DeleteChar(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_DELETECHAR, NULL));
	return 1;
}

static int editor_DeleteBlock(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_DELETEBLOCK, NULL));
	return 1;
}

static int editor_UndoRedo(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	struct EditorUndoRedo eur = {};
	eur.Command = check_env_flag(L, 2);
	lua_pushboolean (L, PSInfo.EditorControlV2(EditorId, ECTL_UNDOREDO, &eur));
	return 1;
}

static int editor_SetKeyBar(lua_State *L)
{
	return SetKeyBar(L, TRUE);
}

static int editor_SetParam(lua_State *L)
{
	wchar_t buf[256];
	int tp;
	int result;
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	struct EditorSetParameter esp = {};
	esp.Type = check_env_flag(L,2);
	//-----------------------------------------------------
	tp = lua_type(L,3);

	if (tp == LUA_TNUMBER)
		esp.Param.iParam = lua_tointeger(L,3);
	else if (tp == LUA_TBOOLEAN)
		esp.Param.iParam = lua_toboolean(L,3);
	else if (tp == LUA_TSTRING)
		esp.Param.wszParam = check_utf8_string(L,3,NULL);

	//-----------------------------------------------------
	if (esp.Type == ESPT_GETWORDDIV)
	{
		esp.Param.wszParam = buf;
		esp.Size = ARRAYSIZE(buf);
	}

	//-----------------------------------------------------
	esp.Flags = GetFlagCombination(L, 4, NULL);
	//-----------------------------------------------------
	result = PSInfo.EditorControlV2(EditorId, ECTL_SETPARAM, &esp);
	lua_pushboolean(L, result);
	if (result && esp.Type == ESPT_GETWORDDIV)
	{
		push_utf8_string(L,buf,-1); return 2;
	}

	return 1;
}

static int editor_SetPosition(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	struct EditorSetPosition esp = {};

	if (lua_istable(L, 2))
	{
		lua_settop(L, 2);
		FillEditorSetPosition(L, &esp);
	}
	else
	{
		esp.CurLine   = luaL_optinteger(L,2,0) - 1;
		esp.CurPos    = luaL_optinteger(L,3,0) - 1;
		esp.CurTabPos = luaL_optinteger(L,4,0) - 1;
		esp.TopScreenLine = luaL_optinteger(L,5,0) - 1;
		esp.LeftPos   = luaL_optinteger(L,6,0) - 1;
		esp.Overtype  = luaL_optinteger(L,7,-1);
	}
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_SETPOSITION, &esp));
	return 1;
}

static int editor_Redraw(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_REDRAW, NULL));
	return 1;
}

static int editor_ExpandTabs(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	int line_num = luaL_optinteger(L, 2, 0) - 1;
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_EXPANDTABS, &line_num));
	return 1;
}

static int PushBookmarks(lua_State *L, int EditorId, int count, int command)
{
	if (count > 0) {
		struct EditorBookMarks ebm;
		ebm.Line = (long*)lua_newuserdata(L, 4 * count * sizeof(long));
		ebm.Cursor     = ebm.Line + count;
		ebm.ScreenLine = ebm.Cursor + count;
		ebm.LeftPos    = ebm.ScreenLine + count;
		if (PSInfo.EditorControlV2(EditorId, command, &ebm)) {
			lua_createtable(L, count, 0);
			for (int i=0; i < count; i++) {
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
	else if (count == 0) { // make compatible with Far3 behavior
		lua_newtable(L);
		return 1;
	}

	return lua_pushnil(L), 1;
}

static int editor_GetBookmarks(lua_State *L)
{
	struct EditorInfo ei = { sizeof(ei) };
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	if (!PSInfo.EditorControlV2(EditorId, ECTL_GETINFO, &ei))
		return 0;
	return PushBookmarks(L, EditorId, ei.BookMarkCount, ECTL_GETBOOKMARKS);
}

static int editor_GetSessionBookmarks(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	int count = PSInfo.EditorControlV2(EditorId, ECTL_GETSTACKBOOKMARKS, NULL);
	return PushBookmarks(L, EditorId, count, ECTL_GETSTACKBOOKMARKS);
}

static int editor_AddSessionBookmark(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_ADDSTACKBOOKMARK, NULL));
	return 1;
}

static int editor_ClearSessionBookmarks(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	lua_pushinteger(L, PSInfo.EditorControlV2(EditorId, ECTL_CLEARSTACKBOOKMARKS, NULL));
	return 1;
}

static int editor_DeleteSessionBookmark(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	intptr_t num = luaL_optinteger(L, 2, 0) - 1;
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_DELETESTACKBOOKMARK, (void*)num));
	return 1;
}

static int editor_NextSessionBookmark(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_NEXTSTACKBOOKMARK, NULL));
	return 1;
}

static int editor_PrevSessionBookmark(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_PREVSTACKBOOKMARK, NULL));
	return 1;
}

static int editor_SetTitle(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	const wchar_t* text = opt_utf8_string(L, 2, NULL);
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_SETTITLE, (wchar_t*)text));
	return 1;
}

static int editor_GetTitle(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	int size = PSInfo.EditorControlV2(EditorId, ECTL_GETTITLE, NULL);
	lua_pushstring(L, "");
	if (size)
	{
		void* str = lua_newuserdata(L, size * sizeof(wchar_t));
		if (PSInfo.EditorControlV2(EditorId, ECTL_GETTITLE, str))
			push_utf8_string(L, (wchar_t*)str, -1);
	}
	return 1;
}

static int editor_Quit(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_QUIT, NULL));
	return 1;
}

int FillEditorSelect(lua_State *L, int pos_table, struct EditorSelect *es)
{
	int success;
	lua_getfield(L, pos_table, "BlockType");
	es->BlockType = (int) get_env_flag(L, -1, &success);

	if (!success)
	{
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
	int success = TRUE;
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	struct EditorSelect es = {};

	if (lua_istable(L, 2))
		success = FillEditorSelect(L, 2, &es);
	else
	{
		es.BlockType = (int) check_env_flag(L, 2);
		es.BlockStartLine = luaL_optinteger(L, 3, 0) - 1;
		es.BlockStartPos  = luaL_optinteger(L, 4, 0) - 1;
		es.BlockWidth     = luaL_optinteger(L, 5, -1);
		es.BlockHeight    = luaL_optinteger(L, 6, -1);
	}

	lua_pushboolean(L, success && PSInfo.EditorControlV2(EditorId, ECTL_SELECT, &es));
	return 1;
}

// This function is that long because FAR API does not supply needed
// information directly.
static int editor_GetSelection(lua_State *L)
{
	int BlockStartPos, h, from, to;
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	struct EditorInfo EI = { sizeof(EI) };
	struct EditorGetString egs = {};
	struct EditorSetPosition esp = {};
	PSInfo.EditorControlV2(EditorId, ECTL_GETINFO, &EI);

	if (EI.BlockType == BTYPE_NONE || !FastGetString(EditorId, EI.BlockStartLine, &egs))
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
		if (!FastGetString(EditorId, to, &egs))
			return lua_pushnil(L), 1;

		if (egs.SelStart < 0)
			break;
	}

	if (to >= EI.TotalLines)
		to = EI.TotalLines - 1;

	// binary search for the last block line
	while(from != to)
	{
		int curr = (from + to + 1) / 2;

		if (!FastGetString(EditorId, curr, &egs))
			return lua_pushnil(L), 1;

		if (egs.SelStart < 0)
		{
			if (curr == to)
				break;

			to = curr;      // curr was not selected
		}
		else
		{
			from = curr;    // curr was selected
		}
	}

	if (!FastGetString(EditorId, from, &egs))
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
	PSInfo.EditorControlV2(EditorId, ECTL_SETPOSITION, &esp);
	return 1;
}

static int _EditorTabConvert(lua_State *L, int Operation)
{
	int EditorId = luaL_optinteger(L,1,CURRENT_EDITOR);
	struct EditorConvertPos ecp = {};
	ecp.StringNumber = luaL_optinteger(L,2,0) - 1;
	ecp.SrcPos = luaL_checkinteger(L,3) - 1;

	if (PSInfo.EditorControlV2(EditorId, Operation, &ecp))
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
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	PSInfo.EditorControlV2(EditorId, ECTL_TURNOFFMARKINGBLOCK, NULL);
	return 0;
}

static int editor_AddColor(lua_State *L)
{
	const uint32_t MASK_COLOR = 0x0000FFFF;

	struct EditorTrueColor etc = {};
	int isTrueColor = 0;

	int EditorId          = luaL_optinteger(L, 1, CURRENT_EDITOR);
	etc.Base.StringNumber = luaL_optinteger(L, 2, 0) - 1;
	etc.Base.StartPos     = luaL_checkinteger(L, 3) - 1;
	etc.Base.EndPos       = luaL_checkinteger(L, 4) - 1;
	int Flags             = (int) OptFlags(L, 5, 0);

	GetFarColor(L, 6, &etc.TrueColor, &etc.Base.Color, &isTrueColor);

	etc.Base.Color |= (Flags & ~MASK_COLOR);

	if (etc.Base.Color == 0) // prevent color deletion
		etc.Base.Color = 0x0F;

	if (isTrueColor)
		lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_ADDTRUECOLOR, &etc));
	else
		lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_ADDCOLOR, &etc.Base));

	return 1;
}

static int editor_DelColor(lua_State *L)
{
	struct EditorColor ec = { 0 }; // set ec.Color = 0
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	ec.StringNumber = luaL_optinteger(L, 2, 0) - 1;
	ec.StartPos     = luaL_optinteger(L, 3, 0) - 1;
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_ADDCOLOR, &ec)); // ECTL_ADDCOLOR (sic)
	return 1;
}

static int editor_GetColor(lua_State *L)
{
	struct EditorTrueColor etc;
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	memset(&etc, 0, sizeof(etc));
	etc.Base.StringNumber = luaL_optinteger(L, 2, 0) - 1;
	etc.Base.ColorItem    = luaL_checkinteger(L, 3);
	if (PSInfo.EditorControlV2(EditorId, ECTL_GETTRUECOLOR, &etc))
	{
		DWORD Flags = 0;
		lua_createtable(L, 0, 5);
		PutNumToTable(L, "StartPos", etc.Base.StartPos+1);
		PutNumToTable(L, "EndPos", etc.Base.EndPos+1);

		lua_newtable(L); // Color

		if (etc.TrueColor.Fore.Flags & 0x1)
			PutNumToTable(L, "ForegroundColor", RGBFromFarTrueColor(&etc.TrueColor.Fore));
		else {
			PutNumToTable(L, "ForegroundColor", etc.Base.Color & 0x0F);
			Flags |= FCF_FG_INDEX;
		}

		if (etc.TrueColor.Back.Flags & 0x1)
			PutNumToTable(L, "BackgroundColor", RGBFromFarTrueColor(&etc.TrueColor.Back));
		else {
			PutNumToTable(L, "BackgroundColor", (etc.Base.Color & 0xF0) >> 4);
			Flags |= FCF_BG_INDEX;
		}

		if (etc.Base.Color & COMMON_LVB_UNDERSCORE) Flags |= FCF_FG_UNDERLINE_MASK;
		if (etc.Base.Color & COMMON_LVB_STRIKEOUT)  Flags |= FCF_FG_STRIKEOUT;

		PutNumToTable(L, "Flags", Flags);

		lua_setfield(L, -2, "Color");
	}
	else
		lua_pushnil(L);

	return 1;
}

static int editor_SaveFile(lua_State *L)
{
	struct EditorSaveFile esf;
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	esf.FileName = opt_utf8_string(L, 2, L"");
	esf.FileEOL = opt_utf8_string(L, 3, NULL);
	esf.CodePage = luaL_optinteger(L, 4, 0);
	if (esf.CodePage == 0) {
		struct EditorInfo ei = { sizeof(ei) };
		if (PSInfo.EditorControlV2(EditorId, ECTL_GETINFO, &ei))
			esf.CodePage = ei.CodePage;
	}
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_SAVEFILE, &esf));
	return 1;
}

static int editor_ReadInput(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	INPUT_RECORD ir;

	if (PSInfo.EditorControlV2(EditorId, ECTL_READINPUT, &ir))
		PushInputRecord(L, &ir);
	else
		lua_pushnil(L);

	return 1;
}

static int editor_ProcessInput(lua_State *L)
{
	INPUT_RECORD ir;
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	luaL_checktype(L, 2, LUA_TTABLE);
	FillInputRecord(L, 2, &ir);
	lua_pushboolean(L, PSInfo.EditorControlV2(EditorId, ECTL_PROCESSINPUT, &ir) != 0);
	return 1;
}

static int editor_ProcessKey(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	INT_PTR key = luaL_checkinteger(L,2);
	PSInfo.EditorControlV2(EditorId, ECTL_PROCESSKEY, (void*)key);
	return 0;
}

static int editor_SetVirtualFileName(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	wchar_t *name = check_utf8_string(L, 2, NULL);
	int ret = PSInfo.EditorControlV2(EditorId, ECTL_SETVIRTUALFILENAME, name);
	lua_pushboolean(L, ret);
	return 1;
}

static int editor_GetVirtualFileName(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	size_t size = PSInfo.EditorControlV2(EditorId, ECTL_GETVIRTUALFILENAME, NULL);
	wchar_t *buf = (wchar_t*) lua_newuserdata(L, size * sizeof(wchar_t));
	PSInfo.EditorControlV2(EditorId, ECTL_GETVIRTUALFILENAME, buf);
	push_utf8_string(L, buf, -1);
	return 1;
}

static int editor_SetSavedState(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	int value = lua_toboolean(L, 2);
	int ret = PSInfo.EditorControlV2(EditorId, ECTL_SETSAVEDSTATE, &value);
	lua_pushboolean(L, ret);
	return 1;
}

static int editor_Reparse(lua_State *L)
{
	int EditorId = luaL_optinteger(L, 1, CURRENT_EDITOR);
	int ret = PSInfo.EditorControlV2(EditorId, ECTL_REPARSE, NULL);
	lua_pushboolean(L, ret);
	return 1;
}

static int editor_Editor(lua_State *L)
{
	const wchar_t* FileName = check_utf8_string(L, 1, NULL);
	const wchar_t* Title    = opt_utf8_string(L, 2, NULL);
	int X1 = luaL_optinteger(L, 3, 0);
	int Y1 = luaL_optinteger(L, 4, 0);
	int X2 = luaL_optinteger(L, 5, -1);
	int Y2 = luaL_optinteger(L, 6, -1);
	int Flags = OptFlags(L,7,0);
	int StartLine = luaL_optinteger(L, 8, -1);
	int StartChar = luaL_optinteger(L, 9, -1);
	int CodePage  = luaL_optinteger(L, 10, CP_AUTODETECT);
	int ret = PSInfo.Editor(FileName, Title, X1, Y1, X2, Y2, Flags, StartLine, StartChar, CodePage);
	lua_pushinteger(L, ret);
	return 1;
}

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
	PAIR( editor, GetVirtualFileName),
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
	PAIR( editor, Reparse),
	PAIR( editor, SaveFile),
	PAIR( editor, Select),
	PAIR( editor, SetKeyBar),
	PAIR( editor, SetParam),
	PAIR( editor, SetPosition),
	PAIR( editor, SetSavedState),
	PAIR( editor, SetString),
	PAIR( editor, SetStringW),
	PAIR( editor, SetTitle),
	PAIR( editor, SetVirtualFileName),
	PAIR( editor, TabToReal),
	PAIR( editor, TurnOffMarkingBlock),
	PAIR( editor, UndoRedo),

	{NULL, NULL},
};

int luaopen_editor(lua_State *L)
{
	luaL_register(L, "editor", editor_funcs);
	lua_pop(L, 1);
	return 0;
}
