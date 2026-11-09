#include "obj.h"

#define TRIG_BITS 15
#define LUT_MAX   1024

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

static inline uint32_t piso255(uint32_t v) { return (v + 1 + (v >> 8)) >> 8; }

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

static void intervalo_linear(int64_t f0, int64_t d, int64_t lo, int64_t hi, int64_t* k0, int64_t* k1) {
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

typedef struct {
    int      neutro, subst, cor_neutra, alpha_neutro;
    uint32_t tr, tg, tb, ta, alpha;
} sombra;

static inline uint32_t sombrear(const sombra* s, uint32_t p) {
    if (s->neutro) return p;
    uint32_t a = p >> 24;
    if (a == 0) return 0;
    uint32_t r = (p >> 16) & 255, g = (p >> 8) & 255, b = p & 255;

    if (s->subst) {
        r = s->tr; g = s->tg; b = s->tb;
    } else if (!s->cor_neutra) {
        r = piso255(r * s->tr);
        g = piso255(g * s->tg);
        b = piso255(b * s->tb);
    }
    if (!s->alpha_neutro) a = piso255(piso255(a * s->alpha) * s->ta);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

void obj_iniciar(obj_handle* o, fb_atlas_handle* sprite, uint16_t pw, uint16_t ph, int32_t x, int32_t y, uint16_t coluna, uint16_t linha) {
    o->sprite = sprite;
    o->pw = pw; o->ph = ph;
    o->x = x; o->y = y;
    o->col = coluna; o->lin = linha;
    o->ang = 0.0f;
    o->alpha = 255;
    o->flip_x = o->flip_y = 0;
    o->tr = o->tg = o->tb = o->ta = 255;
    o->subst = 0;
}

void obj_colorir(obj_handle* o, int substituir) { if (o) o->subst = substituir != 0; }

obj_handle* obj_criar(fb_atlas_handle* sprite, uint16_t pw, uint16_t ph, int32_t x, int32_t y, uint16_t coluna, uint16_t linha) {
    obj_handle* o = (obj_handle*)memoria_malloc(sizeof(obj_handle));
    if (!o) return NULL;
    obj_iniciar(o, sprite, pw, ph, x, y, coluna, linha);
    return o;
}

void obj_destruir(obj_handle* o)                { if (o) memoria_free(o); }
int32_t  obj_x(const obj_handle* o)             { return o ? o->x : 0; }
int32_t  obj_y(const obj_handle* o)             { return o ? o->y : 0; }
uint16_t obj_largura(const obj_handle* o)       { return o ? o->pw : 0; }
uint16_t obj_altura(const obj_handle* o)        { return o ? o->ph : 0; }
void obj_posicionar(obj_handle* o, int32_t x, int32_t y)       { if (o) { o->x = x; o->y = y; } }
void obj_mover(obj_handle* o, int32_t x, int32_t y)            { if (o) { o->x += x; o->y += y; } }
void obj_tile(obj_handle* o, uint16_t coluna, uint16_t linha)  { if (o) { o->col = coluna; o->lin = linha; } }
void obj_alpha(obj_handle* o, uint8_t a)                       { if (o) o->alpha = a; }
void obj_ampliar(obj_handle* o, uint16_t pw, uint16_t ph)      { if (o) { o->pw = pw; o->ph = ph; } }
void obj_angulo(obj_handle* o, float graus)                    { if (o) o->ang = graus; }
void obj_espelhar_x(obj_handle* o, int e)                      { if (o) o->flip_x = e != 0; }
void obj_espelhar_y(obj_handle* o, int e)                      { if (o) o->flip_y = e != 0; }

void obj_cor(obj_handle* o, uint32_t cor) {
    if (!o) return;
    o->ta = (uint8_t)(cor >> 24);
    o->tr = (uint8_t)(cor >> 16);
    o->tg = (uint8_t)(cor >> 8);
    o->tb = (uint8_t)cor;
}

void obj_desenhar(const obj_handle* o, fb_handle* fb) {
    if (!o || !fb || !o->sprite || !fb_atlas_valido(o->sprite) || !fb_valido(fb)) return;
    if (o->pw == 0 || o->ph == 0 || o->alpha == 0 || o->ta == 0) return;
    if (o->col >= fb_atlas_colunas(o->sprite) || o->lin >= fb_atlas_linhas(o->sprite)) return;

    const int fw = fb_atlas_tile_largura(o->sprite), fh = fb_atlas_tile_altura(o->sprite);
    const int passo = fb_atlas_passo(o->sprite);
    const uint32_t* tile = fb_atlas_tile_dados(o->sprite, o->col, o->lin);

    const int32_t x_ = o->x, y_ = o->y;
    const int w = o->pw, h = o->ph;
    const int cx2 = 2 * x_ + w, cy2 = 2 * y_ + h;
    const uint16_t ang = fb_graus(o->ang);
    const int fbw = fb->w, fbh = fb->h;
    uint32_t* const base = fb->px;
    const int flip_x = o->flip_x, flip_y = o->flip_y;

    sombra sb;
    sb.subst = o->subst;
    sb.cor_neutra = !o->subst && o->tr == 255 && o->tg == 255 && o->tb == 255;
    sb.alpha_neutro = o->alpha == 255 && o->ta == 255;
    sb.neutro = sb.cor_neutra && sb.alpha_neutro;
    sb.tr = o->tr; sb.tg = o->tg; sb.tb = o->tb; sb.ta = o->ta;
    sb.alpha = o->alpha;

    if (!ang) {
        const int xa = x_ < 0 ? 0 : x_;
        const int ya = y_ < 0 ? 0 : y_;
        const int xb = x_ + w > fbw ? fbw : x_ + w;
        const int yb = y_ + h > fbh ? fbh : y_ + h;
        if (xa >= xb || ya >= yb) return;

        const int i0 = xa - x_;
        const int escala_x_1 = (w == fw);

        const int64_t w2 = 2 * (int64_t)w;
        const int64_t n0 = (2 * (int64_t)i0 + 1) * fw;
        const int64_t q0 = n0 / w2, r0 = n0 % w2;
        const int64_t pq = fw / w;
        const int64_t pr = 2 * (int64_t)fw - pq * w2;

        for (int y = ya; y < yb; y++) {
            int ty = (int)(((2 * (int64_t)(y - y_) + 1) * fh) / (2 * (int64_t)h));
            if (flip_y) ty = fh - 1 - ty;

            const uint32_t* linha = tile + (size_t)ty * passo;
            uint32_t* d = base + (size_t)y * fbw + xa;

            if (escala_x_1) {
                for (int i = i0; i < i0 + (xb - xa); i++, d++) {
                    const uint32_t p = linha[flip_x ? fw - 1 - i : i];
                    *d = mesclar(*d, sombrear(&sb, p));
                }
            } else {
                int64_t q = q0, r = r0;
                for (int i = xa; i < xb; i++, d++) {
                    const int tx = flip_x ? fw - 1 - (int)q : (int)q;
                    *d = mesclar(*d, sombrear(&sb, linha[tx]));
                    r += pr; q += pq;
                    if (r >= w2) { r -= w2; q++; }
                }
            }
        }
        return;
    }

    const int32_t c = fb_cosseno(ang), s = fb_seno(ang);

    const int r = (w + h) / 2 + 1;
    const int cx = cx2 / 2, cy = cy2 / 2;
    int x0 = cx - r, x1 = cx + r;
    int y0 = cy - r, y1 = cy + r;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > fbw) x1 = fbw;
    if (y1 > fbh) y1 = fbh;
    if (x0 >= x1 || y0 >= y1) return;

    uint16_t lut_x[LUT_MAX], lut_y[LUT_MAX];
    const int usa_lut = (2 * w <= LUT_MAX) && (2 * h <= LUT_MAX);
    if (usa_lut) {
        for (int i = 0; i < 2 * w; i++) {
            int t = (int)(((int64_t)i * fw) / (2 * (int64_t)w));
            lut_x[i] = (uint16_t)(flip_x ? fw - 1 - t : t);
        }
        for (int j = 0; j < 2 * h; j++) {
            int t = (int)(((int64_t)j * fh) / (2 * (int64_t)h));
            lut_y[j] = (uint16_t)(flip_y ? fh - 1 - t : t);
        }
    }

    const int64_t lim_x = (int64_t)w << TRIG_BITS;
    const int64_t lim_y = (int64_t)h << TRIG_BITS;
    const int64_t passo_x = 2 * (int64_t)c, passo_y = -2 * (int64_t)s;
    const int64_t n = x1 - x0;

    for (int y = y0; y < y1; y++) {
        const int64_t uy  = 2 * (int64_t)y + 1 - cy2;
        const int64_t ux0 = 2 * (int64_t)x0 + 1 - cx2;
        int64_t nx = ux0 * c + uy * s;
        int64_t ny = uy * c - ux0 * s;

        int64_t k0 = 0, k1 = n;
        intervalo_linear(nx, passo_x, -lim_x, lim_x, &k0, &k1);
        intervalo_linear(ny, passo_y, -lim_y, lim_y, &k0, &k1);
        if (k0 >= k1) continue;

        nx += k0 * passo_x;
        ny += k0 * passo_y;
        uint32_t* d = base + (size_t)y * fbw + x0 + k0;

        for (int64_t k = k0; k < k1; k++, d++, nx += passo_x, ny += passo_y) {
            const int ix = (int)(nx >> TRIG_BITS) + w;
            const int iy = (int)(ny >> TRIG_BITS) + h;

            int tx, ty;
            if (usa_lut) {
                tx = lut_x[ix];
                ty = lut_y[iy];
            } else {
                tx = (int)(((int64_t)ix * fw) / (2 * (int64_t)w));
                ty = (int)(((int64_t)iy * fh) / (2 * (int64_t)h));
                if (flip_x) tx = fw - 1 - tx;
                if (flip_y) ty = fh - 1 - ty;
            }

            *d = mesclar(*d, sombrear(&sb, tile[(size_t)ty * passo + tx]));
        }
    }
}

static void anim_aplicar(anim_handle* a) {
    if (!a->obj || !a->arr || a->total == 0) return;
    obj_tile(a->obj, (uint16_t)a->arr[a->idx][0], (uint16_t)a->arr[a->idx][1]);
}

anim_handle* anim_criar(obj_handle* o, const int16_t* quadros, uint8_t total, uint8_t fps, int loop) {
    anim_handle* a = (anim_handle*)memoria_malloc(sizeof(anim_handle));
    if (!a) return NULL;

    a->obj = o;
    a->arr = (const int16_t (*)[2])quadros;
    a->total = total;
    a->ms_quadro = fps ? 1000u / fps : 1000u;
    if (a->ms_quadro == 0) a->ms_quadro = 1;
    a->loop = loop != 0;
    a->idx = 0;
    a->acum = 0;
    a->tocando = 0;
    anim_aplicar(a);
    return a;
}

void anim_destruir(anim_handle* a) { if (a) memoria_free(a); }
void anim_rodar(anim_handle* a)    { if (a) a->tocando = 1; }
void anim_parar(anim_handle* a)    { if (a) a->tocando = 0; }

void anim_voltar(anim_handle* a) {
    if (!a) return;
    a->idx = 0;
    a->acum = 0;
    anim_aplicar(a);
}

void anim_atualizar(anim_handle* a, uint32_t dt_ms) {
    if (!a || !a->tocando || !a->obj || !a->arr || a->total == 0) return;

    a->acum += dt_ms;
    while (a->acum >= a->ms_quadro) {
        a->acum -= a->ms_quadro;
        if (++a->idx >= a->total) {
            if (a->loop) {
                a->idx = 0;
            } else {
                a->idx = a->total - 1;
                a->tocando = 0;
                a->acum = 0;
                break;
            }
        }
    }
    anim_aplicar(a);
}

void aabb_2d_sincronizar(aabb_2d_handle* a, obj_handle* o) {
    if (!a || !o) return;
    a->x = o->x;
    a->y = o->y;
}

void aabb_2d_iniciar(aabb_2d_handle* a, obj_handle* o, const char* mascara, uint16_t mascara_w, uint16_t mascara_h) {
    a->x = a->y = 0;
    a->mascara = mascara;
    a->mascara_w = mascara_w;
    a->mascara_h = mascara_h;
    a->w = mascara_w;
    a->h = mascara_h;
    if (o) aabb_2d_sincronizar(a, o);
}

aabb_2d_handle* aabb_2d_criar(obj_handle* o, const char* mascara, uint16_t mascara_w, uint16_t mascara_h) {
    aabb_2d_handle* a = (aabb_2d_handle*)memoria_malloc(sizeof(aabb_2d_handle));
    if (!a) return NULL;
    aabb_2d_iniciar(a, o, mascara, mascara_w, mascara_h);
    return a;
}

void aabb_2d_destruir(aabb_2d_handle* a) { if (a) memoria_free(a); }

void aabb_2d_posicionar(aabb_2d_handle* a, int32_t x, int32_t y) { if (a) { a->x = x; a->y = y; } }
int32_t  aabb_2d_x(const aabb_2d_handle* a)        { return a ? a->x : 0; }
int32_t  aabb_2d_y(const aabb_2d_handle* a)        { return a ? a->y : 0; }
uint16_t aabb_2d_largura(const aabb_2d_handle* a)  { return a ? a->w : 0; }
uint16_t aabb_2d_altura(const aabb_2d_handle* a)   { return a ? a->h : 0; }

static int solido_local(const aabb_2d_handle* a, int32_t lx, int32_t ly) {
    if (lx < 0 || ly < 0 || lx >= a->w || ly >= a->h) return 0;
    if (!a->mascara) return 1;
    return a->mascara[(size_t)ly * a->mascara_w + lx] == '#';
}

static int testar(const aabb_2d_handle* a, int32_t ax, int32_t ay, const aabb_2d_handle* b, int32_t bx, int32_t by) {
    const int32_t x0 = ax > bx ? ax : bx;
    const int32_t y0 = ay > by ? ay : by;
    const int32_t ea = ax + a->w, eb = bx + b->w;
    const int32_t fa = ay + a->h, fb = by + b->h;
    const int32_t x1 = ea < eb ? ea : eb;
    const int32_t y1 = fa < fb ? fa : fb;

    if (x0 >= x1 || y0 >= y1) return 0;
    if (!a->mascara && !b->mascara) return 1;

    for (int32_t y = y0; y < y1; y++)
        for (int32_t x = x0; x < x1; x++)
            if (solido_local(a, x - ax, y - ay) && solido_local(b, x - bx, y - by)) return 1;
    return 0;
}

int aabb_2d_colidio(const aabb_2d_handle* a, const aabb_2d_handle* b) {
    if (!a || !b) return 0;
    return testar(a, a->x, a->y, b, b->x, b->y);
}

int aabb_2d_colidiu_x(const aabb_2d_handle* a, const aabb_2d_handle* outro, int32_t dx) {
    if (!a || !outro) return 0;
    return testar(a, a->x + dx, a->y, outro, outro->x, outro->y);
}

int aabb_2d_colidiu_y(const aabb_2d_handle* a, const aabb_2d_handle* outro, int32_t dy) {
    if (!a || !outro) return 0;
    return testar(a, a->x, a->y + dy, outro, outro->x, outro->y);
}