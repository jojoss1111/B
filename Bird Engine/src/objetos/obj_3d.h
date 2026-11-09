#ifndef OBJ_3D_H
#define OBJ_3D_H

#include "../frame buffer/z_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum obj3d_forma {
    OBJ3D_CUBO     = 0,
    OBJ3D_ESFERA   = 1,
    OBJ3D_CONE     = 2,
    OBJ3D_CILINDRO = 3,
    OBJ3D_MODELO   = 4,
    OBJ3D_POLIGONO = 5,
    OBJ3D_BILLBOARD = 6
} obj3d_forma;

enum {
    OBJ3D_COL_NENHUMA = 0,
    OBJ3D_COL_CAIXA   = 1
};

enum {
    OBJ3D_BB_NENHUM     = 0,
    OBJ3D_BB_ESFERICO   = 1,
    OBJ3D_BB_CILINDRICO = 2
};

typedef struct obj3d_caixa {
    float c[3];
    float u[3][3];
    float e[3];
} obj3d_caixa;

typedef struct obj3d_cil {
    float x, z;
    float y0, y1;
    float r;
} obj3d_cil;

typedef struct obj3d_parte {
    zb_vertice* vertices;
    uint16_t*   indices;
    int         n_vertices;
    int         n_indices;
    uint16_t    col, lin;
    int         visivel;
    char        nome[48];
    float       rep_u, rep_v;
    float       dens_u, dens_v;
    int         repetir;
} obj3d_parte;

typedef struct obj3d_handle {
    obj3d_forma      forma;
    fb_atlas_handle* atlas;
    float            x, y, z;
    float            rx, ry, rz;
    float            sx, sy, sz;
    uint32_t         cor;
    int              modo;
    int              cull;
    int              visivel;
    int              n_partes;
    int              cap_partes;
    int              colisao;
    int              billboard;
    float            col_raio;
    float            bmin[3], bmax[3];
    obj3d_parte*     partes;
} obj3d_handle;

FB_API obj3d_handle* obj3d_criar_cubo(fb_atlas_handle* atlas, float x, float y, float z, float largura, float altura, float profundidade, float rep_largura, float rep_altura, int repetir);
FB_API obj3d_handle* obj3d_criar_esfera(fb_atlas_handle* atlas, float x, float y, float z, float raio, int fatias, int aneis, float rep_largura, float rep_altura, int repetir);
FB_API obj3d_handle* obj3d_criar_cone(fb_atlas_handle* atlas, float x, float y, float z, float raio, float altura, int fatias, float rep_largura, float rep_altura, int repetir);
FB_API obj3d_handle* obj3d_criar_cilindro(fb_atlas_handle* atlas, float x, float y, float z, float raio, float altura, int fatias, float rep_largura, float rep_altura, int repetir);
FB_API obj3d_handle* obj3d_criar_poligono(fb_atlas_handle* atlas, float x, float y, float z, const float* pontos, int n, float altura, float rep_largura, float rep_altura, int repetir);
FB_API obj3d_handle* obj3d_criar_billboard(fb_atlas_handle* atlas, float x, float y, float z, float largura, float altura, float rep_largura, float rep_altura, int repetir);
FB_API void          obj3d_destruir(obj3d_handle* o);

FB_API void obj3d_billboard(obj3d_handle* o, int tipo);
FB_API int  obj3d_billboard_tipo(const obj3d_handle* o);

FB_API int  obj3d_poligono_n(const obj3d_handle* o);
FB_API int  obj3d_poligono_ler(const obj3d_handle* o, int i, float* x, float* z);
FB_API int  obj3d_poligono_pontos(obj3d_handle* o, const float* pontos, int n);
FB_API int  obj3d_poligono_ponto(obj3d_handle* o, int i, float x, float z);
FB_API int  obj3d_poligono_girar_pontos(obj3d_handle* o, float graus, float cx, float cz);
FB_API int  obj3d_poligono_pivo(obj3d_handle* o, float px, float pz);
FB_API void obj3d_poligono_altura(obj3d_handle* o, float altura);

FB_API void obj3d_repetir(obj3d_handle* o, float rep_largura, float rep_altura, int repetir);
FB_API void obj3d_repetir_cima(obj3d_handle* o, float rep_largura, float rep_altura, int repetir);
FB_API void obj3d_repetir_lado(obj3d_handle* o, float rep_largura, float rep_altura, int repetir);
FB_API void obj3d_repetir_baixo(obj3d_handle* o, float rep_largura, float rep_altura, int repetir);
FB_API void obj3d_parte_repetir(obj3d_handle* o, int i, float rep_largura, float rep_altura, int repetir);

FB_API obj3d_forma obj3d_forma_de(const obj3d_handle* o);
FB_API float obj3d_x(const obj3d_handle* o);
FB_API float obj3d_y(const obj3d_handle* o);
FB_API float obj3d_z(const obj3d_handle* o);
FB_API float obj3d_rot_x(const obj3d_handle* o);
FB_API float obj3d_rot_y(const obj3d_handle* o);
FB_API float obj3d_rot_z(const obj3d_handle* o);
FB_API float obj3d_escala_x(const obj3d_handle* o);
FB_API float obj3d_escala_y(const obj3d_handle* o);
FB_API float obj3d_escala_z(const obj3d_handle* o);

FB_API void obj3d_posicionar(obj3d_handle* o, float x, float y, float z);
FB_API void obj3d_mover(obj3d_handle* o, float x, float y, float z);
FB_API void obj3d_rotacionar(obj3d_handle* o, float rx, float ry, float rz);
FB_API void obj3d_girar(obj3d_handle* o, float rx, float ry, float rz);
FB_API void obj3d_rotacionar_x(obj3d_handle* o, float graus);
FB_API void obj3d_rotacionar_y(obj3d_handle* o, float graus);
FB_API void obj3d_rotacionar_z(obj3d_handle* o, float graus);
FB_API void obj3d_escalar(obj3d_handle* o, float sx, float sy, float sz);
FB_API void obj3d_escalar_uniforme(obj3d_handle* o, float s);
FB_API void obj3d_ampliar(obj3d_handle* o, float sx, float sy, float sz);

FB_API void obj3d_atlas(obj3d_handle* o, fb_atlas_handle* atlas);
FB_API void obj3d_tile(obj3d_handle* o, uint16_t coluna, uint16_t linha);
FB_API void obj3d_tile_cima(obj3d_handle* o, uint16_t coluna, uint16_t linha);
FB_API void obj3d_tile_lado(obj3d_handle* o, uint16_t coluna, uint16_t linha);
FB_API void obj3d_tile_baixo(obj3d_handle* o, uint16_t coluna, uint16_t linha);
FB_API void obj3d_tile_frente(obj3d_handle* o, uint16_t coluna, uint16_t linha);
FB_API void obj3d_tile_tras(obj3d_handle* o, uint16_t coluna, uint16_t linha);
FB_API void obj3d_tile_esquerda(obj3d_handle* o, uint16_t coluna, uint16_t linha);
FB_API void obj3d_tile_direita(obj3d_handle* o, uint16_t coluna, uint16_t linha);
FB_API void obj3d_cor(obj3d_handle* o, uint32_t cor);
FB_API void obj3d_modo(obj3d_handle* o, int modo);
FB_API void obj3d_cull(obj3d_handle* o, int cull);
FB_API void obj3d_visivel(obj3d_handle* o, int visivel);

FB_API void obj3d_colisao(obj3d_handle* o, int tipo);
FB_API void obj3d_colisao_raio(obj3d_handle* o, float raio);
FB_API int  obj3d_cilindro_mundo(const obj3d_handle* o, obj3d_cil* saida);
FB_API int  obj3d_caixa_mundo(const obj3d_handle* o, obj3d_caixa* saida);
FB_API void obj3d_recalcular_limites(obj3d_handle* o);

FB_API obj3d_handle* obj3d_carregar(fb_atlas_handle* atlas, const char* caminho, float x, float y, float z, float escala);
FB_API obj3d_handle* obj3d_carregar_memoria(fb_atlas_handle* atlas, const uint8_t* dados, size_t tamanho, float x, float y, float z, float escala);
FB_API void          obj3d_normalizar(obj3d_handle* o);

FB_API int         obj3d_partes(const obj3d_handle* o);
FB_API const char* obj3d_parte_nome(const obj3d_handle* o, int i);
FB_API int         obj3d_parte_buscar(const obj3d_handle* o, const char* nome);
FB_API void        obj3d_parte_visivel(obj3d_handle* o, int i, int visivel);
FB_API void        obj3d_parte_tile(obj3d_handle* o, int i, uint16_t coluna, uint16_t linha);

FB_API zb_mat4 obj3d_matriz(const obj3d_handle* o);
FB_API void    obj3d_desenhar(const obj3d_handle* o, fb_handle* fb, zb_handle* zb, const zb_mat4* vp);

#ifdef __cplusplus
}
#endif

#endif
