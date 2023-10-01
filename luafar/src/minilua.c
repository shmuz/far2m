#include <string.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int main(int argc, char **argv)
{
	lua_State *L;
	int status = 0;

	if (argc >= 2 && (L = lua_open()))
	{
		int i, j=-1, execute=0;

		luaL_openlibs(L);

		for (i=1,status=0; i<argc && status==0; i++)
		{
			if (j < 0)
			{
				if (execute)
				{
					status = luaL_loadstring(L, argv[i]) || lua_pcall(L,0,0,0);
					execute = 0;
				}
				else if (!strncmp(argv[i], "-e", 2))
				{
					if (argv[i][2])
						status = luaL_loadstring(L, argv[i]+2) || lua_pcall(L,0,0,0);
					else
						execute = 1;
				}
				else
				{
					status = luaL_loadfile(L, argv[i]);
					j = 0;
				}
			}
			else
			{
				lua_pushstring(L, argv[i]);
				j++;
			}
		}

		if (status == 0 && j >= 0)
			status = lua_pcall(L, j, 0, 0);

		if (status)
			fprintf(stderr, "%s\n", lua_tostring(L,-1));

		lua_close(L);
	}

	return status;
}
