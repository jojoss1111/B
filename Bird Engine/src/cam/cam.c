#include "cam.h"

#define LIMITE_LADO 4096
#define FOLGA       4
#define SOBRA       32
#define HISTERESE   96
#define TRIG_UM     32768

static inline uint32_t div255(uint32_t v) {
    v += 128;
    return (v + (v >> 8)) >> 8;
}

static inline uint32_t mesclar(uint32_t dst, uint32_t src) {
    const uint32_t a = src >> 24;
    if (a == 255) return src;
    if (a == 0)   return dst;
    const uint32_t ia = 255 - a;
    const uint32_t r = div255(((src >> 16) & 255) * a + ((dst >> 16) & 255) * ia);
    const uint32_t g = div255(((src >>  8) & 255) * a + ((dst >>  8) & 255) * ia);
    const uint32_t b = div255(( src        & 255) * a + ( dst        & 255) * ia);
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static int32_t arredondar(float v) {
    return v >= 0.0f ? (int32_t)(v + 0.5f) : -(int32_t)(-v + 0.5f);
}

static int64_t arredondar64(float v) {
    return v >= 0.0f ? (int64_t)(v + 0.5f) : -(int64_t)(-v + 0.5f);
}

static int32_t piso(float v) {
    const int32_t i = (int32_t)v;
    return (float)i > v ? i - 1 : i;
}

static float modulo(float v) { return v < 0.0f ? -v : v; }

static float normalizar_angulo(float a) {
    if (a >= 0.0f && a < 360.0f) return a;
    a -= (float)piso(a / 360.0f) * 360.0f;
    if (a < 0.0f || a >= 360.0f) a = 0.0f;
    return a;
}

static float aproximar(float atual, float alvo, uint8_t suavidade, float delta) {
    if (suavidade == 0) return alvo;

    const float k = ((256 - suavidade) / 256.0f) * (delta * 60.0f);
    if (k >= 1.0f) return alvo;
    if (k <= 0.0f) return atual;

    const float novo = atual + (alvo - atual) * k;
    const float resto = alvo - novo;
    return (resto < 0.01f && resto > -0.01f) ? alvo : novo;
}

static float prender_centro(float c, int32_t ini, int32_t fim, float meio) {
    const float lo = (float)ini + meio, hi = (float)fim - meio;
    if (lo >= hi) return ((float)ini + (float)fim) * 0.5f;
    return c < lo ? lo : (c > hi ? hi : c);
}

static int cam_garantir_buffer(camera_handle* c);
static void cam_preparar_quadro(camera_handle* c);

typedef struct { int64_t cx, cy; int32_t ca, sa; } transf;

static transf montar(float cx, float cy, float z, uint16_t ang) {
    transf t;
    t.cx = arredondar64(cx * 65536.0f);
    t.cy = arredondar64(cy * 65536.0f);
    t.ca = (int32_t)arredondar64((float)fb_cosseno(ang) / z);
    t.sa = (int32_t)arredondar64((float)fb_seno(ang) / z);
    return t;
}

static inline void projetar(const transf* t, int32_t u2, int32_t v2, int64_t* x, int64_t* y) {
    *x = t->cx + (int64_t)u2 * t->ca + (int64_t)v2 * t->sa;
    *y = t->cy - (int64_t)u2 * t->sa + (int64_t)v2 * t->ca;
}

static inline int64_t div_piso(int64_t n, int64_t d) {
    int64_t q = n / d;
    if ((n % d) != 0 && n < 0) q--;
    return q;
}

static inline int64_t div_teto(int64_t n, int64_t d) {
    int64_t q = n / d;
    if ((n % d) != 0 && n > 0) q++;
    return q;
}

static inline void intervalo_linear(int64_t f0, int64_t d, int64_t lo, int64_t hi, int64_t* k0, int64_t* k1) {
    if (d == 0) {
        if (f0 < lo || f0 >= hi) *k1 = *k0;
        return;
    }

    int64_t a, b;
    if (d > 0) {
        a = div_teto(lo - f0, d);
        b = div_teto(hi - f0, d);
    } else {
        const int64_t dn = -d;
        a = div_piso(f0 - hi, dn) + 1;
        b = div_piso(f0 - lo, dn) + 1;
    }
    if (a > *k0) *k0 = a;
    if (b < *k1) *k1 = b;
}

static uint16_t cam_ang16(const camera_handle* c) { return fb_graus(c->ang); }

static void cam_extensao(const camera_handle* c, float* ex, float* ey) {
    const float co = modulo((float)fb_cosseno(cam_ang16(c))) / (float)TRIG_UM;
    const float s  = modulo((float)fb_seno(cam_ang16(c)))    / (float)TRIG_UM;
    *ex = (co * (float)c->w + s * (float)c->h) / c->zoom;
    *ey = (s * (float)c->w + co * (float)c->h) / c->zoom;
}

static int cam_reto(const camera_handle* c) {
    return cam_ang16(c) == 0 && arredondar64(32768.0f / c->zoom) == (int64_t)TRIG_UM;
}

static void cam_aplicar(camera_handle* c) {
    if (c->zoom < c->zoom_min) c->zoom = c->zoom_min;
    if (c->zoom > c->zoom_max) c->zoom = c->zoom_max;

    if (c->travada) {
        float ex, ey;
        cam_extensao(c, &ex, &ey);
        c->cx = prender_centro(c->cx, c->lx0, c->lx1, ex * 0.5f);
        c->cy = prender_centro(c->cy, c->ly0, c->ly1, ey * 0.5f);
    }

    /* Toda mudança de câmera fica pronta para desenhar imediatamente.
       camera_iniciar ainda limpa o viewport e prepara cada quadro. */
    if (c->w && cam_garantir_buffer(c)) cam_preparar_quadro(c);
}

static void cam_tamanho_necessario(const camera_handle* c, int32_t* nw, int32_t* nh) {
    if (cam_reto(c)) { *nw = c->w; *nh = c->h; return; }

    float ex, ey;
    cam_extensao(c, &ex, &ey);
    *nw = piso(ex) + 1 + FOLGA;
    *nh = piso(ey) + 1 + FOLGA;
    if (*nw > LIMITE_LADO) *nw = LIMITE_LADO;
    if (*nh > LIMITE_LADO) *nh = LIMITE_LADO;
}

static int cam_garantir_buffer(camera_handle* c) {
    int32_t nw, nh;
    cam_tamanho_necessario(c, &nw, &nh);
    const int exato = cam_reto(c);

    if (fb_valido(c->view)) {
        const int32_t aw = c->view->w, ah = c->view->h;
        const int32_t lim = exato ? 1 : HISTERESE;
        const int falta = nw > aw || nh > ah;
        const int sobra = aw - nw >= lim || ah - nh >= lim;
        if (!falta && !sobra) return 1;
    }

    int32_t rw = nw, rh = nh;
    if (!exato) {
        rw = (nw + SOBRA + 15) & ~15;
        rh = (nh + SOBRA + 15) & ~15;
        if (rw > LIMITE_LADO) rw = LIMITE_LADO;
        if (rh > LIMITE_LADO) rh = LIMITE_LADO;
    }

    fb_handle* novo = fb_criar(rw, rh);
    if (!novo) return fb_valido(c->view);
    fb_destruir(c->view);
    c->view = novo;
    return 1;
}

static void cam_montar_lut(camera_handle* c) {
    c->usa_lut = 0;
    if (c->reto || c->sa != 0 || c->ca <= 0) return;

    const int32_t bw = c->view->w;

    if (!c->colunas || c->colunas->w != (int)c->w) {
        fb_destruir(c->colunas);
        c->colunas = fb_criar(c->w, 1);
        c->lut_w = 0;
        if (!c->colunas) return;
    }

    if (c->lut_w == (int32_t)c->w && c->lut_bw == bw && c->lut_cx == c->bcx && c->lut_ca == c->ca) {
        c->usa_lut = 1;
        return;
    }

    uint32_t* l = c->colunas->px;
    int32_t lo = 0, hi = (int32_t)c->w;

    for (int32_t x = 0; x < (int32_t)c->w; x++) {
        const int32_t v = (int32_t)((c->bcx + (int64_t)(2 * x + 1 - (int32_t)c->w) * c->ca) >> 16);
        if (v < 0)   { lo = x + 1; continue; }
        if (v >= bw) { hi = x; break; }
        l[x] = (uint32_t)v;
    }

    c->cmin = lo;
    c->cmax = hi > lo ? hi : lo;

    c->lut_w = (int32_t)c->w;
    c->lut_bw = bw;
    c->lut_cx = c->bcx;
    c->lut_ca = c->ca;
    c->usa_lut = 1;
}

static void cam_preparar_quadro(camera_handle* c) {
    if (!fb_valido(c->view)) return;

    const int32_t bw = c->view->w, bh = c->view->h;
    const transf t = montar(c->cx, c->cy, c->zoom, cam_ang16(c));

    c->reto = (uint8_t)cam_reto(c);
    c->ca = t.ca;
    c->sa = t.sa;

    if (c->reto) {
        c->ox = (int32_t)((t.cx - (int64_t)bw * 32768 + 32768) >> 16);
        c->oy = (int32_t)((t.cy - (int64_t)bh * 32768 + 32768) >> 16);
        c->bcx = (int64_t)bw * 32768;
        c->bcy = (int64_t)bh * 32768;
    } else {
        c->ox = (int32_t)((t.cx - (int64_t)bw * 32768) >> 16);
        c->oy = (int32_t)((t.cy - (int64_t)bh * 32768) >> 16);
        c->bcx = t.cx - (int64_t)c->ox * 65536;
        c->bcy = t.cy - (int64_t)c->oy * 65536;
    }

    cam_montar_lut(c);
}

static void cam_zerar(camera_handle* c) {
    fb_destruir(c->view);
    fb_destruir(c->colunas);
    c->view = NULL;
    c->colunas = NULL;
    c->cx = c->cy = 0.0f;
    c->w = c->h = 0;
    c->ang = 0.0f;
    c->opaca = 1;

    c->zoom = c->zoom_alvo = 1.0f;
    c->zoom_min = 0.25f;
    c->zoom_max = 8.0f;
    c->zoom_suave = 0;
    c->zoom_alvo_ativo = 0;

    c->ox = c->oy = 0;
    c->bcx = c->bcy = 0;
    c->ca = 32768;
    c->sa = 0;
    c->reto = 1;

    c->cmin = c->cmax = 0;
    c->usa_lut = 0;
    c->lut_cx = 0;
    c->lut_ca = c->lut_w = c->lut_bw = 0;

    c->alvo = NULL;
    c->alvo_dx = c->alvo_dy = 0;
    c->alvo_suave = 0;
    c->travada = 0;
    c->lx0 = c->ly0 = c->lx1 = c->ly1 = 0;
}

camera_handle* camera_criar(uint16_t largura, uint16_t altura, int32_t x, int32_t y) {
    if (largura == 0 || altura == 0) return NULL;

    camera_handle* c = (camera_handle*)memoria_malloc(sizeof(camera_handle));
    if (!c) return NULL;
    c->view = NULL;
    c->colunas = NULL;
    cam_zerar(c);

    c->w = largura;
    c->h = altura;
    c->cx = (float)x + (float)largura * 0.5f;
    c->cy = (float)y + (float)altura * 0.5f;

    if (!cam_garantir_buffer(c)) { camera_destruir(c); return NULL; }
    cam_aplicar(c);
    cam_preparar_quadro(c);
    return c;
}

void camera_destruir(camera_handle* c) {
    if (!c) return;
    cam_zerar(c);
    memoria_free(c);
}

int camera_valida(const camera_handle* c) { return c && fb_valido(c->view); }

int32_t  camera_x(const camera_handle* c)             { return c ? arredondar(c->cx - 0.5f * (float)c->w / c->zoom) : 0; }
int32_t  camera_y(const camera_handle* c)             { return c ? arredondar(c->cy - 0.5f * (float)c->h / c->zoom) : 0; }
int32_t  camera_largura_mundo(const camera_handle* c) { return c ? arredondar((float)c->w / c->zoom) : 0; }
int32_t  camera_altura_mundo(const camera_handle* c)  { return c ? arredondar((float)c->h / c->zoom) : 0; }
uint16_t camera_largura(const camera_handle* c)       { return c ? c->w : 0; }
uint16_t camera_altura(const camera_handle* c)        { return c ? c->h : 0; }

float camera_angulo(const camera_handle* c) { return c ? c->ang : 0.0f; }

void camera_angulo_definir(camera_handle* c, float graus) {
    if (!c) return;
    c->ang = normalizar_angulo(graus);
    cam_aplicar(c);
}

void camera_girar(camera_handle* c, float delta) {
    if (!c) return;
    c->ang = normalizar_angulo(c->ang + delta);
    cam_aplicar(c);
}

float camera_zoom(const camera_handle* c) { return c ? c->zoom : 1.0f; }

void camera_zoom_definir(camera_handle* c, float z) {
    if (!c) return;
    if (!(z > 0.0f)) return;
    c->zoom = z;
    c->zoom_alvo_ativo = 0;
    cam_aplicar(c);
}

void camera_zoom_relativo(camera_handle* c, float fator) {
    if (!c) return;
    camera_zoom_definir(c, c->zoom * fator);
}

void camera_zoom_em(camera_handle* c, float z, int32_t tela_x, int32_t tela_y) {
    if (!c || !(z > 0.0f)) return;

    const float co = (float)fb_cosseno(cam_ang16(c)) / (float)TRIG_UM;
    const float s  = (float)fb_seno(cam_ang16(c))    / (float)TRIG_UM;
    const float u = (float)tela_x + 0.5f - (float)c->w * 0.5f;
    const float v = (float)tela_y + 0.5f - (float)c->h * 0.5f;

    const float wx = c->cx + (u * co + v * s) / c->zoom;
    const float wy = c->cy + (v * co - u * s) / c->zoom;

    c->zoom = z;
    c->zoom_alvo_ativo = 0;
    if (c->zoom < c->zoom_min) c->zoom = c->zoom_min;
    if (c->zoom > c->zoom_max) c->zoom = c->zoom_max;

    c->cx = wx - (u * co + v * s) / c->zoom;
    c->cy = wy - (v * co - u * s) / c->zoom;
    cam_aplicar(c);
}

void camera_zoom_suave(camera_handle* c, float alvo, uint8_t suavidade) {
    if (!c) return;
    if (alvo < c->zoom_min) alvo = c->zoom_min;
    if (alvo > c->zoom_max) alvo = c->zoom_max;

    if (suavidade == 0) { camera_zoom_definir(c, alvo); return; }
    c->zoom_alvo = alvo;
    c->zoom_suave = suavidade;
    c->zoom_alvo_ativo = 1;
}

void camera_limites_zoom(camera_handle* c, float minimo, float maximo) {
    if (!c) return;
    if (minimo <= 0.01f) minimo = 0.01f;
    if (maximo < minimo) { const float t = minimo; minimo = maximo; maximo = t; }
    if (minimo <= 0.01f) minimo = 0.01f;
    if (maximo < minimo) maximo = minimo;

    c->zoom_min = minimo;
    c->zoom_max = maximo;
    if (c->zoom_alvo < c->zoom_min) c->zoom_alvo = c->zoom_min;
    if (c->zoom_alvo > c->zoom_max) c->zoom_alvo = c->zoom_max;
    cam_aplicar(c);
}

void camera_posicionar(camera_handle* c, int32_t x, int32_t y) {
    if (!c) return;
    c->cx = (float)x + 0.5f * (float)c->w / c->zoom;
    c->cy = (float)y + 0.5f * (float)c->h / c->zoom;
    cam_aplicar(c);
}

void camera_mover(camera_handle* c, int32_t dx, int32_t dy) {
    if (!c) return;
    c->cx += (float)dx;
    c->cy += (float)dy;
    cam_aplicar(c);
}

void camera_centralizar(camera_handle* c, int32_t mundo_x, int32_t mundo_y) {
    if (!c) return;
    c->cx = (float)mundo_x;
    c->cy = (float)mundo_y;
    cam_aplicar(c);
}

void camera_mover_suave(camera_handle* c, int32_t mundo_x, int32_t mundo_y, uint8_t suavidade, float delta) {
    if (!c) return;
    c->cx = aproximar(c->cx, (float)mundo_x, suavidade, delta);
    c->cy = aproximar(c->cy, (float)mundo_y, suavidade, delta);
    cam_aplicar(c);
}

void camera_centralizar_mundo(camera_handle* c, const mapa_handle* mapa, uint16_t pw, uint16_t ph) {
    if (!c || !mapa) return;
    if (pw == 0) pw = mapa_tile_largura(mapa);
    if (ph == 0) ph = mapa_tile_altura(mapa);

    camera_centralizar(c, mapa_x(mapa, MAPA_MUNDO) + (int32_t)(mapa_colunas(mapa) * pw) / 2,
                          mapa_y(mapa, MAPA_MUNDO) + (int32_t)(mapa_linhas(mapa)  * ph) / 2);
}

static void cam_centro_do_alvo(const camera_handle* c, int32_t* cx, int32_t* cy) {
    *cx = obj_x(c->alvo) + (int32_t)obj_largura(c->alvo) / 2 + c->alvo_dx;
    *cy = obj_y(c->alvo) + (int32_t)obj_altura(c->alvo)  / 2 + c->alvo_dy;
}

void camera_ancorar(camera_handle* c, const obj_handle* alvo, int32_t desloc_x, int32_t desloc_y, uint8_t suavidade) {
    if (!c) return;
    c->alvo = alvo;
    c->alvo_dx = desloc_x;
    c->alvo_dy = desloc_y;
    c->alvo_suave = suavidade;

    if (!c->alvo) return;
    int32_t cx, cy;
    cam_centro_do_alvo(c, &cx, &cy);
    camera_centralizar(c, cx, cy);
}

void camera_soltar(camera_handle* c) { if (c) c->alvo = NULL; }

void camera_atualizar(camera_handle* c, float delta) {
    if (!c) return;

    if (c->zoom_alvo_ativo) {
        c->zoom = aproximar(c->zoom, c->zoom_alvo, c->zoom_suave, delta);
        if (c->zoom == c->zoom_alvo) c->zoom_alvo_ativo = 0;
    }

    if (!c->alvo) { cam_aplicar(c); return; }

    int32_t cx, cy;
    cam_centro_do_alvo(c, &cx, &cy);
    camera_mover_suave(c, cx, cy, c->alvo_suave, delta);
}

void camera_travar_limites(camera_handle* c, int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    if (!c) return;
    c->lx0 = x0 < x1 ? x0 : x1;
    c->lx1 = x0 < x1 ? x1 : x0;
    c->ly0 = y0 < y1 ? y0 : y1;
    c->ly1 = y0 < y1 ? y1 : y0;
    c->travada = 1;
    cam_aplicar(c);
}

void camera_destravar(camera_handle* c) { if (c) c->travada = 0; }

void camera_travar_mapa(camera_handle* c, const mapa_handle* mapa, uint16_t tile_w, uint16_t tile_h) {
    if (!c) return;
    if (!mapa) { camera_destravar(c); return; }
    if (tile_w == 0) tile_w = mapa_tile_largura(mapa);
    if (tile_h == 0) tile_h = mapa_tile_altura(mapa);

    camera_travar_limites(c, mapa_x(mapa, MAPA_MUNDO), mapa_y(mapa, MAPA_MUNDO),
                          mapa_x(mapa, MAPA_MUNDO) + (int32_t)(mapa_colunas(mapa) * tile_w),
                          mapa_y(mapa, MAPA_MUNDO) + (int32_t)(mapa_linhas(mapa)  * tile_h));
}

void camera_mundo_para_tela(const camera_handle* c, int32_t mundo_x, int32_t mundo_y, int32_t* tela_x, int32_t* tela_y) {
    int32_t a = 0, b = 0;

    if (c) {
        if (cam_reto(c)) {
            const transf t = montar(c->cx, c->cy, 1.0f, 0);
            a = mundo_x - (int32_t)((t.cx + (int64_t)(1 - (int32_t)c->w) * 32768) >> 16);
            b = mundo_y - (int32_t)((t.cy + (int64_t)(1 - (int32_t)c->h) * 32768) >> 16);
        } else {
            const float co = (float)fb_cosseno(cam_ang16(c)) / (float)TRIG_UM;
            const float s  = (float)fb_seno(cam_ang16(c))    / (float)TRIG_UM;
            const float dx = (float)mundo_x + 0.5f - c->cx;
            const float dy = (float)mundo_y + 0.5f - c->cy;

            a = piso((dx * co - dy * s) * c->zoom + (float)c->w * 0.5f);
            b = piso((dx * s + dy * co) * c->zoom + (float)c->h * 0.5f);
        }
    }

    if (tela_x) *tela_x = a;
    if (tela_y) *tela_y = b;
}

void camera_tela_para_mundo(const camera_handle* c, int32_t tela_x, int32_t tela_y, int32_t* mundo_x, int32_t* mundo_y) {
    int32_t a = 0, b = 0;

    if (c) {
        const transf t = montar(c->cx, c->cy, c->zoom, cam_ang16(c));
        int64_t x, y;
        projetar(&t, 2 * tela_x + 1 - (int32_t)c->w, 2 * tela_y + 1 - (int32_t)c->h, &x, &y);
        a = (int32_t)(x >> 16);
        b = (int32_t)(y >> 16);
    }

    if (mundo_x) *mundo_x = a;
    if (mundo_y) *mundo_y = b;
}

void camera_iniciar(camera_handle* c, uint32_t fundo) {
    if (!c || c->w == 0) return;

    cam_aplicar(c);
    if (!cam_garantir_buffer(c)) return;
    cam_preparar_quadro(c);

    fb_limpar(c->view, fundo);
    c->opaca = (fundo >> 24) == 255;
}

int camera_visivel(const camera_handle* c, int32_t x, int32_t y, int32_t w, int32_t h) {
    return c && fb_valido(c->view) &&
           x < c->ox + c->view->w && y < c->oy + c->view->h &&
           x + w > c->ox && y + h > c->oy;
}

void camera_desenhar_obj(camera_handle* c, obj_handle* o) {
    if (!c || !o || !fb_valido(c->view)) return;
    const int32_t px = obj_x(o), py = obj_y(o);
    obj_posicionar(o, px - c->ox, py - c->oy);
    obj_desenhar(o, c->view);
    obj_posicionar(o, px, py);
}

void camera_desenhar_mapa(camera_handle* c, mapa_handle* m, int layer) {
    if (!c || !m || !fb_valido(c->view)) return;
    const int32_t px = mapa_x(m,layer), py = mapa_y(m,layer);
    mapa_posicionar(m,layer,px-c->ox,py-c->oy);
    mapa_desenhar(m,layer,c->view);
    mapa_posicionar(m,layer,px,py);
}

void camera_apresentar(const camera_handle* c, fb_handle* janela, int sx, int sy) {
    if (!c || !fb_valido(c->view) || !fb_valido(janela)) return;

    const int w = c->w, h = c->h;
    const int jw = janela->w, jh = janela->h;
    const int bw = c->view->w, bh = c->view->h;

    const int x0 = sx < 0 ? -sx : 0;
    const int y0 = sy < 0 ? -sy : 0;
    const int x1 = sx + w > jw ? jw - sx : w;
    const int y1 = sy + h > jh ? jh - sy : h;
    if (x0 >= x1 || y0 >= y1) return;

    const uint32_t* src = c->view->px;
    uint32_t* dst = janela->px;

    if (c->reto) {
        const int kx = (int)((c->bcx + (int64_t)(1 - w) * 32768) >> 16);
        const int ky = (int)((c->bcy + (int64_t)(1 - h) * 32768) >> 16);

        const int a0 = x0 > -kx ? x0 : -kx;
        const int a1 = x1 < bw - kx ? x1 : bw - kx;
        const int b0 = y0 > -ky ? y0 : -ky;
        const int b1 = y1 < bh - ky ? y1 : bh - ky;
        if (a0 >= a1 || b0 >= b1) return;

        for (int y = b0; y < b1; y++) {
            const uint32_t* o = src + (size_t)(y + ky) * bw;
            uint32_t* d = dst + (size_t)(sy + y) * jw + sx;

            if (c->opaca) {
                memoria_copia(d + a0, o + (a0 + kx), (size_t)(a1 - a0) * sizeof(uint32_t));
                continue;
            }
            for (int x = a0; x < a1; x++) d[x] = mesclar(d[x], o[x + kx]);
        }
        return;
    }

    if (c->usa_lut) {
        const int a0 = x0 > c->cmin ? x0 : c->cmin;
        const int a1 = x1 < c->cmax ? x1 : c->cmax;
        if (a0 >= a1) return;

        const uint32_t* col = c->colunas->px;

        for (int y = y0; y < y1; y++) {
            const int64_t by = c->bcy + (int64_t)(2 * y + 1 - h) * c->ca;
            const int ry = (int)(by >> 16);
            if ((unsigned)ry >= (unsigned)bh) continue;

            const uint32_t* o = src + (size_t)ry * bw;
            uint32_t* d = dst + (size_t)(sy + y) * jw + sx;
            for (int x = a0; x < a1; x++) d[x] = mesclar(d[x], o[col[x]]);
        }
        return;
    }

    transf t;
    t.cx = c->bcx;
    t.cy = c->bcy;
    t.ca = c->ca;
    t.sa = c->sa;
    const int64_t pa = 2 * (int64_t)c->ca, pb = 2 * (int64_t)c->sa;
    const int64_t lim_x = (int64_t)bw << 16, lim_y = (int64_t)bh << 16;
    const int64_t n = x1 - x0;

    for (int y = y0; y < y1; y++) {
        int64_t bx, by;
        projetar(&t, 2 * x0 + 1 - w, 2 * y + 1 - h, &bx, &by);

        int64_t k0 = 0, k1 = n;
        intervalo_linear(bx,  pa, 0, lim_x, &k0, &k1);
        intervalo_linear(by, -pb, 0, lim_y, &k0, &k1);
        if (k0 >= k1) continue;

        bx += k0 * pa;
        by -= k0 * pb;
        uint32_t* d = dst + (size_t)(sy + y) * jw + sx + x0 + k0;

        for (int64_t k = k0; k < k1; k++, d++, bx += pa, by -= pb)
            *d = mesclar(*d, src[(size_t)(by >> 16) * bw + (size_t)(bx >> 16)]);
    }
}

uint32_t* camera_viewport_dados(camera_handle* c)   { return c && c->view ? c->view->px : NULL; }
int camera_viewport_largura(camera_handle* c)       { return c && c->view ? c->view->w : 0; }
int camera_viewport_altura(camera_handle* c)        { return c && c->view ? c->view->h : 0; }
