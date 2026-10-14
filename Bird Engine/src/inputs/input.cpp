#include "input.hpp"

#include <SDL2/SDL.h>
#include <stdlib.h>

namespace {

SDL_Scancode mapear_scancode(uint8_t c) {
    if (c >= 'a' && c <= 'z') return (SDL_Scancode)(SDL_SCANCODE_A + (c - 'a'));
    if (c >= '0' && c <= '9')
        return c == '0' ? SDL_SCANCODE_0 : (SDL_Scancode)(SDL_SCANCODE_1 + (c - '1'));

    // F1..F12: os scancodes do SDL são contíguos
    const uint8_t f1 = (uint8_t)Tecla::f1, f12 = (uint8_t)Tecla::f12;
    if (c >= f1 && c <= f12) return (SDL_Scancode)(SDL_SCANCODE_F1 + (c - f1));

    switch ((Tecla)c) {
        case Tecla::espaco:        return SDL_SCANCODE_SPACE;
        case Tecla::start:         return SDL_SCANCODE_RETURN;
        case Tecla::backspace:     return SDL_SCANCODE_BACKSPACE;
        case Tecla::tab:           return SDL_SCANCODE_TAB;
        case Tecla::sair:          return SDL_SCANCODE_ESCAPE;
        case Tecla::seta_cima:     return SDL_SCANCODE_UP;
        case Tecla::seta_baixo:    return SDL_SCANCODE_DOWN;
        case Tecla::seta_esquerda: return SDL_SCANCODE_LEFT;
        case Tecla::seta_direita:  return SDL_SCANCODE_RIGHT;
        default:                   return SDL_SCANCODE_UNKNOWN;
    }
}

} // namespace

void Entrada::iniciar() {
    for (int i = 0; i < TECLAS; i++) atual_[i] = anterior_[i] = false;
    for (int i = 0; i < BOTOES; i++) mouse_atual_[i] = mouse_anterior_[i] = false;
    x_ = y_ = fb_x_ = fb_y_ = roda_ = 0;
    for (int i = 0; i < MAX_TOQUES; i++) toques_[i] = Toque();
    ultima_ = Tecla::nenhuma;
    pediu_fechar_ = false;
    texto_[0] = '\0';
    SDL_StartTextInput();
}

void Entrada::atualizar() {
    for (int i = 0; i < TECLAS; i++) anterior_[i] = atual_[i];
    for (int i = 0; i < BOTOES; i++) mouse_anterior_[i] = mouse_atual_[i];
    ultima_ = Tecla::nenhuma;
    texto_[0] = '\0';
    roda_ = 0;

    // toques: limpa os eventos do frame e libera os slots dos dedos que já saíram
    for (int i = 0; i < MAX_TOQUES; i++) {
        toques_[i].caiu = false;
        toques_[i].subiu = false;
        if (!toques_[i].ativo) toques_[i].usado = false;
    }

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (cb_) cb_(ev, cb_ctx_);

        if (ev.type == SDL_QUIT) {
            pediu_fechar_ = true;
        } else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_CLOSE) {
            pediu_fechar_ = true;
        } else if (ev.type == SDL_TEXTINPUT) {
            SDL_strlcat(texto_, ev.text.text, sizeof(texto_));
        } else if (ev.type == SDL_MOUSEWHEEL) {
            roda_ += ev.wheel.y;
        } else if (ev.type == SDL_FINGERDOWN) {
            int slot = -1;
            for (int i = 0; i < MAX_TOQUES; i++) {
                if (!toques_[i].usado) { slot = i; break; }
            }
            if (slot >= 0) {
                Toque& t = toques_[slot];
                t.id = ev.tfinger.fingerId;
                t.usado = t.ativo = t.caiu = true;
                t.subiu = false;
                normalizado_para_fb(ev.tfinger.x, ev.tfinger.y, t.x, t.y);
            }
        } else if (ev.type == SDL_FINGERMOTION) {
            int slot = achar_toque(ev.tfinger.fingerId);
            if (slot >= 0) normalizado_para_fb(ev.tfinger.x, ev.tfinger.y, toques_[slot].x, toques_[slot].y);
        } else if (ev.type == SDL_FINGERUP) {
            int slot = achar_toque(ev.tfinger.fingerId);
            if (slot >= 0) {
                Toque& t = toques_[slot];
                normalizado_para_fb(ev.tfinger.x, ev.tfinger.y, t.x, t.y);
                t.ativo = false;
                t.subiu = true;
            }
        }
    }

    const uint8_t* teclado = SDL_GetKeyboardState(nullptr);

    atual_[(uint8_t)Tecla::ctrl]  = teclado[SDL_SCANCODE_LCTRL]  || teclado[SDL_SCANCODE_RCTRL];
    atual_[(uint8_t)Tecla::shift] = teclado[SDL_SCANCODE_LSHIFT] || teclado[SDL_SCANCODE_RSHIFT];

    for (int c = 0; c < TECLAS; c++) {
        if (c == (int)Tecla::ctrl || c == (int)Tecla::shift) continue;
        SDL_Scancode sc = mapear_scancode((uint8_t)c);
        if (sc == SDL_SCANCODE_UNKNOWN) continue;
        atual_[c] = teclado[sc] != 0;
    }

    for (int c = 0; c < TECLAS; c++) {
        if (atual_[c] && !anterior_[c]) { ultima_ = (Tecla)c; break; }
    }

    Uint32 mascara = SDL_GetMouseState(&x_, &y_);
    mouse_atual_[(uint8_t)MouseBotao::esquerdo] = (mascara & SDL_BUTTON(SDL_BUTTON_LEFT))   != 0;
    mouse_atual_[(uint8_t)MouseBotao::direito]  = (mascara & SDL_BUTTON(SDL_BUTTON_RIGHT))  != 0;
    mouse_atual_[(uint8_t)MouseBotao::meio]     = (mascara & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;

    fb_x_ = x_;
    fb_y_ = y_;
}

bool Entrada::capslock() const {
    return (SDL_GetModState() & KMOD_CAPS) != 0;
}

const char* Entrada::layout_teclado() {
    const char* lang = getenv("LANG");
    if (lang && *lang) return lang; 
    lang = getenv("LC_ALL");
    if (lang && *lang) return lang;
    return "desconhecido";
}

int Entrada::toques_ativos() const {
    int n = 0;
    for (int i = 0; i < MAX_TOQUES; i++) if (toques_[i].ativo) n++;
    return n;
}

int Entrada::achar_toque(int64_t id) const {
    for (int i = 0; i < MAX_TOQUES; i++) {
        if (toques_[i].usado && toques_[i].ativo && toques_[i].id == id) return i;
    }
    return -1;
}

// o SDL entrega o toque normalizado (0..1); converte para pixels do framebuffer
void Entrada::normalizado_para_fb(float nx, float ny, int& x, int& y) const {
    int w = area_w_, h = area_h_;
    if (w <= 0 || h <= 0) {
        SDL_Window* j = SDL_GetKeyboardFocus();
        if (!j) j = SDL_GetMouseFocus();
        if (j) SDL_GetWindowSize(j, &w, &h);
        else   w = h = 1;
    }
    x = (int)(nx * w);
    y = (int)(ny * h);
    if (x < 0) x = 0; else if (x > w - 1) x = w - 1;
    if (y < 0) y = 0; else if (y > h - 1) y = h - 1;
}

/* ---- API C ---- */

namespace {
inline bool mb_ok(int b) { return b >= 0 && b < 3; }

void tramp(const SDL_Event& ev, void* c) {
    entrada_handle* h = static_cast<entrada_handle*>(c);
    if (h && h->cb) h->cb(&ev, h->ctx);
}
}

extern "C" {

entrada_handle* entrada_criar(void)             { return new entrada_handle; }
void entrada_destruir(entrada_handle* e)        { delete e; }
void entrada_iniciar(entrada_handle* e)         { if (e) e->e.iniciar(); }
void entrada_atualizar(entrada_handle* e)       { if (e) e->e.atualizar(); }
void entrada_callback_evento(entrada_handle* e, entrada_callback cb, void* ctx) {
    if (!e) return;
    e->cb = cb;
    e->ctx = ctx;
    e->e.callback_evento(cb ? tramp : nullptr, e);
}

int         entrada_pediu_fechar(const entrada_handle* e)   { return e && e->e.pediu_fechar(); }
const char* entrada_texto_digitado(const entrada_handle* e) { return e ? e->e.texto_digitado() : ""; }
int         entrada_capslock(const entrada_handle* e)       { return e && e->e.capslock(); }
const char* entrada_layout_teclado(void)                    { return Entrada::layout_teclado(); }

int entrada_pressionou(const entrada_handle* e, int t) { return e && e->e.pressionou((Tecla)(uint8_t)t); }
int entrada_soltou(const entrada_handle* e, int t)     { return e && e->e.soltou((Tecla)(uint8_t)t); }
int entrada_segurando(const entrada_handle* e, int t)  { return e && e->e.segurando((Tecla)(uint8_t)t); }
int entrada_ultima(const entrada_handle* e)            { return e ? (int)e->e.ultima() : 0; }

int entrada_mouse_pressionou(const entrada_handle* e, int b) { return e && mb_ok(b) && e->e.mouse_pressionou((MouseBotao)b); }
int entrada_mouse_soltou(const entrada_handle* e, int b)     { return e && mb_ok(b) && e->e.mouse_soltou((MouseBotao)b); }
int entrada_mouse_segurando(const entrada_handle* e, int b)  { return e && mb_ok(b) && e->e.mouse_segurando((MouseBotao)b); }
int entrada_mouse_x(const entrada_handle* e)    { return e ? e->e.mouse_x() : 0; }
int entrada_mouse_y(const entrada_handle* e)    { return e ? e->e.mouse_y() : 0; }
int entrada_mouse_roda(const entrada_handle* e) { return e ? e->e.mouse_roda() : 0; }
void entrada_mouse_janela(const entrada_handle* e, int* x, int* y) {
    int a = 0, b = 0;
    if (e) e->e.mouse_janela(a, b);
    if (x) *x = a;
    if (y) *y = b;
}
void entrada_definir_mouse(entrada_handle* e, int x, int y) { if (e) e->e.definir_mouse(x, y); }

int entrada_toque_pressionou(const entrada_handle* e, int i) { return e && e->e.toque_pressionou(i); }
int entrada_toque_soltou(const entrada_handle* e, int i)     { return e && e->e.toque_soltou(i); }
int entrada_toque_segurando(const entrada_handle* e, int i)  { return e && e->e.toque_segurando(i); }
int entrada_toque_x(const entrada_handle* e, int i)          { return e ? e->e.toque_x(i) : 0; }
int entrada_toque_y(const entrada_handle* e, int i)          { return e ? e->e.toque_y(i) : 0; }
int entrada_toques_ativos(const entrada_handle* e)           { return e ? e->e.toques_ativos() : 0; }
void entrada_definir_area_toque(entrada_handle* e, int w, int h) { if (e) e->e.definir_area_toque(w, h); }

}