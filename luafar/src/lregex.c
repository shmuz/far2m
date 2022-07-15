/* lregex.cpp */

#include "util.h"
#include "ustring.h"

#define TYPE_REGEX "far_regex"

typedef struct PluginStartupInfo PSInfo;

typedef struct {
  HANDLE hnd;
} TFarRegex;

TFarRegex* CheckFarRegex(lua_State *L, int pos)
{
  TFarRegex* fr = (TFarRegex*)luaL_checkudata(L, pos, TYPE_REGEX);
  luaL_argcheck(L, fr->hnd != INVALID_HANDLE_VALUE, pos, "attempt to access freed regex");
  return fr;
}

int regex_gc(lua_State *L)
{
  TFarRegex* fr = CheckFarRegex(L, 1);
  if (fr->hnd != INVALID_HANDLE_VALUE) {
    GetPluginStartupInfo(L)->RegExpControl(fr->hnd, RECTL_FREE, 0);
    fr->hnd = INVALID_HANDLE_VALUE;
  }
  return 0;
}

int regex_tostring(lua_State *L)
{
  TFarRegex* fr = CheckFarRegex(L, 1);
  lua_pushfstring(L, "%s (%p)", TYPE_REGEX, fr);
  return 1;
}

const wchar_t* check_regex_pattern (lua_State *L, int pos_pat, int pos_cflags)
{
  const char* pat = luaL_checkstring(L, pos_pat);
  if (*pat != '/') {
    const char* cflags = pos_cflags ? luaL_optstring(L, pos_cflags, NULL) : NULL;
    lua_pushliteral(L, "/");
    lua_pushvalue(L, pos_pat);
    lua_pushliteral(L, "/");
    if (cflags) lua_pushvalue(L, pos_cflags);
    lua_concat(L, 3 + (cflags?1:0));
    lua_replace(L, pos_pat);
  }
  return check_utf8_string(L, pos_pat, NULL);
}

TFarRegex* push_far_regex (lua_State *L, PSInfo *Info, const wchar_t* pat)
{
  TFarRegex* fr = (TFarRegex*)lua_newuserdata(L, sizeof(TFarRegex));
  if (!Info->RegExpControl(NULL, RECTL_CREATE, (LONG_PTR)&fr->hnd))
    luaL_error(L, "RECTL_CREATE failed");
  if (!Info->RegExpControl(fr->hnd, RECTL_COMPILE, (LONG_PTR)pat))
    luaL_error(L, "invalid regular expression");
//(void)Info->RegExpControl(fr->hnd, RECTL_OPTIMIZE, 0); // very slow operation
  luaL_getmetatable(L, TYPE_REGEX);
  lua_setmetatable(L, -2);
  return fr;
}

int regex_gmatch_closure(lua_State *L)
{
  TFarRegex* fr = (TFarRegex*)lua_touserdata(L, lua_upvalueindex(1));
  struct RegExpSearch* pData = (struct RegExpSearch*)lua_touserdata(L, lua_upvalueindex(2));
  PSInfo *Info = GetPluginStartupInfo(L);
  int prev_end = pData->Match[0].end;
  while (Info->RegExpControl(fr->hnd, RECTL_SEARCHEX, (LONG_PTR)pData)) {
    if (pData->Match[0].end == prev_end) {
      if (++pData->Position > pData->Length)
        break;
      continue;
    }
    int i, skip = pData->Count>1 ? 1 : 0;
    for(i=skip; i<pData->Count; i++) {
      if (pData->Match[i].start >= 0)
        push_utf8_string(L, pData->Text+pData->Match[i].start, pData->Match[i].end-pData->Match[i].start);
      else
        lua_pushboolean(L, 0);
    }
    if (pData->Position < pData->Match[0].end)
      pData->Position = pData->Match[0].end;
    else
      pData->Position++;
    return pData->Count - skip;
  }
  return lua_pushnil(L), 1;
}

int far_Gmatch(lua_State *L)
{
  int len;
  const wchar_t* Text = check_utf8_string(L, 1, &len);
  const wchar_t* pat = check_regex_pattern(L, 2, 3);
  PSInfo *Info = GetPluginStartupInfo(L);
  TFarRegex* fr = push_far_regex(L, Info, pat);
  struct RegExpSearch* pData = (struct RegExpSearch*)lua_newuserdata(L, sizeof(struct RegExpSearch));
  memset(pData, 0, sizeof(struct RegExpSearch));
  pData->Text = Text;
  pData->Position = 0;
  pData->Length = len;
  pData->Count = Info->RegExpControl(fr->hnd, RECTL_BRACKETSCOUNT, 0);
  pData->Match = (struct RegExpMatch*)lua_newuserdata(L, pData->Count*sizeof(struct RegExpMatch));
  pData->Match[0].end = -1;
  lua_pushcclosure(L, regex_gmatch_closure, 3);//also pData->Match to prevent it being gc'ed
  return 1;
}

enum { OP_MATCH, OP_FIND, OP_TFIND };

int rx_find_match(lua_State *L, int Op, int is_function)
{
  struct RegExpSearch data;
  memset(&data, 0, sizeof(data));
  PSInfo *Info = GetPluginStartupInfo(L);
  TFarRegex* fr;

  if (is_function) {
    data.Text = check_utf8_string(L, 1, &data.Length);
    fr = push_far_regex(L, Info, check_regex_pattern(L, 2, 4));
    lua_replace(L, 2);
  }
  else {
    fr = CheckFarRegex(L, 1);
    data.Text = check_utf8_string(L, 2, &data.Length);
  }

  data.Position = luaL_optinteger(L, 3, 1);
  if (data.Position > 0 && --data.Position > data.Length)
    data.Position = data.Length;
  if (data.Position < 0 && (data.Position += data.Length) < 0)
    data.Position = 0;

  data.Count = Info->RegExpControl(fr->hnd, RECTL_BRACKETSCOUNT, 0);
  data.Match = (struct RegExpMatch*)lua_newuserdata(L, data.Count*sizeof(struct RegExpMatch));
  if (Info->RegExpControl(fr->hnd, RECTL_SEARCHEX, (LONG_PTR)&data)) {
    int i, skip;
    if (Op == OP_FIND || Op == OP_TFIND) {
      lua_pushinteger(L, data.Match[0].start+1);
      lua_pushinteger(L, data.Match[0].end);
    }

    skip = (Op == OP_FIND || Op == OP_TFIND || data.Count>1) ? 1 : 0;
    if (Op == OP_TFIND)
      lua_newtable(L);
    else {
      if (!lua_checkstack(L, (int)data.Count - skip))
        luaL_error(L, "cannot add %d stack slots", (int)data.Count - skip);
    }

    for(i=skip; i<data.Count; i++) {
      if (data.Match[i].start >= 0)
        push_utf8_string(L, data.Text+data.Match[i].start, data.Match[i].end-data.Match[i].start);
      else
        lua_pushboolean(L, 0);
      if (Op == OP_TFIND)
        lua_rawseti(L, -2, i);
    }

    switch(Op) {
      case OP_MATCH: return data.Count - skip;
      case OP_FIND:  return data.Count - skip + 2;
      case OP_TFIND: return 3;
    }
  }
  return lua_pushnil(L), 1;
}

int regex_bracketscount (lua_State *L)
{
  TFarRegex* fr = CheckFarRegex(L, 1);
  PSInfo *Info = GetPluginStartupInfo(L);
  lua_pushinteger(L, Info->RegExpControl(fr->hnd, RECTL_BRACKETSCOUNT, 0));
  return 1;
}

int rx_gsub (lua_State *L, int is_function)
{
  struct RegExpSearch data;
  memset(&data, 0, sizeof(data));
  TFarRegex* fr;
  PSInfo *Info = GetPluginStartupInfo(L);

  if (is_function) {
    data.Text = check_utf8_string(L, 1, &data.Length);
    fr = push_far_regex(L, Info, check_regex_pattern(L, 2, 5));
    lua_replace(L, 2);
  }
  else {
    fr = CheckFarRegex(L, 1);
    data.Text = check_utf8_string(L, 2, &data.Length);
  }

  const wchar_t* s = data.Text;
  const wchar_t* f = NULL;
  int flen = 0;
  int max_rep_capture = 0;
  int ftype = lua_type(L, 3);
  if (ftype == LUA_TSTRING) {
    const wchar_t* p;
    f = check_utf8_string(L, 3, &flen);
    for (p=f; *p; p++) {
      if (*p == L'%') {
        if (*++p == 0) break;
        if (*p >= L'0' && *p <= L'9') {
          int num = *p - L'0';
          if (max_rep_capture < num) max_rep_capture = num;
        }
      }
    }
  }
  else if (ftype != LUA_TTABLE && ftype != LUA_TFUNCTION)
    luaL_argerror(L, 3, "string or table or function");

  int n;
  if (lua_isnoneornil(L, 4)) n = -1;
  else {
    n = luaL_checkinteger(L, 4);
    if (n < 0) n = 0;
  }
  lua_settop(L, 3);

  data.Count = Info->RegExpControl(fr->hnd, RECTL_BRACKETSCOUNT, 0);
  if ( (ftype == LUA_TSTRING) &&
       !(max_rep_capture == 1 && data.Count == 1) &&
       (data.Count <= max_rep_capture))
    luaL_error(L, "replace string: invalid capture index");
  data.Match = (struct RegExpMatch*)lua_newuserdata(L, data.Count*sizeof(struct RegExpMatch));
  data.Match[0].end = -1;

  int matches = 0, reps = 0;
  luaL_Buffer out;
  luaL_buffinit(L, &out);

  while (n < 0 || reps < n) {
    int prev_end = data.Match[0].end;
    if (!Info->RegExpControl(fr->hnd, RECTL_SEARCHEX, (LONG_PTR)&data))
      break;
    if (data.Match[0].end == prev_end) {
      if (data.Position < data.Length) {
        luaL_addlstring(&out, (const char*)(s+data.Position), sizeof(wchar_t));
        data.Position++;
        continue;
      }
      break;
    }
    matches++;
    int rep = 0;
    int from = data.Match[0].start;
    int to = data.Match[0].end;
    luaL_addlstring(&out, (const char*)(s + data.Position),
      (from - data.Position) * sizeof(wchar_t));
    if (ftype == LUA_TSTRING) {
      int i, start = 0;
      for (i=0; i<flen; i++) {
        if (f[i] == L'%') {
          if (++i < flen) {
            if (f[i] >= L'0' && f[i] <= L'9') {
              int n = f[i] - L'0';
              if (n==1 && data.Count==1) n = 0;
              luaL_addlstring(&out, (const char*)(f+start), (i-1-start)*sizeof(wchar_t));
              if (data.Match[n].start >= 0) {
                luaL_addlstring(&out, (const char*)(s + data.Match[n].start),
                    (data.Match[n].end - data.Match[n].start) * sizeof(wchar_t));
              }
            }
            else { // delete the percent sign
              luaL_addlstring(&out, (const char*)(f+start), (i-1-start)*sizeof(wchar_t));
              luaL_addlstring(&out, (const char*)(f+i), sizeof(wchar_t));
            }
            start = i+1;
          }
          else {
            luaL_addlstring(&out, (const char*)(f+start), (i-1-start)*sizeof(wchar_t));
            start = flen;
            break;
          }
        }
      }
      rep++;
      luaL_addlstring(&out, (const char*)(f+start), (flen-start)*sizeof(wchar_t));
    }
    else if (ftype == LUA_TTABLE) {
      int n = data.Count==1 ? 0:1;
      if (data.Match[n].start >= 0) {
        push_utf8_string(L, s + data.Match[n].start,
          (data.Match[n].end - data.Match[n].start));
        lua_gettable(L, 3);
        if (lua_isstring(L, -1)) {
          int len;
          const wchar_t* ws = check_utf8_string(L, -1, &len);
          lua_pushlstring(L, (const char*)ws, len*sizeof(wchar_t));
          lua_remove(L, -2);
          luaL_addvalue(&out);
          rep++;
        }
        else if (lua_toboolean(L,-1))
          luaL_error(L, "invalid replacement type");
        else
          lua_pop(L, 1);
      }
    }
    else { // if (ftype == LUA_TFUNCTION)
      int i, skip = data.Count==1 ? 0:1;
      lua_checkstack(L, data.Count+1-skip);
      lua_pushvalue(L, 3);
      for (i=skip; i<data.Count; i++) {
        if (data.Match[i].start >= 0) {
          push_utf8_string(L, s + data.Match[i].start,
            (data.Match[i].end - data.Match[i].start));
        }
        else
          lua_pushboolean(L, 0);
      }
      int ret = lua_pcall(L, data.Count-skip, 1, 0);
      if (ret == 0) {
        if (lua_isstring(L, -1)) {
          int len;
          const wchar_t* ws = check_utf8_string(L, -1, &len);
          lua_pushlstring(L, (const char*)ws, len*sizeof(wchar_t));
          lua_remove(L, -2);
          luaL_addvalue(&out);
          rep++;
        }
        else if (lua_toboolean(L,-1))
          luaL_error(L, "invalid return type");
        else
          lua_pop(L, 1);
      }
      else
        luaL_error(L, lua_tostring(L, -1));
    }
    if (rep)
      reps++;
    else
      luaL_addlstring(&out, (const char*)(s+from), (to-from)*sizeof(wchar_t));
    if (data.Position < to)
      data.Position = to;
    else if (data.Position < data.Length) {
      luaL_addlstring(&out, (const char*)(s + data.Position), sizeof(wchar_t));
      data.Position++;
    }
    else
      break;
  }
  luaL_addlstring(&out, (const char*)(s + data.Position),
    (data.Length - data.Position) * sizeof(wchar_t));
  luaL_pushresult(&out);
  push_utf8_string(L, (const wchar_t*)lua_tostring(L, -1),
    lua_objlen(L, -1) / sizeof(wchar_t));
  lua_pushinteger(L, matches);
  lua_pushinteger(L, reps);
  return 3;
}

int far_Regex (lua_State *L)
{
  const wchar_t* pat = check_regex_pattern(L, 1, 2);
  PSInfo *Info = GetPluginStartupInfo(L);
  push_far_regex(L, Info, pat);
  return 1;
}

int regex_find  (lua_State *L)  { return rx_find_match(L, OP_FIND, 0); }
int far_Find    (lua_State *L)  { return rx_find_match(L, OP_FIND, 1); }

int regex_tfind  (lua_State *L)  { return rx_find_match(L, OP_TFIND, 0); }
int far_Tfind    (lua_State *L)  { return rx_find_match(L, OP_TFIND, 1); }

int regex_match (lua_State *L)  { return rx_find_match(L, OP_MATCH, 0); }
int far_Match   (lua_State *L)  { return rx_find_match(L, OP_MATCH, 1); }

int regex_gsub  (lua_State *L)  { return rx_gsub(L, 0); }
int far_Gsub    (lua_State *L)  { return rx_gsub(L, 1); }

static const luaL_Reg regex_methods[] = {
  {"find",          regex_find},
  {"tfind",         regex_tfind},
  {"match",         regex_match},
  {"gsub",          regex_gsub},
  {"bracketscount", regex_bracketscount},
  {"__gc",          regex_gc},
  {"__tostring",    regex_tostring},
  {NULL, NULL}
};

int luaopen_regex (lua_State *L)
{
  luaL_newmetatable(L, TYPE_REGEX);
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -2);
  lua_rawset(L, -3);
  luaL_register(L, NULL, regex_methods);
  lua_pop(L, 1);
  return 0;
}
