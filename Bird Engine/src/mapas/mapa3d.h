#ifndef MAPA3D_H
#define MAPA3D_H

#include "../objetos/obj_3d.h"
#include "../cam/cam3d.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAPA3D_MAX_ANEXOS 8
#define MAPA3D_MAX_LIGADOS 8
#define MAPA3D_MAX_TILES 32

enum { MAPA3D_MOV_ALTERNAR = 0, MAPA3D_MOV_VAIVEM = 1 };
enum { MAPA3D_CURVA_LINEAR = 0, MAPA3D_CURVA_SUAVE = 1 };
enum { MAPA3D_ENTRADA_APERTAR = 0, MAPA3D_ENTRADA_SEGURAR = 1, MAPA3D_ENTRADA_PROXIMIDADE = 2 };
enum {
    MAPA3D_ACAO_ALTERNAR  = 0,
    MAPA3D_ACAO_ABRIR     = 1,
    MAPA3D_ACAO_FECHAR    = 2,
    MAPA3D_ACAO_IR        = 3,
    MAPA3D_ACAO_TRAVAR    = 4,
    MAPA3D_ACAO_DESTRAVAR = 5
};

typedef struct mapa3d_tile {
    char     nome[24];
    uint16_t coluna, linha;
} mapa3d_tile;

typedef struct mapa3d_atuador {
    int           usado;
    obj3d_handle* obj;
    obj3d_handle* anexos[MAPA3D_MAX_ANEXOS];
    float         anexo_base[MAPA3D_MAX_ANEXOS][3];
    float         anexo_ang0[MAPA3D_MAX_ANEXOS];
    int           n_anexos;
    float         base[3];
    float         desloc[3];
    float         graus, pivo_x, pivo_z, ang0;
    float         vel, dur;
    int           modo, curva;
    float         t, alvo;
    float         pausa, espera;
    float         auto_fechar, tempo_auto_fechar;
    int           ativo, dir;
    int           carregar, travado, movendo;
    float         delta[3];
} mapa3d_atuador_info;

typedef struct mapa3d_interativo {
    int           usado, ativo;
    obj3d_handle* obj;
    float         x, y, z;
    float         alcance, angulo;
    int           entrada, acao;
    float         valor;
    float         recarga, espera;
    int           uma_vez;
    int           ligados[MAPA3D_MAX_LIGADOS];
    int           n_ligados;
    int           dentro, segurando_ativo;
    int           disparou, disparos;
} mapa3d_interativo_info;

typedef struct mapa3d_giro {
    obj3d_handle* obj;
    float         rx, ry, rz;
} mapa3d_giro;

typedef struct mapa3d_handle {
    fb_atlas_handle*   atlas;
    obj3d_handle**     objetos;
    int                n_objetos, cap_objetos;
    mapa3d_tile        tiles[MAPA3D_MAX_TILES];
    int                n_tiles;
    mapa3d_atuador_info*    atuadores;
    int                n_atuadores, cap_atuadores;
    mapa3d_interativo_info* interativos;
    int                n_interativos, cap_interativos;
    mapa3d_giro*       giros;
    int                n_giros, cap_giros;
    int                foco;
} mapa3d_handle;

FB_API mapa3d_handle* mapa3d_criar(void);
FB_API void           mapa3d_destruir(mapa3d_handle* m);
FB_API void           mapa3d_limpar(mapa3d_handle* m);
FB_API void           mapa3d_atlas(mapa3d_handle* m, fb_atlas_handle* atlas);

FB_API int  mapa3d_tile_definir(mapa3d_handle* m, const char* nome, int coluna, int linha);
FB_API int  mapa3d_tile_buscar(const mapa3d_handle* m, const char* nome, int* coluna, int* linha);

FB_API int                   mapa3d_adicionar(mapa3d_handle* m, obj3d_handle* o);
FB_API int                   mapa3d_remover(mapa3d_handle* m, obj3d_handle* o);
FB_API obj3d_handle* const*  mapa3d_objetos(const mapa3d_handle* m, int* n);

FB_API obj3d_handle* mapa3d_cubo(mapa3d_handle* m, float x, float y, float z, float largura, float altura, float profundidade, int coluna, int linha);
FB_API obj3d_handle* mapa3d_cilindro(mapa3d_handle* m, float x, float y, float z, float raio, float altura, int fatias, int coluna, int linha);
FB_API obj3d_handle* mapa3d_cone(mapa3d_handle* m, float x, float y, float z, float raio, float altura, int fatias, int coluna, int linha);
FB_API obj3d_handle* mapa3d_esfera(mapa3d_handle* m, float x, float y, float z, float raio, int fatias, int aneis, int coluna, int linha);
FB_API obj3d_handle* mapa3d_poligono(mapa3d_handle* m, float x, float y, float z, const float* pontos, int n, float altura, int coluna, int linha);
FB_API obj3d_handle* mapa3d_billboard(mapa3d_handle* m, float x, float y, float z, float largura, float altura, int coluna, int linha);

FB_API void mapa3d_chao(mapa3d_handle* m, float tamanho, int divisao, int coluna, int linha);
FB_API void mapa3d_paredes(mapa3d_handle* m, float tamanho, float altura, int coluna, int linha);
FB_API void mapa3d_escada(mapa3d_handle* m, float x, float z, float largura, int degraus, float altura_degrau, float profundidade, int coluna, int linha);
FB_API void mapa3d_girar(mapa3d_handle* m, obj3d_handle* o, float rx, float ry, float rz);

FB_API int   mapa3d_atuador(mapa3d_handle* m, obj3d_handle* o);
FB_API void  mapa3d_atuador_deslocar(mapa3d_handle* m, int id, float dx, float dy, float dz);
FB_API void  mapa3d_atuador_giro(mapa3d_handle* m, int id, float pivo_x, float pivo_z, float graus);
FB_API void  mapa3d_atuador_velocidade(mapa3d_handle* m, int id, float velocidade);
FB_API void  mapa3d_atuador_modo(mapa3d_handle* m, int id, int modo);
FB_API void  mapa3d_atuador_curva(mapa3d_handle* m, int id, int curva);
FB_API void  mapa3d_atuador_pausa(mapa3d_handle* m, int id, float segundos);
FB_API void  mapa3d_atuador_auto_fechar(mapa3d_handle* m, int id, float segundos);
FB_API void  mapa3d_atuador_carregar(mapa3d_handle* m, int id, int ligado);
FB_API void  mapa3d_atuador_ativo(mapa3d_handle* m, int id, int ligado);
FB_API int   mapa3d_atuador_anexar(mapa3d_handle* m, int id, obj3d_handle* o);
FB_API void  mapa3d_atuador_acao(mapa3d_handle* m, int id, int acao, float valor);
FB_API float mapa3d_atuador_t(const mapa3d_handle* m, int id);
FB_API int   mapa3d_atuador_aberto(const mapa3d_handle* m, int id);
FB_API int   mapa3d_atuador_movendo(const mapa3d_handle* m, int id);
FB_API int   mapa3d_atuador_travado(const mapa3d_handle* m, int id);

FB_API int mapa3d_porta_deslizar(mapa3d_handle* m, obj3d_handle* o, float dx, float dy, float dz, float velocidade);
FB_API int mapa3d_porta_subir(mapa3d_handle* m, obj3d_handle* o, float altura, float velocidade);
FB_API int mapa3d_porta_descer(mapa3d_handle* m, obj3d_handle* o, float altura, float velocidade);
FB_API int mapa3d_porta_girar(mapa3d_handle* m, obj3d_handle* o, float pivo_x, float pivo_z, float graus, float velocidade);
FB_API int mapa3d_elevador(mapa3d_handle* m, obj3d_handle* o, float altura, float velocidade);
FB_API int mapa3d_andador(mapa3d_handle* m, obj3d_handle* o, float dx, float dy, float dz, float velocidade, float pausa, int ativo);

FB_API int   mapa3d_interativo(mapa3d_handle* m, obj3d_handle* ancora, float x, float y, float z, float alcance);
FB_API void  mapa3d_interativo_entrada(mapa3d_handle* m, int id, int entrada);
FB_API void  mapa3d_interativo_acao(mapa3d_handle* m, int id, int acao, float valor);
FB_API int   mapa3d_interativo_ligar(mapa3d_handle* m, int id, int atuador);
FB_API void  mapa3d_interativo_alcance(mapa3d_handle* m, int id, float alcance);
FB_API void  mapa3d_interativo_olhar(mapa3d_handle* m, int id, float graus);
FB_API void  mapa3d_interativo_recarga(mapa3d_handle* m, int id, float segundos);
FB_API void  mapa3d_interativo_uma_vez(mapa3d_handle* m, int id, int ligado);
FB_API void  mapa3d_interativo_ativar(mapa3d_handle* m, int id, int ligado);
FB_API int   mapa3d_interativo_disparou(const mapa3d_handle* m, int id);
FB_API int   mapa3d_interativo_disparos(const mapa3d_handle* m, int id);
FB_API int   mapa3d_interativo_dentro(const mapa3d_handle* m, int id);
FB_API int   mapa3d_foco(const mapa3d_handle* m);

FB_API void  mapa3d_atualizar(mapa3d_handle* m, float dt, cam3d_handle* cam, int apertou, int segurando);
FB_API void  mapa3d_desenhar(mapa3d_handle* m, cam3d_handle* cam, fb_handle* fb, zb_handle* zb);

#ifdef __cplusplus
}
#endif

#endif
