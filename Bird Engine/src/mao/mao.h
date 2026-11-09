#ifndef BIRD_MAO_H
#define BIRD_MAO_H

#include "../objetos/obj_3d.h"
#include "../cam/cam3d.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { MAO_CURTA = 0, MAO_LONGA = 1 };
typedef struct mao_handle mao_handle;

FB_API mao_handle* mao_criar(void);
FB_API void mao_destruir(mao_handle* m);
FB_API void mao_limpar(mao_handle* m);
FB_API int mao_adicionar_3d(mao_handle* m, obj3d_handle* o, float x, float y, float z, float rx, float ry, float rz, int grupo);
FB_API int mao_adicionar_2d(mao_handle* m, fb_atlas_handle* atlas, int coluna, int linha, float x, float y, float w, float h, int grupo);
FB_API void mao_selecionar(mao_handle* m, int grupo);
FB_API int mao_grupo_ativo(const mao_handle* m);
FB_API void mao_atualizar(mao_handle* m, float dt, cam3d_handle* cam);
FB_API void mao_desenhar(mao_handle* m, cam3d_handle* cam, fb_handle* fb, zb_handle* zb);
FB_API void mao_arma(mao_handle* m, int grupo, int tipo, float dano, float alcance, float intervalo, int pente, int reserva, float recuo);
FB_API int mao_usar(mao_handle* m, cam3d_handle* cam, obj3d_handle* const* objs, int n, int* alvo, float* distancia);
FB_API int mao_recarregar(mao_handle* m, int grupo);
FB_API int mao_municao(const mao_handle* m, int grupo, int* reserva);
FB_API float mao_dano(const mao_handle* m, int grupo);

#ifdef __cplusplus
}
#endif
#endif
