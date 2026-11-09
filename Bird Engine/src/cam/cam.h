#ifndef CAM_H
#define CAM_H

#include "../frame buffer/frame_buffer.h"
#include "../objetos/obj.h"
#include "../mapas/mapas.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct camera_handle {
    fb_handle* view;

    float    cx, cy;
    uint16_t w, h;
    float    ang;
    uint8_t  opaca;

    float    zoom, zoom_min, zoom_max;
    float    zoom_alvo;
    uint8_t  zoom_suave;
    uint8_t  zoom_alvo_ativo;

    int32_t  ox, oy;
    int64_t  bcx, bcy;
    int32_t  ca, sa;
    uint8_t  reto;

    fb_handle* colunas;
    int32_t  cmin, cmax;
    uint8_t  usa_lut;
    int64_t  lut_cx;
    int32_t  lut_ca;
    int32_t  lut_w, lut_bw;

    const obj_handle* alvo;
    int32_t  alvo_dx, alvo_dy;
    uint8_t  alvo_suave;

    uint8_t  travada;
    int32_t  lx0, ly0, lx1, ly1;
} camera_handle;

FB_API camera_handle* camera_criar(uint16_t largura, uint16_t altura, int32_t x, int32_t y);
FB_API void     camera_destruir(camera_handle* c);
FB_API int      camera_valida(const camera_handle* c);

FB_API int32_t  camera_x(const camera_handle* c);
FB_API int32_t  camera_y(const camera_handle* c);
FB_API int32_t  camera_largura_mundo(const camera_handle* c);
FB_API int32_t  camera_altura_mundo(const camera_handle* c);
FB_API uint16_t camera_largura(const camera_handle* c);
FB_API uint16_t camera_altura(const camera_handle* c);

FB_API float    camera_angulo(const camera_handle* c);
FB_API void     camera_angulo_definir(camera_handle* c, float graus);
FB_API void     camera_girar(camera_handle* c, float delta);
FB_API float    camera_zoom(const camera_handle* c);
FB_API void     camera_zoom_definir(camera_handle* c, float z);
FB_API void     camera_zoom_relativo(camera_handle* c, float fator);
FB_API void     camera_zoom_em(camera_handle* c, float z, int32_t tela_x, int32_t tela_y);
FB_API void     camera_zoom_suave(camera_handle* c, float alvo, uint8_t suavidade);
FB_API void     camera_limites_zoom(camera_handle* c, float minimo, float maximo);

FB_API void     camera_posicionar(camera_handle* c, int32_t x, int32_t y);
FB_API void     camera_mover(camera_handle* c, int32_t dx, int32_t dy);
FB_API void     camera_centralizar(camera_handle* c, int32_t mundo_x, int32_t mundo_y);
FB_API void     camera_mover_suave(camera_handle* c, int32_t mundo_x, int32_t mundo_y, uint8_t suavidade, float delta);
FB_API void     camera_centralizar_mundo(camera_handle* c, const mapa_handle* mapa, uint16_t pw, uint16_t ph);

FB_API void     camera_ancorar(camera_handle* c, const obj_handle* alvo, int32_t desloc_x, int32_t desloc_y, uint8_t suavidade);
FB_API void     camera_soltar(camera_handle* c);
FB_API void     camera_atualizar(camera_handle* c, float delta);
FB_API void     camera_travar_mapa(camera_handle* c, const mapa_handle* mapa, uint16_t tile_w, uint16_t tile_h);
FB_API void     camera_travar_limites(camera_handle* c, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
FB_API void     camera_destravar(camera_handle* c);

FB_API void     camera_mundo_para_tela(const camera_handle* c, int32_t mundo_x, int32_t mundo_y, int32_t* tela_x, int32_t* tela_y);
FB_API void     camera_tela_para_mundo(const camera_handle* c, int32_t tela_x, int32_t tela_y, int32_t* mundo_x, int32_t* mundo_y);

FB_API void     camera_iniciar(camera_handle* c, uint32_t fundo);
FB_API int      camera_visivel(const camera_handle* c, int32_t x, int32_t y, int32_t w, int32_t h);
FB_API void     camera_desenhar_obj(camera_handle* c, obj_handle* o);
FB_API void     camera_desenhar_mapa(camera_handle* c, mapa_handle* mapa, int camada);
FB_API void     camera_apresentar(const camera_handle* c, fb_handle* janela, int sx, int sy);

FB_API uint32_t* camera_viewport_dados(camera_handle* c);
FB_API int      camera_viewport_largura(camera_handle* c);
FB_API int      camera_viewport_altura(camera_handle* c);

#ifdef __cplusplus
}
#endif

#endif
