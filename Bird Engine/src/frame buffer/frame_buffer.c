#include "frame_buffer.h"
#include "stb_image.h"

/* ---- Trigonometria por LUT de quarto de onda ------------------------
 * 258 entradas uint16 (8 bits de índice + 6 de interpolação por quadrante).
 * O resto da onda é obtido por espelhamento/inversão de sinal.
 * ------------------------------------------------------------------- */
#define TRIG_BITS_FRACAO     6
#define TRIG_MASCARA_FRACAO  ((1 << TRIG_BITS_FRACAO) - 1)
#define TRIG_BITS            15
#define TRIG_QUARTO_VOLTA    16384
#define TRIG_MEIA_VOLTA      32768

#define FX_BITS 4
#define FX_UM   (1 << FX_BITS)

static const uint16_t TAB_SENO[258] = {
    0, 201, 402, 603, 804, 1005, 1206, 1407, 1608, 1809, 2009, 2210,
    2411, 2611, 2811, 3012, 3212, 3412, 3612, 3812, 4011, 4211, 4410, 4609,
    4808, 5007, 5205, 5404, 5602, 5800, 5998, 6195, 6393, 6590, 6787, 6983,
    7180, 7376, 7571, 7767, 7962, 8157, 8351, 8546, 8740, 8933, 9127, 9319,
    9512, 9704, 9896, 10088, 10279, 10469, 10660, 10850, 11039, 11228, 11417, 11605,
    11793, 11980, 12167, 12354, 12540, 12725, 12910, 13095, 13279, 13463, 13646, 13828,
    14010, 14192, 14373, 14553, 14733, 14912, 15091, 15269, 15447, 15624, 15800, 15976,
    16151, 16326, 16500, 16673, 16846, 17018, 17190, 17361, 17531, 17700, 17869, 18037,
    18205, 18372, 18538, 18703, 18868, 19032, 19195, 19358, 19520, 19681, 19841, 20001,
    20160, 20318, 20475, 20632, 20788, 20943, 21097, 21251, 21403, 21555, 21706, 21856,
    22006, 22154, 22302, 22449, 22595, 22740, 22884, 23028, 23170, 23312, 23453, 23593,
    23732, 23870, 24008, 24144, 24279, 24414, 24548, 24680, 24812, 24943, 25073, 25202,
    25330, 25457, 25583, 25708, 25833, 25956, 26078, 26199, 26320, 26439, 26557, 26674,
    26791, 26906, 27020, 27133, 27246, 27357, 27467, 27576, 27684, 27791, 27897, 28002,
    28106, 28209, 28311, 28411, 28511, 28610, 28707, 28803, 28899, 28993, 29086, 29178,
    29269, 29359, 29448, 29535, 29622, 29707, 29792, 29875, 29957, 30038, 30118, 30196,
    30274, 30350, 30425, 30499, 30572, 30644, 30715, 30784, 30853, 30920, 30986, 31050,
    31114, 31177, 31238, 31298, 31357, 31415, 31471, 31527, 31581, 31634, 31686, 31737,
    31786, 31834, 31881, 31927, 31972, 32015, 32058, 32099, 32138, 32177, 32214, 32251,
    32286, 32319, 32352, 32383, 32413, 32442, 32470, 32496, 32522, 32546, 32568, 32590,
    32610, 32629, 32647, 32664, 32679, 32693, 32706, 32718, 32729, 32738, 32746, 32753,
    32758, 32762, 32766, 32767, 32768, 32767,
};

static int32_t trig_seno(uint16_t a) {
    const unsigned q = a >> 14;
    unsigned pos = a & (TRIG_QUARTO_VOLTA - 1);
    if (q & 1) pos = TRIG_QUARTO_VOLTA - pos;

    const int i = (int)(pos >> TRIG_BITS_FRACAO), f = (int)(pos & TRIG_MASCARA_FRACAO);
    const int32_t v0 = TAB_SENO[i], v1 = TAB_SENO[i + 1];
    const int32_t r  = v0 + (((v1 - v0) * f) >> TRIG_BITS_FRACAO);
    return (q & 2) ? -r : r;
}

static int32_t trig_cosseno(uint16_t a) { return trig_seno((uint16_t)(a + TRIG_QUARTO_VOLTA)); }

static uint16_t trig_de_graus(float g) {
    const float v = g * (65536.0f / 360.0f);
    return (uint16_t)(int32_t)(v >= 0.0f ? v + 0.5f : v - 0.5f);
}

/* ---- Cores (ARGB empacotado) ---- */

/* round(v / 255) exato para v em 0..65025, sem divisão */
static inline uint32_t div255(uint32_t v) {
    v += 128;
    return (v + (v >> 8)) >> 8;
}

/* blend direto em pixels empacotados ARGB (resultado sempre opaco) */
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

/* ---- Utilidades internas ---- */

typedef struct { int ini, fim; } faixa;
typedef struct { int x, y; } pt;

static faixa recortar(int pos, int tam, int limite) {
    faixa f;
    f.ini = pos < 0 ? -pos : 0;
    f.fim = pos + tam > limite ? limite - pos : tam;
    return f;
}

/* maior r tal que r*r <= n; -1 se n < 0 */
static int raiz_inteira(int64_t n) {
    if (n < 0) return -1;
    uint64_t x = (uint64_t)n, r = 0, bit = 1ULL << 62;
    while (bit > x) bit >>= 2;
    while (bit) {
        if (x >= r + bit) { x -= r + bit; r = (r >> 1) + bit; }
        else r >>= 1;
        bit >>= 2;
    }
    return (int)r;
}

static inline int dentro(const fb_handle* fb, int x, int y) {
    return (unsigned)x < (unsigned)fb->w && (unsigned)y < (unsigned)fb->h;
}

static void px_misturar(fb_handle* fb, int x, int y, uint32_t cor) {
    if ((cor >> 24) == 0 || !dentro(fb, x, y)) return;
    uint32_t* d = &fb->px[(size_t)y * fb->w + x];
    *d = mesclar(*d, cor);
}

/* ---- frame_buffer ---- */

fb_handle* fb_criar(int largura, int altura) {
    if (largura <= 0 || altura <= 0) return NULL;

    fb_handle* h = (fb_handle*)memoria_malloc(sizeof(fb_handle));
    if (!h) return NULL;

    h->w = largura;
    h->h = altura;
    h->px = (uint32_t*)memoria_malloc((size_t)largura * altura * sizeof(uint32_t));
    if (!h->px) { memoria_free(h); return NULL; }

    fb_limpar(h, 0xFF000000u);
    return h;
}

void fb_destruir(fb_handle* fb) {
    if (!fb) return;
    memoria_free(fb->px);
    memoria_free(fb);
}

int       fb_valido(const fb_handle* fb)  { return fb && fb->px != NULL; }
int       fb_largura(const fb_handle* fb) { return fb ? fb->w : 0; }
int       fb_altura(const fb_handle* fb)  { return fb ? fb->h : 0; }
uint32_t* fb_dados(fb_handle* fb)         { return fb ? fb->px : NULL; }
size_t    fb_pitch(const fb_handle* fb)        { return fb ? (size_t)fb->w * sizeof(uint32_t) : 0; }
size_t    fb_total_pixels(const fb_handle* fb) { return fb ? (size_t)fb->w * fb->h : 0; }

void fb_limpar(fb_handle* fb, uint32_t cor) {
    if (!fb || !fb->px) return;
    for (size_t i = 0, n = (size_t)fb->w * fb->h; i < n; i++) fb->px[i] = cor;
}

void fb_pixel(fb_handle* fb, int x, int y, uint32_t cor) {
    if (!fb || !fb->px || !dentro(fb, x, y)) return;
    fb->px[(size_t)y * fb->w + x] = cor;
}

void fb_misturar(fb_handle* fb, int x, int y, uint32_t cor) {
    if (!fb || !fb->px) return;
    px_misturar(fb, x, y, cor);
}

uint32_t fb_ler(const fb_handle* fb, int x, int y) {
    if (!fb || !fb->px || !dentro(fb, x, y)) return 0;
    return fb->px[(size_t)y * fb->w + x];
}

void fb_pixel_fx(fb_handle* fb, int32_t x, int32_t y, uint32_t cor) {
    fb_pixel(fb, fb_fx_para_int(x), fb_fx_para_int(y), cor);
}

/* ---- fb_atlas ---- */

fb_atlas_handle* fb_atlas_criar(void) {
    fb_atlas_handle* a = (fb_atlas_handle*)memoria_malloc(sizeof(fb_atlas_handle));
    if (!a) return NULL;
    a->px = NULL;
    a->w = a->h = a->fw = a->fh = 0;
    return a;
}

static void atlas_liberar(fb_atlas_handle* a) {
    memoria_free(a->px);
    a->px = NULL;
    a->w = a->h = a->fw = a->fh = 0;
}

void fb_atlas_destruir(fb_atlas_handle* a) {
    if (!a) return;
    atlas_liberar(a);
    memoria_free(a);
}

static int atlas_adotar_rgba(fb_atlas_handle* a, const uint8_t* rgba, int w, int h, uint16_t frame_w, uint16_t frame_h) {
    if (frame_w == 0) frame_w = (uint16_t)w;
    if (frame_h == 0) frame_h = (uint16_t)h;
    if (frame_w > w || frame_h > h) return 0;

    const size_t n = (size_t)w * h;
    a->px = (uint32_t*)memoria_malloc(n * sizeof(uint32_t));
    if (!a->px) return 0;

    for (size_t i = 0; i < n; i++) {
        const uint8_t* p = rgba + i * 4;
        a->px[i] = ((uint32_t)p[3] << 24) | ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
    }

    a->w = w; a->h = h; a->fw = frame_w; a->fh = frame_h;
    return 1;
}

int fb_atlas_carregar_memoria(fb_atlas_handle* a, const uint8_t* dados, size_t tamanho, uint16_t frame_w, uint16_t frame_h) {
    if (!a) return 0;
    atlas_liberar(a);

    int w, h, canais;
    uint8_t* rgba = stbi_load_from_memory(dados, (int)tamanho, &w, &h, &canais, 4);
    if (!rgba) return 0;

    const int ok = atlas_adotar_rgba(a, rgba, w, h, frame_w, frame_h);
    stbi_image_free(rgba);
    return ok;
}

int fb_atlas_carregar(fb_atlas_handle* a, const char* caminho, uint16_t frame_w, uint16_t frame_h) {
    if (!a) return 0;

    size_t tam = 0;
    uint8_t* dados = (uint8_t*)arquivo_ler_tudo_tam(caminho, &tam);
    if (!dados) return 0;

    const int ok = fb_atlas_carregar_memoria(a, dados, tam, frame_w, frame_h);
    memoria_free(dados);
    return ok;
}

int fb_atlas_valido(const fb_atlas_handle* a)       { return a && a->px != NULL; }
int fb_atlas_largura(const fb_atlas_handle* a)      { return a ? a->w : 0; }
int fb_atlas_altura(const fb_atlas_handle* a)       { return a ? a->h : 0; }
int fb_atlas_tile_largura(const fb_atlas_handle* a) { return a ? a->fw : 0; }
int fb_atlas_tile_altura(const fb_atlas_handle* a)  { return a ? a->fh : 0; }
int fb_atlas_passo(const fb_atlas_handle* a)        { return a ? a->w : 0; }
int fb_atlas_colunas(const fb_atlas_handle* a)      { return a && a->fw ? a->w / a->fw : 0; }
int fb_atlas_linhas(const fb_atlas_handle* a)       { return a && a->fh ? a->h / a->fh : 0; }

static int atlas_tile_valido(const fb_atlas_handle* a, int pw, int ph) {
    return a->px && pw >= 0 && ph >= 0 && pw < fb_atlas_colunas(a) && ph < fb_atlas_linhas(a);
}

const uint32_t* fb_atlas_tile_dados(const fb_atlas_handle* a, int pw, int ph) {
    if (!a || !atlas_tile_valido(a, pw, ph)) return NULL;
    return a->px + (size_t)ph * a->fh * a->w + (size_t)pw * a->fw;
}

void fb_atlas_desenhar(const fb_atlas_handle* a, fb_handle* fb, int pw, int ph, int x, int y, int w, int h) {
    if (!a || !fb || !fb->px || !atlas_tile_valido(a, pw, ph)) return;
    if (w == 0) w = a->fw;
    if (h == 0) h = a->fh;
    if (w <= 0 || h <= 0) return;

    const faixa cols = recortar(x, w, fb->w);
    const faixa lins = recortar(y, h, fb->h);
    if (cols.ini >= cols.fim || lins.ini >= lins.fim) return;

    const int sx = pw * a->fw, sy = ph * a->fh;
    const int escala_1x = (w == a->fw && h == a->fh);

    for (int j = lins.ini; j < lins.fim; j++) {
        const int linha_src = escala_1x ? j : (j * a->fh) / h;
        const uint32_t* origem = a->px + (size_t)(sy + linha_src) * a->w + sx;
        uint32_t* d = fb->px + (size_t)(y + j) * fb->w + x + cols.ini;

        if (escala_1x) {
            for (int i = cols.ini; i < cols.fim; i++, d++)
                *d = mesclar(*d, origem[i]);
        } else {
            int src = (cols.ini * a->fw) / w;
            int acc = (cols.ini * a->fw) % w;
            for (int i = cols.ini; i < cols.fim; i++, d++) {
                *d = mesclar(*d, origem[src]);
                acc += a->fw;
                while (acc >= w) { acc -= w; src++; }
            }
        }
    }
}

void fb_atlas_desenhar_fx(const fb_atlas_handle* a, fb_handle* fb, int pw, int ph, int32_t x, int32_t y, int w, int h) {
    fb_atlas_desenhar(a, fb, pw, ph, fb_fx_para_int(x), fb_fx_para_int(y), w, h);
}

/* ---- Primitivas ---- */

typedef struct {
    int     cx2, cy2, hw, hh;
    int32_t c, s;
} caixa_girada;

static caixa_girada caixa_criar(int x, int y, int w, int h, uint16_t angulo) {
    caixa_girada g;
    g.cx2 = 2 * x + w - 1;
    g.cy2 = 2 * y + h - 1;
    g.hw  = w - 1;
    g.hh  = h - 1;
    g.c   = trig_cosseno(angulo);
    g.s   = trig_seno(angulo);
    return g;
}

static pt caixa_vertice(const caixa_girada* g, int sx, int sy) {
    const int64_t dx = sx * g->hw, dy = sy * g->hh;
    pt p;
    p.x = (g->cx2 + (int)(((dx * g->c) - (dy * g->s)) >> TRIG_BITS)) >> 1;
    p.y = (g->cy2 + (int)(((dx * g->s) + (dy * g->c)) >> TRIG_BITS)) >> 1;
    return p;
}

static void preencher_retangulo(fb_handle* fb, int x, int y, int w, int h, uint32_t cor) {
    const uint32_t alfa = cor >> 24;
    if (alfa == 0) return;

    const faixa cols = recortar(x, w, fb->w);
    const faixa lins = recortar(y, h, fb->h);
    const int opaca = alfa == 255;

    for (int j = lins.ini; j < lins.fim; j++) {
        uint32_t* d = fb->px + (size_t)(y + j) * fb->w + x + cols.ini;
        if (opaca) {
            for (int i = cols.ini; i < cols.fim; i++) *d++ = cor;
        } else {
            for (int i = cols.ini; i < cols.fim; i++, d++) *d = mesclar(*d, cor);
        }
    }
}

static void contorno_retangulo(fb_handle* fb, int x, int y, int w, int h, uint32_t cor) {
    fb_linha(fb, x, y, x + w - 1, y, cor);
    fb_linha(fb, x, y + h - 1, x + w - 1, y + h - 1, cor);
    if (h > 2) {
        fb_linha(fb, x, y + 1, x, y + h - 2, cor);
        fb_linha(fb, x + w - 1, y + 1, x + w - 1, y + h - 2, cor);
    }
}

static inline void cobrir(int x, int* xmin, int* xmax) {
    if (x < *xmin) *xmin = x;
    if (x > *xmax) *xmax = x;
}

static void preencher_convexo(fb_handle* fb, const pt* p, int n, uint32_t cor) {
    int ymin = p[0].y, ymax = p[0].y;
    for (int i = 1; i < n; i++) {
        if (p[i].y < ymin) ymin = p[i].y;
        if (p[i].y > ymax) ymax = p[i].y;
    }
    if (ymin < 0) ymin = 0;
    if (ymax > fb->h - 1) ymax = fb->h - 1;

    for (int y = ymin; y <= ymax; y++) {
        int xmin = INT32_MAX, xmax = INT32_MIN;

        for (int i = 0; i < n; i++) {
            const pt a = p[i], b = p[(i + 1) % n];
            if (a.y == b.y) {
                if (a.y == y) { cobrir(a.x, &xmin, &xmax); cobrir(b.x, &xmin, &xmax); }
                continue;
            }
            const int lo = a.y < b.y ? a.y : b.y;
            const int hi = a.y < b.y ? b.y : a.y;
            if (y < lo || y > hi) continue;
            cobrir(a.x + (int)(((int64_t)(y - a.y) * (b.x - a.x)) / (b.y - a.y)), &xmin, &xmax);
        }

        if (xmin <= xmax) preencher_retangulo(fb, xmin, y, xmax - xmin + 1, 1, cor);
    }
}

static void desenhar_poligono(fb_handle* fb, const pt* p, int n, int preenchido, uint32_t cor) {
    if (preenchido) { preencher_convexo(fb, p, n, cor); return; }
    for (int i = 0; i < n; i++) {
        const pt a = p[i], b = p[(i + 1) % n];
        fb_linha(fb, a.x, a.y, b.x, b.y, cor);
    }
}

static int dentro_fatia(int64_t dx, int64_t dy, uint16_t ang, int64_t c, int64_t s) {
    if (ang <= TRIG_MEIA_VOLTA)
        return dy >= 0 && (dx * s - dy * c) >= 0;
    const int no_complemento = (c * dy - s * dx) >= 0 && dy <= 0;
    return !no_complemento;
}

void fb_linha(fb_handle* fb, int x0, int y0, int x1, int y1, uint32_t cor) {
    if (!fb || !fb->px) return;

    const int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    const int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        px_misturar(fb, x0, y0, cor);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void fb_retangulo(fb_handle* fb, int x, int y, int w, int h, float angulo, int preenchido, uint32_t cor) {
    if (!fb || !fb->px || w <= 0 || h <= 0) return;

    const uint16_t ang = trig_de_graus(angulo);

    if (ang == 0) {
        if (preenchido) preencher_retangulo(fb, x, y, w, h, cor);
        else            contorno_retangulo(fb, x, y, w, h, cor);
        return;
    }

    const caixa_girada g = caixa_criar(x, y, w, h, ang);
    const pt p[4] = {
        caixa_vertice(&g, -1, -1),
        caixa_vertice(&g,  1, -1),
        caixa_vertice(&g,  1,  1),
        caixa_vertice(&g, -1,  1),
    };
    desenhar_poligono(fb, p, 4, preenchido != 0, cor);
}

void fb_triangulo_retangulo(fb_handle* fb, int x, int y, int w, int h, float angulo, int preenchido, uint32_t cor) {
    if (!fb || !fb->px || w <= 0 || h <= 0) return;

    const caixa_girada g = caixa_criar(x, y, w, h, trig_de_graus(angulo));
    const pt p[3] = {
        caixa_vertice(&g, -1, -1),
        caixa_vertice(&g, -1,  1),
        caixa_vertice(&g,  1,  1),
    };
    desenhar_poligono(fb, p, 3, preenchido != 0, cor);
}

/* Círculo por spans: uma raiz inteira por linha.
 *   dentro:  d² <  r² + r
 *   buraco:  d² <  r² - r   (apenas contorno) */
static void circulo_span(fb_handle* fb, int cx, int y, int64_t dy, int a, int b,
                         int fatia, uint16_t ang, int32_t co, int32_t se, uint32_t cor) {
    if (!fatia) { preencher_retangulo(fb, cx + a, y, b - a + 1, 1, cor); return; }

    const int xa = cx + a < 0 ? 0 : cx + a;
    const int xb = cx + b > fb->w - 1 ? fb->w - 1 : cx + b;
    for (int x = xa; x <= xb; x++)
        if (dentro_fatia(x - cx, dy, ang, co, se)) px_misturar(fb, x, y, cor);
}

void fb_circulo(fb_handle* fb, int cx, int cy, int raio, float angulo, int preenchido, uint32_t cor) {
    if (!fb || raio < 0) return;
    if (raio == 0) { fb_misturar(fb, cx, cy, cor); return; }
    if (!fb->px) return;

    const uint16_t ang = trig_de_graus(angulo);

    const int64_t r2 = (int64_t)raio * raio;
    const int32_t co = trig_cosseno(ang), se = trig_seno(ang);
    const int fatia = ang != 0;

    const int y0 = cy - raio < 0 ? 0 : cy - raio;
    const int y1 = cy + raio > fb->h - 1 ? fb->h - 1 : cy + raio;

    for (int y = y0; y <= y1; y++) {
        const int64_t dy  = y - cy;
        const int64_t dy2 = dy * dy;

        const int xo = raiz_inteira(r2 + raio - 1 - dy2);
        if (xo < 0) continue;
        const int xi = preenchido ? -1 : raiz_inteira(r2 - raio - 1 - dy2);

        if (xi < 0) {
            circulo_span(fb, cx, y, dy, -xo, xo, fatia, ang, co, se, cor);
        } else {
            circulo_span(fb, cx, y, dy, -xo, -xi - 1, fatia, ang, co, se, cor);
            circulo_span(fb, cx, y, dy, xi + 1, xo, fatia, ang, co, se, cor);
        }
    }

    if (!preenchido && fatia) {
        fb_linha(fb, cx, cy, cx + raio, cy, cor);
        fb_linha(fb, cx, cy,
                 cx + (int)(((int64_t)raio * co) >> TRIG_BITS),
                 cy + (int)(((int64_t)raio * se) >> TRIG_BITS), cor);
    }
}

void fb_blit(fb_handle* dst, const fb_handle* src, int dx, int dy) {
    if (!fb_valido(dst) || !fb_valido(src)) return;

    const faixa cols = recortar(dx, src->w, dst->w);
    const faixa lins = recortar(dy, src->h, dst->h);

    for (int j = lins.ini; j < lins.fim; j++) {
        const uint32_t* origem = src->px + (size_t)j * src->w;
        uint32_t* d = dst->px + (size_t)(dy + j) * dst->w + dx + cols.ini;
        for (int i = cols.ini; i < cols.fim; i++, d++)
            *d = mesclar(*d, origem[i]);
    }
}

/* ---- Utilidades expostas ---- */

int32_t  fb_fx_de_int(int v)              { return v * FX_UM; }
int      fb_fx_para_int(int32_t v)        { return v >> FX_BITS; }
int32_t  fb_fx_mul(int32_t a, int32_t b)  { return (int32_t)(((int64_t)a * b) >> FX_BITS); }
int32_t  fb_fx_div(int32_t a, int32_t b)  { return b ? (int32_t)(((int64_t)a * FX_UM) / b) : 0; }
uint16_t fb_graus(float g)                { return trig_de_graus(g); }
int32_t  fb_seno(uint16_t a)              { return trig_seno(a); }
int32_t  fb_cosseno(uint16_t a)           { return trig_cosseno(a); }

uint32_t fb_cor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}