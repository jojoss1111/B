#include "z_buffer.h"

#define ZB_INFINITO 1e30f
#define ZB_EPS_W    1e-6f

/* ---- z_buffer ---- */

zb_handle* zb_criar(int largura, int altura) {
    if (largura <= 0 || altura <= 0) return NULL;

    zb_handle* h = (zb_handle*)memoria_malloc(sizeof(zb_handle));
    if (!h) return NULL;

    h->w = largura;
    h->h = altura;
    h->z = (float*)memoria_malloc((size_t)largura * altura * sizeof(float));
    if (!h->z) { memoria_free(h); return NULL; }

    zb_limpar(h);
    return h;
}

void zb_destruir(zb_handle* zb) {
    if (!zb) return;
    memoria_free(zb->z);
    memoria_free(zb);
}

int zb_valido(const zb_handle* zb)  { return zb && zb->z != NULL; }
int zb_largura(const zb_handle* zb) { return zb ? zb->w : 0; }
int zb_altura(const zb_handle* zb)  { return zb ? zb->h : 0; }

void zb_limpar(zb_handle* zb) {
    if (!zb_valido(zb)) return;
    for (size_t i = 0, n = (size_t)zb->w * zb->h; i < n; i++) zb->z[i] = ZB_INFINITO;
}

float zb_ler(const zb_handle* zb, int x, int y) {
    if (!zb_valido(zb) || (unsigned)x >= (unsigned)zb->w || (unsigned)y >= (unsigned)zb->h) return ZB_INFINITO;
    return zb->z[(size_t)y * zb->w + x];
}

/* ---- matemática ---- */

static float seno_graus(float g)    { return (float)fb_seno(fb_graus(g)) * (1.0f / 32768.0f); }
static float cosseno_graus(float g) { return (float)fb_cosseno(fb_graus(g)) * (1.0f / 32768.0f); }

zb_vec3 zb_vec3_criar(float x, float y, float z) {
    zb_vec3 v = { x, y, z };
    return v;
}

zb_mat4 zb_mat4_identidade(void) {
    zb_mat4 r;
    for (int i = 0; i < 16; i++) r.m[i] = 0.0f;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

zb_mat4 zb_mat4_multiplicar(const zb_mat4* a, const zb_mat4* b) {
    zb_mat4 r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r.m[i * 4 + j] = a->m[i * 4 + 0] * b->m[0 * 4 + j]
                           + a->m[i * 4 + 1] * b->m[1 * 4 + j]
                           + a->m[i * 4 + 2] * b->m[2 * 4 + j]
                           + a->m[i * 4 + 3] * b->m[3 * 4 + j];
    return r;
}

zb_mat4 zb_mat4_translacao(float x, float y, float z) {
    zb_mat4 r = zb_mat4_identidade();
    r.m[3] = x; r.m[7] = y; r.m[11] = z;
    return r;
}

zb_mat4 zb_mat4_escala(float x, float y, float z) {
    zb_mat4 r = zb_mat4_identidade();
    r.m[0] = x; r.m[5] = y; r.m[10] = z;
    return r;
}

zb_mat4 zb_mat4_rotacao_x(float graus) {
    const float c = cosseno_graus(graus), s = seno_graus(graus);
    zb_mat4 r = zb_mat4_identidade();
    r.m[5] = c;  r.m[6] = -s;
    r.m[9] = s;  r.m[10] = c;
    return r;
}

zb_mat4 zb_mat4_rotacao_y(float graus) {
    const float c = cosseno_graus(graus), s = seno_graus(graus);
    zb_mat4 r = zb_mat4_identidade();
    r.m[0] = c;   r.m[2] = s;
    r.m[8] = -s;  r.m[10] = c;
    return r;
}

zb_mat4 zb_mat4_rotacao_z(float graus) {
    const float c = cosseno_graus(graus), s = seno_graus(graus);
    zb_mat4 r = zb_mat4_identidade();
    r.m[0] = c;  r.m[1] = -s;
    r.m[4] = s;  r.m[5] = c;
    return r;
}

zb_mat4 zb_mat4_perspectiva(float fov_graus, float aspecto, float perto, float longe) {
    const float meio = fov_graus * 0.5f;
    const float s = seno_graus(meio);
    zb_mat4 r;
    for (int i = 0; i < 16; i++) r.m[i] = 0.0f;
    if (s == 0.0f || aspecto == 0.0f || perto == longe) return zb_mat4_identidade();

    const float f = cosseno_graus(meio) / s;
    r.m[0]  = f / aspecto;
    r.m[5]  = f;
    r.m[10] = (longe + perto) / (perto - longe);
    r.m[11] = (2.0f * longe * perto) / (perto - longe);
    r.m[14] = -1.0f;
    return r;
}

zb_mat4 zb_mat4_ortografica(float esq, float dir, float baixo, float cima, float perto, float longe) {
    if (esq == dir || baixo == cima || perto == longe) return zb_mat4_identidade();

    zb_mat4 r = zb_mat4_identidade();
    r.m[0]  = 2.0f / (dir - esq);
    r.m[5]  = 2.0f / (cima - baixo);
    r.m[10] = -2.0f / (longe - perto);
    r.m[3]  = -(dir + esq) / (dir - esq);
    r.m[7]  = -(cima + baixo) / (cima - baixo);
    r.m[11] = -(longe + perto) / (longe - perto);
    return r;
}

/* ---- vértices em clip space ---- */

typedef struct {
    float x, y, z, w;
    float u, v;
    float r, g, b, a;
} cv;

typedef struct {
    float x, y, z, iw;
    float u, v, r, g, b, a;     /* já divididos por w */
} sv;

static cv vertice_para_clip(const zb_mat4* m, const zb_vertice* p) {
    const float* k = m->m;
    cv o;
    o.x = k[0]  * p->x + k[1]  * p->y + k[2]  * p->z + k[3];
    o.y = k[4]  * p->x + k[5]  * p->y + k[6]  * p->z + k[7];
    o.z = k[8]  * p->x + k[9]  * p->y + k[10] * p->z + k[11];
    o.w = k[12] * p->x + k[13] * p->y + k[14] * p->z + k[15];
    o.u = p->u;
    o.v = p->v;
    o.a = (float)(p->cor >> 24);
    o.r = (float)((p->cor >> 16) & 255);
    o.g = (float)((p->cor >> 8) & 255);
    o.b = (float)(p->cor & 255);
    return o;
}

static cv clip_lerp(const cv* a, const cv* b, float t) {
    cv o;
    o.x = a->x + (b->x - a->x) * t;
    o.y = a->y + (b->y - a->y) * t;
    o.z = a->z + (b->z - a->z) * t;
    o.w = a->w + (b->w - a->w) * t;
    o.u = a->u + (b->u - a->u) * t;
    o.v = a->v + (b->v - a->v) * t;
    o.r = a->r + (b->r - a->r) * t;
    o.g = a->g + (b->g - a->g) * t;
    o.b = a->b + (b->b - a->b) * t;
    o.a = a->a + (b->a - a->a) * t;
    return o;
}

/* recorta contra o plano próximo (z >= -w); triângulo gera no máximo 4 vértices */
static int clipar_perto(const cv* in, int n, cv* out) {
    int m = 0;
    for (int i = 0; i < n; i++) {
        const cv* a = &in[i];
        const cv* b = &in[(i + 1) % n];
        const float da = a->z + a->w, db = b->z + b->w;
        const int ia = da >= 0.0f, ib = db >= 0.0f;
        if (ia) out[m++] = *a;
        if (ia != ib) out[m++] = clip_lerp(a, b, da / (da - db));
    }
    return m;
}

static sv clip_para_tela(const cv* c, int w, int h) {
    const float iw = 1.0f / c->w;
    sv s;
    s.x  = (c->x * iw * 0.5f + 0.5f) * (float)w;
    s.y  = (0.5f - c->y * iw * 0.5f) * (float)h;
    s.z  = c->z * iw;
    s.iw = iw;
    s.u  = c->u * iw;
    s.v  = c->v * iw;
    s.r  = c->r * iw;
    s.g  = c->g * iw;
    s.b  = c->b * iw;
    s.a  = c->a * iw;
    return s;
}

/* ---- rasterização ---- */

typedef struct {
    float a, b;     /* d(e)/dx, d(e)/dy */
    float e;        /* valor no centro do primeiro pixel */
    int   tl;       /* aresta superior/esquerda */
} aresta;

static aresta aresta_criar(const sv* p, const sv* q, float px, float py) {
    const float dx = q->x - p->x, dy = q->y - p->y;
    aresta r;
    r.a  = -dy;
    r.b  = dx;
    r.e  = dx * (py - p->y) - dy * (px - p->x);
    r.tl = (dy == 0.0f && dx > 0.0f) || dy < 0.0f;
    return r;
}

static inline int aresta_dentro(float e, int tl) { return e > 0.0f || (e == 0.0f && tl); }

static inline float limitar(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static inline uint32_t canal(float v) {
    return (uint32_t)(limitar(v, 0.0f, 255.0f) + 0.5f);
}

static inline uint32_t empacotar(float r, float g, float b, float a) {
    return (canal(a) << 24) | (canal(r) << 16) | (canal(g) << 8) | canal(b);
}

static void rasterizar(fb_handle* fb, zb_handle* zb, const zb_estado* est, uint32_t cor_plana,
                       sv a, sv b, sv c) {
    float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (area == 0.0f) return;
    if (area < 0.0f) { sv t = b; b = c; c = t; area = -area; }

    const int W = fb->w, H = fb->h;

    float fx0 = a.x < b.x ? a.x : b.x; if (c.x < fx0) fx0 = c.x;
    float fx1 = a.x > b.x ? a.x : b.x; if (c.x > fx1) fx1 = c.x;
    float fy0 = a.y < b.y ? a.y : b.y; if (c.y < fy0) fy0 = c.y;
    float fy1 = a.y > b.y ? a.y : b.y; if (c.y > fy1) fy1 = c.y;

    fx0 = limitar(fx0, 0.0f, (float)(W - 1));
    fx1 = limitar(fx1, 0.0f, (float)(W - 1));
    fy0 = limitar(fy0, 0.0f, (float)(H - 1));
    fy1 = limitar(fy1, 0.0f, (float)(H - 1));

    const int x0 = (int)fx0, x1 = (int)fx1 + 1 > W - 1 ? W - 1 : (int)fx1 + 1;
    const int y0 = (int)fy0, y1 = (int)fy1 + 1 > H - 1 ? H - 1 : (int)fy1 + 1;

    const float px = (float)x0 + 0.5f, py = (float)y0 + 0.5f;
    aresta e0 = aresta_criar(&b, &c, px, py);
    aresta e1 = aresta_criar(&c, &a, px, py);
    aresta e2 = aresta_criar(&a, &b, px, py);

    const float inv_area = 1.0f / area;
    const int   textura  = est->modo == ZB_MODO_TEXTURA;
    const int   gouraud  = est->modo == ZB_MODO_GOURAUD;

    const float ru = est->rep_u > 0.0f ? est->rep_u : 1.0f;   /* repeticoes do tile */
    const float rv = est->rep_v > 0.0f ? est->rep_v : 1.0f;

    const uint32_t* tile = 0;
    int tpasso = 0, tw = 0, th = 0;
    if (textura) {
        tile = fb_atlas_tile_dados(est->atlas, est->tile_x, est->tile_y);
        if (!tile) return;
        tpasso = fb_atlas_passo(est->atlas);
        tw = fb_atlas_tile_largura(est->atlas);
        th = fb_atlas_tile_altura(est->atlas);
    }

    uint32_t* cores = fb_dados(fb);

    for (int y = y0; y <= y1; y++) {
        float r0 = e0.e, r1 = e1.e, r2 = e2.e;

        for (int x = x0; x <= x1; x++, r0 += e0.a, r1 += e1.a, r2 += e2.a) {
            if (!aresta_dentro(r0, e0.tl) || !aresta_dentro(r1, e1.tl) || !aresta_dentro(r2, e2.tl)) continue;

            const float l0 = r0 * inv_area, l1 = r1 * inv_area, l2 = r2 * inv_area;

            const float z = l0 * a.z + l1 * b.z + l2 * c.z;
            if (z > 1.0f) continue;

            const size_t idx = (size_t)y * W + x;
            if (z >= zb->z[idx]) continue;

            uint32_t cor = cor_plana;

            if (textura || gouraud) {
                const float iw  = l0 * a.iw + l1 * b.iw + l2 * c.iw;
                const float rcp = 1.0f / iw;

                if (textura) {
                    /* uv da face (0..1) multiplicado pelas repeticoes; a parte fracionaria escolhe o texel */
                    const float u = limitar((l0 * a.u + l1 * b.u + l2 * c.u) * rcp, 0.0f, 1.0f) * ru;
                    const float v = limitar((l0 * a.v + l1 * b.v + l2 * c.v) * rcp, 0.0f, 1.0f) * rv;
                    float fu = u - (float)(int)u;
                    float fv = v - (float)(int)v;
                    if (fu == 0.0f && u > 0.0f) fu = 1.0f;   /* borda final = ultimo texel, como antes */
                    if (fv == 0.0f && v > 0.0f) fv = 1.0f;
                    int tx = (int)(fu * (float)tw);
                    int ty = (int)(fv * (float)th);
                    if (tx >= tw) tx = tw - 1;
                    if (ty >= th) ty = th - 1;
                    cor = tile[(size_t)ty * tpasso + tx];
                } else {
                    cor = empacotar((l0 * a.r + l1 * b.r + l2 * c.r) * rcp,
                                    (l0 * a.g + l1 * b.g + l2 * c.g) * rcp,
                                    (l0 * a.b + l1 * b.b + l2 * c.b) * rcp,
                                    (l0 * a.a + l1 * b.a + l2 * c.a) * rcp);
                }
            }

            const uint32_t alfa = cor >> 24;
            if (alfa == 0) continue;

            if (alfa == 255) {
                cores[idx] = cor;
                zb->z[idx] = z;
            } else {
                fb_misturar(fb, x, y, cor);
            }
        }

        e0.e += e0.b;
        e1.e += e1.b;
        e2.e += e2.b;
    }
}

/* ---- API de desenho ---- */

zb_estado zb_estado_padrao(void) {
    zb_estado e;
    e.modo   = ZB_MODO_GOURAUD;
    e.cull   = ZB_CULL_TRAS;
    e.atlas  = NULL;
    e.tile_x = 0;
    e.tile_y = 0;
    e.rep_u  = 1.0f;
    e.rep_v  = 1.0f;
    return e;
}

void zb_triangulo(fb_handle* fb, zb_handle* zb, const zb_estado* est, const zb_mat4* mvp,
                  const zb_vertice* a, const zb_vertice* b, const zb_vertice* c) {
    if (!fb_valido(fb) || !zb_valido(zb) || !est || !mvp || !a || !b || !c) return;
    if (fb->w != zb->w || fb->h != zb->h) return;
    if (est->modo == ZB_MODO_TEXTURA && !fb_atlas_valido(est->atlas)) return;

    const cv entrada[3] = {
        vertice_para_clip(mvp, a),
        vertice_para_clip(mvp, b),
        vertice_para_clip(mvp, c),
    };

    cv poli[4];
    const int n = clipar_perto(entrada, 3, poli);
    if (n < 3) return;

    sv s[4];
    for (int i = 0; i < n; i++) {
        if (poli[i].w < ZB_EPS_W) return;
        s[i] = clip_para_tela(&poli[i], fb->w, fb->h);
    }

    const float area = (s[1].x - s[0].x) * (s[2].y - s[0].y) - (s[1].y - s[0].y) * (s[2].x - s[0].x);
    if (est->cull == ZB_CULL_TRAS   && area > 0.0f) return;
    if (est->cull == ZB_CULL_FRENTE && area < 0.0f) return;

    for (int i = 1; i + 1 < n; i++)
        rasterizar(fb, zb, est, a->cor, s[0], s[i], s[i + 1]);
}

void zb_malha_desenhar(fb_handle* fb, zb_handle* zb, const zb_estado* est, const zb_mat4* mvp,
                       const zb_malha* malha) {
    if (!malha || !malha->vertices || !malha->indices) return;

    for (int i = 0; i + 2 < malha->n_indices; i += 3) {
        const int ia = malha->indices[i], ib = malha->indices[i + 1], ic = malha->indices[i + 2];
        if (ia >= malha->n_vertices || ib >= malha->n_vertices || ic >= malha->n_vertices) continue;
        zb_triangulo(fb, zb, est, mvp, &malha->vertices[ia], &malha->vertices[ib], &malha->vertices[ic]);
    }
}