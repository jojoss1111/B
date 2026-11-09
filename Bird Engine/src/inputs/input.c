#include "input.h"

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <crock.h>

static SDL_Scancode mapear_scancode(uint8_t c) {
    if (c >= 'a' && c <= 'z') return (SDL_Scancode)(SDL_SCANCODE_A + (c - 'a'));
    if (c >= '0' && c <= '9')
        return c == '0' ? SDL_SCANCODE_0 : (SDL_Scancode)(SDL_SCANCODE_1 + (c - '1'));

    if (c >= TECLA_F1 && c <= TECLA_F12) return (SDL_Scancode)(SDL_SCANCODE_F1 + (c - TECLA_F1));

    switch (c) {
        case TECLA_ESPACO:        return SDL_SCANCODE_SPACE;
        case TECLA_START:         return SDL_SCANCODE_RETURN;
        case TECLA_BACKSPACE:     return SDL_SCANCODE_BACKSPACE;
        case TECLA_TAB:           return SDL_SCANCODE_TAB;
        case TECLA_SAIR:          return SDL_SCANCODE_ESCAPE;
        case TECLA_SETA_CIMA:     return SDL_SCANCODE_UP;
        case TECLA_SETA_BAIXO:    return SDL_SCANCODE_DOWN;
        case TECLA_SETA_ESQUERDA: return SDL_SCANCODE_LEFT;
        case TECLA_SETA_DIREITA:  return SDL_SCANCODE_RIGHT;
        default:                  return SDL_SCANCODE_UNKNOWN;
    }
}

static void toque_zerar(entrada_toque* t) {
    t->id = -1;
    t->x = t->y = 0;
    t->usado = t->ativo = t->caiu = t->subiu = 0;
}

static void entrada_zerar(entrada_handle* e) {
    for (int i = 0; i < ENTRADA_TECLAS; i++) e->atual[i] = e->anterior[i] = 0;
    for (int i = 0; i < ENTRADA_BOTOES; i++) e->mouse_atual[i] = e->mouse_anterior[i] = 0;
    e->x = e->y = e->fb_x = e->fb_y = e->roda = 0;
    for (int i = 0; i < ENTRADA_MAX_TOQUES; i++) toque_zerar(&e->toques[i]);
    e->ultima = TECLA_NENHUMA;
    e->pediu_fechar = 0;
    e->texto[0] = '\0';
}

entrada_handle* entrada_criar(void) {
    entrada_handle* e = (entrada_handle*)memoria_malloc(sizeof(entrada_handle));
    if (!e) return NULL;
    entrada_zerar(e);
    e->area_w = e->area_h = 0;
    e->cb = NULL;
    e->ctx = NULL;
    return e;
}

void entrada_destruir(entrada_handle* e) { if (e) memoria_free(e); }

void entrada_iniciar(entrada_handle* e) {
    if (!e) return;
    entrada_zerar(e);
    SDL_StartTextInput();
}

static int achar_toque(const entrada_handle* e, int64_t id) {
    for (int i = 0; i < ENTRADA_MAX_TOQUES; i++) {
        if (e->toques[i].usado && e->toques[i].ativo && e->toques[i].id == id) return i;
    }
    return -1;
}

static void normalizado_para_fb(const entrada_handle* e, float nx, float ny, int* x, int* y) {
    int w = e->area_w, h = e->area_h;
    if (w <= 0 || h <= 0) {
        SDL_Window* j = SDL_GetKeyboardFocus();
        if (!j) j = SDL_GetMouseFocus();
        if (j) SDL_GetWindowSize(j, &w, &h);
        else   w = h = 1;
    }
    *x = (int)(nx * w);
    *y = (int)(ny * h);
    if (*x < 0) *x = 0; else if (*x > w - 1) *x = w - 1;
    if (*y < 0) *y = 0; else if (*y > h - 1) *y = h - 1;
}

void entrada_atualizar(entrada_handle* e) {
    if (!e) return;

    for (int i = 0; i < ENTRADA_TECLAS; i++) e->anterior[i] = e->atual[i];
    for (int i = 0; i < ENTRADA_BOTOES; i++) e->mouse_anterior[i] = e->mouse_atual[i];
    e->ultima = TECLA_NENHUMA;
    e->texto[0] = '\0';
    e->roda = 0;

    for (int i = 0; i < ENTRADA_MAX_TOQUES; i++) {
        e->toques[i].caiu = 0;
        e->toques[i].subiu = 0;
        if (!e->toques[i].ativo) e->toques[i].usado = 0;
    }

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (e->cb) e->cb(&ev, e->ctx);

        if (ev.type == SDL_QUIT) {
            e->pediu_fechar = 1;
        } else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_CLOSE) {
            e->pediu_fechar = 1;
        } else if (ev.type == SDL_TEXTINPUT) {
            SDL_strlcat(e->texto, ev.text.text, sizeof(e->texto));
        } else if (ev.type == SDL_MOUSEWHEEL) {
            e->roda += ev.wheel.y;
        } else if (ev.type == SDL_FINGERDOWN) {
            int slot = -1;
            for (int i = 0; i < ENTRADA_MAX_TOQUES; i++) {
                if (!e->toques[i].usado) { slot = i; break; }
            }
            if (slot >= 0) {
                entrada_toque* t = &e->toques[slot];
                t->id = ev.tfinger.fingerId;
                t->usado = t->ativo = t->caiu = 1;
                t->subiu = 0;
                normalizado_para_fb(e, ev.tfinger.x, ev.tfinger.y, &t->x, &t->y);
            }
        } else if (ev.type == SDL_FINGERMOTION) {
            int slot = achar_toque(e, ev.tfinger.fingerId);
            if (slot >= 0) normalizado_para_fb(e, ev.tfinger.x, ev.tfinger.y, &e->toques[slot].x, &e->toques[slot].y);
        } else if (ev.type == SDL_FINGERUP) {
            int slot = achar_toque(e, ev.tfinger.fingerId);
            if (slot >= 0) {
                entrada_toque* t = &e->toques[slot];
                normalizado_para_fb(e, ev.tfinger.x, ev.tfinger.y, &t->x, &t->y);
                t->ativo = 0;
                t->subiu = 1;
            }
        }
    }

    const uint8_t* teclado = SDL_GetKeyboardState(NULL);

    e->atual[TECLA_CTRL]  = teclado[SDL_SCANCODE_LCTRL]  || teclado[SDL_SCANCODE_RCTRL];
    e->atual[TECLA_SHIFT] = teclado[SDL_SCANCODE_LSHIFT] || teclado[SDL_SCANCODE_RSHIFT];

    for (int c = 0; c < ENTRADA_TECLAS; c++) {
        if (c == TECLA_CTRL || c == TECLA_SHIFT) continue;
        SDL_Scancode sc = mapear_scancode((uint8_t)c);
        if (sc == SDL_SCANCODE_UNKNOWN) continue;
        e->atual[c] = teclado[sc] != 0;
    }

    for (int c = 0; c < ENTRADA_TECLAS; c++) {
        if (e->atual[c] && !e->anterior[c]) { e->ultima = (uint8_t)c; break; }
    }

    Uint32 mascara = SDL_GetMouseState(&e->x, &e->y);
    e->mouse_atual[MOUSE_ESQUERDO] = (mascara & SDL_BUTTON(SDL_BUTTON_LEFT))   != 0;
    e->mouse_atual[MOUSE_DIREITO]  = (mascara & SDL_BUTTON(SDL_BUTTON_RIGHT))  != 0;
    e->mouse_atual[MOUSE_MEIO]     = (mascara & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;

    e->fb_x = e->x;
    e->fb_y = e->y;
}

void entrada_callback_evento(entrada_handle* e, entrada_callback cb, void* ctx) {
    if (!e) return;
    e->cb = cb;
    e->ctx = ctx;
}

int         entrada_pediu_fechar(const entrada_handle* e)   { return e && e->pediu_fechar; }
const char* entrada_texto_digitado(const entrada_handle* e) { return e ? e->texto : ""; }

int entrada_capslock(const entrada_handle* e) {
    (void)e;
    return (SDL_GetModState() & KMOD_CAPS) != 0;
}

const char* entrada_layout_teclado(void) {
    const char* lang = getenv("LANG");
    if (lang && *lang) return lang;
    lang = getenv("LC_ALL");
    if (lang && *lang) return lang;
    return "desconhecido";
}

int entrada_pressionou(const entrada_handle* e, int t) { return e && e->atual[(uint8_t)t] && !e->anterior[(uint8_t)t]; }
int entrada_soltou(const entrada_handle* e, int t)     { return e && !e->atual[(uint8_t)t] && e->anterior[(uint8_t)t]; }
int entrada_segurando(const entrada_handle* e, int t)  { return e && e->atual[(uint8_t)t]; }
int entrada_ultima(const entrada_handle* e)            { return e ? (int)e->ultima : 0; }

static int botao_ok(int b) { return b >= 0 && b < ENTRADA_BOTOES; }

int entrada_mouse_pressionou(const entrada_handle* e, int b) { return e && botao_ok(b) && e->mouse_atual[b] && !e->mouse_anterior[b]; }
int entrada_mouse_soltou(const entrada_handle* e, int b)     { return e && botao_ok(b) && !e->mouse_atual[b] && e->mouse_anterior[b]; }
int entrada_mouse_segurando(const entrada_handle* e, int b)  { return e && botao_ok(b) && e->mouse_atual[b]; }
int entrada_mouse_x(const entrada_handle* e)    { return e ? e->fb_x : 0; }
int entrada_mouse_y(const entrada_handle* e)    { return e ? e->fb_y : 0; }
int entrada_mouse_roda(const entrada_handle* e) { return e ? e->roda : 0; }

void entrada_mouse_janela(const entrada_handle* e, int* x, int* y) {
    if (x) *x = e ? e->x : 0;
    if (y) *y = e ? e->y : 0;
}

void entrada_definir_mouse(entrada_handle* e, int x, int y) { if (e) { e->fb_x = x; e->fb_y = y; } }

static int toque_valido(int i) { return i >= 0 && i < ENTRADA_MAX_TOQUES; }

int entrada_toque_pressionou(const entrada_handle* e, int i) { return e && toque_valido(i) && e->toques[i].caiu; }
int entrada_toque_soltou(const entrada_handle* e, int i)     { return e && toque_valido(i) && e->toques[i].subiu; }
int entrada_toque_segurando(const entrada_handle* e, int i)  { return e && toque_valido(i) && (e->toques[i].ativo || e->toques[i].caiu); }
int entrada_toque_x(const entrada_handle* e, int i)          { return e && toque_valido(i) ? e->toques[i].x : 0; }
int entrada_toque_y(const entrada_handle* e, int i)          { return e && toque_valido(i) ? e->toques[i].y : 0; }

int entrada_toques_ativos(const entrada_handle* e) {
    if (!e) return 0;
    int n = 0;
    for (int i = 0; i < ENTRADA_MAX_TOQUES; i++) if (e->toques[i].ativo) n++;
    return n;
}

void entrada_definir_area_toque(entrada_handle* e, int w, int h) { if (e) { e->area_w = w; e->area_h = h; } }