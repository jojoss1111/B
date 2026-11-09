#include "caixas.h"
#include <crock.h>

static const entrada_handle* caixa_entrada = NULL;

caixa_handle* caixa_novo(void) {
    caixa_handle* c = (caixa_handle*)memoria_malloc(sizeof(caixa_handle));
    if (!c) return NULL;
    c->circulo = 0;
    c->ativa = 0;
    c->x = c->y = c->w = c->h = c->raio = 0;
    return c;
}

void caixa_destruir(caixa_handle* c) { if (c) memoria_free(c); }

void caixa_criar_retangulo(caixa_handle* c, int x, int y, int w, int h) {
    if (!c) return;
    c->circulo = 0;
    c->x = x; c->y = y; c->w = w; c->h = h; c->raio = 0;
    c->ativa = 1;
}

void caixa_criar_circulo(caixa_handle* c, int x, int y, int raio) {
    if (!c) return;
    c->circulo = 1;
    c->x = x; c->y = y; c->w = c->h = 0; c->raio = raio;
    c->ativa = 1;
}

void caixa_desativar(caixa_handle* c)             { if (c) c->ativa = 0; }
void caixa_mover(caixa_handle* c, int x, int y)   { if (c) { c->x = x; c->y = y; } }
void caixa_tamanho(caixa_handle* c, int w, int h) { if (c) { c->w = w; c->h = h; } }
void caixa_raio(caixa_handle* c, int r)           { if (c) c->raio = r; }

int caixa_ativa(const caixa_handle* c) { return c && c->ativa; }

int caixa_dentro(const caixa_handle* c, int px, int py) {
    if (!c || !c->ativa) return 0;

    if (!c->circulo)
        return px >= c->x && px < c->x + c->w && py >= c->y && py < c->y + c->h;

    int dx = px - c->x;
    int dy = py - c->y;
    return dx * dx + dy * dy <= c->raio * c->raio;
}

static int botao_ok(int b) { return b >= 0 && b < 3; }

void caixa_usar_entrada(const entrada_handle* e) { caixa_entrada = e; }

int caixa_mouse_em_cima(const caixa_handle* c) {
    if (!c || !caixa_entrada) return 0;
    return caixa_dentro(c, entrada_mouse_x(caixa_entrada), entrada_mouse_y(caixa_entrada));
}

int caixa_pressionou_em(const caixa_handle* c, const entrada_handle* e, int b) {
    return c && e && botao_ok(b) && entrada_mouse_pressionou(e, b) && caixa_dentro(c, entrada_mouse_x(e), entrada_mouse_y(e));
}

int caixa_segurou_em(const caixa_handle* c, const entrada_handle* e, int b) {
    return c && e && botao_ok(b) && entrada_mouse_segurando(e, b) && caixa_dentro(c, entrada_mouse_x(e), entrada_mouse_y(e));
}

int caixa_soltou_em(const caixa_handle* c, const entrada_handle* e, int b) {
    return c && e && botao_ok(b) && entrada_mouse_soltou(e, b) && caixa_dentro(c, entrada_mouse_x(e), entrada_mouse_y(e));
}

int caixa_pressionou(const caixa_handle* c, int b) { return caixa_pressionou_em(c, caixa_entrada, b); }
int caixa_segurou(const caixa_handle* c, int b)    { return caixa_segurou_em(c, caixa_entrada, b); }
int caixa_soltou(const caixa_handle* c, int b)     { return caixa_soltou_em(c, caixa_entrada, b); }