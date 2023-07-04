#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int main(int argc, char **argv)
{
  lua_State *L;
  int status = 1;

  if (argc >= 2 && (L = lua_open()))
  {
    luaL_openlibs(L);
    status = luaL_loadfile(L, argv[1]);
    if (status == 0)
    {
      int i;
      for (i=2; i<argc; i++)
      {
        lua_pushstring(L, argv[i]);
      }
      status = lua_pcall(L, argc-2, 0, 0);
    }
    lua_close(L);
  }

  return status;
}
