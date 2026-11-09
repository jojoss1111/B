#ifndef CAIXAS_H
#define CAIXAS_H

#include <stdint.h>
#include "input.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct caixa_handle {
    uint8_t circulo;
    uint8_t ativa;
    int     x, y;
    int     w, h;
    int     raio;
} caixa_handle;

FB_API caixa_handle* caixa_novo(void);
FB_API void  caixa_destruir(caixa_handle* c);
FB_API void  caixa_criar_retangulo(caixa_handle* c, int x, int y, int w, int h);
FB_API void  caixa_criar_circulo(caixa_handle* c, int x, int y, int raio);
FB_API void  caixa_desativar(caixa_handle* c);

FB_API void  caixa_mover(caixa_handle* c, int x, int y);
FB_API void  caixa_tamanho(caixa_handle* c, int w, int h);
FB_API void  caixa_raio(caixa_handle* c, int r);

FB_API int   caixa_ativa(const caixa_handle* c);
FB_API int   caixa_dentro(const caixa_handle* c, int px, int py);

FB_API void  caixa_usar_entrada(const entrada_handle* e);
FB_API int   caixa_mouse_em_cima(const caixa_handle* c);
FB_API int   caixa_pressionou(const caixa_handle* c, int botao);
FB_API int   caixa_segurou(const caixa_handle* c, int botao);
FB_API int   caixa_soltou(const caixa_handle* c, int botao);

FB_API int   caixa_pressionou_em(const caixa_handle* c, const entrada_handle* e, int botao);
FB_API int   caixa_segurou_em(const caixa_handle* c, const entrada_handle* e, int botao);
FB_API int   caixa_soltou_em(const caixa_handle* c, const entrada_handle* e, int botao);

#ifdef __cplusplus
}
#endif

#endif