#include "frame_buffer.hpp"
#include "stb_image.h"

namespace {

struct faixa { int ini, fim; };

faixa recortar(int pos, int tam, int limite) {
    return { pos < 0 ? -pos : 0, pos + tam > limite ? limite - pos : tam };
}

}

frame_buffer::frame_buffer(int largura, int altura) {
    if (largura <= 0 || altura <= 0) return;

    w_ = largura;
    h_ = altura;
    px_ = (uint32_t*)memoria_malloc(total_pixels() * sizeof(uint32_t));
    if (!px_) { w_ = h_ = 0; return; }

    limpar({0, 0, 0, 255});
}

frame_buffer::~frame_buffer() { liberar(); }

void frame_buffer::liberar() {
    memoria_free(px_);
    px_ = nullptr;
    w_ = h_ = 0;
}

void frame_buffer::roubar(frame_buffer& o) noexcept {
    px_ = o.px_; w_ = o.w_; h_ = o.h_;
    o.px_ = nullptr; o.w_ = o.h_ = 0;
}

frame_buffer::frame_buffer(frame_buffer&& o) noexcept { roubar(o); }

frame_buffer& frame_buffer::operator=(frame_buffer&& o) noexcept {
    if (this != &o) {
        liberar();
        roubar(o);
    }
    return *this;
}

void frame_buffer::limpar(fb_cores c) {
    if (!px_) return;
    const uint32_t p = c.empacotar();
    for (size_t i = 0, n = total_pixels(); i < n; i++) px_[i] = p;
}

void frame_buffer::pixel(int x, int y, fb_cores c) {
    if (!dentro(x, y)) return;
    px_[(size_t)y * w_ + x] = c.empacotar();
}

void frame_buffer::misturar(int x, int y, fb_cores c) {
    if (c.a == 0 || !dentro(x, y)) return;
    uint32_t& d = px_[(size_t)y * w_ + x];
    d = (c.a == 255 ? c : fb_cores::misturar(fb_cores::desempacotar(d), c)).empacotar();
}

fb_cores frame_buffer::ler(int x, int y) const {
    if (!dentro(x, y)) return {0, 0, 0, 0};
    return fb_cores::desempacotar(px_[(size_t)y * w_ + x]);
}

fb_atlas::~fb_atlas() { liberar(); }

void fb_atlas::liberar() {
    memoria_free(px_);
    px_ = nullptr;
    w_ = h_ = fw_ = fh_ = 0;
}

void fb_atlas::roubar(fb_atlas& o) noexcept {
    px_ = o.px_; w_ = o.w_; h_ = o.h_; fw_ = o.fw_; fh_ = o.fh_;
    o.px_ = nullptr;
    o.w_ = o.h_ = o.fw_ = o.fh_ = 0;
}

fb_atlas::fb_atlas(fb_atlas&& o) noexcept { roubar(o); }

fb_atlas& fb_atlas::operator=(fb_atlas&& o) noexcept {
    if (this != &o) {
        liberar();
        roubar(o);
    }
    return *this;
}

bool fb_atlas::carregar(const char* caminho, uint16_t frame_w, uint16_t frame_h) {
    size_t tam = 0;
    uint8_t* dados = (uint8_t*)arquivo_ler_tudo_tam(caminho, &tam);
    if (!dados) return false;

    const bool ok = carregar_memoria(dados, tam, frame_w, frame_h);
    memoria_free(dados);
    return ok;
}

bool fb_atlas::carregar_memoria(const uint8_t* dados, size_t tamanho, uint16_t frame_w, uint16_t frame_h) {
    liberar();

    int w, h, canais;
    uint8_t* rgba = stbi_load_from_memory(dados, (int)tamanho, &w, &h, &canais, 4);
    if (!rgba) return false;

    const bool ok = adotar_rgba(rgba, w, h, frame_w, frame_h);
    stbi_image_free(rgba);
    return ok;
}

bool fb_atlas::adotar_rgba(const uint8_t* rgba, int w, int h, uint16_t frame_w, uint16_t frame_h) {
    if (frame_w == 0) frame_w = (uint16_t)w;
    if (frame_h == 0) frame_h = (uint16_t)h;
    if (frame_w > w || frame_h > h) return false;

    const size_t n = (size_t)w * h;
    px_ = (uint32_t*)memoria_malloc(n * sizeof(uint32_t));
    if (!px_) return false;

    for (size_t i = 0; i < n; i++) {
        const uint8_t* p = rgba + i * 4;
        px_[i] = fb_cores{p[0], p[1], p[2], p[3]}.empacotar();
    }

    w_ = w; h_ = h; fw_ = frame_w; fh_ = frame_h;
    return true;
}

bool fb_atlas::tile_valido(int pw, int ph) const {
    return px_ && pw >= 0 && ph >= 0 && pw < colunas() && ph < linhas();
}

void fb_atlas::desenhar(frame_buffer& fb, int pw, int ph, int x, int y, int w, int h) const {
    if (!tile_valido(pw, ph)) return;
    if (w == 0) w = fw_;
    if (h == 0) h = fh_;
    if (w <= 0 || h <= 0) return;

    const faixa cols = recortar(x, w, fb.largura());
    const faixa lins = recortar(y, h, fb.altura());
    const int sx = pw * fw_, sy = ph * fh_;

    for (int j = lins.ini; j < lins.fim; j++) {
        const uint32_t* origem = px_ + (size_t)(sy + (j * fh_) / h) * w_ + sx;
        for (int i = cols.ini; i < cols.fim; i++)
            fb.misturar(x + i, y + j, fb_cores::desempacotar(origem[(i * fw_) / w]));
    }
}

namespace {

struct pt { int x, y; };

struct caixa_girada {
    int     cx2, cy2, hw, hh;
    int32_t c, s;

    caixa_girada(int x, int y, int w, int h, uint16_t angulo)
        : cx2(2 * x + w - 1), cy2(2 * y + h - 1), hw(w - 1), hh(h - 1),
          c(fb_trig::cosseno(angulo)), s(fb_trig::seno(angulo)) {}

    pt vertice(int sx, int sy) const {
        const int64_t dx = sx * hw, dy = sy * hh;
        return { (cx2 + (int)(((dx * c) - (dy * s)) >> fb_trig::BITS)) >> 1,
                 (cy2 + (int)(((dx * s) + (dy * c)) >> fb_trig::BITS)) >> 1 };
    }
};

void preencher_retangulo(frame_buffer& fb, int x, int y, int w, int h, fb_cores c) {
    if (c.a == 0) return;

    const faixa cols = recortar(x, w, fb.largura());
    const faixa lins = recortar(y, h, fb.altura());
    const uint32_t cor = c.empacotar();
    const bool opaca = c.a == 255;

    for (int j = lins.ini; j < lins.fim; j++) {
        const int py = y + j;
        if (opaca) {
            uint32_t* d = fb.dados() + (size_t)py * fb.largura() + x + cols.ini;
            for (int i = cols.ini; i < cols.fim; i++) *d++ = cor;
        } else {
            for (int i = cols.ini; i < cols.fim; i++) fb.misturar(x + i, py, c);
        }
    }
}

void contorno_retangulo(frame_buffer& fb, int x, int y, int w, int h, fb_cores c) {
    linha(fb, x, y, x + w - 1, y, c);
    linha(fb, x, y + h - 1, x + w - 1, y + h - 1, c);
    if (h > 2) {
        linha(fb, x, y + 1, x, y + h - 2, c);
        linha(fb, x + w - 1, y + 1, x + w - 1, y + h - 2, c);
    }
}

void preencher_convexo(frame_buffer& fb, const pt* p, int n, fb_cores cor) {
    int ymin = p[0].y, ymax = p[0].y;
    for (int i = 1; i < n; i++) {
        if (p[i].y < ymin) ymin = p[i].y;
        if (p[i].y > ymax) ymax = p[i].y;
    }
    if (ymin < 0) ymin = 0;
    if (ymax > fb.altura() - 1) ymax = fb.altura() - 1;

    for (int y = ymin; y <= ymax; y++) {
        int xmin = INT32_MAX, xmax = INT32_MIN;
        auto cobrir = [&](int x) {
            if (x < xmin) xmin = x;
            if (x > xmax) xmax = x;
        };

        for (int i = 0; i < n; i++) {
            const pt a = p[i], b = p[(i + 1) % n];
            if (a.y == b.y) {
                if (a.y == y) { cobrir(a.x); cobrir(b.x); }
                continue;
            }
            const int lo = a.y < b.y ? a.y : b.y;
            const int hi = a.y < b.y ? b.y : a.y;
            if (y < lo || y > hi) continue;
            cobrir(a.x + (int)(((int64_t)(y - a.y) * (b.x - a.x)) / (b.y - a.y)));
        }

        if (xmin <= xmax) preencher_retangulo(fb, xmin, y, xmax - xmin + 1, 1, cor);
    }
}

void desenhar_poligono(frame_buffer& fb, const pt* p, int n, bool preenchido, fb_cores cor) {
    if (preenchido) { preencher_convexo(fb, p, n, cor); return; }
    for (int i = 0; i < n; i++) {
        const pt a = p[i], b = p[(i + 1) % n];
        linha(fb, a.x, a.y, b.x, b.y, cor);
    }
}

bool dentro_fatia(int64_t dx, int64_t dy, uint16_t ang, int64_t c, int64_t s) {
    if (ang <= fb_trig::MEIA_VOLTA)
        return dy >= 0 && (dx * s - dy * c) >= 0;
    const bool no_complemento = (c * dy - s * dx) >= 0 && dy <= 0;
    return !no_complemento;
}

bool no_anel(int64_t d2, int64_t r2, int raio, bool preenchido) {
    if (d2 >= r2 + raio) return false;
    return preenchido || d2 >= r2 - raio;
}

}

void linha(frame_buffer& fb, int x0, int y0, int x1, int y1, fb_cores c) {
    const int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    const int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        fb.misturar(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void retangulo(frame_buffer& fb, int x, int y, int w, int h, float angulo, bool preenchido, fb_cores c) {
    if (w <= 0 || h <= 0) return;

    const uint16_t ang = fb_trig::de_graus(angulo);

    if (ang == 0) {
        if (preenchido) preencher_retangulo(fb, x, y, w, h, c);
        else            contorno_retangulo(fb, x, y, w, h, c);
        return;
    }

    const caixa_girada g(x, y, w, h, ang);
    const pt p[4] = {
        g.vertice(-1, -1),
        g.vertice( 1, -1),
        g.vertice( 1,  1),
        g.vertice(-1,  1),
    };
    desenhar_poligono(fb, p, 4, preenchido, c);
}

void triangulo_retangulo(frame_buffer& fb, int x, int y, int w, int h, float angulo, bool preenchido, fb_cores c) {
    if (w <= 0 || h <= 0) return;

    const caixa_girada g(x, y, w, h, fb_trig::de_graus(angulo));
    const pt p[3] = {
        g.vertice(-1, -1),
        g.vertice(-1,  1),
        g.vertice( 1,  1),
    };
    desenhar_poligono(fb, p, 3, preenchido, c);
}

void circulo(frame_buffer& fb, int cx, int cy, int raio, float angulo, bool preenchido, fb_cores c) {
    if (raio < 0) return;
    if (raio == 0) { fb.misturar(cx, cy, c); return; }

    const uint16_t ang = fb_trig::de_graus(angulo);

    const int64_t r2 = (int64_t)raio * raio;
    const int32_t co = fb_trig::cosseno(ang), se = fb_trig::seno(ang);
    const bool fatia = ang != 0;

    const int x0 = cx - raio < 0 ? 0 : cx - raio;
    const int y0 = cy - raio < 0 ? 0 : cy - raio;
    const int x1 = cx + raio > fb.largura() - 1 ? fb.largura() - 1 : cx + raio;
    const int y1 = cy + raio > fb.altura()  - 1 ? fb.altura()  - 1 : cy + raio;

    for (int y = y0; y <= y1; y++) {
        const int64_t dy = y - cy;
        for (int x = x0; x <= x1; x++) {
            const int64_t dx = x - cx;
            if (!no_anel(dx * dx + dy * dy, r2, raio, preenchido)) continue;
            if (fatia && !dentro_fatia(dx, dy, ang, co, se)) continue;
            fb.misturar(x, y, c);
        }
    }

    if (!preenchido && fatia) {
        linha(fb, cx, cy, cx + raio, cy, c);
        linha(fb, cx, cy,
              cx + (int)(((int64_t)raio * co) >> fb_trig::BITS),
              cy + (int)(((int64_t)raio * se) >> fb_trig::BITS), c);
    }
}

void blit(frame_buffer& dst, const frame_buffer& src, int dx, int dy) {
    const faixa cols = recortar(dx, src.largura(), dst.largura());
    const faixa lins = recortar(dy, src.altura(), dst.altura());

    for (int j = lins.ini; j < lins.fim; j++) {
        const uint32_t* origem = src.dados() + (size_t)j * src.largura();
        for (int i = cols.ini; i < cols.fim; i++)
            dst.misturar(dx + i, dy + j, fb_cores::desempacotar(origem[i]));
    }
}

/* ---- API C ---- */
namespace {
inline fb_cores cor_de(uint32_t p) { return fb_cores::desempacotar(p); }
}

extern "C" {

int32_t  fb_fx_de_int(int v)              { return fx::de_int(v); }
int      fb_fx_para_int(int32_t v)        { return fx::para_int(v); }
int32_t  fb_fx_mul(int32_t a, int32_t b)  { return fx::mul(a, b); }
int32_t  fb_fx_div(int32_t a, int32_t b)  { return b ? fx::div(a, b) : 0; }
uint16_t fb_graus(float g)                { return fb_trig::de_graus(g); }
int32_t  fb_seno(uint16_t a)              { return fb_trig::seno(a); }
int32_t  fb_cosseno(uint16_t a)           { return fb_trig::cosseno(a); }
uint32_t fb_cor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return fb_cores{r, g, b, a}.empacotar(); }

size_t fb_pitch(const fb_handle* fb)         { return fb ? fb->fb.pitch() : 0; }
size_t fb_total_pixels(const fb_handle* fb)  { return fb ? fb->fb.total_pixels() : 0; }
void   fb_pixel_fx(fb_handle* fb, int32_t x, int32_t y, uint32_t cor) { if (fb) fb->fb.pixel_fx(x, y, cor_de(cor)); }

int fb_atlas_largura(const fb_atlas_handle* a)      { return a ? a->at.largura() : 0; }
int fb_atlas_altura(const fb_atlas_handle* a)       { return a ? a->at.altura() : 0; }
int fb_atlas_tile_largura(const fb_atlas_handle* a) { return a ? a->at.tile_largura() : 0; }
int fb_atlas_tile_altura(const fb_atlas_handle* a)  { return a ? a->at.tile_altura() : 0; }
int fb_atlas_passo(const fb_atlas_handle* a)        { return a ? a->at.passo() : 0; }

const uint32_t* fb_atlas_tile_dados(const fb_atlas_handle* a, int pw, int ph) {
    if (!a || !a->at.valido() || pw < 0 || ph < 0 || pw >= a->at.colunas() || ph >= a->at.linhas()) return nullptr;
    return a->at.tile_dados(pw, ph);
}

void fb_atlas_desenhar_fx(const fb_atlas_handle* a, fb_handle* fb, int pw, int ph, int32_t x, int32_t y, int w, int h) {
    if (a && fb) a->at.desenhar_fx(fb->fb, pw, ph, x, y, w, h);
}

fb_handle* fb_criar(int largura, int altura) {
    fb_handle* h = new fb_handle;
    if (!h) return nullptr;
    h->fb = frame_buffer(largura, altura);
    if (!h->fb.valido()) { delete h; return nullptr; }
    return h;
}

void fb_destruir(fb_handle* fb)             { delete fb; }
int  fb_valido(const fb_handle* fb)         { return fb && fb->fb.valido(); }
int  fb_largura(const fb_handle* fb)        { return fb ? fb->fb.largura() : 0; }
int  fb_altura(const fb_handle* fb)         { return fb ? fb->fb.altura()  : 0; }
uint32_t* fb_dados(fb_handle* fb)           { return fb ? fb->fb.dados() : nullptr; }

void fb_limpar(fb_handle* fb, uint32_t cor)                    { if (fb) fb->fb.limpar(cor_de(cor)); }
void fb_pixel(fb_handle* fb, int x, int y, uint32_t cor)       { if (fb) fb->fb.pixel(x, y, cor_de(cor)); }
void fb_misturar(fb_handle* fb, int x, int y, uint32_t cor)    { if (fb) fb->fb.misturar(x, y, cor_de(cor)); }
uint32_t fb_ler(const fb_handle* fb, int x, int y)             { return fb ? fb->fb.ler(x, y).empacotar() : 0; }

void fb_linha(fb_handle* fb, int x0, int y0, int x1, int y1, uint32_t cor) {
    if (fb) linha(fb->fb, x0, y0, x1, y1, cor_de(cor));
}
void fb_retangulo(fb_handle* fb, int x, int y, int w, int h, float angulo, int preenchido, uint32_t cor) {
    if (fb) retangulo(fb->fb, x, y, w, h, angulo, preenchido != 0, cor_de(cor));
}
void fb_triangulo_retangulo(fb_handle* fb, int x, int y, int w, int h, float angulo, int preenchido, uint32_t cor) {
    if (fb) triangulo_retangulo(fb->fb, x, y, w, h, angulo, preenchido != 0, cor_de(cor));
}
void fb_circulo(fb_handle* fb, int cx, int cy, int raio, float angulo, int preenchido, uint32_t cor) {
    if (fb) circulo(fb->fb, cx, cy, raio, angulo, preenchido != 0, cor_de(cor));
}
void fb_blit(fb_handle* dst, const fb_handle* src, int dx, int dy) {
    if (dst && src) blit(dst->fb, src->fb, dx, dy);
}

fb_atlas_handle* fb_atlas_criar(void)                { return new fb_atlas_handle; }
void fb_atlas_destruir(fb_atlas_handle* a)           { delete a; }

int fb_atlas_carregar(fb_atlas_handle* a, const char* caminho, uint16_t frame_w, uint16_t frame_h) {
    return a && a->at.carregar(caminho, frame_w, frame_h);
}
int fb_atlas_carregar_memoria(fb_atlas_handle* a, const uint8_t* dados, size_t tamanho, uint16_t frame_w, uint16_t frame_h) {
    return a && a->at.carregar_memoria(dados, tamanho, frame_w, frame_h);
}
int fb_atlas_valido(const fb_atlas_handle* a)   { return a && a->at.valido(); }
int fb_atlas_colunas(const fb_atlas_handle* a)  { return a ? a->at.colunas() : 0; }
int fb_atlas_linhas(const fb_atlas_handle* a)   { return a ? a->at.linhas()  : 0; }

void fb_atlas_desenhar(const fb_atlas_handle* a, fb_handle* fb, int pw, int ph, int x, int y, int w, int h) {
    if (a && fb) a->at.desenhar(fb->fb, pw, ph, x, y, w, h);
}

}

void operator delete(void* p, unsigned long) noexcept { memoria_free(p); }
void operator delete[](void* p, unsigned long) noexcept { memoria_free(p); }