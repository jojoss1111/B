#include "lua_comum.h"

static Tempo* arg_tempo(lua_State* L, int i) {
    return (Tempo*)luaL_checkudata(L, i, MT_TEMPO);
}

static int ret_tempo(lua_State* L, Tempo valor) {
    *(Tempo*)novo_ud(L, MT_TEMPO, sizeof(Tempo)) = valor;
    return 1;
}

static int l_criar(lua_State* L) {
    return ret_tempo(L, criar_tempo((float)luaL_checknumber(L, 1)));
}

static int l_criar_repetido(lua_State* L) {
    return ret_tempo(L, criar_tempo_repetido((float)luaL_checknumber(L, 1)));
}

static int l_agora(lua_State* L) {
    lua_pushnumber(L, tempo_agora());
    return 1;
}

static int l_global_parar(lua_State* L) {
    (void)L;
    tempo_global_parar();
    return 0;
}

static int l_global_retomar(lua_State* L) {
    (void)L;
    tempo_global_retomar();
    return 0;
}

static int l_global_parado(lua_State* L) {
    lua_pushboolean(L, tempo_global_parado());
    return 1;
}

static int l_escala(lua_State* L) {
    tempo_escala((float)luaL_checknumber(L, 1));
    return 0;
}

static int l_escala_atual(lua_State* L) {
    lua_pushnumber(L, tempo_escala_atual());
    return 1;
}

static int m_passou(lua_State* L) {
    lua_pushboolean(L, passou_tempo(arg_tempo(L, 1)));
    return 1;
}

static int m_nao_passou(lua_State* L) {
    lua_pushboolean(L, nao_passou_tempo(arg_tempo(L, 1)));
    return 1;
}

static int m_parar(lua_State* L) {
    parar_tempo(arg_tempo(L, 1));
    return 0;
}

static int m_retomar(lua_State* L) {
    retomar_tempo(arg_tempo(L, 1));
    return 0;
}

static int m_reiniciar(lua_State* L) {
    reiniciar_tempo(arg_tempo(L, 1));
    return 0;
}

static int m_definir(lua_State* L) {
    definir_tempo(arg_tempo(L, 1), (float)luaL_checknumber(L, 2));
    return 0;
}

static int m_adicionar(lua_State* L) {
    adicionar_tempo(arg_tempo(L, 1), (float)luaL_checknumber(L, 2));
    return 0;
}

static int m_restante(lua_State* L) {
    lua_pushnumber(L, tempo_restante(arg_tempo(L, 1)));
    return 1;
}

static int m_decorrido(lua_State* L) {
    lua_pushnumber(L, tempo_decorrido(arg_tempo(L, 1)));
    return 1;
}

static int m_progresso(lua_State* L) {
    lua_pushnumber(L, tempo_progresso(arg_tempo(L, 1)));
    return 1;
}

static int m_parado(lua_State* L) {
    lua_pushboolean(L, tempo_esta_parado(arg_tempo(L, 1)));
    return 1;
}

static const luaL_Reg metodos_tempo[] = {
    {"passou",      m_passou},
    {"nao_passou",  m_nao_passou},
    {"parar",       m_parar},
    {"retomar",     m_retomar},
    {"reiniciar",   m_reiniciar},
    {"definir",     m_definir},
    {"adicionar",   m_adicionar},
    {"restante",    m_restante},
    {"decorrido",   m_decorrido},
    {"progresso",   m_progresso},
    {"parado",      m_parado},
    {NULL, NULL}
};

static const luaL_Reg globais_tempo[] = {
    {"criar",           l_criar},
    {"criar_repetido",  l_criar_repetido},
    {"agora",           l_agora},
    {"parar",           l_global_parar},
    {"retomar",         l_global_retomar},
    {"parado",          l_global_parado},
    {"escala",          l_escala},
    {"escala_atual",    l_escala_atual},
    {NULL, NULL}
};

void bindings_tempo(lua_State* L) {
    registrar_classe(L, MT_TEMPO, metodos_tempo, NULL);
    registrar_global(L, "tempo", globais_tempo);
}