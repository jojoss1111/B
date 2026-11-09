#include "lua_comum.h"

DEF_ARG(arg_fonte,   fonte_handle,   MT_FONTE)
DEF_ARG(arg_digitar, digitar_handle, MT_DIGITAR)

DEF_GC(fonte, fonte_handle, fonte_destruir)

static uint32_t decodificar_utf8(const char* s, size_t n) {
    const uint8_t* p = (const uint8_t*)s;
    if (n == 0) return 0;
    if (p[0] < 0x80) return p[0];
    if ((p[0] & 0xE0) == 0xC0 && n >= 2) return ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
    if ((p[0] & 0xF0) == 0xE0 && n >= 3) return ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    if ((p[0] & 0xF8) == 0xF0 && n >= 4)
        return ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
    return p[0];
}

static uint32_t arg_letra(lua_State* L, int i) {
    if (lua_type(L, i) == LUA_TSTRING) {
        size_t n;
        const char* s = lua_tolstring(L, i, &n);
        return decodificar_utf8(s, n);
    }
    return (uint32_t)luaL_checkinteger(L, i);
}

static int l_fonte_novo(lua_State* L) {
    return ret_ptr(L, MT_FONTE, fonte_novo());
}

static int l_fonte_criar(lua_State* L) {
    fonte_handle* f = arg_fonte(L, 1);
    fb_atlas_handle* atlas = arg_atlas(L, 2);
    int padrao = (int)luaL_optinteger(L, 3, PADRAO_ASCII);
    fonte_criar(f, atlas, padrao, (uint16_t)luaL_optinteger(L, 4, 0), (uint16_t)luaL_optinteger(L, 5, 0));
    ref_definir(L, 1, "atlas", 2);
    return 0;
}

static int l_fonte_criar_proprio(lua_State* L) {
    fonte_handle* f = arg_fonte(L, 1);
    fb_atlas_handle* atlas = arg_atlas(L, 2);
    uint16_t pw = (uint16_t)luaL_checkinteger(L, 3);
    uint16_t ph = (uint16_t)luaL_checkinteger(L, 4);
    if (lua_type(L, 5) == LUA_TSTRING) {
        fonte_criar_proprio_texto(f, atlas, pw, ph, lua_tostring(L, 5));
    } else {
        int n, i;
        fonte_letra_tile* v;
        luaL_checktype(L, 5, LUA_TTABLE);
        n = tam_tabela(L, 5);
        v = (fonte_letra_tile*)lua_newuserdata(L, (n > 0 ? n : 1) * sizeof(fonte_letra_tile));
        for (i = 0; i < n; i++) {
            lua_rawgeti(L, 5, i + 1);
            luaL_checktype(L, -1, LUA_TTABLE);
            lua_rawgeti(L, -1, 1);
            lua_rawgeti(L, -2, 2);
            lua_rawgeti(L, -3, 3);
            v[i].letra  = arg_letra(L, -3);
            v[i].coluna = (uint16_t)lua_tointeger(L, -2);
            v[i].linha  = (uint16_t)lua_tointeger(L, -1);
            lua_pop(L, 4);
        }
        fonte_criar_proprio_tiles(f, atlas, pw, ph, v, (size_t)n);
        lua_pop(L, 1);
    }
    ref_definir(L, 1, "atlas", 2);
    return 0;
}

static int l_fonte_escrever(lua_State* L) {
    fonte_handle* f = arg_fonte(L, 1);
    fb_handle* fb = arg_fb(L, 2);
    const char* frase = luaL_checkstring(L, 3);
    int32_t x = (int32_t)luaL_checkinteger(L, 4);
    int32_t y = (int32_t)luaL_checkinteger(L, 5);
    uint32_t cor = arg_cor_opcional(L, 6);
    fonte_escrever(f, fb, frase, x, y, cor, (uint8_t)luaL_optinteger(L, 7, 1));
    return 0;
}

static int l_fonte_escrever_lento(lua_State* L) {
    fonte_handle* f = arg_fonte(L, 1);
    fb_handle* fb = arg_fb(L, 2);
    const char* frase = luaL_checkstring(L, 3);
    int32_t x = (int32_t)luaL_checkinteger(L, 4);
    int32_t y = (int32_t)luaL_checkinteger(L, 5);
    uint32_t cor = arg_cor_opcional(L, 6);
    ref_definir(L, 1, "frase_lenta", 3);
    fonte_escrever_lento(f, fb, frase, x, y, cor, (uint8_t)luaL_optinteger(L, 7, 1), (float)luaL_optnumber(L, 8, 0.05));
    return 0;
}

static int l_fonte_largura_texto(lua_State* L) {
    lua_pushinteger(L, fonte_largura_texto(arg_fonte(L, 1), luaL_checkstring(L, 2), (uint8_t)luaL_optinteger(L, 3, 1)));
    return 1;
}

static int l_fonte_altura_texto(lua_State* L) {
    lua_pushinteger(L, fonte_altura_texto(arg_fonte(L, 1), luaL_checkstring(L, 2), (uint8_t)luaL_optinteger(L, 3, 1)));
    return 1;
}

static const luaL_Reg fonte_metodos[] = {
    {"criar",          l_fonte_criar},
    {"criar_proprio",  l_fonte_criar_proprio},
    {"escrever",       l_fonte_escrever},
    {"escrever_lento", l_fonte_escrever_lento},
    {"largura_texto",  l_fonte_largura_texto},
    {"altura_texto",   l_fonte_altura_texto},
    {"__gc",           gc_fonte},
    {NULL, NULL}
};

static const luaL_Reg fonte_funcoes[] = {
    {"novo", l_fonte_novo},
    {NULL, NULL}
};

DEF_GC(digitar, digitar_handle, digitar_destruir)

static int l_digitar_novo(lua_State* L) {
    return ret_ptr(L, MT_DIGITAR, digitar_novo());
}

static int l_digitar_inteiro(lua_State* L) {
    lua_pushinteger(L, digitar_inteiro(arg_digitar(L, 1), arg_entrada(L, 2), (uint8_t)luaL_optinteger(L, 3, 10)));
    return 1;
}

static int l_digitar_decimal(lua_State* L) {
    lua_pushnumber(L, digitar_decimal(arg_digitar(L, 1), arg_entrada(L, 2), (uint8_t)luaL_optinteger(L, 3, 10)));
    return 1;
}

static int l_digitar_texto(lua_State* L) {
    lua_pushstring(L, digitar_texto(arg_digitar(L, 1), arg_entrada(L, 2), (uint16_t)luaL_optinteger(L, 3, 255)));
    return 1;
}

static int l_digitar_conteudo(lua_State* L) {
    lua_pushstring(L, digitar_conteudo(arg_digitar(L, 1)));
    return 1;
}

static int l_digitar_desenhar(lua_State* L) {
    digitar_handle* d = arg_digitar(L, 1);
    fb_handle* fb = arg_fb(L, 2);
    fonte_handle* f = arg_fonte(L, 3);
    int32_t x = (int32_t)luaL_checkinteger(L, 4);
    int32_t y = (int32_t)luaL_checkinteger(L, 5);
    uint32_t cor = arg_cor_opcional(L, 6);
    digitar_desenhar(d, fb, f, x, y, cor, (uint8_t)luaL_optinteger(L, 7, 1), arg_bool(L, 8, true));
    return 0;
}

static int l_digitar_limpar(lua_State* L)     { digitar_limpar(arg_digitar(L, 1)); return 0; }
static int l_digitar_deletar(lua_State* L)    { digitar_deletar(arg_digitar(L, 1)); return 0; }
static int l_digitar_travar(lua_State* L)     { digitar_travar(arg_digitar(L, 1)); return 0; }
static int l_digitar_destravar(lua_State* L)  { digitar_destravar(arg_digitar(L, 1)); return 0; }
static int l_digitar_finalizado(lua_State* L) { lua_pushboolean(L, digitar_finalizado(arg_digitar(L, 1))); return 1; }
static int l_digitar_travado(lua_State* L)    { lua_pushboolean(L, digitar_travado(arg_digitar(L, 1))); return 1; }

static const luaL_Reg digitar_metodos[] = {
    {"inteiro",    l_digitar_inteiro},
    {"decimal",    l_digitar_decimal},
    {"texto",      l_digitar_texto},
    {"conteudo",   l_digitar_conteudo},
    {"desenhar",   l_digitar_desenhar},
    {"limpar",     l_digitar_limpar},
    {"deletar",    l_digitar_deletar},
    {"travar",     l_digitar_travar},
    {"destravar",  l_digitar_destravar},
    {"finalizado", l_digitar_finalizado},
    {"travado",    l_digitar_travado},
    {"__gc",       gc_digitar},
    {NULL, NULL}
};

static const luaL_Reg digitar_funcoes[] = {
    {"novo", l_digitar_novo},
    {NULL, NULL}
};

void bindings_textos(lua_State* L) {
    registrar_classe(L, MT_FONTE, fonte_metodos, NULL);
    registrar_classe(L, MT_DIGITAR, digitar_metodos, NULL);
    registrar_global(L, "fonte", fonte_funcoes);
    registrar_global(L, "digitar", digitar_funcoes);

    lua_newtable(L);
    campo(L, "ascii", PADRAO_ASCII);
    campo(L, "utf8", PADRAO_UTF8);
    lua_setglobal(L, "padrao");
}