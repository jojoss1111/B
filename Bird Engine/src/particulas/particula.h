#ifndef PARTICULA_H
#define PARTICULA_H

#include "../frame buffer/frame_buffer.h"

struct camera_handle;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PARTICULA_CIRCULO = 0,
    PARTICULA_PIXEL,
    PARTICULA_QUADRADO,
    PARTICULA_LINHA
} particula_forma;

typedef struct {
    float n_min, n_max;
    float vida_min, vida_max;
    float vel_min, vel_max;
    float angulo, abertura;
    float dispersao, gravidade, vento, arrasto;
    float chao, quique, atrito;
    uint8_t tem_chao;
    float tam_ini_min, tam_ini_max, tam_fim_min, tam_fim_max;
    float rastro, desvanecer;
    uint32_t cores[16];
    uint8_t n_cores;
    particula_forma forma;
} particula_config;

typedef struct {
    float x, y, vx, vy, vida, tempo;
    float raio_ini, raio_fim, gravidade, vento, arrasto;
    float chao, quique, atrito, rastro, desvanecer;
    uint32_t cores[16];
    uint8_t n_cores;
    uint8_t tem_chao;
    particula_forma forma;
} particula_item;

typedef struct particulas_handle {
    particula_item* itens;
    uint32_t capacidade, quantidade;
    uint32_t aleatorio;
} particulas_handle;

FB_API particulas_handle* particulas_criar(uint32_t capacidade);
FB_API void particulas_destruir(particulas_handle* p);
FB_API void particulas_limpar(particulas_handle* p);
FB_API uint32_t particulas_quantidade(const particulas_handle* p);
FB_API uint32_t particulas_emitir(particulas_handle* p, const particula_config* cfg, float x, float y);
FB_API void particulas_atualizar(particulas_handle* p, float delta);
FB_API void particulas_desenhar(const particulas_handle* p, fb_handle* tela,
                                const struct camera_handle* camera, int ox, int oy);

#ifdef __cplusplus
}
#endif

#endif
