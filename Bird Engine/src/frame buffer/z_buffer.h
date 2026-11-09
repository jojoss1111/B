#ifndef Z_BUFFER_H
#define Z_BUFFER_H

#include "frame_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Convenções:
 *  - Matrizes row-major: m[linha*4 + coluna], vetor coluna (v' = M * v).
 *  - Espaço de clip estilo OpenGL: câmera olha para -Z, NDC z em -1..1, y para cima.
 *  - Ângulos em graus. Cores ARGB (0xAARRGGBB), iguais às do frame_buffer.
 *  - Face da frente = anti-horário visto na tela.
 *  - O z_buffer deve ter o mesmo tamanho do frame_buffer onde se desenha.
 */

typedef struct zb_vec3 { float x, y, z; } zb_vec3;

typedef struct zb_mat4 { float m[16]; } zb_mat4;

typedef struct zb_vertice {
    float    x, y, z;
    float    u, v;
    uint32_t cor;
} zb_vertice;

typedef struct zb_malha {
    const zb_vertice* vertices;
    int               n_vertices;
    const uint16_t*   indices;
    int               n_indices;
} zb_malha;

enum {
    ZB_MODO_PLANO   = 0,
    ZB_MODO_GOURAUD = 1,
    ZB_MODO_TEXTURA = 2 
};

enum {
    ZB_CULL_NENHUM = 0,
    ZB_CULL_TRAS   = 1,
    ZB_CULL_FRENTE = 2
};

typedef struct zb_estado {
    int                    modo;
    int                    cull;
    const fb_atlas_handle* atlas;
    int                    tile_x;
    int                    tile_y;
    float                  rep_u;
    float                  rep_v;
} zb_estado;

typedef struct zb_handle {
    float* z;
    int    w;
    int    h;
} zb_handle;

/* z_buffer */
FB_API zb_handle*  zb_criar(int largura, int altura);
FB_API void        zb_destruir(zb_handle* zb);
FB_API int         zb_valido(const zb_handle* zb);
FB_API int         zb_largura(const zb_handle* zb);
FB_API int         zb_altura(const zb_handle* zb);
FB_API void        zb_limpar(zb_handle* zb);
FB_API float       zb_ler(const zb_handle* zb, int x, int y);

/* matemática */
FB_API zb_vec3  zb_vec3_criar(float x, float y, float z);
FB_API zb_mat4  zb_mat4_identidade(void);
FB_API zb_mat4  zb_mat4_multiplicar(const zb_mat4* a, const zb_mat4* b);
FB_API zb_mat4  zb_mat4_translacao(float x, float y, float z);
FB_API zb_mat4  zb_mat4_escala(float x, float y, float z);
FB_API zb_mat4  zb_mat4_rotacao_x(float graus);
FB_API zb_mat4  zb_mat4_rotacao_y(float graus);
FB_API zb_mat4  zb_mat4_rotacao_z(float graus);
FB_API zb_mat4  zb_mat4_perspectiva(float fov_graus, float aspecto, float perto, float longe);
FB_API zb_mat4  zb_mat4_ortografica(float esq, float dir, float baixo, float cima, float perto, float longe);

/* renderização */
FB_API zb_estado zb_estado_padrao(void);

FB_API void zb_triangulo(fb_handle* fb, zb_handle* zb, const zb_estado* est, const zb_mat4* mvp,
                         const zb_vertice* a, const zb_vertice* b, const zb_vertice* c);

FB_API void zb_malha_desenhar(fb_handle* fb, zb_handle* zb, const zb_estado* est, const zb_mat4* mvp,
                              const zb_malha* malha);

#ifdef __cplusplus
}
#endif

#endif