#include "lua_comum.h"

DEF_GC(camera, camera_handle, camera_destruir)
DEF_DESTRUIR(l_destruir, camera_handle, MT_CAMERA, camera_destruir)
DEF_ARG(arg_camera, camera_handle, MT_CAMERA)

#define GET_INT(nome, fn)                                                      \
  static int nome(lua_State *L) {                                              \
    lua_pushinteger(L, fn(arg_camera(L, 1)));                                  \
    return 1;                                                                  \
  }
#define GET_NUM(nome, fn)                                                      \
  static int nome(lua_State *L) {                                              \
    lua_pushnumber(L, fn(arg_camera(L, 1)));                                   \
    return 1;                                                                  \
  }

/* ---------- criacao ---------- */

static int l_criar(lua_State *L) {
  lua_Integer w = luaL_checkinteger(L, 1);
  lua_Integer h = luaL_checkinteger(L, 2);
  int32_t x = (int32_t)luaL_optinteger(L, 3, 0);
  int32_t y = (int32_t)luaL_optinteger(L, 4, 0);
  luaL_argcheck(L, w > 0 && w <= 65535, 1, "largura invalida");
  luaL_argcheck(L, h > 0 && h <= 65535, 2, "altura invalida");
  return ret_ptr(L, MT_CAMERA, camera_criar((uint16_t)w, (uint16_t)h, x, y));
}

static int l_valida(lua_State *L) {
  camera_handle **p = (camera_handle **)luaL_checkudata(L, 1, MT_CAMERA);
  lua_pushboolean(L, *p && camera_valida(*p));
  return 1;
}

/* ---------- consultas ---------- */

GET_INT(l_x, camera_x)
GET_INT(l_y, camera_y)
GET_INT(l_largura_mundo, camera_largura_mundo)
GET_INT(l_altura_mundo, camera_altura_mundo)
GET_INT(l_largura, camera_largura)
GET_INT(l_altura, camera_altura)
GET_NUM(l_angulo, camera_angulo)
static int l_zoom(lua_State *L) {
  camera_handle *c = arg_camera(L, 1);
  if (!lua_isnoneornil(L, 2))
    camera_zoom_definir(c, (float)luaL_checknumber(L, 2));
  lua_pushnumber(L, camera_zoom(c));
  return 1;
}

static int l_viewport_largura(lua_State *L) {
  lua_pushinteger(L, camera_viewport_largura(arg_camera(L, 1)));
  return 1;
}

static int l_viewport_altura(lua_State *L) {
  lua_pushinteger(L, camera_viewport_altura(arg_camera(L, 1)));
  return 1;
}

/* devolve o framebuffer interno da camera (referencia, o dono e a camera) */
static int l_viewport(lua_State *L) {
  camera_handle *c = arg_camera(L, 1);
  return ret_ref(L, MT_FBREF, c->view, 1);
}

/* ---------- angulo / zoom ---------- */

static int l_angulo_definir(lua_State *L) {
  camera_angulo_definir(arg_camera(L, 1), (float)luaL_checknumber(L, 2));
  return 0;
}

static int l_girar(lua_State *L) {
  camera_girar(arg_camera(L, 1), (float)luaL_checknumber(L, 2));
  return 0;
}

static int l_zoom_definir(lua_State *L) {
  camera_zoom_definir(arg_camera(L, 1), (float)luaL_checknumber(L, 2));
  return 0;
}

static int l_zoom_relativo(lua_State *L) {
  camera_zoom_relativo(arg_camera(L, 1), (float)luaL_checknumber(L, 2));
  return 0;
}

static int l_zoom_em(lua_State *L) {
  camera_zoom_em(arg_camera(L, 1), (float)luaL_checknumber(L, 2),
                 (int32_t)luaL_checkinteger(L, 3),
                 (int32_t)luaL_checkinteger(L, 4));
  return 0;
}

static int l_zoom_suave(lua_State *L) {
  lua_Integer suavidade = luaL_optinteger(L, 3, 180);
  luaL_argcheck(L, suavidade >= 0 && suavidade <= 255, 3,
                "suavidade deve estar entre 0 e 255");
  camera_zoom_suave(arg_camera(L, 1), (float)luaL_checknumber(L, 2),
                    (uint8_t)suavidade);
  return 0;
}

static int l_limites_zoom(lua_State *L) {
  camera_limites_zoom(arg_camera(L, 1), (float)luaL_checknumber(L, 2),
                      (float)luaL_checknumber(L, 3));
  return 0;
}

/* ---------- posicao ---------- */

static int l_posicionar(lua_State *L) {
  camera_posicionar(arg_camera(L, 1), (int32_t)luaL_checkinteger(L, 2),
                    (int32_t)luaL_checkinteger(L, 3));
  return 0;
}

static int l_mover(lua_State *L) {
  camera_mover(arg_camera(L, 1), (int32_t)luaL_checkinteger(L, 2),
               (int32_t)luaL_checkinteger(L, 3));
  return 0;
}

static int l_centralizar(lua_State *L) {
  camera_centralizar(arg_camera(L, 1), (int32_t)luaL_checkinteger(L, 2),
                     (int32_t)luaL_checkinteger(L, 3));
  return 0;
}

static int l_mover_suave(lua_State *L) {
  camera_mover_suave(arg_camera(L, 1), (int32_t)luaL_checkinteger(L, 2),
                     (int32_t)luaL_checkinteger(L, 3),
                     (uint8_t)luaL_checkinteger(L, 4),
                     (float)luaL_checknumber(L, 5));
  return 0;
}

static int l_centralizar_mundo(lua_State *L) {
  camera_centralizar_mundo(arg_camera(L, 1), arg_mapa(L, 2),
                           (uint16_t)luaL_checkinteger(L, 3),
                           (uint16_t)luaL_checkinteger(L, 4));
  return 0;
}

/* ---------- alvo / limites ---------- */

static int l_ancorar(lua_State *L) {
  camera_handle *c = arg_camera(L, 1);
  obj_handle *alvo = arg_obj(L, 2);
  camera_ancorar(c, alvo, (int32_t)luaL_optinteger(L, 3, 0),
                 (int32_t)luaL_optinteger(L, 4, 0),
                 (uint8_t)luaL_optinteger(L, 5, 0));
  ref_definir(L, 1, "alvo", 2); /* impede o GC de liberar o alvo */
  return 0;
}

static int l_soltar(lua_State *L) {
  camera_soltar(arg_camera(L, 1));
  ref_limpar(L, 1, "alvo");
  return 0;
}

static int l_atualizar(lua_State *L) {
  camera_atualizar(arg_camera(L, 1), (float)luaL_optnumber(L, 2, 1.0));
  return 0;
}

static int l_travar_mapa(lua_State *L) {
  camera_travar_mapa(arg_camera(L, 1), arg_mapa(L, 2),
                     (uint16_t)luaL_checkinteger(L, 3),
                     (uint16_t)luaL_checkinteger(L, 4));
  return 0;
}

static int l_travar_limites(lua_State *L) {
  camera_travar_limites(arg_camera(L, 1), (int32_t)luaL_checkinteger(L, 2),
                        (int32_t)luaL_checkinteger(L, 3),
                        (int32_t)luaL_checkinteger(L, 4),
                        (int32_t)luaL_checkinteger(L, 5));
  return 0;
}

static int l_destravar(lua_State *L) {
  camera_destravar(arg_camera(L, 1));
  return 0;
}

/* ---------- conversao de coordenadas ---------- */

static int l_mundo_para_tela(lua_State *L) {
  int32_t tx = 0, ty = 0;
  camera_mundo_para_tela(arg_camera(L, 1), (int32_t)luaL_checkinteger(L, 2),
                         (int32_t)luaL_checkinteger(L, 3), &tx, &ty);
  lua_pushinteger(L, tx);
  lua_pushinteger(L, ty);
  return 2;
}

static int l_tela_para_mundo(lua_State *L) {
  int32_t mx = 0, my = 0;
  camera_tela_para_mundo(arg_camera(L, 1), (int32_t)luaL_checkinteger(L, 2),
                         (int32_t)luaL_checkinteger(L, 3), &mx, &my);
  lua_pushinteger(L, mx);
  lua_pushinteger(L, my);
  return 2;
}

/* ---------- desenho ---------- */

static int l_iniciar(lua_State *L) {
  camera_iniciar(arg_camera(L, 1), arg_cor_opcional(L, 2));
  return 0;
}

static int l_visivel(lua_State *L) {
  lua_pushboolean(L, camera_visivel(arg_camera(L, 1),
                                    (int32_t)luaL_checkinteger(L, 2),
                                    (int32_t)luaL_checkinteger(L, 3),
                                    (int32_t)luaL_checkinteger(L, 4),
                                    (int32_t)luaL_checkinteger(L, 5)));
  return 1;
}

static int l_desenhar_obj(lua_State *L) {
  camera_desenhar_obj(arg_camera(L, 1), arg_obj(L, 2));
  return 0;
}

static int camada_mapa_arg(lua_State *L, int i) {
  if (lua_isnumber(L, i)) {
    int n = (int)luaL_checkinteger(L, i);
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

static int l_desenhar_mapa(lua_State *L) {
  camera_desenhar_mapa(arg_camera(L, 1), arg_mapa(L, 2), camada_mapa_arg(L, 3));
  return 0;
}

static int l_apresentar(lua_State *L) {
  camera_apresentar(arg_camera(L, 1), arg_fb(L, 2),
                    (int)luaL_optinteger(L, 3, 0),
                    (int)luaL_optinteger(L, 4, 0));
  return 0;
}

/* ---------- registro ---------- */

static const luaL_Reg metodos_camera[] = {
    {"__gc", gc_camera},
    {"destruir", l_destruir},
    {"valida", l_valida},

    {"x", l_x},
    {"y", l_y},
    {"largura_mundo", l_largura_mundo},
    {"altura_mundo", l_altura_mundo},
    {"largura", l_largura},
    {"altura", l_altura},
    {"viewport", l_viewport},
    {"viewport_largura", l_viewport_largura},
    {"viewport_altura", l_viewport_altura},

    {"angulo", l_angulo},
    {"angulo_definir", l_angulo_definir},
    {"girar", l_girar},
    {"zoom", l_zoom},
    {"zoom_definir", l_zoom_definir},
    {"zoom_relativo", l_zoom_relativo},
    {"zoom_em", l_zoom_em},
    {"zoom_suave", l_zoom_suave},
    {"limites_zoom", l_limites_zoom},

    {"posicionar", l_posicionar},
    {"mover", l_mover},
    {"centralizar", l_centralizar},
    {"mover_suave", l_mover_suave},
    {"centralizar_mundo", l_centralizar_mundo},

    {"ancorar", l_ancorar},
    {"soltar", l_soltar},
    {"atualizar", l_atualizar},
    {"travar_mapa", l_travar_mapa},
    {"travar_limites", l_travar_limites},
    {"destravar", l_destravar},

    {"mundo_para_tela", l_mundo_para_tela},
    {"tela_para_mundo", l_tela_para_mundo},

    {"iniciar", l_iniciar},
    {"visivel", l_visivel},
    {"desenhar_obj", l_desenhar_obj},
    {"desenhar_mapa", l_desenhar_mapa},
    {"apresentar", l_apresentar},
    {NULL, NULL}};

static const luaL_Reg funcs_camera[] = {{"criar", l_criar}, {"novo", l_criar}, {NULL, NULL}};

void bindings_camera(lua_State *L) {
  registrar_classe(L, MT_CAMERA, metodos_camera, NULL);
  registrar_global(L, "camera", funcs_camera);
  lua_getglobal(L, "camera");
  lua_setglobal(L, "camera25d");
}
