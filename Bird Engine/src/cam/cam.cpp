#include "cam.hpp"

namespace {

constexpr int32_t LIMITE_LADO = 4096;   // maior lado permitido do buffer interno
constexpr int32_t FOLGA       = 4;      // margem de amostragem
constexpr int32_t SOBRA       = 32;     // folga extra ao crescer (evita realocar a cada frame no zoom animado)
constexpr int32_t HISTERESE   = 96;     // so encolhe o buffer se sobrar mais que isso

int32_t arredondar(float v) {
    return v >= 0.0f ? (int32_t)(v + 0.5f) : -(int32_t)(-v + 0.5f);
}

int64_t arredondar64(float v) {
    return v >= 0.0f ? (int64_t)(v + 0.5f) : -(int64_t)(-v + 0.5f);
}

int32_t piso(float v) {
    const int32_t i = (int32_t)v;
    return (float)i > v ? i - 1 : i;
}

float modulo(float v) { return v < 0.0f ? -v : v; }

float normalizar_angulo(float a) {
    if (a >= 0.0f && a < 360.0f) return a;
    a -= (float)piso(a / 360.0f) * 360.0f;
    if (a < 0.0f || a >= 360.0f) a = 0.0f;
    return a;
}

float aproximar(float atual, float alvo, uint8_t suavidade, float delta) {
    if (suavidade == 0) return alvo;

    const float k = ((256 - suavidade) / 256.0f) * (delta * 60.0f);
    if (k >= 1.0f) return alvo;
    if (k <= 0.0f) return atual;

    const float novo = atual + (alvo - atual) * k;
    const float resto = alvo - novo;
    return (resto < 0.01f && resto > -0.01f) ? alvo : novo;
}

// Mantem o centro dentro de [ini, fim] considerando a meia-extensao visivel.
float prender_centro(float c, int32_t ini, int32_t fim, float meio) {
    const float lo = (float)ini + meio, hi = (float)fim - meio;
    if (lo >= hi) return ((float)ini + (float)fim) * 0.5f;
    return c < lo ? lo : (c > hi ? hi : c);
}

inline void escrever(frame_buffer &janela, uint32_t *d, int x, int y, uint32_t p) {
    if ((p >> 24) == 255) *d = p;
    else janela.misturar(x, y, fb_cores::desempacotar(p));
}

// Transformacao tela -> mundo em ponto fixo.
// Para o pixel de tela (x, y): u2 = 2x+1-w, v2 = 2y+1-h (coordenadas dobradas, relativas ao centro).
//   mundo_x = cx + (u2*ca + v2*sa) / 65536
//   mundo_y = cy + (v2*ca - u2*sa) / 65536
// onde ca = cos/zoom e sa = sen/zoom (Q15), cx/cy em Q16.
struct Transf { int64_t cx, cy; int32_t ca, sa; };

Transf montar(float cx, float cy, float z, uint16_t ang) {
    Transf t;
    t.cx = arredondar64(cx * 65536.0f);
    t.cy = arredondar64(cy * 65536.0f);
    t.ca = (int32_t)arredondar64((float)fb_trig::cosseno(ang) / z);
    t.sa = (int32_t)arredondar64((float)fb_trig::seno(ang) / z);
    return t;
}

inline void projetar(const Transf &t, int32_t u2, int32_t v2, int64_t &x, int64_t &y) {
    x = t.cx + (int64_t)u2 * t.ca + (int64_t)v2 * t.sa;
    y = t.cy - (int64_t)u2 * t.sa + (int64_t)v2 * t.ca;
}

}

bool Camera::criar(uint16_t largura, uint16_t altura, int32_t x, int32_t y) {
    destruir();
    if (largura == 0 || altura == 0) return false;

    w_ = largura;
    h_ = altura;
    cx_ = (float)x + (float)largura * 0.5f;
    cy_ = (float)y + (float)altura * 0.5f;

    if (!garantir_buffer()) { destruir(); return false; }
    aplicar();
    preparar_quadro();
    return true;
}

void Camera::destruir() {
    view_ = frame_buffer();
    cx_ = cy_ = 0.0f;
    w_ = h_ = 0;
    ang_ = 0.0f;
    opaca_ = true;

    zoom_ = zoom_alvo_ = 1.0f;
    zoom_min_ = 0.25f;
    zoom_max_ = 8.0f;
    zoom_suave_ = 0;
    zoom_alvo_ativo_ = false;

    ox_ = oy_ = 0;
    bcx_ = bcy_ = 0;
    ca_ = 32768;
    sa_ = 0;
    reto_ = true;

    alvo_ = nullptr;
    alvo_dx_ = alvo_dy_ = 0;
    alvo_suave_ = 0;
    travada_ = false;
    lx0_ = ly0_ = lx1_ = ly1_ = 0;
}

void Camera::extensao(float &ex, float &ey) const {
    const float c = modulo((float)fb_trig::cosseno(ang16())) / (float)fb_trig::UM;
    const float s = modulo((float)fb_trig::seno(ang16()))    / (float)fb_trig::UM;
    ex = (c * (float)w_ + s * (float)h_) / zoom_;
    ey = (s * (float)w_ + c * (float)h_) / zoom_;
}

bool Camera::reto() const {
    return ang16() == 0 && arredondar64(32768.0f / zoom_) == (int64_t)fb_trig::UM;
}

void Camera::aplicar() {
    if (zoom_ < zoom_min_) zoom_ = zoom_min_;
    if (zoom_ > zoom_max_) zoom_ = zoom_max_;

    if (travada_) {
        float ex, ey;
        extensao(ex, ey);
        cx_ = prender_centro(cx_, lx0_, lx1_, ex * 0.5f);
        cy_ = prender_centro(cy_, ly0_, ly1_, ey * 0.5f);
    }
}

int32_t Camera::x() const { return arredondar(cx_ - 0.5f * (float)w_ / zoom_); }
int32_t Camera::y() const { return arredondar(cy_ - 0.5f * (float)h_ / zoom_); }
int32_t Camera::largura_mundo() const { return arredondar((float)w_ / zoom_); }
int32_t Camera::altura_mundo()  const { return arredondar((float)h_ / zoom_); }

// ---------------------------------------------------------------- angulo

void Camera::angulo(float a) {
    ang_ = normalizar_angulo(a);
    aplicar();
}

void Camera::girar(float delta) {
    ang_ = normalizar_angulo(ang_ + delta);
    aplicar();
}

// ---------------------------------------------------------------- zoom

void Camera::zoom(float z) {
    zoom_ = z;
    zoom_alvo_ativo_ = false;
    aplicar();
}

void Camera::zoom_relativo(float fator) {
    zoom(zoom_ * fator);
}

void Camera::zoom_em(float z, int32_t tela_x, int32_t tela_y) {
    const float c = (float)fb_trig::cosseno(ang16()) / (float)fb_trig::UM;
    const float s = (float)fb_trig::seno(ang16())    / (float)fb_trig::UM;
    const float u = (float)tela_x + 0.5f - (float)w_ * 0.5f;
    const float v = (float)tela_y + 0.5f - (float)h_ * 0.5f;

    // Ponto do mundo que esta sob o pixel da tela, com o zoom atual.
    const float wx = cx_ + (u * c + v * s) / zoom_;
    const float wy = cy_ + (v * c - u * s) / zoom_;

    zoom_ = z;
    zoom_alvo_ativo_ = false;
    if (zoom_ < zoom_min_) zoom_ = zoom_min_;
    if (zoom_ > zoom_max_) zoom_ = zoom_max_;

    // Recentra para que o mesmo ponto do mundo continue sob o mesmo pixel.
    cx_ = wx - (u * c + v * s) / zoom_;
    cy_ = wy - (v * c - u * s) / zoom_;
    aplicar();
}

void Camera::zoom_suave(float alvo, uint8_t suavidade) {
    if (alvo < zoom_min_) alvo = zoom_min_;
    if (alvo > zoom_max_) alvo = zoom_max_;

    if (suavidade == 0) { zoom(alvo); return; }
    zoom_alvo_ = alvo;
    zoom_suave_ = suavidade;
    zoom_alvo_ativo_ = true;
}

void Camera::limites_zoom(float minimo, float maximo) {
    if (minimo <= 0.01f) minimo = 0.01f;
    if (maximo < minimo) { const float t = minimo; minimo = maximo; maximo = t; }
    if (minimo <= 0.01f) minimo = 0.01f;
    if (maximo < minimo) maximo = minimo;

    zoom_min_ = minimo;
    zoom_max_ = maximo;
    if (zoom_alvo_ < zoom_min_) zoom_alvo_ = zoom_min_;
    if (zoom_alvo_ > zoom_max_) zoom_alvo_ = zoom_max_;
    aplicar();
}

// ---------------------------------------------------------------- posicao

void Camera::posicionar(int32_t x, int32_t y) {
    cx_ = (float)x + 0.5f * (float)w_ / zoom_;
    cy_ = (float)y + 0.5f * (float)h_ / zoom_;
    aplicar();
}

void Camera::mover(int32_t dx, int32_t dy) {
    cx_ += (float)dx;
    cy_ += (float)dy;
    aplicar();
}

void Camera::centralizar(int32_t mundo_x, int32_t mundo_y) {
    cx_ = (float)mundo_x;
    cy_ = (float)mundo_y;
    aplicar();
}

void Camera::mover_suave(int32_t mundo_x, int32_t mundo_y, uint8_t suavidade, float delta) {
    cx_ = aproximar(cx_, (float)mundo_x, suavidade, delta);
    cy_ = aproximar(cy_, (float)mundo_y, suavidade, delta);
    aplicar();
}

void Camera::centralizar_mundo(const Mapa &mapa, uint16_t pw, uint16_t ph) {
    if (pw == 0) pw = mapa.tile_largura();
    if (ph == 0) ph = mapa.tile_altura();

    centralizar(mapa.x() + (int32_t)(mapa.colunas() * pw) / 2,
                mapa.y() + (int32_t)(mapa.linhas()  * ph) / 2);
}

void Camera::centro_do_alvo(int32_t &cx, int32_t &cy) const {
    cx = alvo_->x() + (int32_t)alvo_->largura() / 2 + alvo_dx_;
    cy = alvo_->y() + (int32_t)alvo_->altura()  / 2 + alvo_dy_;
}

void Camera::ancorar(const Obj *alvo, int32_t desloc_x, int32_t desloc_y, uint8_t suavidade) {
    alvo_ = alvo;
    alvo_dx_ = desloc_x;
    alvo_dy_ = desloc_y;
    alvo_suave_ = suavidade;

    if (!alvo_) return;
    int32_t cx, cy;
    centro_do_alvo(cx, cy);
    centralizar(cx, cy);
}

void Camera::soltar() { alvo_ = nullptr; }

void Camera::atualizar(float delta) {
    if (zoom_alvo_ativo_) {
        zoom_ = aproximar(zoom_, zoom_alvo_, zoom_suave_, delta);
        if (zoom_ == zoom_alvo_) zoom_alvo_ativo_ = false;
    }

    if (!alvo_) { aplicar(); return; }

    int32_t cx, cy;
    centro_do_alvo(cx, cy);
    mover_suave(cx, cy, alvo_suave_, delta);
}

void Camera::travar_mapa(const Mapa *mapa, uint16_t tile_w, uint16_t tile_h) {
    if (!mapa) { destravar(); return; }
    if (tile_w == 0) tile_w = mapa->tile_largura();
    if (tile_h == 0) tile_h = mapa->tile_altura();

    travar_limites(mapa->x(), mapa->y(),
                   mapa->x() + (int32_t)(mapa->colunas() * tile_w),
                   mapa->y() + (int32_t)(mapa->linhas()  * tile_h));
}

void Camera::travar_limites(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    lx0_ = x0 < x1 ? x0 : x1;
    lx1_ = x0 < x1 ? x1 : x0;
    ly0_ = y0 < y1 ? y0 : y1;
    ly1_ = y0 < y1 ? y1 : y0;
    travada_ = true;
    aplicar();
}

void Camera::destravar() { travada_ = false; }

// ---------------------------------------------------------------- conversoes

void Camera::mundo_para_tela(int32_t mundo_x, int32_t mundo_y, int32_t &tela_x, int32_t &tela_y) const {
    if (reto()) {
        const Transf t = montar(cx_, cy_, 1.0f, 0);
        tela_x = mundo_x - (int32_t)((t.cx + (int64_t)(1 - (int32_t)w_) * 32768) >> 16);
        tela_y = mundo_y - (int32_t)((t.cy + (int64_t)(1 - (int32_t)h_) * 32768) >> 16);
        return;
    }

    const float c = (float)fb_trig::cosseno(ang16()) / (float)fb_trig::UM;
    const float s = (float)fb_trig::seno(ang16())    / (float)fb_trig::UM;
    const float dx = (float)mundo_x + 0.5f - cx_;
    const float dy = (float)mundo_y + 0.5f - cy_;

    tela_x = piso((dx * c - dy * s) * zoom_ + (float)w_ * 0.5f);
    tela_y = piso((dx * s + dy * c) * zoom_ + (float)h_ * 0.5f);
}

void Camera::tela_para_mundo(int32_t tela_x, int32_t tela_y, int32_t &mundo_x, int32_t &mundo_y) const {
    const Transf t = montar(cx_, cy_, zoom_, ang16());
    int64_t x, y;
    projetar(t, 2 * tela_x + 1 - (int32_t)w_, 2 * tela_y + 1 - (int32_t)h_, x, y);
    mundo_x = (int32_t)(x >> 16);
    mundo_y = (int32_t)(y >> 16);
}

// ---------------------------------------------------------------- buffer interno

void Camera::tamanho_necessario(int32_t &nw, int32_t &nh) const {
    if (reto()) { nw = w_; nh = h_; return; }

    float ex, ey;
    extensao(ex, ey);
    nw = piso(ex) + 1 + FOLGA;
    nh = piso(ey) + 1 + FOLGA;
    if (nw > LIMITE_LADO) nw = LIMITE_LADO;
    if (nh > LIMITE_LADO) nh = LIMITE_LADO;
}

bool Camera::garantir_buffer() {
    int32_t nw, nh;
    tamanho_necessario(nw, nh);
    const bool exato = reto();

    if (view_.valido()) {
        const int32_t aw = view_.largura(), ah = view_.altura();
        const int32_t lim = exato ? 1 : HISTERESE;
        const bool falta = nw > aw || nh > ah;
        const bool sobra = aw - nw >= lim || ah - nh >= lim;
        if (!falta && !sobra) return true;
    }

    int32_t rw = nw, rh = nh;
    if (!exato) {
        rw = (nw + SOBRA + 15) & ~15;
        rh = (nh + SOBRA + 15) & ~15;
        if (rw > LIMITE_LADO) rw = LIMITE_LADO;
        if (rh > LIMITE_LADO) rh = LIMITE_LADO;
    }

    frame_buffer novo(rw, rh);
    if (!novo.valido()) return view_.valido();   // sem memoria: mantem o buffer atual
    view_ = static_cast<frame_buffer &&>(novo);
    return true;
}

// Congela a transformacao do frame: origem do buffer, centro e matriz de rotacao/zoom.
void Camera::preparar_quadro() {
    if (!view_.valido()) return;

    const int32_t bw = view_.largura(), bh = view_.altura();
    const Transf t = montar(cx_, cy_, zoom_, ang16());

    reto_ = reto();
    ca_ = t.ca;
    sa_ = t.sa;

    if (reto_) {
        // 1:1: alinha em pixel inteiro (arredonda a posicao da camera, sem tremer).
        ox_ = (int32_t)((t.cx - (int64_t)bw * 32768 + 32768) >> 16);
        oy_ = (int32_t)((t.cy - (int64_t)bh * 32768 + 32768) >> 16);
        bcx_ = (int64_t)bw * 32768;
        bcy_ = (int64_t)bh * 32768;
    } else {
        ox_ = (int32_t)((t.cx - (int64_t)bw * 32768) >> 16);
        oy_ = (int32_t)((t.cy - (int64_t)bh * 32768) >> 16);
        bcx_ = t.cx - (int64_t)ox_ * 65536;
        bcy_ = t.cy - (int64_t)oy_ * 65536;
    }
}

// ---------------------------------------------------------------- desenho

void Camera::iniciar(fb_cores fundo) {
    if (w_ == 0) return;

    aplicar();
    if (!garantir_buffer()) return;
    preparar_quadro();

    view_.limpar(fundo);
    opaca_ = fundo.a == 255;
}

void Camera::apresentar(frame_buffer &janela, int sx, int sy) const {
    if (!view_.valido() || !janela.valido()) return;

    const int w = w_, h = h_;
    const int jw = janela.largura(), jh = janela.altura();
    const int bw = view_.largura(), bh = view_.altura();

    const int x0 = sx < 0 ? -sx : 0;
    const int y0 = sy < 0 ? -sy : 0;
    const int x1 = sx + w > jw ? jw - sx : w;
    const int y1 = sy + h > jh ? jh - sy : h;
    if (x0 >= x1 || y0 >= y1) return;

    const uint32_t *src = view_.dados();
    uint32_t *dst = janela.dados();

    // Zoom 1 e sem rotacao: copia direta por linhas.
    if (reto_) {
        const int kx = (int)((bcx_ + (int64_t)(1 - w) * 32768) >> 16);
        const int ky = (int)((bcy_ + (int64_t)(1 - h) * 32768) >> 16);

        const int a0 = x0 > -kx ? x0 : -kx;
        const int a1 = x1 < bw - kx ? x1 : bw - kx;
        const int b0 = y0 > -ky ? y0 : -ky;
        const int b1 = y1 < bh - ky ? y1 : bh - ky;
        if (a0 >= a1 || b0 >= b1) return;

        for (int y = b0; y < b1; y++) {
            const uint32_t *o = src + (size_t)(y + ky) * bw;
            uint32_t *d = dst + (size_t)(sy + y) * jw + sx;

            if (opaca_) {
                memoria_copia(d + a0, o + (a0 + kx), (size_t)(a1 - a0) * sizeof(uint32_t));
                continue;
            }
            for (int x = a0; x < a1; x++) escrever(janela, d + x, sx + x, sy + y, o[x + kx]);
        }
        return;
    }

    // Caso geral (zoom e/ou rotacao): para cada pixel da janela, acha o pixel do buffer.
    const Transf t = { bcx_, bcy_, ca_, sa_ };
    const int64_t pa = 2 * (int64_t)ca_, pb = 2 * (int64_t)sa_;

    for (int y = y0; y < y1; y++) {
        int64_t bx, by;
        projetar(t, 2 * x0 + 1 - w, 2 * y + 1 - h, bx, by);
        uint32_t *d = dst + (size_t)(sy + y) * jw + sx;

        for (int x = x0; x < x1; x++, bx += pa, by -= pb) {
            const int rx = (int)(bx >> 16);
            const int ry = (int)(by >> 16);
            if ((unsigned)rx >= (unsigned)bw || (unsigned)ry >= (unsigned)bh) continue;

            escrever(janela, d + x, sx + x, sy + y, src[(size_t)ry * bw + rx]);
        }
    }
}

/* ---- API C ---- */

namespace {
inline Camera*       CM(camera_handle* h)         { return reinterpret_cast<Camera*>(h); }
inline const Camera* CM(const camera_handle* h)   { return reinterpret_cast<const Camera*>(h); }
inline const Mapa*   MP(const mapa_handle* h)     { return reinterpret_cast<const Mapa*>(h); }
inline Obj*          OB(obj_handle* h)            { return reinterpret_cast<Obj*>(h); }
inline const Obj*    OB(const obj_handle* h)      { return reinterpret_cast<const Obj*>(h); }
inline Tiles*        TL(tiles_handle* h)          { return reinterpret_cast<Tiles*>(h); }
}

extern "C" {

camera_handle* camera_criar(uint16_t largura, uint16_t altura, int32_t x, int32_t y) {
    Camera* c = new Camera;
    if (!c->criar(largura, altura, x, y)) { delete c; return nullptr; }
    return reinterpret_cast<camera_handle*>(c);
}
void camera_destruir(camera_handle* c)          { if (c) { CM(c)->destruir(); delete CM(c); } }
int  camera_valida(const camera_handle* c)      { return c && CM(c)->valida(); }

int32_t  camera_x(const camera_handle* c)             { return c ? CM(c)->x() : 0; }
int32_t  camera_y(const camera_handle* c)             { return c ? CM(c)->y() : 0; }
int32_t  camera_largura_mundo(const camera_handle* c) { return c ? CM(c)->largura_mundo() : 0; }
int32_t  camera_altura_mundo(const camera_handle* c)  { return c ? CM(c)->altura_mundo() : 0; }
uint16_t camera_largura(const camera_handle* c)       { return c ? CM(c)->largura() : 0; }
uint16_t camera_altura(const camera_handle* c)        { return c ? CM(c)->altura() : 0; }

float camera_angulo(const camera_handle* c)                  { return c ? CM(c)->angulo() : 0.0f; }
void  camera_angulo_definir(camera_handle* c, float graus)   { if (c) CM(c)->angulo(graus); }
void  camera_girar(camera_handle* c, float delta)            { if (c) CM(c)->girar(delta); }
float camera_zoom(const camera_handle* c)                    { return c ? CM(c)->zoom() : 1.0f; }
void  camera_zoom_definir(camera_handle* c, float z)         { if (c) CM(c)->zoom(z); }
void  camera_zoom_relativo(camera_handle* c, float fator)    { if (c) CM(c)->zoom_relativo(fator); }
void  camera_zoom_em(camera_handle* c, float z, int32_t tx, int32_t ty) { if (c) CM(c)->zoom_em(z, tx, ty); }
void  camera_zoom_suave(camera_handle* c, float alvo, uint8_t suavidade) { if (c) CM(c)->zoom_suave(alvo, suavidade); }
void  camera_limites_zoom(camera_handle* c, float mn, float mx)          { if (c) CM(c)->limites_zoom(mn, mx); }

void camera_posicionar(camera_handle* c, int32_t x, int32_t y)     { if (c) CM(c)->posicionar(x, y); }
void camera_mover(camera_handle* c, int32_t dx, int32_t dy)        { if (c) CM(c)->mover(dx, dy); }
void camera_centralizar(camera_handle* c, int32_t x, int32_t y)    { if (c) CM(c)->centralizar(x, y); }
void camera_mover_suave(camera_handle* c, int32_t x, int32_t y, uint8_t suavidade, float delta) {
    if (c) CM(c)->mover_suave(x, y, suavidade, delta);
}
void camera_centralizar_mundo(camera_handle* c, const mapa_handle* mapa, uint16_t pw, uint16_t ph) {
    if (c && mapa) CM(c)->centralizar_mundo(*MP(mapa), pw, ph);
}

void camera_ancorar(camera_handle* c, const obj_handle* alvo, int32_t dx, int32_t dy, uint8_t suavidade) {
    if (c) CM(c)->ancorar(OB(alvo), dx, dy, suavidade);
}
void camera_soltar(camera_handle* c)                       { if (c) CM(c)->soltar(); }
void camera_atualizar(camera_handle* c, float delta)       { if (c) CM(c)->atualizar(delta); }
void camera_travar_mapa(camera_handle* c, const mapa_handle* mapa, uint16_t tw, uint16_t th) {
    if (c) CM(c)->travar_mapa(MP(mapa), tw, th);
}
void camera_travar_limites(camera_handle* c, int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    if (c) CM(c)->travar_limites(x0, y0, x1, y1);
}
void camera_destravar(camera_handle* c)                    { if (c) CM(c)->destravar(); }

void camera_mundo_para_tela(const camera_handle* c, int32_t mx, int32_t my, int32_t* tx, int32_t* ty) {
    int32_t a = 0, b = 0;
    if (c) CM(c)->mundo_para_tela(mx, my, a, b);
    if (tx) *tx = a;
    if (ty) *ty = b;
}
void camera_tela_para_mundo(const camera_handle* c, int32_t tx, int32_t ty, int32_t* mx, int32_t* my) {
    int32_t a = 0, b = 0;
    if (c) CM(c)->tela_para_mundo(tx, ty, a, b);
    if (mx) *mx = a;
    if (my) *my = b;
}

void camera_iniciar(camera_handle* c, uint32_t fundo)      { if (c) CM(c)->iniciar(fb_cores::desempacotar(fundo)); }
int  camera_visivel(const camera_handle* c, int32_t x, int32_t y, int32_t w, int32_t h) { return c && CM(c)->visivel(x, y, w, h); }
void camera_desenhar_obj(camera_handle* c, obj_handle* o)       { if (c && o) CM(c)->desenhar(*OB(o)); }
void camera_desenhar_tiles(camera_handle* c, tiles_handle* t)   { if (c && t) CM(c)->desenhar(*TL(t)); }
void camera_apresentar(const camera_handle* c, fb_handle* janela, int sx, int sy) {
    if (c && janela) CM(c)->apresentar(janela->fb, sx, sy);
}

uint32_t* camera_viewport_dados(camera_handle* c)   { return c ? CM(c)->viewport().dados() : nullptr; }
int camera_viewport_largura(camera_handle* c)       { return c ? CM(c)->viewport().largura() : 0; }
int camera_viewport_altura(camera_handle* c)        { return c ? CM(c)->viewport().altura() : 0; }

}