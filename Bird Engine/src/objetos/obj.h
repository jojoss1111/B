#ifndef OBJ_H
#define OBJ_H

#include "../frame buffer/frame_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct obj_handle {
    fb_atlas_handle* sprite;
    int32_t  x, y;
    uint16_t pw, ph;
    uint16_t col, lin;
    float    ang;
    uint8_t  alpha;
    uint8_t  flip_x, flip_y;
    uint8_t  tr, tg, tb, ta;
    uint8_t  subst;
} obj_handle;

typedef struct anim_handle {
    obj_handle*     obj;
    const int16_t (*arr)[2];
    uint8_t         total, idx;
    uint32_t        ms_quadro, acum;
    uint8_t         loop, tocando;
} anim_handle;

typedef struct aabb_2d_handle {
    int32_t     x, y;
    uint16_t    w, h;
    const char* mascara;
    uint16_t    mascara_w, mascara_h;
} aabb_2d_handle;

void obj_iniciar(obj_handle* o, fb_atlas_handle* sprite, uint16_t pw, uint16_t ph, int32_t x, int32_t y, uint16_t coluna, uint16_t linha);
void obj_colorir(obj_handle* o, int substituir);
void aabb_2d_iniciar(aabb_2d_handle* a, obj_handle* o, const char* mascara, uint16_t mascara_w, uint16_t mascara_h);

FB_API obj_handle* obj_criar(fb_atlas_handle* sprite, uint16_t pw, uint16_t ph, int32_t x, int32_t y, uint16_t coluna, uint16_t linha);
FB_API void     obj_destruir(obj_handle* o);
FB_API int32_t  obj_x(const obj_handle* o);
FB_API int32_t  obj_y(const obj_handle* o);
FB_API uint16_t obj_largura(const obj_handle* o);
FB_API uint16_t obj_altura(const obj_handle* o);
FB_API void     obj_posicionar(obj_handle* o, int32_t x, int32_t y);
FB_API void     obj_mover(obj_handle* o, int32_t x, int32_t y);
FB_API void     obj_tile(obj_handle* o, uint16_t coluna, uint16_t linha);
FB_API void     obj_cor(obj_handle* o, uint32_t cor);
FB_API void     obj_alpha(obj_handle* o, uint8_t a);
FB_API void     obj_ampliar(obj_handle* o, uint16_t pw, uint16_t ph);
FB_API void     obj_angulo(obj_handle* o, float graus);
FB_API void     obj_espelhar_x(obj_handle* o, int espelhar);
FB_API void     obj_espelhar_y(obj_handle* o, int espelhar);
FB_API void     obj_desenhar(const obj_handle* o, fb_handle* fb);

FB_API anim_handle* anim_criar(obj_handle* o, const int16_t* quadros, uint8_t total, uint8_t fps, int loop);
FB_API void anim_destruir(anim_handle* a);
FB_API void anim_rodar(anim_handle* a);
FB_API void anim_parar(anim_handle* a);
FB_API void anim_voltar(anim_handle* a);
FB_API void anim_atualizar(anim_handle* a, uint32_t dt_ms);

FB_API aabb_2d_handle* aabb_2d_criar(obj_handle* o, const char* mascara, uint16_t mascara_w, uint16_t mascara_h);
FB_API void     aabb_2d_destruir(aabb_2d_handle* a);
FB_API void     aabb_2d_sincronizar(aabb_2d_handle* a, obj_handle* o);
FB_API void     aabb_2d_posicionar(aabb_2d_handle* a, int32_t x, int32_t y);
FB_API int32_t  aabb_2d_x(const aabb_2d_handle* a);
FB_API int32_t  aabb_2d_y(const aabb_2d_handle* a);
FB_API uint16_t aabb_2d_largura(const aabb_2d_handle* a);
FB_API uint16_t aabb_2d_altura(const aabb_2d_handle* a);
FB_API int      aabb_2d_colidio(const aabb_2d_handle* a, const aabb_2d_handle* b);
FB_API int      aabb_2d_colidiu_x(const aabb_2d_handle* a, const aabb_2d_handle* outro, int32_t dx);
FB_API int      aabb_2d_colidiu_y(const aabb_2d_handle* a, const aabb_2d_handle* outro, int32_t dy);

#ifdef __cplusplus
}
#endif

#endif