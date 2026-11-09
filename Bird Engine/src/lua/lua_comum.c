#include "lua_comum.h"

const char* const MT_FB         = "frame_buffer";
const char* const MT_FBREF      = "frame_buffer_ref";
const char* const MT_ATLAS      = "fb_atlas";
const char* const MT_AUDIO      = "audio";
const char* const MT_ENTRADA    = "entrada";
const char* const MT_ENTRADAREF = "entrada_ref";
const char* const MT_CAIXA      = "caixa";
const char* const MT_OBJ        = "obj";
const char* const MT_ANIM       = "animacao";
const char* const MT_AABB       = "aabb";
const char* const MT_MAPA       = "mapa";
const char* const MT_CAMERA     = "camera";
const char* const MT_CAM25D     = "camera25d";
const char* const MT_FONTE      = "fonte";
const char* const MT_DIGITAR    = "digitar";
const char* const MT_TEMPO      = "tempo";
const char* const MT_PARTICULAS = "particulas";
const char* const MT_OBJ3D    = "obj3d";
const char* const MT_CAM3D    = "camera3d";
const char* const MT_ZBUFFER  = "zbuffer";
const char* const MT_MAPA3D   = "mapa3d";
const char* const MT_OBJ3DREF = "obj3d_ref";
const char* const MT_MAO      = "mao";

const luaL_Reg metodos_vazios[] = {
    {NULL, NULL}
};

void definir_funcoes(lua_State* L, const luaL_Reg* funcs) {
#if LUA_VERSION_NUM >= 502
    luaL_setfuncs(L, funcs, 0);
#else
    luaL_register(L, NULL, funcs);
#endif
}

void registrar_classe(lua_State* L, const char* nome, const luaL_Reg* metodos, const luaL_Reg* base) {
    luaL_newmetatable(L, nome);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    if (base) definir_funcoes(L, base);
    definir_funcoes(L, metodos);
    lua_pop(L, 1);
}

void registrar_global(lua_State* L, const char* nome, const luaL_Reg* funcs) {
    lua_newtable(L);
    definir_funcoes(L, funcs);
    lua_setglobal(L, nome);
}

void campo(lua_State* L, const char* nome, int v) {
    lua_pushinteger(L, v);
    lua_setfield(L, -2, nome);
}

int tam_tabela(lua_State* L, int i) {
#if LUA_VERSION_NUM >= 502
    return (int)luaL_len(L, i);
#else
    return (int)lua_objlen(L, i);
#endif
}

static void refs_iniciar(lua_State* L, int dono) {
    lua_newtable(L);
#if LUA_VERSION_NUM >= 502
    lua_setuservalue(L, dono);
#else
    lua_setfenv(L, dono);
#endif
}

void refs_pegar(lua_State* L, int dono) {
#if LUA_VERSION_NUM >= 502
    lua_getuservalue(L, dono);
#else
    lua_getfenv(L, dono);
#endif
}

void ref_definir(lua_State* L, int dono, const char* chave, int valor) {
    refs_pegar(L, dono);
    lua_pushvalue(L, valor);
    lua_setfield(L, -2, chave);
    lua_pop(L, 1);
}

void ref_limpar(lua_State* L, int dono, const char* chave) {
    refs_pegar(L, dono);
    lua_pushnil(L);
    lua_setfield(L, -2, chave);
    lua_pop(L, 1);
}

void ref_definir_i(lua_State* L, int dono, int chave, int valor) {
    refs_pegar(L, dono);
    lua_pushvalue(L, valor);
    lua_rawseti(L, -2, chave);
    lua_pop(L, 1);
}

void ref_limpar_i(lua_State* L, int dono, int chave) {
    refs_pegar(L, dono);
    lua_pushnil(L);
    lua_rawseti(L, -2, chave);
    lua_pop(L, 1);
}

void* testar_ud(lua_State* L, int i, const char* nome) {
    void* p = lua_touserdata(L, i);
    bool igual;
    if (!p || !lua_getmetatable(L, i)) return NULL;
    luaL_getmetatable(L, nome);
    igual = lua_rawequal(L, -1, -2) != 0;
    lua_pop(L, 2);
    return igual ? p : NULL;
}

void* novo_ud(lua_State* L, const char* mt, size_t tam) {
    void* p = lua_newuserdata(L, tam);
    int ud = lua_gettop(L);
    memset(p, 0, tam);
    refs_iniciar(L, ud);
    luaL_getmetatable(L, mt);
    lua_setmetatable(L, ud);
    return p;
}

int ret_ptr(lua_State* L, const char* mt, void* ptr) {
    if (!ptr) {
        lua_pushnil(L);
        return 1;
    }
    *(void**)novo_ud(L, mt, sizeof(void*)) = ptr;
    return 1;
}

int ret_ref(lua_State* L, const char* mt, void* ptr, int dono) {
    if (!ptr) {
        lua_pushnil(L);
        return 1;
    }
    *(void**)novo_ud(L, mt, sizeof(void*)) = ptr;
    ref_definir(L, lua_gettop(L), "dono", dono);
    return 1;
}

void* arg_ptr(lua_State* L, int i, const char* mt) {
    void** p = (void**)luaL_checkudata(L, i, mt);
    if (!*p) luaL_error(L, "objeto '%s' ja foi destruido", mt);
    return *p;
}

void* arg_ptr_ref(lua_State* L, int i, const char* mt_ref, const char* mt) {
    void** p = (void**)testar_ud(L, i, mt_ref);
    if (!p) p = (void**)luaL_checkudata(L, i, mt);
    if (!*p) luaL_error(L, "objeto '%s' ja foi destruido", mt);
    return *p;
}

bool arg_bool(lua_State* L, int i, bool padrao) {
    return lua_isnoneornil(L, i) ? padrao : lua_toboolean(L, i) != 0;
}

int arg_tile(lua_State* L, int i) {
    if (lua_type(L, i) == LUA_TSTRING) {
        size_t n;
        const char* s = lua_tolstring(L, i, &n);
        if (n != 1) luaL_argerror(L, i, "esperado 1 caractere");
        return (unsigned char)s[0];
    }
    return (int)luaL_checkinteger(L, i);
}

int ret_tile(lua_State* L, int c) {
    char ch = (char)c;
    if (ch == 0) lua_pushnil(L);
    else lua_pushlstring(L, &ch, 1);
    return 1;
}

uint32_t arg_cor(lua_State* L, int i) {
    return (uint32_t)(int64_t)luaL_checknumber(L, i);
}

uint32_t arg_cor_opcional(lua_State* L, int i) {
    if (lua_isnoneornil(L, i)) return COR_NENHUMA;
    if (lua_type(L, i) == LUA_TNUMBER && lua_tonumber(L, i) == 0) return COR_NENHUMA;
    return arg_cor(L, i);
}

int ret_cor(lua_State* L, uint32_t c) {
    lua_pushnumber(L, (lua_Number)c);
    return 1;
}

void ler_par(lua_State* L, int tabela, int indice, int* a, int* b) {
    lua_rawgeti(L, tabela, indice);
    if (!lua_istable(L, -1)) luaL_error(L, "item %d precisa ser {coluna, linha}", indice);
    lua_rawgeti(L, -1, 1);
    lua_rawgeti(L, -2, 2);
    *a = (int)lua_tointeger(L, -2);
    *b = (int)lua_tointeger(L, -1);
    lua_pop(L, 3);
}

fb_handle* arg_fb(lua_State* L, int i) {
    return (fb_handle*)arg_ptr_ref(L, i, MT_FBREF, MT_FB);
}

entrada_handle* arg_entrada(lua_State* L, int i) {
    return (entrada_handle*)arg_ptr_ref(L, i, MT_ENTRADAREF, MT_ENTRADA);
}

fb_atlas_handle* arg_atlas(lua_State* L, int i) { return (fb_atlas_handle*)arg_ptr(L, i, MT_ATLAS); }
obj_handle*      arg_obj(lua_State* L, int i)   { return (obj_handle*)arg_ptr(L, i, MT_OBJ); }
aabb_2d_handle*  arg_aabb(lua_State* L, int i)  { return (aabb_2d_handle*)arg_ptr(L, i, MT_AABB); }

mapa_handle* arg_mapa(lua_State* L, int i) {
    return (mapa_handle*)arg_ptr(L, i, MT_MAPA);
}

