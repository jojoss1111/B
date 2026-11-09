#include "lua_comum.h"
#include "../objetos/obj_3d.h"
#include "../cam/cam3d.h"

#define LN(i)      ((float)luaL_checknumber(L, (i)))
#define LNO(i, d)  ((float)luaL_optnumber(L, (i), (d)))

/* O ponteiro da camera DEVE ser o primeiro membro: lua_obj3d.c le
 * arg_ptr(L, i, MT_CAM3D) como um cam3d_handle*. */
typedef struct {
    cam3d_handle*  cam;
    obj3d_handle** lista;
    int            n, cap;
} cam3d_ud;

/* ======================= zbuffer ======================= */

DEF_GC(zbuffer, zb_handle, zb_destruir)
DEF_DESTRUIR(z_destruir, zb_handle, MT_ZBUFFER, zb_destruir)
DEF_ARG(arg_zb, zb_handle, MT_ZBUFFER)

/* zbuffer.criar(largura, altura)  -- deve ter o mesmo tamanho do frame buffer */
static int z_criar(lua_State* L) {
    const int w = (int)luaL_checkinteger(L, 1);
    const int h = (int)luaL_checkinteger(L, 2);
    return ret_ptr(L, MT_ZBUFFER, zb_criar(w, h));
}

static int z_limpar(lua_State* L)  { zb_limpar(arg_zb(L, 1)); return 0; }
static int z_largura(lua_State* L) { lua_pushinteger(L, zb_largura(arg_zb(L, 1))); return 1; }
static int z_altura(lua_State* L)  { lua_pushinteger(L, zb_altura(arg_zb(L, 1))); return 1; }

static int z_ler(lua_State* L) {
    lua_pushnumber(L, zb_ler(arg_zb(L, 1), (int)luaL_checkinteger(L, 2), (int)luaL_checkinteger(L, 3)));
    return 1;
}

static const luaL_Reg metodos_zb[] = {
    {"destruir", z_destruir},
    {"__gc", gc_zbuffer},
    {"limpar", z_limpar},
    {"largura", z_largura},
    {"altura", z_altura},
    {"ler", z_ler},
    {NULL, NULL}
};

static const luaL_Reg funcoes_zb[] = {
    {"criar", z_criar},
    {NULL, NULL}
};

/* ======================= camera ======================= */

static int gc_cam3d(lua_State* L) {
    cam3d_ud* u = (cam3d_ud*)lua_touserdata(L, 1);
    if (!u) return 0;
    if (u->cam) cam3d_destruir(u->cam);
    if (u->lista) memoria_free(u->lista);
    u->cam = NULL;
    u->lista = NULL;
    u->n = u->cap = 0;
    return 0;
}

static cam3d_ud* arg_cam(lua_State* L, int i) {
    cam3d_ud* u = (cam3d_ud*)luaL_checkudata(L, i, MT_CAM3D);
    if (!u->cam) luaL_error(L, "objeto '%s' ja foi destruido", MT_CAM3D);
    return u;
}

cam3d_handle* lua_arg_cam3d(lua_State* L, int i) { return arg_cam(L, i)->cam; }

static int c_destruir(lua_State* L) {
    cam3d_ud* u = (cam3d_ud*)luaL_checkudata(L, 1, MT_CAM3D);
    gc_cam3d(L);
    (void)u;
    return 0;
}

/* ---- listas de objetos ---- */

static void garantir(lua_State* L, cam3d_ud* u, int n) {
    if (n <= u->cap) return;
    int nc = u->cap ? u->cap : 16;
    while (nc < n) nc *= 2;
    const size_t bytes = (size_t)nc * sizeof(obj3d_handle*);
    obj3d_handle** q = (obj3d_handle**)(u->lista ? memoria_realloc(u->lista, bytes) : memoria_malloc(bytes));
    if (!q) luaL_error(L, "sem memoria");
    u->lista = q;
    u->cap = nc;
}

/* copia os objetos da tabela t para dst; objetos destruidos viram NULL (a camera ignora NULL) */
static void preencher(lua_State* L, int t, obj3d_handle** dst, int n) {
    for (int i = 1; i <= n; i++) {
        lua_rawgeti(L, t, i);
        void** p = (void**)testar_ud(L, -1, MT_OBJ3D);
        if (!p) p = (void**)testar_ud(L, -1, MT_OBJ3DREF);
        if (!p) luaL_error(L, "item %d da lista nao e um obj3d", i);
        dst[i - 1] = (obj3d_handle*)*p;
        lua_pop(L, 1);
    }
}

/* le a tabela "mundo" guardada na camera e atualiza o array usado na colisao */
static void sincronizar(lua_State* L, cam3d_ud* u) {
    const int topo = lua_gettop(L);
    refs_pegar(L, 1);
    lua_getfield(L, -1, "mundo");
    if (lua_istable(L, -1)) {
        const int t = lua_gettop(L);
        const int n = tam_tabela(L, t);
        garantir(L, u, n);
        preencher(L, t, u->lista, n);
        u->n = n;
        cam3d_colisao_mundo(u->cam, u->lista, n);
    } else {
        u->n = 0;
        cam3d_colisao_mundo(u->cam, NULL, 0);
    }
    lua_settop(L, topo);
}

/* array temporario (userdata deixado na pilha) a partir do argumento `arg` ou, se ausente, do mundo */
static obj3d_handle** lista_temp(lua_State* L, int arg, int* n) {
    int t;
    *n = 0;
    if (!lua_isnoneornil(L, arg)) {
        luaL_checktype(L, arg, LUA_TTABLE);
        t = arg;
    } else {
        refs_pegar(L, 1);
        lua_getfield(L, -1, "mundo");
        if (!lua_istable(L, -1)) return NULL;
        t = lua_gettop(L);
    }
    *n = tam_tabela(L, t);
    obj3d_handle** b = (obj3d_handle**)lua_newuserdata(L, (size_t)(*n > 0 ? *n : 1) * sizeof(obj3d_handle*));
    preencher(L, t, b, *n);
    return b;
}

/* ---- criacao ---- */

/* cam3d.criar(x, y, z [, angulo_x, angulo_y, angulo_z]) */
static int l_criar(lua_State* L) {
    const float x = LN(1), y = LN(2), z = LN(3);
    const float ax = LNO(4, 0.0f), ay = LNO(5, 0.0f), az = LNO(6, 0.0f);
    cam3d_ud* u = (cam3d_ud*)novo_ud(L, MT_CAM3D, sizeof(cam3d_ud));
    u->cam = cam3d_criar(x, y, z, ax, ay, az);
    if (!u->cam) {
        lua_pushnil(L);
        return 1;
    }
    return 1;
}

/* ---- lente ---- */

static int c_lente(lua_State* L)    { cam3d_lente(arg_cam(L, 1)->cam, LN(2), LN(3), LN(4)); return 0; }
static int c_fov(lua_State* L)      { cam3d_fov(arg_cam(L, 1)->cam, LN(2)); return 0; }
static int c_aspecto(lua_State* L)  { cam3d_aspecto(arg_cam(L, 1)->cam, LN(2)); return 0; }

/* ---- chao / ceu ---- */

static int c_chao(lua_State* L)         { cam3d_chao(arg_cam(L, 1)->cam, arg_bool(L, 2, true)); return 0; }
static int c_definir_chao(lua_State* L) { cam3d_definir_chao(arg_cam(L, 1)->cam, LN(2)); return 0; }
static int c_ceu(lua_State* L)          { cam3d_ceu(arg_cam(L, 1)->cam, arg_bool(L, 2, true)); return 0; }
static int c_maximo_ceu(lua_State* L)   { cam3d_maximo_ceu(arg_cam(L, 1)->cam, LN(2)); return 0; }

/* ---- corpo / fisica ---- */

/* definir_corpo(altura_olhos, olhos_agachado, altura_colisao, raio [, tipo=cam3d.COL_AABB]) */
static int c_definir_corpo(lua_State* L) {
    cam3d_definir_corpo(arg_cam(L, 1)->cam, LN(2), LN(3), LN(4), LN(5), (int)luaL_optinteger(L, 6, CAM3D_COL_AABB));
    return 0;
}

static int c_fisica(lua_State* L)         { cam3d_fisica(arg_cam(L, 1)->cam, LN(2), LN(3)); return 0; }
static int c_degrau(lua_State* L)         { cam3d_degrau(arg_cam(L, 1)->cam, LN(2)); return 0; }
static int c_multiplicadores(lua_State* L){ cam3d_multiplicadores(arg_cam(L, 1)->cam, LN(2), LN(3)); return 0; }
static int c_recuperacao(lua_State* L)    { cam3d_recuperacao(arg_cam(L, 1)->cam, LN(2)); return 0; }

/* balanco(ligado [, amplitude=0.05, frequencia=10]) */
static int c_balanco(lua_State* L) {
    cam3d_balanco(arg_cam(L, 1)->cam, arg_bool(L, 2, true), LNO(3, 0.05f), LNO(4, 10.0f));
    return 0;
}

/* ---- movimento ---- */

static int c_mover(lua_State* L)      { cam3d_mover(arg_cam(L, 1)->cam, LN(2), LN(3), LN(4)); return 0; }
static int c_posicionar(lua_State* L) { cam3d_posicionar(arg_cam(L, 1)->cam, LN(2), LN(3), LN(4)); return 0; }
static int c_girar(lua_State* L)      { cam3d_girar(arg_cam(L, 1)->cam, LN(2), LN(3), LN(4)); return 0; }
static int c_angulo(lua_State* L)     { cam3d_angulo(arg_cam(L, 1)->cam, LN(2), LN(3), LN(4)); return 0; }
static int c_olhar_para(lua_State* L) { cam3d_olhar_para(arg_cam(L, 1)->cam, LN(2), LN(3), LN(4)); return 0; }
static int c_pular(lua_State* L)      { cam3d_pular(arg_cam(L, 1)->cam); return 0; }
static int c_agachar(lua_State* L)    { cam3d_agachar(arg_cam(L, 1)->cam, arg_bool(L, 2, true)); return 0; }
static int c_correr(lua_State* L)     { cam3d_correr(arg_cam(L, 1)->cam, arg_bool(L, 2, true)); return 0; }
static int c_tremer(lua_State* L)     { cam3d_tremer(arg_cam(L, 1)->cam, LN(2), LN(3)); return 0; }
static int c_recuo(lua_State* L)      { cam3d_recuo(arg_cam(L, 1)->cam, LN(2), LN(3)); return 0; }

/* ---- mundo / atualizacao ---- */

/* mundo(tabela_de_obj3d)  ou  mundo(nil)
 * A tabela e lida de novo a cada atualizar(), entao adicionar/remover objetos nela basta. */
static int c_mundo(lua_State* L) {
    cam3d_ud* u = arg_cam(L, 1);
    if (lua_isnoneornil(L, 2)) {
        ref_limpar(L, 1, "mundo");
    } else {
        luaL_checktype(L, 2, LUA_TTABLE);
        ref_definir(L, 1, "mundo", 2);
    }
    sincronizar(L, u);
    return 0;
}

static int c_atualizar(lua_State* L) {
    cam3d_ud* u = arg_cam(L, 1);
    const float dt = LN(2);
    sincronizar(L, u);
    cam3d_atualizar(u->cam, dt);
    return 0;
}

/* ---- leitura ---- */

static int c_x(lua_State* L)            { lua_pushnumber(L, cam3d_x(arg_cam(L, 1)->cam)); return 1; }
static int c_y(lua_State* L)            { lua_pushnumber(L, cam3d_y(arg_cam(L, 1)->cam)); return 1; }
static int c_z(lua_State* L)            { lua_pushnumber(L, cam3d_z(arg_cam(L, 1)->cam)); return 1; }
static int c_pes_y(lua_State* L)         { lua_pushnumber(L, cam3d_pes_y(arg_cam(L, 1)->cam)); return 1; }
static int c_altura_corpo(lua_State* L)  { lua_pushnumber(L, cam3d_altura_corpo(arg_cam(L, 1)->cam)); return 1; }
static int c_velocidade(lua_State* L)    { lua_pushnumber(L, cam3d_velocidade(arg_cam(L, 1)->cam)); return 1; }
static int c_no_chao(lua_State* L)       { lua_pushboolean(L, cam3d_no_chao(arg_cam(L, 1)->cam)); return 1; }
static int c_agachado(lua_State* L)      { lua_pushboolean(L, cam3d_agachado(arg_cam(L, 1)->cam)); return 1; }

static int c_posicao(lua_State* L) {
    const cam3d_handle* c = arg_cam(L, 1)->cam;
    lua_pushnumber(L, cam3d_x(c));
    lua_pushnumber(L, cam3d_y(c));
    lua_pushnumber(L, cam3d_z(c));
    return 3;
}

/* pitch, yaw, roll */
static int c_angulos(lua_State* L) {
    const cam3d_handle* c = arg_cam(L, 1)->cam;
    lua_pushnumber(L, cam3d_angulo_x(c));
    lua_pushnumber(L, cam3d_angulo_y(c));
    lua_pushnumber(L, cam3d_angulo_z(c));
    return 3;
}

static int c_frente(lua_State* L) {
    float x, y, z;
    cam3d_frente(arg_cam(L, 1)->cam, &x, &y, &z);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, z);
    return 3;
}

/* ex, ey, ez, yaw, pitch, roll  (com balanco, tremor e recuo aplicados) */
static int c_vista(lua_State* L) {
    float ex, ey, ez, yw, pt, rl;
    cam3d_vista(arg_cam(L, 1)->cam, &ex, &ey, &ez, &yw, &pt, &rl);
    lua_pushnumber(L, ex);
    lua_pushnumber(L, ey);
    lua_pushnumber(L, ez);
    lua_pushnumber(L, yw);
    lua_pushnumber(L, pt);
    lua_pushnumber(L, rl);
    return 6;
}

/* ---- iluminacao ---- */

#define LID(i) ((int)luaL_checkinteger(L, (i)))

static int c_ambiente(lua_State* L) { cam3d_ambiente(arg_cam(L, 1)->cam, LN(2)); return 0; }
static int c_sombras(lua_State* L)  { cam3d_sombras(arg_cam(L, 1)->cam, arg_bool(L, 2, true)); return 0; }

/* aura(ligada [, desloc_y=0]) */
static int c_aura(lua_State* L) {
    cam3d_aura(arg_cam(L, 1)->cam, arg_bool(L, 2, true), LNO(3, 0.0f));
    return 0;
}

/* lanterna(ligada [, desloc_y=0]) */
static int c_lanterna(lua_State* L) {
    cam3d_lanterna(arg_cam(L, 1)->cam, arg_bool(L, 2, true), LNO(3, 0.0f));
    return 0;
}

/* cone_lanterna(interno, externo)  -- meio-angulo em graus */
static int c_cone_lanterna(lua_State* L) {
    cam3d_cone_lanterna(arg_cam(L, 1)->cam, LN(2), LN(3));
    return 0;
}

/* luz_criar(x, y, z [, cor=branco, intensidade=1, alcance=10])  ->  id | nil */
static int c_luz_criar(lua_State* L) {
    const uint32_t cor = lua_isnoneornil(L, 5) ? 0xFFFFFFFFu : arg_cor(L, 5);
    const int id = cam3d_luz_criar(arg_cam(L, 1)->cam, LN(2), LN(3), LN(4), cor, LNO(6, 1.0f), LNO(7, 10.0f));
    if (id < 0) lua_pushnil(L);
    else lua_pushinteger(L, id);
    return 1;
}

static int c_luz_remover(lua_State* L)     { cam3d_luz_remover(arg_cam(L, 1)->cam, LID(2)); return 0; }
static int c_luz_mover(lua_State* L)       { cam3d_luz_mover(arg_cam(L, 1)->cam, LID(2), LN(3), LN(4), LN(5)); return 0; }
static int c_luz_ativa(lua_State* L)       { cam3d_luz_ativa(arg_cam(L, 1)->cam, LID(2), arg_bool(L, 3, true)); return 0; }
static int c_luz_cor(lua_State* L)         { cam3d_luz_cor(arg_cam(L, 1)->cam, LID(2), arg_cor(L, 3)); return 0; }
static int c_luz_intensidade(lua_State* L) { cam3d_luz_intensidade(arg_cam(L, 1)->cam, LID(2), LN(3)); return 0; }
static int c_luz_alcance(lua_State* L)     { cam3d_luz_alcance(arg_cam(L, 1)->cam, LID(2), LN(3)); return 0; }
static int c_luz_sombra(lua_State* L)      { cam3d_luz_sombra(arg_cam(L, 1)->cam, LID(2), arg_bool(L, 3, true)); return 0; }
static int c_luz_ponto(lua_State* L)       { cam3d_luz_ponto(arg_cam(L, 1)->cam, LID(2)); return 0; }

static int c_luz_spot(lua_State* L) {
    cam3d_luz_spot(arg_cam(L, 1)->cam, LID(2), LN(3), LN(4), LN(5), LNO(6, 20.0f), LNO(7, 35.0f));
    return 0;
}

static int c_iluminar(lua_State* L) {
    cam3d_ud* u = arg_cam(L, 1);
    fb_handle* fb = arg_fb(L, 2);
    zb_handle* zb = arg_zb(L, 3);
    if (fb->w != zb->w || fb->h != zb->h)
        return luaL_error(L, "zbuffer (%dx%d) com tamanho diferente do frame buffer (%dx%d)", zb->w, zb->h, fb->w, fb->h);
    int n;
    obj3d_handle** b = lista_temp(L, 4, &n);
    cam3d_iluminar(u->cam, fb, zb, b, n);
    return 0;
}

static int c_neblina(lua_State* L) {
    const uint32_t cor = lua_isnoneornil(L, 6) ? 0xFF000000u : arg_cor(L, 6);
    cam3d_neblina(arg_cam(L, 1)->cam, arg_bool(L, 2, true), LNO(3, 0.0f), LNO(4, 0.0f), arg_bool(L, 5, true), cor, LNO(7, 0.6f));
    return 0;
}

/* ---- raio e desenho ---- */

static int c_raio(lua_State* L) {
    cam3d_ud* u = arg_cam(L, 1);
    const float alcance = LN(2);
    int n;
    float dist = 0.0f;
    obj3d_handle** b = lista_temp(L, 3, &n);
    const int i = b ? cam3d_raio(u->cam, b, n, alcance, &dist) : -1;
    if (i < 0) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, i + 1);
    lua_pushnumber(L, dist);
    return 2;
}

static int c_desenhar(lua_State* L) {
    cam3d_ud* u = arg_cam(L, 1);
    fb_handle* fb = arg_fb(L, 2);
    zb_handle* zb = arg_zb(L, 3);
    if (fb->w != zb->w || fb->h != zb->h)
        return luaL_error(L, "zbuffer (%dx%d) com tamanho diferente do frame buffer (%dx%d)", zb->w, zb->h, fb->w, fb->h);
    int n;
    obj3d_handle** b = lista_temp(L, 4, &n);
    cam3d_desenhar(u->cam, fb, zb, b, n);
    return 0;
}

static const luaL_Reg metodos_cam3d[] = {
    {"destruir", c_destruir},
    {"__gc", gc_cam3d},

    {"lente", c_lente},
    {"fov", c_fov},
    {"aspecto", c_aspecto},

    {"chao", c_chao},
    {"definir_chao", c_definir_chao},
    {"ceu", c_ceu},
    {"maximo_ceu", c_maximo_ceu},

    {"definir_corpo", c_definir_corpo},
    {"fisica", c_fisica},
    {"degrau", c_degrau},
    {"multiplicadores", c_multiplicadores},
    {"balanco", c_balanco},
    {"recuperacao", c_recuperacao},

    {"mover", c_mover},
    {"posicionar", c_posicionar},
    {"girar", c_girar},
    {"angulo", c_angulo},
    {"olhar_para", c_olhar_para},
    {"pular", c_pular},
    {"agachar", c_agachar},
    {"correr", c_correr},
    {"tremer", c_tremer},
    {"recuo", c_recuo},

    {"mundo", c_mundo},
    {"atualizar", c_atualizar},

    {"x", c_x},
    {"y", c_y},
    {"z", c_z},
    {"pes_y", c_pes_y},
    {"posicao", c_posicao},
    {"angulos", c_angulos},
    {"altura_corpo", c_altura_corpo},
    {"velocidade", c_velocidade},
    {"no_chao", c_no_chao},
    {"agachado", c_agachado},
    {"frente", c_frente},
    {"vista", c_vista},

    {"ambiente", c_ambiente},
    {"sombras", c_sombras},
    {"aura", c_aura},
    {"lanterna", c_lanterna},
    {"cone_lanterna", c_cone_lanterna},
    {"luz_criar", c_luz_criar},
    {"luz_remover", c_luz_remover},
    {"luz_mover", c_luz_mover},
    {"luz_ativa", c_luz_ativa},
    {"luz_cor", c_luz_cor},
    {"luz_intensidade", c_luz_intensidade},
    {"luz_alcance", c_luz_alcance},
    {"luz_sombra", c_luz_sombra},
    {"luz_ponto", c_luz_ponto},
    {"luz_spot", c_luz_spot},
    {"iluminar", c_iluminar},

    {"neblina", c_neblina},

    {"raio", c_raio},
    {"desenhar", c_desenhar},
    {NULL, NULL}
};

static const luaL_Reg funcoes_cam3d[] = {
    {"criar", l_criar},
    {NULL, NULL}
};

void bindings_cam3d(lua_State* L) {
    registrar_classe(L, MT_ZBUFFER, metodos_zb, NULL);
    registrar_global(L, "zbuffer", funcoes_zb);

    registrar_classe(L, MT_CAM3D, metodos_cam3d, NULL);
    registrar_global(L, "cam3d", funcoes_cam3d);

    lua_getglobal(L, "cam3d");
    campo(L, "COL_AABB", CAM3D_COL_AABB);
    campo(L, "COL_OBB", CAM3D_COL_OBB);
    campo(L, "LUZ_AURA", CAM3D_LUZ_AURA);
    campo(L, "LUZ_LANTERNA", CAM3D_LUZ_LANTERNA);
    campo(L, "LUZ_PONTO", CAM3D_LUZ_PONTO);
    campo(L, "LUZ_SPOT", CAM3D_LUZ_SPOT);
    lua_pop(L, 1);
}
