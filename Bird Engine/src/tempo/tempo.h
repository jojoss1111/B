#ifndef TEMPO_H
#define TEMPO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Tempo {
    double  fim;        /* instante global em que termina (rodando)      */
    double  restante;   /* segundos que faltam (so vale quando parado)   */
    float   duracao;    /* duracao total em segundos                     */
    uint8_t parado;     /* 1 = pausado                                   */
    uint8_t repetir;    /* 1 = passou_tempo rearma sozinho               */
} Tempo;

/* ---- Relogio global ---- */

/* Chamar uma vez por frame, antes de rodar(delta). */
void   tempo_atualizar(float delta);
float  tempo_agora(void);                 /* segundos acumulados no relogio  */

void   tempo_global_parar(void);          /* congela todos os Tempo (pause)  */
void   tempo_global_retomar(void);
int    tempo_global_parado(void);
void   tempo_escala(float escala);        /* 1.0 normal, 0.5 lento, 2.0 rapido */
float  tempo_escala_atual(void);

/* ---- Criacao ---- */

Tempo  criar_tempo(float segundos);           /* ja comeca rodando          */
Tempo  criar_tempo_repetido(float segundos);  /* passou_tempo rearma sozinho */

/* ---- Consulta ---- */

int    passou_tempo(Tempo* t);        /* 1 se acabou (repetido: 1 por ciclo) */
int    nao_passou_tempo(Tempo* t);    /* o inverso                           */

float  tempo_restante(const Tempo* t);    /* segundos que faltam (>= 0)      */
float  tempo_decorrido(const Tempo* t);   /* segundos ja passados            */
float  tempo_progresso(const Tempo* t);   /* 0.0 .. 1.0                      */
int    tempo_esta_parado(const Tempo* t);

/* ---- Controle ---- */

void   parar_tempo(Tempo* t);         /* pausa, guarda o que faltava         */
void   retomar_tempo(Tempo* t);       /* continua de onde parou              */
void   reiniciar_tempo(Tempo* t);     /* volta ao inicio e roda              */
void   definir_tempo(Tempo* t, float segundos);   /* nova duracao + reinicia */
void   adicionar_tempo(Tempo* t, float segundos); /* soma (ou tira) segundos */

#ifdef __cplusplus
}
#endif

#endif