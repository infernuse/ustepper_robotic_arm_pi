#include <stdio.h>
#include <string.h>
#include <lua.hpp>
#include <lauxlib.h>
#include <lualib.h>

int main()
{
     char buf[256];
     int err;
     lua_State *L = luaL_newstate();
     luaL_openlibs(L);
     while( fgets(buf, sizeof(buf), stdin) != NULL )
     {
          err = luaL_loadbuffer(L, buf, strlen(buf), "line") || lua_pcall(L, 0, 0, 0);
          if( err )
          {
               fprintf(stderr, "%s\n", lua_tostring(L, -1));
               lua_pop(L, 1);
          }
     }
     lua_close(L);
     return 0;
}