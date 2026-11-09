#ifndef MAPAS_H
#define MAPAS_H

#include "../objetos/obj.h"

#define MAPA_ABAIXO 1
#define MAPA_MUNDO 2
#define MAPA_FRENTE 3

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mapa_handle mapa_handle;

FB_API mapa_handle *mapa_novo(void);
FB_API void mapa_destruir(mapa_handle *mapa);
FB_API int mapa_criar_mapa(mapa_handle *mapa, uint16_t tile_w, uint16_t tile_h,
                           const char *const textos[3]);
FB_API void mapa_posicionar(mapa_handle *mapa, int camada, int32_t x,
                            int32_t y);
FB_API void mapa_mover(mapa_handle *mapa, int camada, int32_t dx, int32_t dy);
FB_API int32_t mapa_x(const mapa_handle *mapa, int camada);
FB_API int32_t mapa_y(const mapa_handle *mapa, int camada);
FB_API uint32_t mapa_colunas(const mapa_handle *mapa);
FB_API uint32_t mapa_linhas(const mapa_handle *mapa);
FB_API uint16_t mapa_tile_largura(const mapa_handle *mapa);
FB_API uint16_t mapa_tile_altura(const mapa_handle *mapa);
FB_API int mapa_tile(const mapa_handle *mapa, int camada, uint32_t x,
                     uint32_t y);
FB_API void mapa_definir(mapa_handle *mapa, int camada, uint32_t x, uint32_t y,
                         int tile);
FB_API void mapa_substituir(mapa_handle *mapa, int camada, int antigo, int novo,
                            int32_t x, int32_t y, int tudo);

FB_API void mapa_sprites(mapa_handle *mapa, int camada, int tile,
                         fb_atlas_handle *atlas, uint16_t coluna,
                         uint16_t linha);
FB_API void mapa_ampliar(mapa_handle *mapa, int camada, int tile, uint16_t w,
                         uint16_t h);
FB_API void mapa_cor(mapa_handle *mapa, int camada, int tile, uint32_t cor);
FB_API void mapa_angulo(mapa_handle *mapa, int camada, int tile, float graus);
FB_API void mapa_alpha(mapa_handle *mapa, int camada, int tile, uint8_t alpha);
FB_API void mapa_espelhar_x(mapa_handle *mapa, int camada, int tile,
                            int ligado);
FB_API void mapa_espelhar_y(mapa_handle *mapa, int camada, int tile,
                            int ligado);
FB_API void mapa_colisor(mapa_handle *mapa, int camada, int tile,
                         const char *mascara, uint16_t mascara_w,
                         uint16_t mascara_h);
FB_API void mapa_animacao(mapa_handle *mapa, int tile, fb_atlas_handle *atlas,
                          const uint16_t (*quadros)[2], uint16_t total,
                          uint16_t fps, int repetir);
FB_API void mapa_atualizar_animacao(mapa_handle *mapa, uint32_t delta_ms);
FB_API void mapa_parar(mapa_handle *mapa);
FB_API void mapa_rodar(mapa_handle *mapa);
FB_API void mapa_voltar(mapa_handle *mapa);
FB_API int mapa_colidio_x(const mapa_handle *mapa, int camada,
                          const aabb_2d_handle *obj, int32_t dx);
FB_API int mapa_colidio_y(const mapa_handle *mapa, int camada,
                          const aabb_2d_handle *obj, int32_t dy);
FB_API int mapa_colidio_tile_x(const mapa_handle *mapa, int camada, int tile,
                               const aabb_2d_handle *obj, int32_t dx);
FB_API int mapa_colidio_tile_y(const mapa_handle *mapa, int camada, int tile,
                               const aabb_2d_handle *obj, int32_t dy);
FB_API void mapa_desenhar(const mapa_handle *mapa, int camada, fb_handle *tela);

#ifdef __cplusplus
}
#endif

#endif
