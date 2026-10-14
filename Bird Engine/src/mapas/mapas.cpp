#include "mapas.hpp"

namespace {

int divpiso(int a, int b) {
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

}

Mapa::~Mapa() { destruir(); }

void Mapa::destruir() {
    memoria_free(g_);
    g_ = nullptr;
    cols_ = lins_ = 0;
}

bool Mapa::alocar(uint32_t colunas, uint32_t linhas, char fundo) {
    destruir();
    const size_t n = (size_t)colunas * linhas;
    g_ = (char *)memoria_malloc(n);
    if (!g_) return false;
    for (size_t i = 0; i < n; i++) g_[i] = fundo;
    cols_ = colunas;
    lins_ = linhas;
    return true;
}

bool Mapa::criar_txt(fb_atlas *sprites, const char *txt, uint16_t pw, uint16_t ph) {
    if (!txt || pw == 0 || ph == 0) return false;

    uint32_t cols = 0, lins = 0, atual = 0;
    for (const char *p = txt; *p; p++) {
        if (*p == '\r') continue;
        if (*p == '\n') {
            if (atual > cols) cols = atual;
            atual = 0;
            lins++;
            continue;
        }
        atual++;
    }
    if (atual) {
        if (atual > cols) cols = atual;
        lins++;
    }
    if (!cols || !lins) return false;
    if (!alocar(cols, lins, ' ')) return false;

    uint32_t x = 0, y = 0;
    for (const char *p = txt; *p; p++) {
        if (*p == '\r') continue;
        if (*p == '\n') { x = 0; y++; continue; }
        g_[(size_t)y * cols_ + x++] = *p;
    }

    atlas_ = sprites;
    cw_ = pw;
    ch_ = ph;
    return true;
}

bool Mapa::criar_chunk(fb_atlas *sprites, uint32_t colunas, uint32_t linhas, uint16_t pw, uint16_t ph, char fundo) {
    if (!colunas || !linhas || pw == 0 || ph == 0) return false;
    if (!alocar(colunas, linhas, fundo)) return false;

    atlas_ = sprites;
    cw_ = pw;
    ch_ = ph;
    return true;
}

char Mapa::tile(uint32_t x, uint32_t y) const {
    if (!g_ || x >= cols_ || y >= lins_) return '\0';
    return g_[(size_t)y * cols_ + x];
}

void Mapa::definir(uint32_t x, uint32_t y, char t) {
    if (!g_ || x >= cols_ || y >= lins_) return;
    g_[(size_t)y * cols_ + x] = t;
}

void Mapa::substituir(char antigo, char novo) {
    if (!g_) return;
    for (size_t i = 0, n = (size_t)cols_ * lins_; i < n; i++)
        if (g_[i] == antigo) g_[i] = novo;
}

void Mapa::substituir(char antigo, char novo, uint32_t x, uint32_t y) {
    if (tile(x, y) == antigo) definir(x, y, novo);
}

Tiles::Tiles() {
    for (int i = 0; i < 256; i++) idx_[i] = -1;
    defs_ = vetor_criar(sizeof(Def), 8);
    vetor_definir_limite(&defs_, MAXIMO_SPRITES);
}

Tiles::~Tiles() { vetor_liberar(&defs_); }

Tiles::Def *Tiles::obter(char t) {
    const uint8_t k = (uint8_t)t;
    if (idx_[k] >= 0) return (Def *)vetor_get(&defs_, idx_[k]);

    const int32_t antes = vetor_tam(&defs_);
    Def novo;
    vetor_add(&defs_, &novo);
    if (vetor_tam(&defs_) == antes) return nullptr;

    idx_[k] = (int16_t)antes;
    return (Def *)vetor_get(&defs_, antes);
}

const Tiles::Def *Tiles::achar(char t) const {
    const int16_t i = idx_[(uint8_t)t];
    if (i < 0) return nullptr;
    return (const Def *)vetor_get(&defs_, i);
}

void Tiles::sprites(char t, uint16_t coluna, uint16_t linha) {
    Def *d = obter(t);
    if (!d) return;
    d->col = coluna;
    d->lin = linha;
    d->desenha = true;
}

void Tiles::ampliar(char t, uint16_t w, uint16_t h) {
    if (Def *d = obter(t)) { d->w = w; d->h = h; }
}

void Tiles::cor(char t, fb_cores c) {
    if (Def *d = obter(t)) d->c = c;
}

void Tiles::angulo(char t, float a) {
    if (Def *d = obter(t)) d->ang = a;
}

void Tiles::alpha(char t, uint8_t a) {
    if (Def *d = obter(t)) d->a = a;
}

void Tiles::espelhar_x(char t, bool espelhar) {
    if (Def *d = obter(t)) d->fx = espelhar;
}

void Tiles::espelhar_y(char t, bool espelhar) {
    if (Def *d = obter(t)) d->fy = espelhar;
}

void Tiles::colisor(char t, const char *mascara, uint16_t mascara_w, uint16_t mascara_h) {
    Def *d = obter(t);
    if (!d) return;
    d->solido = true;
    d->mascara = mascara;
    d->mw = mascara_w;
    d->mh = mascara_h;
}

void Tiles::desenhar(frame_buffer &fb) const {
    if (!g_ || !atlas_ || !fb.valido() || cw_ == 0 || ch_ == 0) return;

    int cx0 = divpiso(-ox_, cw_), cx1 = divpiso(fb.largura() - 1 - ox_, cw_);
    int cy0 = divpiso(-oy_, ch_), cy1 = divpiso(fb.altura()  - 1 - oy_, ch_);
    if (cx0 < 0) cx0 = 0;
    if (cy0 < 0) cy0 = 0;
    if (cx1 > (int)cols_ - 1) cx1 = (int)cols_ - 1;
    if (cy1 > (int)lins_ - 1) cy1 = (int)lins_ - 1;

    for (int cy = cy0; cy <= cy1; cy++) {
        for (int cx = cx0; cx <= cx1; cx++) {
            const Def *d = achar(g_[(size_t)cy * cols_ + cx]);
            if (!d || !d->desenha) continue;

            Obj o;
            o.criar(atlas_, d->w ? d->w : cw_, d->h ? d->h : ch_,
                    ox_ + cx * cw_, oy_ + cy * ch_, d->col, d->lin);
            
            o.colorir(true);
            o.cor(d->c);
            o.alpha(d->a);
            o.angulo(d->ang);
            o.espelhar_x(d->fx);
            o.espelhar_y(d->fy);
            o.desenhar(fb);
        }
    }
}

bool Tiles::colidir(const AABB_2d *obj, int32_t dx, int32_t dy, bool eixo_x, int filtro) const {
    if (!obj || !g_ || cw_ == 0 || ch_ == 0) return false;

    const int32_t x0 = obj->x() + (dx < 0 ? dx : 0);
    const int32_t y0 = obj->y() + (dy < 0 ? dy : 0);
    const int32_t x1 = obj->x() + obj->largura() + (dx > 0 ? dx : 0);
    const int32_t y1 = obj->y() + obj->altura()  + (dy > 0 ? dy : 0);

    int cx0 = divpiso(x0 - ox_, cw_), cx1 = divpiso(x1 - 1 - ox_, cw_);
    int cy0 = divpiso(y0 - oy_, ch_), cy1 = divpiso(y1 - 1 - oy_, ch_);
    if (cx0 < 0) cx0 = 0;
    if (cy0 < 0) cy0 = 0;
    if (cx1 > (int)cols_ - 1) cx1 = (int)cols_ - 1;
    if (cy1 > (int)lins_ - 1) cy1 = (int)lins_ - 1;

    for (int cy = cy0; cy <= cy1; cy++) {
        for (int cx = cx0; cx <= cx1; cx++) {
            const char t = g_[(size_t)cy * cols_ + cx];
            if (filtro >= 0 && (uint8_t)t != (uint8_t)filtro) continue;

            const Def *d = achar(t);
            if (!d || !d->solido) continue;

            AABB_2d c;
            c.criar(nullptr, d->mascara, d->mw ? d->mw : cw_, d->mh ? d->mh : ch_);
            c.posicionar(ox_ + cx * cw_, oy_ + cy * ch_);

            if (eixo_x ? obj->colidiu_x(&c, dx) : obj->colidiu_y(&c, dy)) return true;
        }
    }
    return false;
}

bool Tiles::colidio_x(const AABB_2d *obj, int32_t dx) const { return colidir(obj, dx, 0, true, -1); }
bool Tiles::colidio_y(const AABB_2d *obj, int32_t dy) const { return colidir(obj, 0, dy, false, -1); }

bool Tiles::colidio_x(char t, const AABB_2d *obj, int32_t dx) const { return colidir(obj, dx, 0, true, (uint8_t)t); }
bool Tiles::colidio_y(char t, const AABB_2d *obj, int32_t dy) const { return colidir(obj, 0, dy, false, (uint8_t)t); }

/* ---- API C ---- */

namespace {
inline Mapa*        M(mapa_handle* h)          { return reinterpret_cast<Mapa*>(h); }
inline const Mapa*  M(const mapa_handle* h)    { return reinterpret_cast<const Mapa*>(h); }
inline Tiles*       T(tiles_handle* h)         { return reinterpret_cast<Tiles*>(h); }
inline const Tiles* T(const tiles_handle* h)   { return reinterpret_cast<const Tiles*>(h); }
inline const AABB_2d*  BB(const aabb_2d_handle* h)   { return reinterpret_cast<const AABB_2d*>(h); }
inline fb_atlas*    AT(fb_atlas_handle* h)     { return h ? &h->at : nullptr; }
}

extern "C" {

/* ---- Mapa ---- */
mapa_handle* mapa_novo(void)                    { return reinterpret_cast<mapa_handle*>(new Mapa); }
void mapa_destruir(mapa_handle* m)              { delete M(m); }
int  mapa_criar_txt(mapa_handle* m, fb_atlas_handle* sprites, const char* txt, uint16_t pw, uint16_t ph) {
    return m && M(m)->criar_txt(AT(sprites), txt, pw, ph);
}
int  mapa_criar_chunk(mapa_handle* m, fb_atlas_handle* sprites, uint32_t colunas, uint32_t linhas, uint16_t pw, uint16_t ph, int fundo) {
    return m && M(m)->criar_chunk(AT(sprites), colunas, linhas, pw, ph, (char)fundo);
}
void mapa_liberar(mapa_handle* m)               { if (m) M(m)->destruir(); }
void mapa_posicionar(mapa_handle* m, int32_t x, int32_t y) { if (m) M(m)->posicionar(x, y); }
void mapa_mover(mapa_handle* m, int32_t x, int32_t y)      { if (m) M(m)->mover(x, y); }
int32_t  mapa_x(const mapa_handle* m)           { return m ? M(m)->x() : 0; }
int32_t  mapa_y(const mapa_handle* m)           { return m ? M(m)->y() : 0; }
uint32_t mapa_colunas(const mapa_handle* m)     { return m ? M(m)->colunas() : 0; }
uint32_t mapa_linhas(const mapa_handle* m)      { return m ? M(m)->linhas() : 0; }
uint16_t mapa_tile_largura(const mapa_handle* m){ return m ? M(m)->tile_largura() : 0; }
uint16_t mapa_tile_altura(const mapa_handle* m) { return m ? M(m)->tile_altura() : 0; }
int  mapa_tile(const mapa_handle* m, uint32_t x, uint32_t y)         { return m ? (uint8_t)M(m)->tile(x, y) : 0; }
void mapa_definir(mapa_handle* m, uint32_t x, uint32_t y, int t)     { if (m) M(m)->definir(x, y, (char)t); }
void mapa_substituir_tudo(mapa_handle* m, int antigo, int novo)      { if (m) M(m)->substituir((char)antigo, (char)novo); }
void mapa_substituir_em(mapa_handle* m, int antigo, int novo, uint32_t x, uint32_t y) {
    if (m) M(m)->substituir((char)antigo, (char)novo, x, y);
}

/* ---- Tiles ---- */
tiles_handle* tiles_novo(void)                  { return reinterpret_cast<tiles_handle*>(new Tiles); }
void tiles_destruir(tiles_handle* t)            { delete T(t); }
mapa_handle* tiles_como_mapa(tiles_handle* t)   { return t ? reinterpret_cast<mapa_handle*>(static_cast<Mapa*>(T(t))) : nullptr; }

void tiles_sprites(tiles_handle* t, int tile, uint16_t coluna, uint16_t linha) { if (t) T(t)->sprites((char)tile, coluna, linha); }
void tiles_ampliar(tiles_handle* t, int tile, uint16_t w, uint16_t h)          { if (t) T(t)->ampliar((char)tile, w, h); }
void tiles_cor(tiles_handle* t, int tile, uint32_t cor)    { if (t) T(t)->cor((char)tile, fb_cores::desempacotar(cor)); }
void tiles_angulo(tiles_handle* t, int tile, float graus)  { if (t) T(t)->angulo((char)tile, graus); }
void tiles_alpha(tiles_handle* t, int tile, uint8_t a)     { if (t) T(t)->alpha((char)tile, a); }
void tiles_espelhar_x(tiles_handle* t, int tile, int e)    { if (t) T(t)->espelhar_x((char)tile, e != 0); }
void tiles_espelhar_y(tiles_handle* t, int tile, int e)    { if (t) T(t)->espelhar_y((char)tile, e != 0); }
void tiles_colisor(tiles_handle* t, int tile, const char* mascara, uint16_t mw, uint16_t mh) {
    if (t) T(t)->colisor((char)tile, mascara, mw, mh);
}

int tiles_colidio_x(const tiles_handle* t, const aabb_2d_handle* obj, int32_t dx) { return t && T(t)->colidio_x(BB(obj), dx); }
int tiles_colidio_y(const tiles_handle* t, const aabb_2d_handle* obj, int32_t dy) { return t && T(t)->colidio_y(BB(obj), dy); }
int tiles_colidio_tile_x(const tiles_handle* t, int tile, const aabb_2d_handle* obj, int32_t dx) {
    return t && T(t)->colidio_x((char)tile, BB(obj), dx);
}
int tiles_colidio_tile_y(const tiles_handle* t, int tile, const aabb_2d_handle* obj, int32_t dy) {
    return t && T(t)->colidio_y((char)tile, BB(obj), dy);
}

void tiles_desenhar(const tiles_handle* t, fb_handle* fb) { if (t && fb) T(t)->desenhar(fb->fb); }

}