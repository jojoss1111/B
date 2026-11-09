#include "tempo.h"

/* double no relogio: float perde precisao depois de algumas horas de jogo */
static double  g_agora  = 0.0;
static double  g_escala = 1.0;
static uint8_t g_parado = 0;

/* ---- Relogio global ---- */

void tempo_atualizar(float delta) {
    if (g_parado || delta <= 0.0f) return;
    g_agora += (double)delta * g_escala;
}

float tempo_agora(void)             { return (float)g_agora; }

void  tempo_global_parar(void)      { g_parado = 1; }
void  tempo_global_retomar(void)    { g_parado = 0; }
int   tempo_global_parado(void)     { return g_parado; }

void tempo_escala(float escala) {
    g_escala = escala < 0.0f ? 0.0 : (double)escala;
}

float tempo_escala_atual(void)      { return (float)g_escala; }

/* ---- Interno ---- */

static double restante_de(const Tempo* t) {
    const double r = t->parado ? t->restante : t->fim - g_agora;
    return r > 0.0 ? r : 0.0;
}

static Tempo novo(float segundos, uint8_t repetir) {
    Tempo t;
    if (segundos < 0.0f) segundos = 0.0f;
    t.duracao  = segundos;
    t.restante = (double)segundos;
    t.fim      = g_agora + (double)segundos;
    t.parado   = 0;
    t.repetir  = repetir;
    return t;
}

/* ---- Criacao ---- */

Tempo criar_tempo(float segundos)          { return novo(segundos, 0); }
Tempo criar_tempo_repetido(float segundos) { return novo(segundos, 1); }

/* ---- Consulta ---- */

int passou_tempo(Tempo* t) {
    if (!t || restante_de(t) > 0.0) return 0;

    if (t->repetir && !t->parado) {
        t->fim += (double)t->duracao;
        /* travou por muito tempo: nao dispara varias vezes de uma vez */
        if (t->fim <= g_agora) t->fim = g_agora + (double)t->duracao;
    }
    return 1;
}

int nao_passou_tempo(Tempo* t) { return !passou_tempo(t); }

float tempo_restante(const Tempo* t) {
    return t ? (float)restante_de(t) : 0.0f;
}

float tempo_decorrido(const Tempo* t) {
    float d;
    if (!t) return 0.0f;
    d = t->duracao - (float)restante_de(t);
    return d > 0.0f ? d : 0.0f;
}

float tempo_progresso(const Tempo* t) {
    float p;
    if (!t) return 0.0f;
    if (t->duracao <= 0.0f) return 1.0f;
    p = tempo_decorrido(t) / t->duracao;
    return p > 1.0f ? 1.0f : p;
}

int tempo_esta_parado(const Tempo* t) { return t ? t->parado : 0; }

/* ---- Controle ---- */

void parar_tempo(Tempo* t) {
    if (!t || t->parado) return;
    t->restante = restante_de(t);
    t->parado = 1;
}

void retomar_tempo(Tempo* t) {
    if (!t || !t->parado) return;
    t->fim = g_agora + t->restante;
    t->parado = 0;
}

void reiniciar_tempo(Tempo* t) {
    if (!t) return;
    t->restante = (double)t->duracao;
    t->fim      = g_agora + (double)t->duracao;
    t->parado   = 0;
}

void definir_tempo(Tempo* t, float segundos) {
    if (!t) return;
    t->duracao = segundos < 0.0f ? 0.0f : segundos;
    reiniciar_tempo(t);
}

void adicionar_tempo(Tempo* t, float segundos) {
    double r;
    if (!t) return;
    r = restante_de(t) + (double)segundos;
    if (r < 0.0) r = 0.0;
    t->restante = r;
    t->fim = g_agora + r;
    if ((float)r > t->duracao) t->duracao = (float)r;
}