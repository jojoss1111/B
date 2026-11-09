#include "lua_comum.h"

DEF_GC(obj, obj_handle, obj_destruir)
DEF_DESTRUIR(l_obj_destruir, obj_handle, MT_OBJ, obj_destruir)

static int l_obj_novo(lua_State* L) {
    fb_atlas_handle* atlas = arg_atlas(L, 1);
    uint16_t pw = (uint16_t)luaL_checkinteger(L, 2);
    uint16_t ph = (uint16_t)luaL_checkinteger(L, 3);
    int32_t  x  = (int32_t)luaL_optinteger(L, 4, 0);
    int32_t  y  = (int32_t)luaL_optinteger(L, 5, 0);
    uint16_t coluna = (uint16_t)luaL_optinteger(L, 6, 0);
    uint16_t linha  = (uint16_t)luaL_optinteger(L, 7, 0);
    obj_handle* o = obj_criar(atlas, pw, ph, x, y, coluna, linha);
    ret_ptr(L, MT_OBJ, o);
    if (o) ref_definir(L, lua_gettop(L), "atlas", 1);
    return 1;
}

static int l_obj_x(lua_State* L)       { lua_pushinteger(L, obj_x(arg_obj(L, 1))); return 1; }
static int l_obj_y(lua_State* L)       { lua_pushinteger(L, obj_y(arg_obj(L, 1))); return 1; }
static int l_obj_largura(lua_State* L) { lua_pushinteger(L, obj_largura(arg_obj(L, 1))); return 1; }
static int l_obj_altura(lua_State* L)  { lua_pushinteger(L, obj_altura(arg_obj(L, 1))); return 1; }

static int l_obj_posicionar(lua_State* L) {
    obj_posicionar(arg_obj(L, 1), (int32_t)luaL_checkinteger(L, 2), (int32_t)luaL_checkinteger(L, 3));
    return 0;
}

static int l_obj_mover(lua_State* L) {
    obj_mover(arg_obj(L, 1), (int32_t)luaL_checkinteger(L, 2), (int32_t)luaL_checkinteger(L, 3));
    return 0;
}

static int l_obj_tile(lua_State* L) {
    obj_tile(arg_obj(L, 1), (uint16_t)luaL_checkinteger(L, 2), (uint16_t)luaL_checkinteger(L, 3));
    return 0;
}

static int l_obj_cor(lua_State* L) {
    obj_cor(arg_obj(L, 1), arg_cor(L, 2));
    return 0;
}

static int l_obj_colorir(lua_State* L) {
    obj_colorir(arg_obj(L, 1), arg_bool(L, 2, true));
    return 0;
}

static int l_obj_alpha(lua_State* L) {
    obj_alpha(arg_obj(L, 1), (uint8_t)luaL_checkinteger(L, 2));
    return 0;
}

static int l_obj_ampliar(lua_State* L) {
    obj_ampliar(arg_obj(L, 1), (uint16_t)luaL_checkinteger(L, 2), (uint16_t)luaL_checkinteger(L, 3));
    return 0;
}

static int l_obj_angulo(lua_State* L) {
    obj_angulo(arg_obj(L, 1), (float)luaL_checknumber(L, 2));
    return 0;
}

static int l_obj_espelhar_x(lua_State* L) {
    obj_espelhar_x(arg_obj(L, 1), arg_bool(L, 2, true));
    return 0;
}

static int l_obj_espelhar_y(lua_State* L) {
    obj_espelhar_y(arg_obj(L, 1), arg_bool(L, 2, true));
    return 0;
}

static int l_obj_desenhar(lua_State* L) {
    obj_desenhar(arg_obj(L, 1), arg_fb(L, 2));
    return 0;
}

static const luaL_Reg obj_metodos[] = {
    {"destruir",   l_obj_destruir},
    {"x",          l_obj_x},
    {"y",          l_obj_y},
    {"largura",    l_obj_largura},
    {"altura",     l_obj_altura},
    {"posicionar", l_obj_posicionar},
    {"mover",      l_obj_mover},
    {"tile",       l_obj_tile},
    {"cor",        l_obj_cor},
    {"colorir",    l_obj_colorir},
    {"alpha",      l_obj_alpha},
    {"ampliar",    l_obj_ampliar},
    {"angulo",     l_obj_angulo},
    {"espelhar_x", l_obj_espelhar_x},
    {"espelhar_y", l_obj_espelhar_y},
    {"desenhar",   l_obj_desenhar},
    {"__gc",       gc_obj},
    {NULL, NULL}
};

static const luaL_Reg obj_funcoes[] = {
    {"novo", l_obj_novo},
    {NULL, NULL}
};

typedef struct anim_lua {
    anim_handle* anim;
    int16_t      quadros[255][2];
} anim_lua;

DEF_GC(anim, anim_handle, anim_destruir)

static anim_handle* arg_anim(lua_State* L, int i) {
    return (anim_handle*)arg_ptr(L, i, MT_ANIM);
}

static int l_anim_novo(lua_State* L) {
    obj_handle* o = arg_obj(L, 1);
    int fps, total, ud, i;
    bool loop;
    anim_lua* a;
    luaL_checktype(L, 2, LUA_TTABLE);
    fps = (int)luaL_checkinteger(L, 3);
    loop = arg_bool(L, 4, true);
    total = tam_tabela(L, 2);
    luaL_argcheck(L, fps >= 1 && fps <= 255, 3, "fps entre 1 e 255");
    luaL_argcheck(L, total >= 1 && total <= 255, 2, "quantidade de quadros entre 1 e 255");
    a = (anim_lua*)novo_ud(L, MT_ANIM, sizeof(anim_lua));
    ud = lua_gettop(L);
    for (i = 0; i < total; i++) {
        int c, l;
        ler_par(L, 2, i + 1, &c, &l);
        a->quadros[i][0] = (int16_t)c;
        a->quadros[i][1] = (int16_t)l;
    }
    a->anim = anim_criar(o, &a->quadros[0][0], (uint8_t)total, (uint8_t)fps, loop);
    if (!a->anim) return luaL_error(L, "falha ao criar animacao");
    ref_definir(L, ud, "obj", 1);
    return 1;
}

static int l_anim_destruir(lua_State* L) {
    anim_lua* a = (anim_lua*)luaL_checkudata(L, 1, MT_ANIM);
    if (a->anim) {
        anim_destruir(a->anim);
        a->anim = NULL;
    }
    return 0;
}

static int l_anim_rodar(lua_State* L)  { anim_rodar(arg_anim(L, 1)); return 0; }
static int l_anim_parar(lua_State* L)  { anim_parar(arg_anim(L, 1)); return 0; }
static int l_anim_voltar(lua_State* L) { anim_voltar(arg_anim(L, 1)); return 0; }

static int l_anim_atualizar(lua_State* L) {
    anim_atualizar(arg_anim(L, 1), (uint32_t)luaL_checkinteger(L, 2));
    return 0;
}

static const luaL_Reg anim_metodos[] = {
    {"destruir",  l_anim_destruir},
    {"rodar",     l_anim_rodar},
    {"parar",     l_anim_parar},
    {"voltar",    l_anim_voltar},
    {"atualizar", l_anim_atualizar},
    {"__gc",      gc_anim},
    {NULL, NULL}
};

static const luaL_Reg anim_funcoes[] = {
    {"novo", l_anim_novo},
    {NULL, NULL}
};

DEF_GC(aabb, aabb_2d_handle, aabb_2d_destruir)

static int l_aabb_novo(lua_State* L) {
    obj_handle* o = lua_isnoneornil(L, 1) ? NULL : arg_obj(L, 1);
    const char* mascara = lua_isnoneornil(L, 2) ? NULL : luaL_checkstring(L, 2);
    uint16_t mw = (uint16_t)luaL_optinteger(L, 3, 0);
    uint16_t mh = (uint16_t)luaL_optinteger(L, 4, 0);
    aabb_2d_handle* a = aabb_2d_criar(o, mascara, mw, mh);
    ret_ptr(L, MT_AABB, a);
    if (a && mascara) ref_definir(L, lua_gettop(L), "mascara", 2);
    return 1;
}

static int l_aabb_sincronizar(lua_State* L) {
    aabb_2d_sincronizar(arg_aabb(L, 1), arg_obj(L, 2));
    return 0;
}

static int l_aabb_posicionar(lua_State* L) {
    aabb_2d_posicionar(arg_aabb(L, 1), (int32_t)luaL_checkinteger(L, 2), (int32_t)luaL_checkinteger(L, 3));
    return 0;
}

static int l_aabb_x(lua_State* L)       { lua_pushinteger(L, aabb_2d_x(arg_aabb(L, 1))); return 1; }
static int l_aabb_y(lua_State* L)       { lua_pushinteger(L, aabb_2d_y(arg_aabb(L, 1))); return 1; }
static int l_aabb_largura(lua_State* L) { lua_pushinteger(L, aabb_2d_largura(arg_aabb(L, 1))); return 1; }
static int l_aabb_altura(lua_State* L)  { lua_pushinteger(L, aabb_2d_altura(arg_aabb(L, 1))); return 1; }

static int l_aabb_colidio(lua_State* L) {
    lua_pushboolean(L, aabb_2d_colidio(arg_aabb(L, 1), arg_aabb(L, 2)));
    return 1;
}

static int l_aabb_colidiu_x(lua_State* L) {
    lua_pushboolean(L, aabb_2d_colidiu_x(arg_aabb(L, 1), arg_aabb(L, 2), (int32_t)luaL_checkinteger(L, 3)));
    return 1;
}

static int l_aabb_colidiu_y(lua_State* L) {
    lua_pushboolean(L, aabb_2d_colidiu_y(arg_aabb(L, 1), arg_aabb(L, 2), (int32_t)luaL_checkinteger(L, 3)));
    return 1;
}

static const luaL_Reg aabb_metodos[] = {
    {"sincronizar", l_aabb_sincronizar},
    {"posicionar",  l_aabb_posicionar},
    {"x",           l_aabb_x},
    {"y",           l_aabb_y},
    {"largura",     l_aabb_largura},
    {"altura",      l_aabb_altura},
    {"colidio",     l_aabb_colidio},
    {"colidiu_x",   l_aabb_colidiu_x},
    {"colidiu_y",   l_aabb_colidiu_y},
    {"__gc",        gc_aabb},
    {NULL, NULL}
};

static const luaL_Reg aabb_funcoes[] = {
    {"novo", l_aabb_novo},
    {NULL, NULL}
};

void bindings_objetos(lua_State* L) {
    registrar_classe(L, MT_OBJ, obj_metodos, NULL);
    registrar_classe(L, MT_ANIM, anim_metodos, NULL);
    registrar_classe(L, MT_AABB, aabb_metodos, NULL);
    registrar_global(L, "obj", obj_funcoes);
    registrar_global(L, "animacao", anim_funcoes);
    registrar_global(L, "aabb", aabb_funcoes);
}