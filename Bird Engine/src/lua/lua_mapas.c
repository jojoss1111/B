#include "lua_comum.h"

DEF_ARG(arg_map, mapa_handle, MT_MAPA)
DEF_GC(mapa, mapa_handle, mapa_destruir)

static int layer_arg(lua_State *L, int i) {
  if (lua_isnumber(L, i)) {
    int n = (int)lua_tointeger(L, i);
    luaL_argcheck(L, n >= 1 && n <= 3, i, "camada deve ser 1, 2 ou 3");
    return n;
  }
  const char *s = luaL_checkstring(L, i);
  if (!strcmp(s, "abaixo") || !strcmp(s, "fundo"))
    return MAPA_ABAIXO;
  if (!strcmp(s, "mundo") || !strcmp(s, "jogador"))
    return MAPA_MUNDO;
  if (!strcmp(s, "frente") || !strcmp(s, "acima"))
    return MAPA_FRENTE;
  luaL_argerror(L, i, "camada deve ser abaixo, mundo ou frente");
  return MAPA_MUNDO;
}

static int l_novo(lua_State *L) { return ret_ptr(L, MT_MAPA, mapa_novo()); }
static int l_destruir(lua_State *L) {
  mapa_handle **p = (mapa_handle **)luaL_checkudata(L, 1, MT_MAPA);
  if (*p) {
    mapa_destruir(*p);
    *p = NULL;
  }
  return 0;
}
static int l_criar_mapa(lua_State *L) {
  mapa_handle *m = arg_map(L, 1);
  const char *textos[3] = {"", "", ""};
  luaL_checktype(L, 2, LUA_TTABLE);
  const char *nomes[3] = {"abaixo", "mundo", "frente"};
  for (int i = 0; i < 3; i++) {
    lua_rawgeti(L, 2, i + 1);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      lua_getfield(L, 2, nomes[i]);
    }
    if (!lua_isnil(L, -1))
      textos[i] = luaL_checkstring(L, -1);
    lua_pop(L, 1);
  }
  int ok = mapa_criar_mapa(m, (uint16_t)luaL_checkinteger(L, 3),
                           (uint16_t)luaL_checkinteger(L, 4), textos);
  lua_pushboolean(L, ok);
  return 1;
}
static int l_posicionar(lua_State *L) {
  mapa_posicionar(arg_map(L, 1), layer_arg(L, 2),
                  (int32_t)luaL_checkinteger(L, 3),
                  (int32_t)luaL_checkinteger(L, 4));
  return 0;
}
static int l_mover(lua_State *L) {
  mapa_mover(arg_map(L, 1), layer_arg(L, 2), (int32_t)luaL_checkinteger(L, 3),
             (int32_t)luaL_checkinteger(L, 4));
  return 0;
}
static int l_x(lua_State *L) {
  lua_pushinteger(L, mapa_x(arg_map(L, 1), layer_arg(L, 2)));
  return 1;
}
static int l_y(lua_State *L) {
  lua_pushinteger(L, mapa_y(arg_map(L, 1), layer_arg(L, 2)));
  return 1;
}
static int l_colunas(lua_State *L) {
  lua_pushinteger(L, mapa_colunas(arg_map(L, 1)));
  return 1;
}
static int l_linhas(lua_State *L) {
  lua_pushinteger(L, mapa_linhas(arg_map(L, 1)));
  return 1;
}
static int l_tile_largura(lua_State *L) {
  lua_pushinteger(L, mapa_tile_largura(arg_map(L, 1)));
  return 1;
}
static int l_tile_altura(lua_State *L) {
  lua_pushinteger(L, mapa_tile_altura(arg_map(L, 1)));
  return 1;
}
static int l_tile(lua_State *L) {
  lua_pushinteger(L, mapa_tile(arg_map(L, 1), layer_arg(L, 2),
                               (uint32_t)luaL_checkinteger(L, 3),
                               (uint32_t)luaL_checkinteger(L, 4)));
  return 1;
}
static int l_definir(lua_State *L) {
  mapa_definir(arg_map(L, 1), layer_arg(L, 2),
               (uint32_t)luaL_checkinteger(L, 3),
               (uint32_t)luaL_checkinteger(L, 4), arg_tile(L, 5));
  return 0;
}
static int l_substituir(lua_State *L) {
  mapa_handle *m = arg_map(L, 1);
  int layer = layer_arg(L, 2), old = arg_tile(L, 3), newv = arg_tile(L, 4);
  int all = lua_isnoneornil(L, 5);
  mapa_substituir(m, layer, old, newv,
                  all ? 0 : (int32_t)luaL_checkinteger(L, 5),
                  all ? 0 : (int32_t)luaL_checkinteger(L, 6), all);
  return 0;
}

static int l_sprites(lua_State *L) {
  int layer = layer_arg(L, 2), tile = arg_tile(L, 3);
  fb_atlas_handle *a = arg_atlas(L, 4);
  mapa_sprites(arg_map(L, 1), layer, tile, a, (uint16_t)luaL_checkinteger(L, 5),
               (uint16_t)luaL_checkinteger(L, 6));
  ref_definir_i(L, 1, REF_MAPA_ATLAS + layer * 256 + (uint8_t)tile, 4);
  return 0;
}
static int l_ampliar(lua_State *L) {
  mapa_ampliar(arg_map(L, 1), layer_arg(L, 2), arg_tile(L, 3),
               (uint16_t)luaL_checkinteger(L, 4),
               (uint16_t)luaL_checkinteger(L, 5));
  return 0;
}
static int l_cor(lua_State *L) {
  mapa_cor(arg_map(L, 1), layer_arg(L, 2), arg_tile(L, 3), arg_cor(L, 4));
  return 0;
}
static int l_angulo(lua_State *L) {
  mapa_angulo(arg_map(L, 1), layer_arg(L, 2), arg_tile(L, 3),
              (float)luaL_checknumber(L, 4));
  return 0;
}
static int l_alpha(lua_State *L) {
  mapa_alpha(arg_map(L, 1), layer_arg(L, 2), arg_tile(L, 3),
             (uint8_t)luaL_checkinteger(L, 4));
  return 0;
}
static int l_flip_x(lua_State *L) {
  mapa_espelhar_x(arg_map(L, 1), layer_arg(L, 2), arg_tile(L, 3),
                  arg_bool(L, 4, true));
  return 0;
}
static int l_flip_y(lua_State *L) {
  mapa_espelhar_y(arg_map(L, 1), layer_arg(L, 2), arg_tile(L, 3),
                  arg_bool(L, 4, true));
  return 0;
}
static int l_colisor(lua_State *L) {
  int layer = layer_arg(L, 2), tile = arg_tile(L, 3),
      key = REF_MAPA_COLISOR + layer * 256 + (uint8_t)tile;
  const char *s = lua_isnoneornil(L, 4) ? NULL : luaL_checkstring(L, 4);
  mapa_colisor(arg_map(L, 1), layer, tile, s,
               (uint16_t)luaL_optinteger(L, 5, 0),
               (uint16_t)luaL_optinteger(L, 6, 0));
  if (s)
    ref_definir_i(L, 1, key, 4);
  else
    ref_limpar_i(L, 1, key);
  return 0;
}

static int l_animacao(lua_State *L) {
  mapa_handle *m = arg_map(L, 1);
  int tile = arg_tile(L, 2), n;
  fb_atlas_handle *a = arg_atlas(L, 3);
  uint16_t frames[255][2];
  luaL_checktype(L, 4, LUA_TTABLE);
  n = tam_tabela(L, 4);
  luaL_argcheck(L, n >= 1 && n <= 255, 4,
                "a animação precisa ter de 1 a 255 quadros");
  int fps = (int)luaL_checkinteger(L, 5);
  luaL_argcheck(L, fps >= 1 && fps <= 1000, 5, "fps entre 1 e 1000");
  for (int i = 0; i < n; i++) {
    int c, r;
    ler_par(L, 4, i + 1, &c, &r);
    luaL_argcheck(L, c >= 0 && c <= 65535 && r >= 0 && r <= 65535, 4,
                  "coordenada do atlas fora do intervalo");
    frames[i][0] = (uint16_t)c;
    frames[i][1] = (uint16_t)r;
  }
  mapa_animacao(m, tile, a, (const uint16_t (*)[2])frames, (uint16_t)n,
                (uint16_t)fps, arg_bool(L, 6, true));
  for (int i = 1; i <= 3; i++)
    ref_definir_i(L, 1, REF_MAPA_ATLAS + i * 256 + (uint8_t)tile, 3);
  return 0;
}
static int l_atualizar_animacao(lua_State *L) {
  int dt = (int)luaL_optinteger(L, 2, 17);
  if (dt > 0)
    mapa_atualizar_animacao(arg_map(L, 1), (uint32_t)dt);
  return 0;
}
static int l_parar(lua_State *L) {
  mapa_parar(arg_map(L, 1));
  return 0;
}
static int l_rodar(lua_State *L) {
  mapa_rodar(arg_map(L, 1));
  return 0;
}
static int l_voltar(lua_State *L) {
  mapa_voltar(arg_map(L, 1));
  return 0;
}
static int l_colidio_x(lua_State *L) {
  lua_pushboolean(L,
                  mapa_colidio_x(arg_map(L, 1), layer_arg(L, 2), arg_aabb(L, 3),
                                 (int32_t)luaL_checkinteger(L, 4)));
  return 1;
}
static int l_colidio_y(lua_State *L) {
  lua_pushboolean(L,
                  mapa_colidio_y(arg_map(L, 1), layer_arg(L, 2), arg_aabb(L, 3),
                                 (int32_t)luaL_checkinteger(L, 4)));
  return 1;
}
static int l_colidio_tile_x(lua_State *L) {
  lua_pushboolean(L, mapa_colidio_tile_x(arg_map(L, 1), layer_arg(L, 2),
                                         arg_tile(L, 3), arg_aabb(L, 4),
                                         (int32_t)luaL_checkinteger(L, 5)));
  return 1;
}
static int l_colidio_tile_y(lua_State *L) {
  lua_pushboolean(L, mapa_colidio_tile_y(arg_map(L, 1), layer_arg(L, 2),
                                         arg_tile(L, 3), arg_aabb(L, 4),
                                         (int32_t)luaL_checkinteger(L, 5)));
  return 1;
}
static int l_desenhar(lua_State *L) {
  mapa_desenhar(arg_map(L, 1), layer_arg(L, 2), arg_fb(L, 3));
  return 0;
}

static const luaL_Reg metodos[] = {{"criar_mapa", l_criar_mapa},
                                   {"posicionar", l_posicionar},
                                   {"mover", l_mover},
                                   {"x", l_x},
                                   {"y", l_y},
                                   {"colunas", l_colunas},
                                   {"linhas", l_linhas},
                                   {"tile_largura", l_tile_largura},
                                   {"tile_altura", l_tile_altura},
                                   {"tile", l_tile},
                                   {"definir", l_definir},
                                   {"substituir", l_substituir},
                                   {"sprites", l_sprites},
                                   {"ampliar", l_ampliar},
                                   {"cor", l_cor},
                                   {"angulo", l_angulo},
                                   {"alpha", l_alpha},
                                   {"espelhar_x", l_flip_x},
                                   {"espelhar_y", l_flip_y},
                                   {"colisor", l_colisor},
                                   {"animacao", l_animacao},
                                   {"atualizar_animacao", l_atualizar_animacao},
                                   {"parar", l_parar},
                                   {"rodar", l_rodar},
                                   {"voltar", l_voltar},
                                   {"colidio_x", l_colidio_x},
                                   {"colidio_y", l_colidio_y},
                                   {"colidio_tile_x", l_colidio_tile_x},
                                   {"colidio_tile_y", l_colidio_tile_y},
                                   {"desenhar", l_desenhar},
                                   {"destruir", l_destruir},
                                   {"__gc", gc_mapa},
                                   {NULL, NULL}};
static const luaL_Reg funcoes[] = {{"novo", l_novo}, {NULL, NULL}};
void bindings_mapas(lua_State *L) {
  registrar_classe(L, MT_MAPA, metodos, NULL);
  registrar_global(L, "mapa", funcoes);
}
