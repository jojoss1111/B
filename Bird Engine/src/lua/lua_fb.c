#include "lua_comum.h"

static int l_fx_de_int(lua_State* L) {
    lua_pushinteger(L, fb_fx_de_int((int)luaL_checkinteger(L, 1)));
    return 1;
}

static int l_fx_para_int(lua_State* L) {
    lua_pushinteger(L, fb_fx_para_int((int32_t)luaL_checkinteger(L, 1)));
    return 1;
}

static int l_fx_mul(lua_State* L) {
    lua_pushinteger(L, fb_fx_mul((int32_t)luaL_checkinteger(L, 1), (int32_t)luaL_checkinteger(L, 2)));
    return 1;
}

static int l_fx_div(lua_State* L) {
    lua_pushinteger(L, fb_fx_div((int32_t)luaL_checkinteger(L, 1), (int32_t)luaL_checkinteger(L, 2)));
    return 1;
}

static int l_graus(lua_State* L) {
    lua_pushinteger(L, fb_graus((float)luaL_checknumber(L, 1)));
    return 1;
}

static int l_seno(lua_State* L) {
    lua_pushinteger(L, fb_seno((uint16_t)luaL_checkinteger(L, 1)));
    return 1;
}

static int l_cosseno(lua_State* L) {
    lua_pushinteger(L, fb_cosseno((uint16_t)luaL_checkinteger(L, 1)));
    return 1;
}

static int l_cor(lua_State* L) {
    return ret_cor(L, fb_cor((uint8_t)luaL_checkinteger(L, 1),
                             (uint8_t)luaL_checkinteger(L, 2),
                             (uint8_t)luaL_checkinteger(L, 3),
                             (uint8_t)luaL_optinteger(L, 4, 255)));
}

DEF_GC(fb, fb_handle, fb_destruir)

static int l_fb_novo(lua_State* L) {
    int w = (int)luaL_checkinteger(L, 1);
    int h = (int)luaL_checkinteger(L, 2);
    fb_handle* fb = fb_criar(w, h);
    if (fb && !fb_valido(fb)) {
        fb_destruir(fb);
        fb = NULL;
    }
    return ret_ptr(L, MT_FB, fb);
}

static int l_fb_valido(lua_State* L) {
    lua_pushboolean(L, fb_valido(arg_fb(L, 1)));
    return 1;
}

static int l_fb_largura(lua_State* L) {
    lua_pushinteger(L, fb_largura(arg_fb(L, 1)));
    return 1;
}

static int l_fb_altura(lua_State* L) {
    lua_pushinteger(L, fb_altura(arg_fb(L, 1)));
    return 1;
}

static int l_fb_pitch(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)fb_pitch(arg_fb(L, 1)));
    return 1;
}

static int l_fb_total_pixels(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)fb_total_pixels(arg_fb(L, 1)));
    return 1;
}

static int l_fb_dados(lua_State* L) {
    lua_pushlightuserdata(L, fb_dados(arg_fb(L, 1)));
    return 1;
}

static int l_fb_dentro(lua_State* L) {
    fb_handle* fb = arg_fb(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    lua_pushboolean(L, x >= 0 && y >= 0 && x < fb_largura(fb) && y < fb_altura(fb));
    return 1;
}

static int l_fb_limpar(lua_State* L) {
    fb_limpar(arg_fb(L, 1), arg_cor(L, 2));
    return 0;
}

static int l_fb_pixel(lua_State* L) {
    fb_pixel(arg_fb(L, 1), (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3), arg_cor(L, 4));
    return 0;
}

static int l_fb_pixel_fx(lua_State* L) {
    fb_pixel_fx(arg_fb(L, 1), (int32_t)luaL_checkinteger(L, 2), (int32_t)luaL_checkinteger(L, 3), arg_cor(L, 4));
    return 0;
}

static int l_fb_misturar(lua_State* L) {
    fb_misturar(arg_fb(L, 1), (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3), arg_cor(L, 4));
    return 0;
}

static int l_fb_ler(lua_State* L) {
    return ret_cor(L, fb_ler(arg_fb(L, 1), (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3)));
}

static int l_fb_linha(lua_State* L) {
    fb_linha(arg_fb(L, 1),
             (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3),
             (int)luaL_checkinteger(L, 4), (int)luaL_checkinteger(L, 5),
             arg_cor(L, 6));
    return 0;
}

static int l_fb_retangulo(lua_State* L) {
    fb_retangulo(arg_fb(L, 1),
                 (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3),
                 (int)luaL_checkinteger(L, 4), (int)luaL_checkinteger(L, 5),
                 (float)luaL_optnumber(L, 6, 0.0), lua_toboolean(L, 7) != 0,
                 arg_cor(L, 8));
    return 0;
}

static int l_fb_triangulo_retangulo(lua_State* L) {
    fb_triangulo_retangulo(arg_fb(L, 1),
                           (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3),
                           (int)luaL_checkinteger(L, 4), (int)luaL_checkinteger(L, 5),
                           (float)luaL_optnumber(L, 6, 0.0), lua_toboolean(L, 7) != 0,
                           arg_cor(L, 8));
    return 0;
}

static int l_fb_circulo(lua_State* L) {
    fb_circulo(arg_fb(L, 1),
               (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3),
               (int)luaL_checkinteger(L, 4),
               (float)luaL_optnumber(L, 5, 0.0), lua_toboolean(L, 6) != 0,
               arg_cor(L, 7));
    return 0;
}

static int l_fb_blit(lua_State* L) {
    fb_blit(arg_fb(L, 1), arg_fb(L, 2), (int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4));
    return 0;
}

const luaL_Reg fb_comum[] = {
    {"valido",              l_fb_valido},
    {"largura",             l_fb_largura},
    {"altura",              l_fb_altura},
    {"pitch",               l_fb_pitch},
    {"total_pixels",        l_fb_total_pixels},
    {"dados",               l_fb_dados},
    {"dentro",              l_fb_dentro},
    {"limpar",              l_fb_limpar},
    {"pixel",               l_fb_pixel},
    {"pixel_fx",            l_fb_pixel_fx},
    {"misturar",            l_fb_misturar},
    {"ler",                 l_fb_ler},
    {"linha",               l_fb_linha},
    {"retangulo",           l_fb_retangulo},
    {"triangulo_retangulo", l_fb_triangulo_retangulo},
    {"circulo",             l_fb_circulo},
    {"blit",                l_fb_blit},
    {NULL, NULL}
};

static const luaL_Reg fb_metodos[] = {
    {"__gc", gc_fb},
    {NULL, NULL}
};

DEF_GC(atlas, fb_atlas_handle, fb_atlas_destruir)

static int l_atlas_novo(lua_State* L) {
    return ret_ptr(L, MT_ATLAS, fb_atlas_criar());
}

static int l_atlas_carregar(lua_State* L) {
    fb_atlas_handle* a = arg_atlas(L, 1);
    const char* caminho = luaL_checkstring(L, 2);
    lua_pushboolean(L, fb_atlas_carregar(a, caminho,
                                         (uint16_t)luaL_optinteger(L, 3, 0),
                                         (uint16_t)luaL_optinteger(L, 4, 0)));
    return 1;
}

static int l_atlas_carregar_memoria(lua_State* L) {
    fb_atlas_handle* a = arg_atlas(L, 1);
    size_t tam;
    const char* dados = luaL_checklstring(L, 2, &tam);
    lua_pushboolean(L, fb_atlas_carregar_memoria(a, (const uint8_t*)dados, tam,
                                                 (uint16_t)luaL_optinteger(L, 3, 0),
                                                 (uint16_t)luaL_optinteger(L, 4, 0)));
    return 1;
}

static int l_atlas_valido(lua_State* L) {
    lua_pushboolean(L, fb_atlas_valido(arg_atlas(L, 1)));
    return 1;
}

static int l_atlas_largura(lua_State* L) {
    lua_pushinteger(L, fb_atlas_largura(arg_atlas(L, 1)));
    return 1;
}

static int l_atlas_altura(lua_State* L) {
    lua_pushinteger(L, fb_atlas_altura(arg_atlas(L, 1)));
    return 1;
}

static int l_atlas_tile_largura(lua_State* L) {
    lua_pushinteger(L, fb_atlas_tile_largura(arg_atlas(L, 1)));
    return 1;
}

static int l_atlas_tile_altura(lua_State* L) {
    lua_pushinteger(L, fb_atlas_tile_altura(arg_atlas(L, 1)));
    return 1;
}

static int l_atlas_colunas(lua_State* L) {
    lua_pushinteger(L, fb_atlas_colunas(arg_atlas(L, 1)));
    return 1;
}

static int l_atlas_linhas(lua_State* L) {
    lua_pushinteger(L, fb_atlas_linhas(arg_atlas(L, 1)));
    return 1;
}

static int l_atlas_passo(lua_State* L) {
    lua_pushinteger(L, fb_atlas_passo(arg_atlas(L, 1)));
    return 1;
}

static int l_atlas_tile_dados(lua_State* L) {
    lua_pushlightuserdata(L, (void*)fb_atlas_tile_dados(arg_atlas(L, 1),
                                                        (int)luaL_checkinteger(L, 2),
                                                        (int)luaL_checkinteger(L, 3)));
    return 1;
}

static int l_atlas_desenhar(lua_State* L) {
    fb_atlas_desenhar(arg_atlas(L, 1), arg_fb(L, 2),
                      (int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4),
                      (int)luaL_checkinteger(L, 5), (int)luaL_checkinteger(L, 6),
                      (int)luaL_optinteger(L, 7, 0), (int)luaL_optinteger(L, 8, 0));
    return 0;
}

static int l_atlas_desenhar_fx(lua_State* L) {
    fb_atlas_desenhar_fx(arg_atlas(L, 1), arg_fb(L, 2),
                         (int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4),
                         (int32_t)luaL_checkinteger(L, 5), (int32_t)luaL_checkinteger(L, 6),
                         (int)luaL_optinteger(L, 7, 0), (int)luaL_optinteger(L, 8, 0));
    return 0;
}

static const luaL_Reg atlas_metodos[] = {
    {"carregar",         l_atlas_carregar},
    {"carregar_memoria", l_atlas_carregar_memoria},
    {"valido",           l_atlas_valido},
    {"largura",          l_atlas_largura},
    {"altura",           l_atlas_altura},
    {"tile_largura",     l_atlas_tile_largura},
    {"tile_altura",      l_atlas_tile_altura},
    {"colunas",          l_atlas_colunas},
    {"linhas",           l_atlas_linhas},
    {"passo",            l_atlas_passo},
    {"tile_dados",       l_atlas_tile_dados},
    {"desenhar",         l_atlas_desenhar},
    {"desenhar_fx",      l_atlas_desenhar_fx},
    {"__gc",             gc_atlas},
    {NULL, NULL}
};

static const luaL_Reg fb_funcoes[] = {
    {"novo",        l_fb_novo},
    {"atlas",       l_atlas_novo},
    {"cor",         l_cor},
    {"graus",       l_graus},
    {"seno",        l_seno},
    {"cosseno",     l_cosseno},
    {"fx_de_int",   l_fx_de_int},
    {"fx_para_int", l_fx_para_int},
    {"fx_mul",      l_fx_mul},
    {"fx_div",      l_fx_div},
    {NULL, NULL}
};

void bindings_frame_buffer(lua_State* L) {
    registrar_classe(L, MT_FB, fb_metodos, fb_comum);
    registrar_classe(L, MT_ATLAS, atlas_metodos, NULL);
    registrar_global(L, "fb", fb_funcoes);
}