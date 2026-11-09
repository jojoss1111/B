#include "mapas.h"
#include <math.h>
#include <string.h>

#define MAPA_CAMADAS 3
#define MAPA_MAX_SPRITES 128

typedef struct {
  fb_atlas_handle *atlas;
  uint16_t col, lin, w, h;
  uint32_t cor;
  float ang;
  uint8_t alpha, fx, fy, desenha, solido;
  const char *mascara;
  uint16_t mw, mh;
  uint8_t colorido;
  uint16_t quadros[255][2];
  uint16_t n_quadros, quadro, ms_quadro, acumulado;
  uint8_t repetir, animando;
} tile_def;

typedef struct {
  char *grid;
  uint32_t cols, linhas;
  int32_t ox, oy;
  Vetor defs;
  int16_t indice[256];
} camada_handle;

struct mapa_handle {
  uint16_t tile_w, tile_h;
  camada_handle camada[MAPA_CAMADAS];
};

static camada_handle *camada_get(mapa_handle *m, int n) {
  return m && n >= 1 && n <= MAPA_CAMADAS ? &m->camada[n - 1] : NULL;
}
static const camada_handle *camada_get_const(const mapa_handle *m, int n) {
  return m && n >= 1 && n <= MAPA_CAMADAS ? &m->camada[n - 1] : NULL;
}

static void grade_liberar(camada_handle *c) {
  if (!c)
    return;
  memoria_free(c->grid);
  c->grid = NULL;
  c->cols = c->linhas = 0;
}

static int grade_criar(camada_handle *c, uint32_t cols, uint32_t linhas) {
  if (!c || !cols || !linhas || (size_t)cols > SIZE_MAX / linhas)
    return 0;
  grade_liberar(c);
  size_t n = (size_t)cols * linhas;
  c->grid = (char *)memoria_malloc(n);
  if (!c->grid)
    return 0;
  memset(c->grid, '.', n);
  c->cols = cols;
  c->linhas = linhas;
  return 1;
}

static void texto_medidas(const char *s, uint32_t *cols, uint32_t *linhas) {
  uint32_t x = 0, mx = 0, y = 0;
  if (s)
    for (const char *p = s; *p; ++p) {
      if (*p == '\r')
        continue;
      if (*p == '\n') {
        if (x > mx)
          mx = x;
        x = 0;
        ++y;
      } else
        ++x;
    }
  if (x) {
    if (x > mx)
      mx = x;
    ++y;
  }
  if (mx > *cols)
    *cols = mx;
  if (y > *linhas)
    *linhas = y;
}

mapa_handle *mapa_novo(void) {
  mapa_handle *m = (mapa_handle *)memoria_malloc(sizeof(*m));
  if (!m)
    return NULL;
  memset(m, 0, sizeof(*m));
  for (int k = 0; k < MAPA_CAMADAS; ++k) {
    camada_handle *c = &m->camada[k];
    for (int i = 0; i < 256; ++i)
      c->indice[i] = -1;
    c->defs = vetor_criar(sizeof(tile_def), 8);
    vetor_definir_limite(&c->defs, MAPA_MAX_SPRITES);
  }
  return m;
}

void mapa_destruir(mapa_handle *m) {
  if (!m)
    return;
  for (int i = 0; i < MAPA_CAMADAS; ++i) {
    grade_liberar(&m->camada[i]);
    vetor_liberar(&m->camada[i].defs);
  }
  memoria_free(m);
}

int mapa_criar_mapa(mapa_handle *m, uint16_t tile_w, uint16_t tile_h,
                    const char *const textos[3]) {
  if (!m || !textos || !tile_w || !tile_h)
    return 0;
  uint32_t cols = 0, rows = 0;
  for (int i = 0; i < 3; ++i)
    texto_medidas(textos[i], &cols, &rows);
  if (!cols || !rows)
    return 0;
  for (int i = 0; i < 3; ++i) {
    camada_handle *c = &m->camada[i];
    if (!grade_criar(c, cols, rows))
      return 0;
    uint32_t x = 0, y = 0;
    const char *s = textos[i] ? textos[i] : "";
    for (const char *p = s; *p && y < rows; ++p) {
      if (*p == '\r')
        continue;
      if (*p == '\n') {
        x = 0;
        ++y;
        continue;
      }
      if (x < cols)
        c->grid[(size_t)y * cols + x] = *p;
      ++x;
    }
  }
  m->tile_w = tile_w;
  m->tile_h = tile_h;
  return 1;
}

void mapa_posicionar(mapa_handle *m, int layer, int32_t x, int32_t y) {
  camada_handle *c = camada_get(m, layer);
  if (c) {
    c->ox = x;
    c->oy = y;
  }
}
void mapa_mover(mapa_handle *m, int layer, int32_t x, int32_t y) {
  camada_handle *c = camada_get(m, layer);
  if (c) {
    c->ox += x;
    c->oy += y;
  }
}
int32_t mapa_x(const mapa_handle *m, int layer) {
  const camada_handle *c = camada_get_const(m, layer);
  return c ? c->ox : 0;
}
int32_t mapa_y(const mapa_handle *m, int layer) {
  const camada_handle *c = camada_get_const(m, layer);
  return c ? c->oy : 0;
}
uint32_t mapa_colunas(const mapa_handle *m) {
  return m && m->camada[1].grid ? m->camada[1].cols : 0;
}
uint32_t mapa_linhas(const mapa_handle *m) {
  return m && m->camada[1].grid ? m->camada[1].linhas : 0;
}
uint16_t mapa_tile_largura(const mapa_handle *m) { return m ? m->tile_w : 0; }
uint16_t mapa_tile_altura(const mapa_handle *m) { return m ? m->tile_h : 0; }

int mapa_tile(const mapa_handle *m, int layer, uint32_t x, uint32_t y) {
  const camada_handle *c = camada_get_const(m, layer);
  if (!c || !c->grid || x >= c->cols || y >= c->linhas)
    return 0;
  return (uint8_t)c->grid[(size_t)y * c->cols + x];
}
void mapa_definir(mapa_handle *m, int layer, uint32_t x, uint32_t y, int tile) {
  camada_handle *c = camada_get(m, layer);
  if (c && c->grid && x < c->cols && y < c->linhas)
    c->grid[(size_t)y * c->cols + x] = (char)tile;
}
void mapa_substituir(mapa_handle *m, int layer, int antigo, int novo, int32_t x,
                     int32_t y, int tudo) {
  camada_handle *c = camada_get(m, layer);
  if (!c || !c->grid)
    return;
  if (tudo) {
    for (size_t i = 0, n = (size_t)c->cols * c->linhas; i < n; i++)
      if ((uint8_t)c->grid[i] == (uint8_t)antigo)
        c->grid[i] = (char)novo;
  } else if (x >= 0 && y >= 0 &&
             mapa_tile(m, layer, (uint32_t)x, (uint32_t)y) == (uint8_t)antigo)
    mapa_definir(m, layer, (uint32_t)x, (uint32_t)y, novo);
}

static tile_def *tile_obter(camada_handle *c, int tile) {
  if (!c)
    return NULL;
  uint8_t k = (uint8_t)tile;
  if (c->indice[k] >= 0)
    return (tile_def *)vetor_get(&c->defs, c->indice[k]);
  int32_t i = vetor_tam(&c->defs);
  tile_def d;
  memset(&d, 0, sizeof(d));
  d.cor = 0xFFFFFFFFu;
  d.alpha = 255;
  vetor_add(&c->defs, &d);
  if (vetor_tam(&c->defs) == i)
    return NULL;
  c->indice[k] = (int16_t)i;
  return (tile_def *)vetor_get(&c->defs, i);
}
static const tile_def *tile_buscar(const camada_handle *c, int tile) {
  int16_t i = c->indice[(uint8_t)tile];
  return i < 0 ? NULL : (const tile_def *)vetor_get((Vetor *)&c->defs, i);
}

void mapa_sprites(mapa_handle *m, int layer, int tile, fb_atlas_handle *a,
                  uint16_t col, uint16_t lin) {
  tile_def *d = tile_obter(camada_get(m, layer), tile);
  if (d) {
    d->atlas = a;
    d->col = col;
    d->lin = lin;
    d->desenha = 1;
  }
}
void mapa_ampliar(mapa_handle *m, int l, int t, uint16_t w, uint16_t h) {
  tile_def *d = tile_obter(camada_get(m, l), t);
  if (d) {
    d->w = w;
    d->h = h;
  }
}
void mapa_cor(mapa_handle *m, int l, int t, uint32_t c) {
  tile_def *d = tile_obter(camada_get(m, l), t);
  if (d) {
    d->cor = c;
    d->colorido = 1;
  }
}
void mapa_angulo(mapa_handle *m, int l, int t, float a) {
  tile_def *d = tile_obter(camada_get(m, l), t);
  if (d)
    d->ang = a;
}
void mapa_alpha(mapa_handle *m, int l, int t, uint8_t a) {
  tile_def *d = tile_obter(camada_get(m, l), t);
  if (d)
    d->alpha = a;
}
void mapa_espelhar_x(mapa_handle *m, int l, int t, int v) {
  tile_def *d = tile_obter(camada_get(m, l), t);
  if (d)
    d->fx = v != 0;
}
void mapa_espelhar_y(mapa_handle *m, int l, int t, int v) {
  tile_def *d = tile_obter(camada_get(m, l), t);
  if (d)
    d->fy = v != 0;
}
void mapa_colisor(mapa_handle *m, int l, int t, const char *s, uint16_t w,
                  uint16_t h) {
  tile_def *d = tile_obter(camada_get(m, l), t);
  if (d) {
    d->solido = 1;
    d->mascara = s;
    d->mw = w;
    d->mh = h;
  }
}

static void animacao_tile(camada_handle *c, int tile, fb_atlas_handle *a,
                          const uint16_t (*q)[2], uint16_t n, uint16_t fps,
                          int loop) {
  if (!c || !q || !n || !fps)
    return;
  if (n > 255)
    n = 255;
  tile_def *d = tile_obter(c, tile);
  if (!d)
    return;
  d->atlas = a;
  d->n_quadros = n;
  d->quadro = 0;
  d->ms_quadro = (uint16_t)(1000u / fps);
  if (!d->ms_quadro)
    d->ms_quadro = 1;
  d->acumulado = 0;
  d->repetir = loop != 0;
  d->animando = 1;
  d->desenha = 1;
  memcpy(d->quadros, q, (size_t)n * sizeof(d->quadros[0]));
  d->col = d->quadros[0][0];
  d->lin = d->quadros[0][1];
}
void mapa_animacao(mapa_handle *m, int tile, fb_atlas_handle *a,
                   const uint16_t (*q)[2], uint16_t n, uint16_t fps, int loop) {
  if (!m)
    return;
  for (int i = 1; i <= 3; i++)
    animacao_tile(&m->camada[i - 1], tile, a, q, n, fps, loop);
}
void mapa_atualizar_animacao(mapa_handle *m, uint32_t dt) {
  if (!m || !dt)
    return;
  for (int k = 0; k < 3; k++) {
    camada_handle *c = &m->camada[k];
    for (int i = 0, n = vetor_tam(&c->defs); i < n; i++) {
      tile_def *d = (tile_def *)vetor_get(&c->defs, i);
      if (!d || !d->animando || d->n_quadros < 2)
        continue;
      uint64_t acc = (uint64_t)d->acumulado + dt, steps = acc / d->ms_quadro;
      acc %= d->ms_quadro;
      if (steps) {
        if (d->repetir)
          d->quadro = (uint16_t)((d->quadro + steps) % d->n_quadros);
        else {
          uint16_t left = (uint16_t)(d->n_quadros - 1 - d->quadro);
          if (steps >= left) {
            d->quadro = d->n_quadros - 1;
            d->animando = 0;
          } else
            d->quadro = (uint16_t)(d->quadro + steps);
        }
        d->col = d->quadros[d->quadro][0];
        d->lin = d->quadros[d->quadro][1];
      }
      d->acumulado = (uint16_t)acc;
    }
  }
}
static void animacao_controle(mapa_handle *m, int op) {
  if (!m)
    return;
  for (int k = 0; k < 3; k++)
    for (int i = 0, n = vetor_tam(&m->camada[k].defs); i < n; i++) {
      tile_def *d = (tile_def *)vetor_get(&m->camada[k].defs, i);
      if (!d || !d->n_quadros)
        continue;
      if (op == 0)
        d->animando = 0;
      else if (op == 1)
        d->animando = 1;
      else {
        d->quadro = 0;
        d->acumulado = 0;
        d->animando = 1;
        d->col = d->quadros[0][0];
        d->lin = d->quadros[0][1];
      }
    }
}
void mapa_parar(mapa_handle *m) { animacao_controle(m, 0); }
void mapa_rodar(mapa_handle *m) { animacao_controle(m, 1); }
void mapa_voltar(mapa_handle *m) { animacao_controle(m, 2); }

static int divpiso(int a, int b) {
  return a >= 0 ? a / b : -((-a + b - 1) / b);
}
void mapa_desenhar(const mapa_handle *m, int layer, fb_handle *fb) {
  const camada_handle *c = camada_get_const(m, layer);
  if (!c || !c->grid || !fb_valido(fb) || !m->tile_w || !m->tile_h)
    return;
  int x0 = divpiso(-c->ox, m->tile_w),
      x1 = divpiso(fb->w - 1 - c->ox, m->tile_w),
      y0 = divpiso(-c->oy, m->tile_h),
      y1 = divpiso(fb->h - 1 - c->oy, m->tile_h);
  if (x0 < 0)
    x0 = 0;
  if (y0 < 0)
    y0 = 0;
  if (x1 > (int)c->cols - 1)
    x1 = (int)c->cols - 1;
  if (y1 > (int)c->linhas - 1)
    y1 = (int)c->linhas - 1;
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++) {
      const tile_def *d =
          tile_buscar(c, (uint8_t)c->grid[(size_t)y * c->cols + x]);
      if (!d || !d->desenha || !d->atlas)
        continue;
      uint16_t w = d->w ? d->w : (uint16_t)fb_atlas_tile_largura(d->atlas),
               h = d->h ? d->h : (uint16_t)fb_atlas_tile_altura(d->atlas);
      if (!w)
        w = m->tile_w;
      if (!h)
        h = m->tile_h;
      obj_handle o;
      obj_iniciar(&o, d->atlas, w, h, c->ox + x * m->tile_w,
                  c->oy + y * m->tile_h, d->col, d->lin);
      obj_colorir(&o, d->colorido);
      obj_cor(&o, d->cor);
      obj_alpha(&o, d->alpha);
      obj_angulo(&o, d->ang);
      obj_espelhar_x(&o, d->fx);
      obj_espelhar_y(&o, d->fy);
      obj_desenhar(&o, fb);
    }
}

static int colidir(const mapa_handle *m, int layer, const aabb_2d_handle *o,
                   int32_t dx, int32_t dy, int eixo, int filtro) {
  const camada_handle *c = camada_get_const(m, layer);
  if (!c || !c->grid || !o || !m->tile_w || !m->tile_h)
    return 0;
  int32_t x0 = o->x + (dx < 0 ? dx : 0), y0 = o->y + (dy < 0 ? dy : 0),
          x1 = o->x + o->w + (dx > 0 ? dx : 0),
          y1 = o->y + o->h + (dy > 0 ? dy : 0);
  int cx0 = divpiso(x0 - c->ox, m->tile_w),
      cx1 = divpiso(x1 - 1 - c->ox, m->tile_w),
      cy0 = divpiso(y0 - c->oy, m->tile_h),
      cy1 = divpiso(y1 - 1 - c->oy, m->tile_h);
  if (cx0 < 0)
    cx0 = 0;
  if (cy0 < 0)
    cy0 = 0;
  if (cx1 > (int)c->cols - 1)
    cx1 = (int)c->cols - 1;
  if (cy1 > (int)c->linhas - 1)
    cy1 = (int)c->linhas - 1;
  for (int y = cy0; y <= cy1; y++) {
    for (int x = cx0; x <= cx1; x++) {
      int ch = (uint8_t)c->grid[(size_t)y * c->cols + x];
      if (filtro >= 0 && ch != (uint8_t)filtro)
        continue;
      const tile_def *d = tile_buscar(c, ch);
      if (!d || !d->solido)
        continue;
      aabb_2d_handle b;
      aabb_2d_iniciar(&b, NULL, d->mascara, d->mw ? d->mw : m->tile_w,
                      d->mh ? d->mh : m->tile_h);
      aabb_2d_posicionar(&b, c->ox + x * m->tile_w, c->oy + y * m->tile_h);
      if (eixo ? aabb_2d_colidiu_x(o, &b, dx) : aabb_2d_colidiu_y(o, &b, dy))
        return 1;
    }
  }
  return 0;
}
int mapa_colidio_x(const mapa_handle *m, int l, const aabb_2d_handle *o,
                   int32_t dx) {
  return colidir(m, l, o, dx, 0, 1, -1);
}
int mapa_colidio_y(const mapa_handle *m, int l, const aabb_2d_handle *o,
                   int32_t dy) {
  return colidir(m, l, o, 0, dy, 0, -1);
}
int mapa_colidio_tile_x(const mapa_handle *m, int l, int t,
                        const aabb_2d_handle *o, int32_t dx) {
  return colidir(m, l, o, dx, 0, 1, (uint8_t)t);
}
int mapa_colidio_tile_y(const mapa_handle *m, int l, int t,
                        const aabb_2d_handle *o, int32_t dy) {
  return colidir(m, l, o, 0, dy, 0, (uint8_t)t);
}
