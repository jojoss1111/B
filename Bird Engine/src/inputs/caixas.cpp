#include "caixas.hpp"

const Entrada* Caixa::entrada_ = nullptr;

void Caixa::criar(int x, int y, int w, int h) {
    forma_ = Forma::retangulo;
    x_ = x; y_ = y; w_ = w; h_ = h; raio_ = 0;
    ativa_ = true;
}

void Caixa::criar(int x, int y, int raio) {
    forma_ = Forma::circulo;
    x_ = x; y_ = y; w_ = h_ = 0; raio_ = raio;
    ativa_ = true;
}

bool Caixa::dentro(int px, int py) const {
    if (!ativa_) return false;

    if (forma_ == Forma::retangulo)
        return px >= x_ && px < x_ + w_ && py >= y_ && py < y_ + h_;

    // círculo: só inteiros, sem raiz quadrada
    int dx = px - x_;
    int dy = py - y_;
    return dx * dx + dy * dy <= raio_ * raio_;
}

bool Caixa::mouse_em_cima() const {
    if (!entrada_) return false;
    return dentro(entrada_->mouse_x(), entrada_->mouse_y());
}

// ---- versões com Entrada explícita ----
bool Caixa::pressionou(const Entrada& in, MouseBotao b) const {
    return in.mouse_pressionou(b) && dentro(in.mouse_x(), in.mouse_y());
}

bool Caixa::segurou(const Entrada& in, MouseBotao b) const {
    return in.mouse_segurando(b) && dentro(in.mouse_x(), in.mouse_y());
}

bool Caixa::soltou(const Entrada& in, MouseBotao b) const {
    return in.mouse_soltou(b) && dentro(in.mouse_x(), in.mouse_y());
}

// ---- versões usando a Entrada global (usar_entrada) ----
bool Caixa::pressionou(MouseBotao b) const { return entrada_ && pressionou(*entrada_, b); }
bool Caixa::segurou(MouseBotao b)    const { return entrada_ && segurou(*entrada_, b); }
bool Caixa::soltou(MouseBotao b)     const { return entrada_ && soltou(*entrada_, b); }

/* ---- API C ---- */

namespace {
inline Caixa*       CX(caixa_handle* h)        { return reinterpret_cast<Caixa*>(h); }
inline const Caixa* CX(const caixa_handle* h)  { return reinterpret_cast<const Caixa*>(h); }
inline bool         mb_ok(int b)               { return b >= 0 && b < 3; }
inline MouseBotao   MB(int b)                  { return (MouseBotao)(uint8_t)b; }
}

extern "C" {

caixa_handle* caixa_novo(void)                  { return reinterpret_cast<caixa_handle*>(new Caixa); }
void caixa_destruir(caixa_handle* c)            { delete CX(c); }
void caixa_criar_retangulo(caixa_handle* c, int x, int y, int w, int h) { if (c) CX(c)->criar(x, y, w, h); }
void caixa_criar_circulo(caixa_handle* c, int x, int y, int raio)       { if (c) CX(c)->criar(x, y, raio); }
void caixa_desativar(caixa_handle* c)           { if (c) CX(c)->destruir(); }

void caixa_mover(caixa_handle* c, int x, int y) { if (c) CX(c)->mover(x, y); }
void caixa_tamanho(caixa_handle* c, int w, int h) { if (c) CX(c)->tamanho(w, h); }
void caixa_raio(caixa_handle* c, int r)         { if (c) CX(c)->raio(r); }

int caixa_ativa(const caixa_handle* c)                  { return c && CX(c)->ativa(); }
int caixa_dentro(const caixa_handle* c, int px, int py) { return c && CX(c)->dentro(px, py); }

void caixa_usar_entrada(const entrada_handle* e) { Caixa::usar_entrada(e ? &e->e : nullptr); }
int caixa_mouse_em_cima(const caixa_handle* c)   { return c && CX(c)->mouse_em_cima(); }
int caixa_pressionou(const caixa_handle* c, int b) { return c && mb_ok(b) && CX(c)->pressionou(MB(b)); }
int caixa_segurou(const caixa_handle* c, int b)    { return c && mb_ok(b) && CX(c)->segurou(MB(b)); }
int caixa_soltou(const caixa_handle* c, int b)     { return c && mb_ok(b) && CX(c)->soltou(MB(b)); }

int caixa_pressionou_em(const caixa_handle* c, const entrada_handle* e, int b) { return c && e && mb_ok(b) && CX(c)->pressionou(e->e, MB(b)); }
int caixa_segurou_em(const caixa_handle* c, const entrada_handle* e, int b)    { return c && e && mb_ok(b) && CX(c)->segurou(e->e, MB(b)); }
int caixa_soltou_em(const caixa_handle* c, const entrada_handle* e, int b)     { return c && e && mb_ok(b) && CX(c)->soltou(e->e, MB(b)); }

}