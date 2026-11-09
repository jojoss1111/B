#include "lua_comum.h"

static void empurrar_tela(lua_State* L, fb_handle* fb) {
    fb_handle** p;
    lua_getfield(L, LUA_REGISTRYINDEX, "janela_tela");
    p = (fb_handle**)testar_ud(L, -1, MT_FBREF);
    if (p && *p == fb) return;
    lua_pop(L, 1);
    p = (fb_handle**)lua_newuserdata(L, sizeof(fb_handle*));
    *p = fb;
    luaL_getmetatable(L, MT_FBREF);
    lua_setmetatable(L, -2);
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, "janela_tela");
}

static void empurrar_entrada(lua_State* L, entrada_handle* e) {
    entrada_handle** p;
    lua_getfield(L, LUA_REGISTRYINDEX, "janela_entrada");
    p = (entrada_handle**)testar_ud(L, -1, MT_ENTRADAREF);
    if (p && *p == e) return;
    lua_pop(L, 1);
    p = (entrada_handle**)lua_newuserdata(L, sizeof(entrada_handle*));
    *p = e;
    luaL_getmetatable(L, MT_ENTRADAREF);
    lua_setmetatable(L, -2);
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, "janela_entrada");
}

static int traceback_lua(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    lua_getglobal(L, "debug");
    if (!lua_istable(L, -1)) {
        lua_settop(L, 1);
        return 1;
    }
    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1)) {
        lua_settop(L, 1);
        return 1;
    }
    lua_pushstring(L, msg ? msg : "erro desconhecido");
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);
    return 1;
}

static void chamar_lua(lua_State* L, int nargs) {
    int base = lua_gettop(L) - nargs;
    lua_pushcfunction(L, traceback_lua);
    lua_insert(L, base);
    if (lua_pcall(L, nargs, 0, base) != 0) {
        fprintf(stderr, "erro lua: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        janela_fechar();
    }
    lua_remove(L, base);
}

static void cb_janela_iniciar(void* ctx) {
    lua_State* L = (lua_State*)ctx;
    lua_getglobal(L, "iniciar");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    chamar_lua(L, 0);
}

static void cb_janela_rodar(float delta, void* ctx) {
    lua_State* L = (lua_State*)ctx;
    lua_getglobal(L, "rodar");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    lua_pushnumber(L, delta);
    chamar_lua(L, 1);
}

static void cb_janela_desenhar(fb_handle* fb, void* ctx) {
    lua_State* L = (lua_State*)ctx;
    lua_getglobal(L, "desenhar");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    empurrar_tela(L, fb);
    chamar_lua(L, 1);
}

static int l_janela_iniciar(lua_State* L) {
    uint16_t largura = (uint16_t)luaL_checkinteger(L, 1);
    uint16_t altura  = (uint16_t)luaL_checkinteger(L, 2);
    uint8_t  fps     = (uint8_t)luaL_optinteger(L, 3, 60);
    uint32_t cor     = lua_isnoneornil(L, 4) ? 0xFF000000u : (uint32_t)(int64_t)luaL_checknumber(L, 4);
    int8_t   ampliar = (int8_t)luaL_optinteger(L, 5, 2);
    int      modo    = (int)luaL_optinteger(L, 6, 1);
    const char* nome = luaL_optstring(L, 7, "bird");
    fb_atlas_handle* icone = lua_isnoneornil(L, 8) ? NULL : arg_atlas(L, 8);
    int r;
    janela_callbacks(cb_janela_iniciar, cb_janela_rodar, cb_janela_desenhar, L);
    r = janela_iniciar(largura, altura, fps, cor, ampliar, modo, nome, icone);
    lua_pushinteger(L, r);
    return 1;
}

static int l_janela_fechar(lua_State* L) {
    (void)L;
    janela_fechar();
    return 0;
}

static int l_janela_mudar_modo(lua_State* L) {
    (void)L;
    janela_mudar_modo();
    return 0;
}

static int l_janela_definir_modo(lua_State* L) {
    janela_definir_modo((int)luaL_checkinteger(L, 1));
    return 0;
}

static int l_janela_modo_atual(lua_State* L) {
    lua_pushinteger(L, janela_modo_atual());
    return 1;
}

static int l_janela_ampliar(lua_State* L) {
    janela_ampliar((int8_t)luaL_checkinteger(L, 1));
    return 0;
}

static int l_janela_ampliar_atual(lua_State* L) {
    lua_pushinteger(L, janela_ampliar_atual());
    return 1;
}

static int l_janela_tela_cheia(lua_State* L) {
    janela_tela_cheia(arg_bool(L, 1, true));
    return 0;
}

static int l_janela_titulo(lua_State* L) {
    janela_titulo(luaL_checkstring(L, 1));
    return 0;
}

static int l_janela_entrada(lua_State* L) {
    empurrar_entrada(L, janela_entrada());
    return 1;
}

static int l_janela_largura(lua_State* L) {
    lua_pushinteger(L, janela_largura());
    return 1;
}

static int l_janela_altura(lua_State* L) {
    lua_pushinteger(L, janela_altura());
    return 1;
}

static int l_janela_fps_atual(lua_State* L) {
    lua_pushnumber(L, janela_fps_atual());
    return 1;
}

static const luaL_Reg janela_funcoes[] = {
    {"iniciar",       l_janela_iniciar},
    {"fechar",        l_janela_fechar},
    {"mudar_modo",    l_janela_mudar_modo},
    {"definir_modo",  l_janela_definir_modo},
    {"modo_atual",    l_janela_modo_atual},
    {"ampliar",       l_janela_ampliar},
    {"ampliar_atual", l_janela_ampliar_atual},
    {"tela_cheia",    l_janela_tela_cheia},
    {"titulo",        l_janela_titulo},
    {"entrada",       l_janela_entrada},
    {"largura",       l_janela_largura},
    {"altura",        l_janela_altura},
    {"fps_atual",     l_janela_fps_atual},
    {NULL, NULL}
};

void bindings_janela(lua_State* L) {
    registrar_classe(L, MT_FBREF, metodos_vazios, fb_comum);
    registrar_classe(L, MT_ENTRADAREF, metodos_vazios, entrada_comum);
    registrar_global(L, "janela", janela_funcoes);

    lua_newtable(L);
    campo(L, "normal", MODO_NORMAL);
    campo(L, "letterbox", MODO_LETTERBOX);
    campo(L, "tela_cheia", MODO_TELA_CHEIA);
    lua_setglobal(L, "modo_tela");
}