#include <lua.hpp>                               /* Always include this */
//#include <lauxlib.h>                           /* Always include this */
//#include <lualib.h>                            /* Always include this */

static int power_isquare(lua_State *L){              /* Internal name of func */
	float rtrn = lua_tonumber(L, -1);      /* Get the single number arg */
	printf("Top of square(), nbr=%f\n",rtrn);
	lua_pushnumber(L,rtrn*rtrn);           /* Push the return */
	return 1;                              /* One return value */
}
static int power_icube(lua_State *L){                /* Internal name of func */
	float rtrn = lua_tonumber(L, -1);      /* Get the single number arg */
	printf("Top of cube(), number=%f\n",rtrn);
	lua_pushnumber(L,rtrn*rtrn*rtrn);      /* Push the return */
	return 1;                              /* One return value */
}


static const struct luaL_reg powerlib[] = {
  {"square",   power_isquare},
  {"cube",   power_icube},
  {NULL, NULL}
};


extern "C"
int luaopen_power(lua_State *L){
   luaL_register(L, "power", powerlib);
   return 1;
}
