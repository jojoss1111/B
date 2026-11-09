#include "lua_comum.h"

DEF_ARG(arg_audio, audio_handle, MT_AUDIO)
DEF_ARG(arg_caixa, caixa_handle, MT_CAIXA)

static int arg_tecla(lua_State* L, int i) { return (int)(uint8_t)luaL_checkinteger(L, i); }
static int arg_botao(lua_State* L, int i) { return (int)(uint8_t)luaL_checkinteger(L, i); }

DEF_GC(audio, audio_handle, audio_destruir)

static int l_audio_novo(lua_State* L) {
    const char* caminho = luaL_checkstring(L, 1);
    int loop = lua_toboolean(L, 2) != 0;
    float vol = (float)luaL_optnumber(L, 3, 1.0);
    int modo = (int)luaL_optinteger(L, 4, AUDIO_MODO_AUTOMATICO);
    return ret_ptr(L, MT_AUDIO, audio_criar(caminho, loop, vol, modo));
}

static int l_audio_liberar(lua_State* L)    { audio_liberar(arg_audio(L, 1)); return 0; }
static int l_audio_tocar(lua_State* L)      { audio_tocar(arg_audio(L, 1)); return 0; }
static int l_audio_parar(lua_State* L)      { audio_parar(arg_audio(L, 1)); return 0; }
static int l_audio_voltar(lua_State* L)     { audio_voltar(arg_audio(L, 1)); return 0; }
static int l_audio_parar_tudo(lua_State* L) { audio_parar_tudo(arg_audio(L, 1)); return 0; }

static int l_audio_disparar(lua_State* L) {
    audio_disparar(arg_audio(L, 1), (float)luaL_optnumber(L, 2, 1.0));
    return 0;
}

static int l_audio_repetir(lua_State* L) {
    audio_handle* a = arg_audio(L, 1);
    if (lua_isnoneornil(L, 2)) {
        lua_pushboolean(L, audio_repetindo(a));
        return 1;
    }
    audio_repetir_definir(a, lua_toboolean(L, 2) != 0);
    return 0;
}

static int l_audio_volume(lua_State* L) {
    audio_handle* a = arg_audio(L, 1);
    if (lua_isnoneornil(L, 2)) {
        lua_pushnumber(L, audio_volume(a));
        return 1;
    }
    audio_volume_definir(a, (float)luaL_checknumber(L, 2));
    return 0;
}

static int l_audio_tocando(lua_State* L)   { lua_pushboolean(L, audio_tocando(arg_audio(L, 1))); return 1; }
static int l_audio_valido(lua_State* L)    { lua_pushboolean(L, audio_valido(arg_audio(L, 1))); return 1; }
static int l_audio_streaming(lua_State* L) { lua_pushboolean(L, audio_streaming(arg_audio(L, 1))); return 1; }
static int l_audio_duracao(lua_State* L)   { lua_pushnumber(L, audio_duracao(arg_audio(L, 1))); return 1; }
static int l_audio_posicao(lua_State* L)   { lua_pushnumber(L, audio_posicao(arg_audio(L, 1))); return 1; }

static const luaL_Reg audio_metodos[] = {
    {"liberar",    l_audio_liberar},
    {"tocar",      l_audio_tocar},
    {"parar",      l_audio_parar},
    {"voltar",     l_audio_voltar},
    {"disparar",   l_audio_disparar},
    {"parar_tudo", l_audio_parar_tudo},
    {"repetir",    l_audio_repetir},
    {"volume",     l_audio_volume},
    {"tocando",    l_audio_tocando},
    {"valido",     l_audio_valido},
    {"streaming",  l_audio_streaming},
    {"duracao",    l_audio_duracao},
    {"posicao",    l_audio_posicao},
    {"__gc",       gc_audio},
    {NULL, NULL}
};

static const luaL_Reg audio_funcoes[] = {
    {"novo", l_audio_novo},
    {NULL, NULL}
};

DEF_GC(entrada, entrada_handle, entrada_destruir)

static int l_entrada_novo(lua_State* L) {
    return ret_ptr(L, MT_ENTRADA, entrada_criar());
}

static int l_entrada_layout_teclado(lua_State* L) {
    lua_pushstring(L, entrada_layout_teclado());
    return 1;
}

static int l_entrada_iniciar(lua_State* L)   { entrada_iniciar(arg_entrada(L, 1)); return 0; }
static int l_entrada_atualizar(lua_State* L) { entrada_atualizar(arg_entrada(L, 1)); return 0; }

static int l_entrada_pediu_fechar(lua_State* L) {
    lua_pushboolean(L, entrada_pediu_fechar(arg_entrada(L, 1)));
    return 1;
}

static int l_entrada_texto_digitado(lua_State* L) {
    lua_pushstring(L, entrada_texto_digitado(arg_entrada(L, 1)));
    return 1;
}

static int l_entrada_capslock(lua_State* L) {
    lua_pushboolean(L, entrada_capslock(arg_entrada(L, 1)));
    return 1;
}

static int l_entrada_pressionou(lua_State* L) {
    lua_pushboolean(L, entrada_pressionou(arg_entrada(L, 1), arg_tecla(L, 2)));
    return 1;
}

static int l_entrada_soltou(lua_State* L) {
    lua_pushboolean(L, entrada_soltou(arg_entrada(L, 1), arg_tecla(L, 2)));
    return 1;
}

static int l_entrada_segurando(lua_State* L) {
    lua_pushboolean(L, entrada_segurando(arg_entrada(L, 1), arg_tecla(L, 2)));
    return 1;
}

static int l_entrada_ultima(lua_State* L) {
    lua_pushinteger(L, entrada_ultima(arg_entrada(L, 1)));
    return 1;
}

static int l_entrada_mouse_pressionou(lua_State* L) {
    lua_pushboolean(L, entrada_mouse_pressionou(arg_entrada(L, 1), arg_botao(L, 2)));
    return 1;
}

static int l_entrada_mouse_soltou(lua_State* L) {
    lua_pushboolean(L, entrada_mouse_soltou(arg_entrada(L, 1), arg_botao(L, 2)));
    return 1;
}

static int l_entrada_mouse_segurando(lua_State* L) {
    lua_pushboolean(L, entrada_mouse_segurando(arg_entrada(L, 1), arg_botao(L, 2)));
    return 1;
}

static int l_entrada_mouse_x(lua_State* L)    { lua_pushinteger(L, entrada_mouse_x(arg_entrada(L, 1))); return 1; }
static int l_entrada_mouse_y(lua_State* L)    { lua_pushinteger(L, entrada_mouse_y(arg_entrada(L, 1))); return 1; }
static int l_entrada_mouse_roda(lua_State* L) { lua_pushinteger(L, entrada_mouse_roda(arg_entrada(L, 1))); return 1; }

static int l_entrada_mouse_janela(lua_State* L) {
    int x, y;
    entrada_mouse_janela(arg_entrada(L, 1), &x, &y);
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 2;
}

static int l_entrada_definir_mouse(lua_State* L) {
    entrada_definir_mouse(arg_entrada(L, 1), (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3));
    return 0;
}

static int l_entrada_toque_pressionou(lua_State* L) {
    lua_pushboolean(L, entrada_toque_pressionou(arg_entrada(L, 1), (int)luaL_checkinteger(L, 2)));
    return 1;
}

static int l_entrada_toque_soltou(lua_State* L) {
    lua_pushboolean(L, entrada_toque_soltou(arg_entrada(L, 1), (int)luaL_checkinteger(L, 2)));
    return 1;
}

static int l_entrada_toque_segurando(lua_State* L) {
    lua_pushboolean(L, entrada_toque_segurando(arg_entrada(L, 1), (int)luaL_checkinteger(L, 2)));
    return 1;
}

static int l_entrada_toque_x(lua_State* L) {
    lua_pushinteger(L, entrada_toque_x(arg_entrada(L, 1), (int)luaL_checkinteger(L, 2)));
    return 1;
}

static int l_entrada_toque_y(lua_State* L) {
    lua_pushinteger(L, entrada_toque_y(arg_entrada(L, 1), (int)luaL_checkinteger(L, 2)));
    return 1;
}

static int l_entrada_toques_ativos(lua_State* L) {
    lua_pushinteger(L, entrada_toques_ativos(arg_entrada(L, 1)));
    return 1;
}

static int l_entrada_definir_area_toque(lua_State* L) {
    entrada_definir_area_toque(arg_entrada(L, 1), (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3));
    return 0;
}

const luaL_Reg entrada_comum[] = {
    {"iniciar",            l_entrada_iniciar},
    {"atualizar",          l_entrada_atualizar},
    {"pediu_fechar",       l_entrada_pediu_fechar},
    {"texto_digitado",     l_entrada_texto_digitado},
    {"capslock",           l_entrada_capslock},
    {"pressionou",         l_entrada_pressionou},
    {"soltou",             l_entrada_soltou},
    {"segurando",          l_entrada_segurando},
    {"ultima",             l_entrada_ultima},
    {"mouse_pressionou",   l_entrada_mouse_pressionou},
    {"mouse_soltou",       l_entrada_mouse_soltou},
    {"mouse_segurando",    l_entrada_mouse_segurando},
    {"mouse_x",            l_entrada_mouse_x},
    {"mouse_y",            l_entrada_mouse_y},
    {"mouse_roda",         l_entrada_mouse_roda},
    {"mouse_janela",       l_entrada_mouse_janela},
    {"definir_mouse",      l_entrada_definir_mouse},
    {"toque_pressionou",   l_entrada_toque_pressionou},
    {"toque_soltou",       l_entrada_toque_soltou},
    {"toque_segurando",    l_entrada_toque_segurando},
    {"toque_x",            l_entrada_toque_x},
    {"toque_y",            l_entrada_toque_y},
    {"toques_ativos",      l_entrada_toques_ativos},
    {"definir_area_toque", l_entrada_definir_area_toque},
    {NULL, NULL}
};

static const luaL_Reg entrada_metodos[] = {
    {"__gc", gc_entrada},
    {NULL, NULL}
};

static const luaL_Reg entrada_funcoes[] = {
    {"novo",           l_entrada_novo},
    {"layout_teclado", l_entrada_layout_teclado},
    {NULL, NULL}
};

DEF_GC(caixa, caixa_handle, caixa_destruir)

static int l_caixa_novo(lua_State* L) {
    return ret_ptr(L, MT_CAIXA, caixa_novo());
}

static int l_caixa_usar_entrada(lua_State* L) {
    if (lua_isnoneornil(L, 1)) {
        caixa_usar_entrada(NULL);
        lua_pushnil(L);
    } else {
        caixa_usar_entrada(arg_entrada(L, 1));
        lua_pushvalue(L, 1);
    }
    lua_setfield(L, LUA_REGISTRYINDEX, "caixa_entrada");
    return 0;
}

static int l_caixa_criar_retangulo(lua_State* L) {
    caixa_criar_retangulo(arg_caixa(L, 1), (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3),
                          (int)luaL_checkinteger(L, 4), (int)luaL_checkinteger(L, 5));
    return 0;
}

static int l_caixa_criar_circulo(lua_State* L) {
    caixa_criar_circulo(arg_caixa(L, 1), (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3),
                        (int)luaL_checkinteger(L, 4));
    return 0;
}

static int l_caixa_destruir(lua_State* L) { caixa_desativar(arg_caixa(L, 1)); return 0; }

static int l_caixa_mover(lua_State* L) {
    caixa_mover(arg_caixa(L, 1), (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3));
    return 0;
}

static int l_caixa_tamanho(lua_State* L) {
    caixa_tamanho(arg_caixa(L, 1), (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3));
    return 0;
}

static int l_caixa_raio(lua_State* L) {
    caixa_raio(arg_caixa(L, 1), (int)luaL_checkinteger(L, 2));
    return 0;
}

static int l_caixa_ativa(lua_State* L) { lua_pushboolean(L, caixa_ativa(arg_caixa(L, 1))); return 1; }

static int l_caixa_dentro(lua_State* L) {
    lua_pushboolean(L, caixa_dentro(arg_caixa(L, 1), (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3)));
    return 1;
}

static int l_caixa_mouse_em_cima(lua_State* L) {
    lua_pushboolean(L, caixa_mouse_em_cima(arg_caixa(L, 1)));
    return 1;
}

static int l_caixa_pressionou(lua_State* L) {
    caixa_handle* c = arg_caixa(L, 1);
    int b = arg_botao(L, 2);
    lua_pushboolean(L, lua_isnoneornil(L, 3) ? caixa_pressionou(c, b) : caixa_pressionou_em(c, arg_entrada(L, 3), b));
    return 1;
}

static int l_caixa_segurou(lua_State* L) {
    caixa_handle* c = arg_caixa(L, 1);
    int b = arg_botao(L, 2);
    lua_pushboolean(L, lua_isnoneornil(L, 3) ? caixa_segurou(c, b) : caixa_segurou_em(c, arg_entrada(L, 3), b));
    return 1;
}

static int l_caixa_soltou(lua_State* L) {
    caixa_handle* c = arg_caixa(L, 1);
    int b = arg_botao(L, 2);
    lua_pushboolean(L, lua_isnoneornil(L, 3) ? caixa_soltou(c, b) : caixa_soltou_em(c, arg_entrada(L, 3), b));
    return 1;
}

static const luaL_Reg caixa_metodos[] = {
    {"criar_retangulo", l_caixa_criar_retangulo},
    {"criar_circulo",   l_caixa_criar_circulo},
    {"destruir",        l_caixa_destruir},
    {"mover",           l_caixa_mover},
    {"tamanho",         l_caixa_tamanho},
    {"raio",            l_caixa_raio},
    {"ativa",           l_caixa_ativa},
    {"dentro",          l_caixa_dentro},
    {"mouse_em_cima",   l_caixa_mouse_em_cima},
    {"pressionou",      l_caixa_pressionou},
    {"segurou",         l_caixa_segurou},
    {"soltou",          l_caixa_soltou},
    {"__gc",            gc_caixa},
    {NULL, NULL}
};

static const luaL_Reg caixa_funcoes[] = {
    {"novo",         l_caixa_novo},
    {"usar_entrada", l_caixa_usar_entrada},
    {NULL, NULL}
};

static void registrar_constantes_entrada(lua_State* L) {
    int i;
    char c;

    lua_newtable(L);
    campo(L, "seta_cima", 1);
    campo(L, "seta_baixo", 2);
    campo(L, "seta_esquerda", 3);
    campo(L, "seta_direita", 4);
    campo(L, "ctrl", 5);
    campo(L, "shift", 6);
    campo(L, "backspace", 8);
    campo(L, "tab", 9);
    campo(L, "start", 13);
    campo(L, "sair", 27);
    campo(L, "espaco", 32);
    for (i = 0; i < 12; i++) {
        char n[4];
        snprintf(n, sizeof n, "f%d", i + 1);
        campo(L, n, 14 + i);
    }
    for (c = 'a'; c <= 'z'; c++) {
        char n[2];
        n[0] = c;
        n[1] = 0;
        campo(L, n, c);
    }
    for (c = '0'; c <= '9'; c++) {
        char n[3];
        n[0] = 'n';
        n[1] = c;
        n[2] = 0;
        campo(L, n, c);
    }
    lua_setglobal(L, "tecla");

    lua_newtable(L);
    campo(L, "esquerdo", 0);
    campo(L, "direito", 1);
    campo(L, "meio", 2);
    lua_setglobal(L, "botao");

    lua_newtable(L);
    campo(L, "automatico", 0);
    campo(L, "memoria", 1);
    campo(L, "stream", 2);
    lua_setglobal(L, "modo_audio");
}

void bindings_audio_input(lua_State* L) {
    registrar_classe(L, MT_AUDIO, audio_metodos, NULL);
    registrar_classe(L, MT_ENTRADA, entrada_metodos, entrada_comum);
    registrar_classe(L, MT_CAIXA, caixa_metodos, NULL);
    registrar_global(L, "audio", audio_funcoes);
    registrar_global(L, "entrada", entrada_funcoes);
    registrar_global(L, "caixa", caixa_funcoes);
    registrar_constantes_entrada(L);
}