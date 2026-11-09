#include "particula.h"
#include "../cam/cam.h"
#include <math.h>

static float aleatorio(particulas_handle* p) {
    uint32_t x = p->aleatorio ? p->aleatorio : 0xA341316Cu;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    p->aleatorio = x;
    return (float)(x >> 8) / 16777216.0f;
}

static float sortear(particulas_handle* p, float a, float b) {
    return a + (b - a) * aleatorio(p);
}

static float limitar(float v, float a, float b) {
    return v < a ? a : (v > b ? b : v);
}

particulas_handle* particulas_criar(uint32_t capacidade) {
    particulas_handle* p;
    if (capacidade == 0) capacidade = 512;
    if (capacidade > 65536) capacidade = 65536;
    p = (particulas_handle*)memoria_malloc(sizeof(*p));
    if (!p) return NULL;
    p->itens = (particula_item*)memoria_malloc((size_t)capacidade * sizeof(*p->itens));
    if (!p->itens) { memoria_free(p); return NULL; }
    p->capacidade = capacidade;
    p->quantidade = 0;
    p->aleatorio = 0x9E3779B9u;
    return p;
}

void particulas_destruir(particulas_handle* p) {
    if (!p) return;
    memoria_free(p->itens);
    memoria_free(p);
}

void particulas_limpar(particulas_handle* p) { if (p) p->quantidade = 0; }
uint32_t particulas_quantidade(const particulas_handle* p) { return p ? p->quantidade : 0; }

uint32_t particulas_emitir(particulas_handle* p, const particula_config* c, float x, float y) {
    if (!p || !c || c->n_cores == 0) return 0;
    int n = (int)(sortear(p, c->n_min, c->n_max) + 0.5f);
    uint32_t antes = p->quantidade;
    if (n < 0) n = 0;
    while (n-- > 0 && p->quantidade < p->capacidade) {
        particula_item* q = &p->itens[p->quantidade++];
        float a = (c->angulo + sortear(p, -c->abertura, c->abertura)) * 0.01745329251994329577f;
        float v = sortear(p, c->vel_min, c->vel_max);
        q->x = x + sortear(p, -c->dispersao, c->dispersao);
        q->y = y + sortear(p, -c->dispersao, c->dispersao);
        q->vx = cosf(a) * v;
        q->vy = -sinf(a) * v;
        q->vida = fmaxf(0.01f, sortear(p, c->vida_min, c->vida_max));
        q->tempo = 0;
        q->raio_ini = sortear(p, c->tam_ini_min, c->tam_ini_max);
        q->raio_fim = sortear(p, c->tam_fim_min, c->tam_fim_max);
        q->gravidade = c->gravidade; q->vento = c->vento; q->arrasto = c->arrasto;
        q->chao = c->chao; q->tem_chao = c->tem_chao;
        q->quique = c->quique; q->atrito = c->atrito;
        q->rastro = c->rastro; q->desvanecer = limitar(c->desvanecer, 0, 1);
        q->forma = c->forma; q->n_cores = c->n_cores;
        for (int i = 0; i < c->n_cores; ++i) q->cores[i] = c->cores[i];
    }
    return p->quantidade - antes;
}

void particulas_atualizar(particulas_handle* p, float dt) {
    if (!p || dt <= 0) return;
    uint32_t i = 0;
    while (i < p->quantidade) {
        particula_item* q = &p->itens[i];
        q->tempo += dt;
        if (q->tempo >= q->vida) {
            p->itens[i] = p->itens[--p->quantidade];
            continue;
        }
        float d = 1.0f - q->arrasto * dt;
        if (d < 0) d = 0;
        q->vx = q->vx * d + q->vento * dt;
        q->vy = q->vy * d + q->gravidade * dt;
        q->x += q->vx * dt; q->y += q->vy * dt;
        if (q->tem_chao && q->y > q->chao) {
            q->y = q->chao;
            q->vy = -q->vy * q->quique;
            q->vx *= q->atrito;
        }
        ++i;
    }
}

static uint32_t alpha(uint32_t cor, float f) {
    uint32_t a = cor >> 24;
    a = (uint32_t)(a * limitar(f, 0, 1) + 0.5f);
    return (cor & 0x00FFFFFFu) | (a << 24);
}

void particulas_desenhar(const particulas_handle* p, fb_handle* tela, const camera_handle* camera, int ox, int oy) {
    if (!p || !fb_valido(tela)) return;
    for (uint32_t i = 0; i < p->quantidade; ++i) {
        const particula_item* q = &p->itens[i];
        float t = q->tempo / q->vida;
        float fade = q->desvanecer > 0 ? fminf(1, (1 - t) / q->desvanecer) : 1;
        if (fade <= 0) continue;
        int ci = (int)(t * q->n_cores);
        if (ci >= q->n_cores) ci = q->n_cores - 1;
        uint32_t cor = alpha(q->cores[ci], fade);
        int x = (int)floorf(q->x + 0.5f), y = (int)floorf(q->y + 0.5f);
        float zoom = camera ? camera_zoom(camera) : 1.0f;
        int tx = (int)floorf(q->x - q->vx * q->rastro + 0.5f);
        int ty = (int)floorf(q->y - q->vy * q->rastro + 0.5f);
        int r = (int)floorf((q->raio_ini + (q->raio_fim - q->raio_ini) * t) * zoom + 0.5f);
        if (camera) {
            camera_mundo_para_tela(camera,x,y,&x,&y);
            camera_mundo_para_tela(camera,tx,ty,&tx,&ty);
        }
        x += ox; y += oy; tx += ox; ty += oy;
        if (q->forma == PARTICULA_LINHA)
            fb_linha(tela, x, y, tx, ty, cor);
        else if (q->forma == PARTICULA_QUADRADO && r > 0)
            fb_retangulo(tela, x-r, y-r, 2*r+1, 2*r+1, 0, 1, cor);
        else if (q->forma == PARTICULA_CIRCULO && r > 0)
            fb_circulo(tela, x, y, r, 0, 1, cor);
        else fb_misturar(tela, x, y, cor);
    }
}
