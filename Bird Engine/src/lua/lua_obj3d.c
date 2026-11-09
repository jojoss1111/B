#include "lua_comum.h"
#include "../objetos/obj_3d.h"
#include "../cam/cam3d.h"

#define LN(i)      ((float)luaL_checknumber(L, (i)))
#define LNO(i, d)  ((float)luaL_optnumber(L, (i), (d)))
#define LI(i)      ((int)luaL_checkinteger(L, (i)))
#define LIO(i, d)  ((int)luaL_optinteger(L, (i), (d)))
#define LT(i)      ((uint16_t)luaL_checkinteger(L, (i)))
#define PARTE(i)   (LI(i) - 1)

DEF_GC(obj3d, obj3d_handle, obj3d_destruir)
DEF_DESTRUIR(l_destruir, obj3d_handle, MT_OBJ3D, obj3d_destruir)
static obj3d_handle* arg_obj3d(lua_State* L, int i) {
    return (obj3d_handle*)arg_ptr_ref(L, i, MT_OBJ3DREF, MT_OBJ3D);
}

static fb_atlas_handle* atlas_opt(lua_State* L, int i) {
    return lua_isnoneornil(L, i) ? NULL : arg_atlas(L, i);
}

static int ret_obj(lua_State* L, obj3d_handle* o, int atlas_i) {
    if (!o) {
        lua_pushnil(L);
        return 1;
    }
    ret_ptr(L, MT_OBJ3D, o);
    if (!lua_isnoneornil(L, atlas_i)) ref_definir(L, lua_gettop(L), "atlas", atlas_i);
    return 1;
}

static int l_cubo(lua_State* L) {
    fb_atlas_handle* a = atlas_opt(L, 1);
    return ret_obj(L, obj3d_criar_cubo(a, LN(2), LN(3), LN(4), LN(5), LN(6), LN(7), LNO(8, 1.0f), LNO(9, 1.0f), arg_bool(L, 10, true)), 1);
}

static int l_esfera(lua_State* L) {
    fb_atlas_handle* a = atlas_opt(L, 1);
    return ret_obj(L, obj3d_criar_esfera(a, LN(2), LN(3), LN(4), LN(5),
                                         LIO(6, 16), LIO(7, 12),
                                         LNO(8, 1.0f), LNO(9, 1.0f), arg_bool(L, 10, true)), 1);
}

static int l_cone(lua_State* L) {
    fb_atlas_handle* a = atlas_opt(L, 1);
    return ret_obj(L, obj3d_criar_cone(a, LN(2), LN(3), LN(4), LN(5), LN(6),
                                       LIO(7, 16), LNO(8, 1.0f), LNO(9, 1.0f), arg_bool(L, 10, true)), 1);
}

static int l_cilindro(lua_State* L) {
    fb_atlas_handle* a = atlas_opt(L, 1);
    return ret_obj(L, obj3d_criar_cilindro(a, LN(2), LN(3), LN(4), LN(5), LN(6),
                                           LIO(7, 16), LNO(8, 1.0f), LNO(9, 1.0f), arg_bool(L, 10, true)), 1);
}

static int l_billboard(lua_State* L) {
    fb_atlas_handle* a = atlas_opt(L, 1);
    return ret_obj(L, obj3d_criar_billboard(a, LN(2), LN(3), LN(4), LN(5), LN(6),
                                            LNO(7, 1.0f), LNO(8, 1.0f), arg_bool(L, 9, true)), 1);
}

static int l_carregar(lua_State* L) {
    fb_atlas_handle* a = atlas_opt(L, 1);
    const char* caminho = luaL_checkstring(L, 2);
    return ret_obj(L, obj3d_carregar(a, caminho, LN(3), LN(4), LN(5), LNO(6, 1.0f)), 1);
}

static int l_carregar_memoria(lua_State* L) {
    size_t n;
    fb_atlas_handle* a = atlas_opt(L, 1);
    const char* d = luaL_checklstring(L, 2, &n);
    return ret_obj(L, obj3d_carregar_memoria(a, (const uint8_t*)d, n, LN(3), LN(4), LN(5), LNO(6, 1.0f)), 1);
}

static int m_forma(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)obj3d_forma_de(arg_obj3d(L, 1)));
    return 1;
}

static int m_posicao(lua_State* L) {
    const obj3d_handle* o = arg_obj3d(L, 1);
    lua_pushnumber(L, obj3d_x(o));
    lua_pushnumber(L, obj3d_y(o));
    lua_pushnumber(L, obj3d_z(o));
    return 3;
}

static int m_rotacao(lua_State* L) {
    const obj3d_handle* o = arg_obj3d(L, 1);
    lua_pushnumber(L, obj3d_rot_x(o));
    lua_pushnumber(L, obj3d_rot_y(o));
    lua_pushnumber(L, obj3d_rot_z(o));
    return 3;
}

static int m_escala(lua_State* L) {
    const obj3d_handle* o = arg_obj3d(L, 1);
    lua_pushnumber(L, obj3d_escala_x(o));
    lua_pushnumber(L, obj3d_escala_y(o));
    lua_pushnumber(L, obj3d_escala_z(o));
    return 3;
}

static int m_posicionar(lua_State* L)   { obj3d_posicionar(arg_obj3d(L, 1), LN(2), LN(3), LN(4)); return 0; }
static int m_mover(lua_State* L)        { obj3d_mover(arg_obj3d(L, 1), LN(2), LN(3), LN(4)); return 0; }
static int m_rotacionar(lua_State* L)   { obj3d_rotacionar(arg_obj3d(L, 1), LN(2), LN(3), LN(4)); return 0; }
static int m_girar(lua_State* L)        { obj3d_girar(arg_obj3d(L, 1), LN(2), LN(3), LN(4)); return 0; }
static int m_rotacionar_x(lua_State* L) { obj3d_rotacionar_x(arg_obj3d(L, 1), LN(2)); return 0; }
static int m_rotacionar_y(lua_State* L) { obj3d_rotacionar_y(arg_obj3d(L, 1), LN(2)); return 0; }
static int m_rotacionar_z(lua_State* L) { obj3d_rotacionar_z(arg_obj3d(L, 1), LN(2)); return 0; }

static int m_escalar(lua_State* L) {
    obj3d_handle* o = arg_obj3d(L, 1);
    if (lua_isnoneornil(L, 3)) obj3d_escalar_uniforme(o, LN(2));
    else obj3d_escalar(o, LN(2), LN(3), LN(4));
    return 0;
}

static int m_escalar_uniforme(lua_State* L) { obj3d_escalar_uniforme(arg_obj3d(L, 1), LN(2)); return 0; }
static int m_ampliar(lua_State* L)          { obj3d_ampliar(arg_obj3d(L, 1), LN(2), LN(3), LN(4)); return 0; }

static int m_atlas(lua_State* L) {
    obj3d_handle* o = arg_obj3d(L, 1);
    if (lua_isnoneornil(L, 2)) {
        obj3d_atlas(o, NULL);
        ref_limpar(L, 1, "atlas");
    } else {
        obj3d_atlas(o, arg_atlas(L, 2));
        ref_definir(L, 1, "atlas", 2);
    }
    return 0;
}

static int m_cor(lua_State* L)     { obj3d_cor(arg_obj3d(L, 1), arg_cor(L, 2)); return 0; }
static int m_modo(lua_State* L)    { obj3d_modo(arg_obj3d(L, 1), LI(2)); return 0; }
static int m_cull(lua_State* L)    { obj3d_cull(arg_obj3d(L, 1), LI(2)); return 0; }
static int m_visivel(lua_State* L) { obj3d_visivel(arg_obj3d(L, 1), arg_bool(L, 2, true)); return 0; }

static int m_tile(lua_State* L)       { obj3d_tile(arg_obj3d(L, 1), LT(2), LT(3)); return 0; }
static int m_tile_cima(lua_State* L)  { obj3d_tile_cima(arg_obj3d(L, 1), LT(2), LT(3)); return 0; }
static int m_tile_lado(lua_State* L)  { obj3d_tile_lado(arg_obj3d(L, 1), LT(2), LT(3)); return 0; }
static int m_tile_baixo(lua_State* L) { obj3d_tile_baixo(arg_obj3d(L, 1), LT(2), LT(3)); return 0; }
static int m_tile_frente(lua_State* L) { obj3d_tile_frente(arg_obj3d(L, 1), LT(2), LT(3)); return 0; }
static int m_tile_tras(lua_State* L) { obj3d_tile_tras(arg_obj3d(L, 1), LT(2), LT(3)); return 0; }
static int m_tile_esquerda(lua_State* L) { obj3d_tile_esquerda(arg_obj3d(L, 1), LT(2), LT(3)); return 0; }
static int m_tile_direita(lua_State* L) { obj3d_tile_direita(arg_obj3d(L, 1), LT(2), LT(3)); return 0; }

static int m_repetir(lua_State* L)       { obj3d_repetir(arg_obj3d(L, 1), LN(2), LN(3), arg_bool(L, 4, true)); return 0; }
static int m_repetir_cima(lua_State* L)  { obj3d_repetir_cima(arg_obj3d(L, 1), LN(2), LN(3), arg_bool(L, 4, true)); return 0; }
static int m_repetir_lado(lua_State* L)  { obj3d_repetir_lado(arg_obj3d(L, 1), LN(2), LN(3), arg_bool(L, 4, true)); return 0; }
static int m_repetir_baixo(lua_State* L) { obj3d_repetir_baixo(arg_obj3d(L, 1), LN(2), LN(3), arg_bool(L, 4, true)); return 0; }

static int m_partes(lua_State* L) {
    lua_pushinteger(L, obj3d_partes(arg_obj3d(L, 1)));
    return 1;
}

static int m_parte_nome(lua_State* L) {
    lua_pushstring(L, obj3d_parte_nome(arg_obj3d(L, 1), PARTE(2)));
    return 1;
}

static int m_parte_buscar(lua_State* L) {
    const int i = obj3d_parte_buscar(arg_obj3d(L, 1), luaL_checkstring(L, 2));
    if (i < 0) lua_pushnil(L);
    else lua_pushinteger(L, i + 1);
    return 1;
}

static int m_parte_visivel(lua_State* L) {
    obj3d_parte_visivel(arg_obj3d(L, 1), PARTE(2), arg_bool(L, 3, true));
    return 0;
}

static int m_parte_tile(lua_State* L) {
    obj3d_parte_tile(arg_obj3d(L, 1), PARTE(2), LT(3), LT(4));
    return 0;
}

static int m_parte_repetir(lua_State* L) {
    obj3d_parte_repetir(arg_obj3d(L, 1), PARTE(2), LN(3), LN(4), arg_bool(L, 5, true));
    return 0;
}

static int m_colisao(lua_State* L) {
    const int tipo = lua_isboolean(L, 2) ? lua_toboolean(L, 2) : LI(2);
    obj3d_colisao(arg_obj3d(L, 1), tipo);
    return 0;
}

static int m_colisao_raio(lua_State* L) { obj3d_colisao_raio(arg_obj3d(L, 1), LN(2)); return 0; }

static int m_caixa(lua_State* L) {
    obj3d_caixa b;
    if (!obj3d_caixa_mundo(arg_obj3d(L, 1), &b)) return 0;
    for (int i = 0; i < 3; i++) lua_pushnumber(L, b.c[i]);
    for (int i = 0; i < 3; i++) lua_pushnumber(L, b.e[i]);
    return 6;
}

static float* ler_pontos(lua_State* L, int idx, int* n_saida) {
    luaL_checktype(L, idx, LUA_TTABLE);
    const int len = (int)lua_objlen(L, idx);

    lua_rawgeti(L, idx, 1);
    const int plano = lua_isnumber(L, -1);
    lua_pop(L, 1);

    const int n = plano ? len / 2 : len;
    if (n < 3) luaL_argerror(L, idx, "poligono precisa de pelo menos 3 pontos");

    float* p = (float*)lua_newuserdata(L, (size_t)n * 2 * sizeof(float));
    for (int i = 0; i < n; i++) {
        if (plano) {
            lua_rawgeti(L, idx, 2 * i + 1);
            lua_rawgeti(L, idx, 2 * i + 2);
        } else {
            lua_rawgeti(L, idx, i + 1);
            luaL_checktype(L, -1, LUA_TTABLE);
            lua_rawgeti(L, -1, 1);
            lua_rawgeti(L, -2, 2);
            lua_remove(L, -3);
        }
        p[2 * i]     = (float)luaL_checknumber(L, -2);
        p[2 * i + 1] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 2);
    }
    *n_saida = n;
    return p;
}

static int l_poligono(lua_State* L) {
    fb_atlas_handle* a = atlas_opt(L, 1);
    const float x = LN(2), y = LN(3), z = LN(4);
    const float altura = LNO(6, 1.0f), rl = LNO(7, 1.0f), ra = LNO(8, 1.0f);
    const int repetir = arg_bool(L, 9, true);
    int n;
    float* p = ler_pontos(L, 5, &n);
    return ret_obj(L, obj3d_criar_poligono(a, x, y, z, p, n, altura, rl, ra, repetir), 1);
}

static int m_poligono_n(lua_State* L) {
    lua_pushinteger(L, obj3d_poligono_n(arg_obj3d(L, 1)));
    return 1;
}

static int m_poligono_ler(lua_State* L) {
    float x, z;
    if (!obj3d_poligono_ler(arg_obj3d(L, 1), PARTE(2), &x, &z)) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, x);
    lua_pushnumber(L, z);
    return 2;
}

static int m_poligono_pontos(lua_State* L) {
    obj3d_handle* o = arg_obj3d(L, 1);
    int n;
    float* p = ler_pontos(L, 2, &n);
    lua_pushboolean(L, obj3d_poligono_pontos(o, p, n));
    return 1;
}

static int m_poligono_ponto(lua_State* L) {
    lua_pushboolean(L, obj3d_poligono_ponto(arg_obj3d(L, 1), PARTE(2), LN(3), LN(4)));
    return 1;
}

static int m_poligono_girar_pontos(lua_State* L) {
    lua_pushboolean(L, obj3d_poligono_girar_pontos(arg_obj3d(L, 1), LN(2), LNO(3, 0.0f), LNO(4, 0.0f)));
    return 1;
}

static int m_poligono_pivo(lua_State* L) {
    lua_pushboolean(L, obj3d_poligono_pivo(arg_obj3d(L, 1), LN(2), LN(3)));
    return 1;
}

static int m_poligono_altura(lua_State* L) {
    obj3d_poligono_altura(arg_obj3d(L, 1), LN(2));
    return 0;
}

static int m_billboard(lua_State* L) {
    const int tipo = lua_isboolean(L, 2) ? (lua_toboolean(L, 2) ? OBJ3D_BB_CILINDRICO : OBJ3D_BB_NENHUM) : LI(2);
    obj3d_billboard(arg_obj3d(L, 1), tipo);
    return 0;
}

static int m_billboard_tipo(lua_State* L) {
    lua_pushinteger(L, obj3d_billboard_tipo(arg_obj3d(L, 1)));
    return 1;
}

static int m_recalcular_limites(lua_State* L) { obj3d_recalcular_limites(arg_obj3d(L, 1)); return 0; }
static int m_normalizar(lua_State* L)         { obj3d_normalizar(arg_obj3d(L, 1)); return 0; }

static int m_desenhar(lua_State* L) {
    obj3d_handle* o = arg_obj3d(L, 1);
    fb_handle* fb = arg_fb(L, 2);
    zb_handle* zb = (zb_handle*)arg_ptr(L, 3, MT_ZBUFFER);
    cam3d_handle* c = (cam3d_handle*)arg_ptr(L, 4, MT_CAM3D);
    if (fb->w != zb->w || fb->h != zb->h)
        return luaL_error(L, "zbuffer (%dx%d) com tamanho diferente do frame buffer (%dx%d)", zb->w, zb->h, fb->w, fb->h);
    const zb_mat4 vp = cam3d_matriz_vp(c);
    obj3d_desenhar(o, fb, zb, &vp);
    return 0;
}

static const luaL_Reg metodos_obj3d[] = {
    {"destruir", l_destruir},
    {"__gc", gc_obj3d},

    {"forma", m_forma},
    {"posicao", m_posicao},
    {"rotacao", m_rotacao},
    {"escala", m_escala},

    {"posicionar", m_posicionar},
    {"mover", m_mover},
    {"rotacionar", m_rotacionar},
    {"rotacionar_x", m_rotacionar_x},
    {"rotacionar_y", m_rotacionar_y},
    {"rotacionar_z", m_rotacionar_z},
    {"girar", m_girar},
    {"escalar", m_escalar},
    {"escalar_uniforme", m_escalar_uniforme},
    {"ampliar", m_ampliar},

    {"atlas", m_atlas},
    {"cor", m_cor},
    {"modo", m_modo},
    {"cull", m_cull},
    {"visivel", m_visivel},

    {"tile", m_tile},
    {"tile_cima", m_tile_cima},
    {"tile_lado", m_tile_lado},
    {"tile_baixo", m_tile_baixo},
    {"tile_frente", m_tile_frente},
    {"tile_tras", m_tile_tras},
    {"tile_esquerda", m_tile_esquerda},
    {"tile_direita", m_tile_direita},
    {"repetir", m_repetir},
    {"repetir_cima", m_repetir_cima},
    {"repetir_lado", m_repetir_lado},
    {"repetir_baixo", m_repetir_baixo},

    {"partes", m_partes},
    {"parte_nome", m_parte_nome},
    {"parte_buscar", m_parte_buscar},
    {"parte_visivel", m_parte_visivel},
    {"parte_tile", m_parte_tile},
    {"parte_repetir", m_parte_repetir},

    {"billboard", m_billboard},
    {"billboard_tipo", m_billboard_tipo},

    {"colisao", m_colisao},
    {"colisao_raio", m_colisao_raio},
    {"caixa", m_caixa},
    {"recalcular_limites", m_recalcular_limites},
    {"normalizar", m_normalizar},

    {"poligono_n", m_poligono_n},
    {"poligono_ler", m_poligono_ler},
    {"poligono_pontos", m_poligono_pontos},
    {"poligono_ponto", m_poligono_ponto},
    {"poligono_girar_pontos", m_poligono_girar_pontos},
    {"poligono_pivo", m_poligono_pivo},
    {"poligono_altura", m_poligono_altura},

    {"desenhar", m_desenhar},
    {NULL, NULL}
};

static const luaL_Reg funcoes_obj3d[] = {
    {"cubo", l_cubo},
    {"esfera", l_esfera},
    {"cone", l_cone},
    {"cilindro", l_cilindro},
    {"carregar", l_carregar},
    {"carregar_memoria", l_carregar_memoria},
    {"poligono", l_poligono},
    {"billboard", l_billboard},
    {NULL, NULL}
};

void bindings_obj3d(lua_State* L) {
    registrar_classe(L, MT_OBJ3D, metodos_obj3d, NULL);
    registrar_global(L, "obj3d", funcoes_obj3d);

    lua_getglobal(L, "obj3d");

    campo(L, "CUBO", OBJ3D_CUBO);
    campo(L, "ESFERA", OBJ3D_ESFERA);
    campo(L, "CONE", OBJ3D_CONE);
    campo(L, "CILINDRO", OBJ3D_CILINDRO);
    campo(L, "MODELO", OBJ3D_MODELO);

    campo(L, "COL_NENHUMA", OBJ3D_COL_NENHUMA);
    campo(L, "COL_CAIXA", OBJ3D_COL_CAIXA);

    campo(L, "MODO_PLANO", ZB_MODO_PLANO);
    campo(L, "MODO_GOURAUD", ZB_MODO_GOURAUD);
    campo(L, "MODO_TEXTURA", ZB_MODO_TEXTURA);

    campo(L, "CULL_NENHUM", ZB_CULL_NENHUM);
    campo(L, "CULL_TRAS", ZB_CULL_TRAS);
    campo(L, "CULL_FRENTE", ZB_CULL_FRENTE);

    campo(L, "POLIGONO", OBJ3D_POLIGONO);
    campo(L, "BILLBOARD", OBJ3D_BILLBOARD);

    campo(L, "BB_NENHUM", OBJ3D_BB_NENHUM);
    campo(L, "BB_ESFERICO", OBJ3D_BB_ESFERICO);
    campo(L, "BB_CILINDRICO", OBJ3D_BB_CILINDRICO);

    lua_pop(L, 1);
}
