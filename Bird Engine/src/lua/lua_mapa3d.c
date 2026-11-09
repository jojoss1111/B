#include "lua_comum.h"

DEF_GC(mapa3d, mapa3d_handle, mapa3d_destruir)
DEF_DESTRUIR(m3d_destruir, mapa3d_handle, MT_MAPA3D, mapa3d_destruir)
DEF_ARG(arg_m3d, mapa3d_handle, MT_MAPA3D)

#define FLT(i)  ((float)luaL_checknumber(L, (i)))
#define INT(i)  ((int)luaL_checkinteger(L, (i)))
#define OFLT(i, d) ((float)luaL_optnumber(L, (i), (d)))
#define OINT(i, d) ((int)luaL_optinteger(L, (i), (d)))

static obj3d_handle* arg_o3d(lua_State* L, int i) {
    return (obj3d_handle*)arg_ptr_ref(L, i, MT_OBJ3DREF, MT_OBJ3D);
}

static int ret_id(lua_State* L, int id) {
    if (id < 0) lua_pushnil(L);
    else lua_pushinteger(L, id);
    return 1;
}

static int ret_obj(lua_State* L, obj3d_handle* o) {
    return ret_ref(L, MT_OBJ3DREF, o, 1);
}

static void tile_args(lua_State* L, const mapa3d_handle* m, int i, int* col, int* lin) {
    *col = *lin = -1;
    if (lua_type(L, i) == LUA_TSTRING) {
        const char* nome = lua_tostring(L, i);
        if (!mapa3d_tile_buscar(m, nome, col, lin))
            luaL_error(L, "tile '%s' nao definido (use mapa:tile)", nome);
    } else if (!lua_isnoneornil(L, i)) {
        *col = INT(i);
        *lin = OINT(i + 1, 0);
    }
}

static float* ler_pontos(lua_State* L, int t, int* n) {
    luaL_checktype(L, t, LUA_TTABLE);
    const int len = tam_tabela(L, t);
    lua_rawgeti(L, t, 1);
    const int aninhado = lua_istable(L, -1);
    lua_pop(L, 1);

    if (!aninhado && (len % 2) != 0)
        luaL_argerror(L, t, "pontos planos precisam ser pares {x, z}");

    const int cnt = aninhado ? len : len / 2;
    if (cnt < 3) luaL_argerror(L, t, "poligono precisa de pelo menos 3 pontos");

    float* p = (float*)lua_newuserdata(L, (size_t)cnt * 2 * sizeof(float));
    for (int i = 0; i < cnt; i++) {
        if (aninhado) {
            lua_rawgeti(L, t, i + 1);
            if (!lua_istable(L, -1))
                luaL_error(L, "ponto %d precisa ser uma tabela {x, z}", i + 1);
            lua_rawgeti(L, -1, 1);
            lua_rawgeti(L, -2, 2);
            p[2 * i]     = (float)luaL_checknumber(L, -2);
            p[2 * i + 1] = (float)luaL_checknumber(L, -1);
            lua_pop(L, 3);
        } else {
            lua_rawgeti(L, t, 2 * i + 1);
            lua_rawgeti(L, t, 2 * i + 2);
            p[2 * i]     = (float)luaL_checknumber(L, -2);
            p[2 * i + 1] = (float)luaL_checknumber(L, -1);
            lua_pop(L, 2);
        }
    }
    *n = cnt;
    return p;
}


static int l_criar(lua_State* L) {
    return ret_ptr(L, MT_MAPA3D, mapa3d_criar());
}

static int l_limpar(lua_State* L) {
    mapa3d_limpar(arg_m3d(L, 1));
    return 0;
}

/* Objetos criados pelo mapa guardam o atlas como ponteiro C. Mantenha vivos
 * todos os atlas usados, mesmo depois de mapa:atlas() selecionar outro. */
static void atlas_reter(lua_State* L, int mapa, int atlas) {
    refs_pegar(L, mapa);
    lua_getfield(L, -1, "atlas_usados");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, "atlas_usados");
    }
    const int lista = lua_gettop(L);
    const int n = tam_tabela(L, lista);
    for (int i = 1; i <= n; i++) {
        lua_rawgeti(L, lista, i);
        const int igual = lua_rawequal(L, -1, atlas);
        lua_pop(L, 1);
        if (igual) {
            lua_pop(L, 2);
            return;
        }
    }
    lua_pushvalue(L, atlas);
    lua_rawseti(L, lista, n + 1);
    lua_pop(L, 2);
}

static int l_atlas(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    if (lua_isnoneornil(L, 2)) {
        mapa3d_atlas(m, NULL);
        ref_limpar(L, 1, "atlas");
        return 0;
    }
    fb_atlas_handle* atlas = arg_atlas(L, 2);
    atlas_reter(L, 1, 2);
    mapa3d_atlas(m, atlas);
    ref_definir(L, 1, "atlas", 2);
    return 0;
}

static int l_tile(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    lua_pushboolean(L, mapa3d_tile_definir(m, luaL_checkstring(L, 2), INT(3), INT(4)));
    return 1;
}

static int l_tile_buscar(lua_State* L) {
    int c, l;
    if (!mapa3d_tile_buscar(arg_m3d(L, 1), luaL_checkstring(L, 2), &c, &l)) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, c);
    lua_pushinteger(L, l);
    return 2;
}

static int l_adicionar(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    void** p = (void**)luaL_checkudata(L, 2, MT_OBJ3D);
    obj3d_handle* o;
    if (!*p) return luaL_error(L, "objeto '%s' ja foi destruido", MT_OBJ3D);
    o = (obj3d_handle*)*p;
    if (!mapa3d_adicionar(m, o)) {
        lua_pushnil(L);
        return 1;
    }
    *p = NULL;
    return ret_obj(L, o);
}

static int l_remover(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    void** p = (void**)testar_ud(L, 2, MT_OBJ3DREF);
    if (!p) p = (void**)luaL_checkudata(L, 2, MT_OBJ3D);
    if (!*p) {
        lua_pushboolean(L, 0);
        return 1;
    }
    if (mapa3d_remover(m, (obj3d_handle*)*p)) {
        *p = NULL;
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

static int l_objetos(lua_State* L) {
    int n;
    obj3d_handle* const* v = mapa3d_objetos(arg_m3d(L, 1), &n);
    lua_createtable(L, n, 0);
    for (int i = 0; i < n; i++) {
        ret_obj(L, v[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static int l_total(lua_State* L) {
    int n;
    mapa3d_objetos(arg_m3d(L, 1), &n);
    lua_pushinteger(L, n);
    return 1;
}

static int l_cubo(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    int c, l;
    tile_args(L, m, 8, &c, &l);
    return ret_obj(L, mapa3d_cubo(m, FLT(2), FLT(3), FLT(4), FLT(5), FLT(6), FLT(7), c, l));
}

static int l_cilindro(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    int c, l;
    tile_args(L, m, 8, &c, &l);
    return ret_obj(L, mapa3d_cilindro(m, FLT(2), FLT(3), FLT(4), FLT(5), FLT(6), OINT(7, 16), c, l));
}

static int l_cone(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    int c, l;
    tile_args(L, m, 8, &c, &l);
    return ret_obj(L, mapa3d_cone(m, FLT(2), FLT(3), FLT(4), FLT(5), FLT(6), OINT(7, 16), c, l));
}

static int l_esfera(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    int c, l;
    tile_args(L, m, 8, &c, &l);
    return ret_obj(L, mapa3d_esfera(m, FLT(2), FLT(3), FLT(4), FLT(5), OINT(6, 16), OINT(7, 12), c, l));
}

static int l_poligono(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    int n, c, l;
    float* p = ler_pontos(L, 5, &n);
    tile_args(L, m, 7, &c, &l);
    return ret_obj(L, mapa3d_poligono(m, FLT(2), FLT(3), FLT(4), p, n, FLT(6), c, l));
}

static int l_billboard(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    int c, l;
    tile_args(L, m, 7, &c, &l);
    return ret_obj(L, mapa3d_billboard(m, FLT(2), FLT(3), FLT(4), FLT(5), FLT(6), c, l));
}

static int l_chao(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    int c, l;
    tile_args(L, m, 4, &c, &l);
    mapa3d_chao(m, FLT(2), OINT(3, 1), c, l);
    return 0;
}

static int l_paredes(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    int c, l;
    tile_args(L, m, 4, &c, &l);
    mapa3d_paredes(m, FLT(2), FLT(3), c, l);
    return 0;
}

static int l_escada(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    int c, l;
    tile_args(L, m, 8, &c, &l);
    mapa3d_escada(m, FLT(2), FLT(3), FLT(4), INT(5), FLT(6), FLT(7), c, l);
    return 0;
}

static int l_girar(lua_State* L) {
    mapa3d_girar(arg_m3d(L, 1), arg_o3d(L, 2), OFLT(3, 0), OFLT(4, 0), OFLT(5, 0));
    return 0;
}


static int l_atuador(lua_State* L) {
    return ret_id(L, mapa3d_atuador(arg_m3d(L, 1), arg_o3d(L, 2)));
}

static int l_atuador_deslocar(lua_State* L) {
    mapa3d_atuador_deslocar(arg_m3d(L, 1), INT(2), FLT(3), FLT(4), FLT(5));
    return 0;
}

static int l_atuador_giro(lua_State* L) {
    mapa3d_atuador_giro(arg_m3d(L, 1), INT(2), FLT(3), FLT(4), FLT(5));
    return 0;
}

static int l_atuador_velocidade(lua_State* L) {
    mapa3d_atuador_velocidade(arg_m3d(L, 1), INT(2), FLT(3));
    return 0;
}

static int l_atuador_modo(lua_State* L) {
    mapa3d_atuador_modo(arg_m3d(L, 1), INT(2), INT(3));
    return 0;
}

static int l_atuador_curva(lua_State* L) {
    mapa3d_atuador_curva(arg_m3d(L, 1), INT(2), INT(3));
    return 0;
}

static int l_atuador_pausa(lua_State* L) {
    mapa3d_atuador_pausa(arg_m3d(L, 1), INT(2), FLT(3));
    return 0;
}

static int l_atuador_auto_fechar(lua_State* L) {
    mapa3d_atuador_auto_fechar(arg_m3d(L, 1), INT(2), FLT(3));
    return 0;
}

static int l_atuador_carregar(lua_State* L) {
    mapa3d_atuador_carregar(arg_m3d(L, 1), INT(2), arg_bool(L, 3, true));
    return 0;
}

static int l_atuador_ativo(lua_State* L) {
    mapa3d_atuador_ativo(arg_m3d(L, 1), INT(2), arg_bool(L, 3, true));
    return 0;
}

static int l_atuador_anexar(lua_State* L) {
    lua_pushboolean(L, mapa3d_atuador_anexar(arg_m3d(L, 1), INT(2), arg_o3d(L, 3)));
    return 1;
}

static int l_atuador_acao(lua_State* L) {
    mapa3d_atuador_acao(arg_m3d(L, 1), INT(2), INT(3), OFLT(4, 0));
    return 0;
}

static int l_atuador_t(lua_State* L) {
    lua_pushnumber(L, mapa3d_atuador_t(arg_m3d(L, 1), INT(2)));
    return 1;
}

static int l_atuador_aberto(lua_State* L) {
    lua_pushboolean(L, mapa3d_atuador_aberto(arg_m3d(L, 1), INT(2)));
    return 1;
}

static int l_atuador_movendo(lua_State* L) {
    lua_pushboolean(L, mapa3d_atuador_movendo(arg_m3d(L, 1), INT(2)));
    return 1;
}

static int l_atuador_travado(lua_State* L) {
    lua_pushboolean(L, mapa3d_atuador_travado(arg_m3d(L, 1), INT(2)));
    return 1;
}

static int l_porta_deslizar(lua_State* L) {
    return ret_id(L, mapa3d_porta_deslizar(arg_m3d(L, 1), arg_o3d(L, 2), FLT(3), FLT(4), FLT(5), OFLT(6, 0)));
}

static int l_porta_subir(lua_State* L) {
    return ret_id(L, mapa3d_porta_subir(arg_m3d(L, 1), arg_o3d(L, 2), FLT(3), OFLT(4, 0)));
}

static int l_porta_descer(lua_State* L) {
    return ret_id(L, mapa3d_porta_descer(arg_m3d(L, 1), arg_o3d(L, 2), FLT(3), OFLT(4, 0)));
}

static int l_porta_girar(lua_State* L) {
    return ret_id(L, mapa3d_porta_girar(arg_m3d(L, 1), arg_o3d(L, 2), FLT(3), FLT(4), FLT(5), OFLT(6, 0)));
}

static int l_elevador(lua_State* L) {
    return ret_id(L, mapa3d_elevador(arg_m3d(L, 1), arg_o3d(L, 2), FLT(3), OFLT(4, 0)));
}

static int l_andador(lua_State* L) {
    return ret_id(L, mapa3d_andador(arg_m3d(L, 1), arg_o3d(L, 2), FLT(3), FLT(4), FLT(5),
                                    OFLT(6, 0), OFLT(7, 0), arg_bool(L, 8, true)));
}

static int l_interativo(lua_State* L) {
    return ret_id(L, mapa3d_interativo(arg_m3d(L, 1), arg_o3d(L, 2), 0.0f, 0.0f, 0.0f, OFLT(3, 2)));
}

static int l_interativo_em(lua_State* L) {
    return ret_id(L, mapa3d_interativo(arg_m3d(L, 1), NULL, FLT(2), FLT(3), FLT(4), OFLT(5, 2)));
}

static int l_interativo_entrada(lua_State* L) {
    mapa3d_interativo_entrada(arg_m3d(L, 1), INT(2), INT(3));
    return 0;
}

static int l_interativo_acao(lua_State* L) {
    mapa3d_interativo_acao(arg_m3d(L, 1), INT(2), INT(3), OFLT(4, 0));
    return 0;
}

static int l_interativo_ligar(lua_State* L) {
    lua_pushboolean(L, mapa3d_interativo_ligar(arg_m3d(L, 1), INT(2), INT(3)));
    return 1;
}

static int l_interativo_alcance(lua_State* L) {
    mapa3d_interativo_alcance(arg_m3d(L, 1), INT(2), FLT(3));
    return 0;
}

static int l_interativo_olhar(lua_State* L) {
    mapa3d_interativo_olhar(arg_m3d(L, 1), INT(2), FLT(3));
    return 0;
}

static int l_interativo_recarga(lua_State* L) {
    mapa3d_interativo_recarga(arg_m3d(L, 1), INT(2), FLT(3));
    return 0;
}

static int l_interativo_uma_vez(lua_State* L) {
    mapa3d_interativo_uma_vez(arg_m3d(L, 1), INT(2), arg_bool(L, 3, true));
    return 0;
}

static int l_interativo_ativar(lua_State* L) {
    mapa3d_interativo_ativar(arg_m3d(L, 1), INT(2), arg_bool(L, 3, true));
    return 0;
}

static int l_interativo_disparou(lua_State* L) {
    lua_pushboolean(L, mapa3d_interativo_disparou(arg_m3d(L, 1), INT(2)));
    return 1;
}

static int l_interativo_disparos(lua_State* L) {
    lua_pushinteger(L, mapa3d_interativo_disparos(arg_m3d(L, 1), INT(2)));
    return 1;
}

static int l_interativo_dentro(lua_State* L) {
    lua_pushboolean(L, mapa3d_interativo_dentro(arg_m3d(L, 1), INT(2)));
    return 1;
}

static int l_foco(lua_State* L) {
    return ret_id(L, mapa3d_foco(arg_m3d(L, 1)));
}

static int l_atualizar(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    const float dt = FLT(2);
    cam3d_handle* cam = lua_isnoneornil(L, 3) ? NULL : (cam3d_handle*)arg_ptr(L, 3, MT_CAM3D);
    mapa3d_atualizar(m, dt, cam, arg_bool(L, 4, false), arg_bool(L, 5, false));
    return 0;
}

static int l_desenhar(lua_State* L) {
    mapa3d_handle* m = arg_m3d(L, 1);
    cam3d_handle* cam = (cam3d_handle*)arg_ptr(L, 2, MT_CAM3D);
    fb_handle* fb = arg_fb(L, 3);
    zb_handle* zb = (zb_handle*)arg_ptr(L, 4, MT_ZBUFFER);
    mapa3d_desenhar(m, cam, fb, zb);
    return 0;
}

static const luaL_Reg mapa3d_metodos[] = {
    {"__gc",       gc_mapa3d},
    {"destruir",   m3d_destruir},
    {"limpar",     l_limpar},
    {"atlas",      l_atlas},
    {"tile",       l_tile},
    {"tile_buscar", l_tile_buscar},

    {"adicionar",  l_adicionar},
    {"remover",    l_remover},
    {"objetos",    l_objetos},
    {"total",      l_total},
    {"cubo",       l_cubo},
    {"cilindro",   l_cilindro},
    {"cone",       l_cone},
    {"esfera",     l_esfera},
    {"poligono",   l_poligono},
    {"billboard",  l_billboard},
    {"chao",       l_chao},
    {"paredes",    l_paredes},
    {"escada",     l_escada},
    {"girar",      l_girar},

    {"atuador",             l_atuador},
    {"atuador_deslocar",    l_atuador_deslocar},
    {"atuador_giro",        l_atuador_giro},
    {"atuador_velocidade",  l_atuador_velocidade},
    {"atuador_modo",        l_atuador_modo},
    {"atuador_curva",       l_atuador_curva},
    {"atuador_pausa",       l_atuador_pausa},
    {"atuador_auto_fechar", l_atuador_auto_fechar},
    {"atuador_carregar",    l_atuador_carregar},
    {"atuador_ativo",       l_atuador_ativo},
    {"atuador_anexar",      l_atuador_anexar},
    {"atuador_acao",        l_atuador_acao},
    {"atuador_t",           l_atuador_t},
    {"atuador_aberto",      l_atuador_aberto},
    {"atuador_movendo",     l_atuador_movendo},
    {"atuador_travado",     l_atuador_travado},

    {"porta_deslizar", l_porta_deslizar},
    {"porta_subir",    l_porta_subir},
    {"porta_descer",   l_porta_descer},
    {"porta_girar",    l_porta_girar},
    {"elevador",       l_elevador},
    {"andador",        l_andador},

    {"interativo",           l_interativo},
    {"interativo_em",        l_interativo_em},
    {"interativo_entrada",   l_interativo_entrada},
    {"interativo_acao",      l_interativo_acao},
    {"interativo_ligar",     l_interativo_ligar},
    {"interativo_alcance",   l_interativo_alcance},
    {"interativo_olhar",     l_interativo_olhar},
    {"interativo_recarga",   l_interativo_recarga},
    {"interativo_uma_vez",   l_interativo_uma_vez},
    {"interativo_ativar",    l_interativo_ativar},
    {"interativo_disparou",  l_interativo_disparou},
    {"interativo_disparos",  l_interativo_disparos},
    {"interativo_dentro",    l_interativo_dentro},
    {"foco",                 l_foco},

    {"atualizar", l_atualizar},
    {"desenhar",  l_desenhar},
    {NULL, NULL}
};

static const luaL_Reg mapa3d_global[] = {
    {"criar", l_criar},
    {NULL, NULL}
};

static void registrar_obj3d_ref(lua_State* L) {
    luaL_newmetatable(L, MT_OBJ3DREF);
    luaL_getmetatable(L, MT_OBJ3D);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

void bindings_mapa3d(lua_State* L) {
    registrar_classe(L, MT_MAPA3D, mapa3d_metodos, NULL);
    registrar_obj3d_ref(L);

    lua_newtable(L);
    definir_funcoes(L, mapa3d_global);

    campo(L, "MOV_ALTERNAR", MAPA3D_MOV_ALTERNAR);
    campo(L, "MOV_VAIVEM",   MAPA3D_MOV_VAIVEM);

    campo(L, "CURVA_LINEAR", MAPA3D_CURVA_LINEAR);
    campo(L, "CURVA_SUAVE",  MAPA3D_CURVA_SUAVE);

    campo(L, "ENTRADA_APERTAR",      MAPA3D_ENTRADA_APERTAR);
    campo(L, "ENTRADA_SEGURAR",      MAPA3D_ENTRADA_SEGURAR);
    campo(L, "ENTRADA_PROXIMIDADE",  MAPA3D_ENTRADA_PROXIMIDADE);

    campo(L, "ACAO_ALTERNAR",  MAPA3D_ACAO_ALTERNAR);
    campo(L, "ACAO_ABRIR",     MAPA3D_ACAO_ABRIR);
    campo(L, "ACAO_FECHAR",    MAPA3D_ACAO_FECHAR);
    campo(L, "ACAO_IR",        MAPA3D_ACAO_IR);
    campo(L, "ACAO_TRAVAR",    MAPA3D_ACAO_TRAVAR);
    campo(L, "ACAO_DESTRAVAR", MAPA3D_ACAO_DESTRAVAR);

    lua_setglobal(L, "mapa3d");
}

