#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include <bird.h>
//sudo ./build.sh clean && sudo ./build.sh build && sudo ./build.sh install && ./build.sh app main.c main

/* Panic: mostra o traceback em vez de so "unprotected error". */
static int ao_entrar_em_panico(lua_State* L) {
    const char* msg = lua_tostring(L, -1);
    luaL_traceback(L, L, msg ? msg : "erro desconhecido", 0);
    fprintf(stderr, "PANIC:\n%s\n", lua_tostring(L, -1));
    return 0;
}

/* Message handler do pcall: anexa o traceback ao erro. */
static int com_traceback(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    if (!msg) msg = "erro desconhecido (nao e string)";
    luaL_traceback(L, L, msg, 1);
    return 1;
}

/* Roda os bindings dentro de pcall (antes ficavam fora de qualquer protecao). */
static int registrar_bindings(lua_State* L) {
    bindings_todos(L);
    return 0;
}

int main(int argc, char** argv) {
    lua_State* L = luaL_newstate();
    if (!L) return 1;
    lua_atpanic(L, ao_entrar_em_panico);
    luaL_openlibs(L);

    lua_pushcfunction(L, com_traceback);          /* indice 1: handler */

    lua_pushcfunction(L, registrar_bindings);
    if (lua_pcall(L, 0, 0, 1) != LUA_OK) {
        fprintf(stderr, "Erro nos bindings:\n%s\n", lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }

    const char* script = argc > 1 ? argv[1] : "main.lua";
    if (luaL_loadfile(L, script) != LUA_OK || lua_pcall(L, 0, 0, 1) != LUA_OK) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }

    lua_close(L);
    return 0;
}
