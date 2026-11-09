#ifndef CAM3D_H
#define CAM3D_H

#include "../objetos/obj_3d.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CAM3D_COL_AABB = 0,
    CAM3D_COL_OBB  = 1
};

#define CAM3D_MAX_LUZES 8

enum { CAM3D_LUZ_AURA = 0, CAM3D_LUZ_LANTERNA = 1 };
enum { CAM3D_LUZ_PONTO = 0, CAM3D_LUZ_SPOT = 1 };

typedef struct cam3d_luz {
    int   usada, ativa, tipo, sombra;
    float x, y, z;
    float dx, dy, dz;
    float cor_r, cor_g, cor_b;
    float intensidade, alcance;
    float cone_interno, cone_externo;
    float desloc_y;
} cam3d_luz;

typedef struct cam3d_sombrista {
    obj3d_caixa b;
    int   forma;
    float raio;
} cam3d_sombrista;

typedef struct cam3d_handle {
    float px, py, pz;
    float py_antes_passo;
    float pitch, yaw, roll;
    float olho, olho_agach, olho_atual, alt, raio;
    int   tipo_col;
    float grav, pulo, degrau;
    float vy, vel_h, suave_y;
    float mult_correr, mult_agachar, mult_atual;
    int   agachado, agachado_real, correndo, no_chao, no_chao_antes;
    float in_x, in_y, in_z;
    int   bob_ativo;
    float bob_amp, bob_freq, bob_fase, bob_k, bob_y;
    float rec_pitch, rec_yaw, rec_vel;
    float trem_int, trem_dur, trem_t, trem_yaw, trem_pitch, trem_roll;
    uint32_t rng;
    int   chao_ativo, ceu_ativo;
    float chao_y, ceu_y;
    float fov, perto, longe, aspecto;
    obj3d_handle* const* mundo;
    int   n_mundo;
    obj3d_caixa* caixas;
    int   n_caixas, cap_caixas;
    float ambiente;
    int   sombras;
    cam3d_luz luzes[CAM3D_MAX_LUZES];
    cam3d_sombrista* sombristas;
    int   n_sombristas, cap_sombristas;
    int*  cand;
    int   cap_cand;
    int      neb_ativa, neb_ceu;
    float    neb_ini, neb_fim;
    uint32_t neb_cor;
    float    neb_int;
} cam3d_handle;

void cam3d_iniciar(cam3d_handle* c, float x, float y, float z, float angulo_x, float angulo_y, float angulo_z);
void cam3d_liberar(cam3d_handle* c);

FB_API cam3d_handle* cam3d_criar(float x, float y, float z, float angulo_x, float angulo_y, float angulo_z);
FB_API void  cam3d_destruir(cam3d_handle* c);

FB_API void  cam3d_lente(cam3d_handle* c, float fov, float perto, float longe);
FB_API void  cam3d_fov(cam3d_handle* c, float graus);
FB_API void  cam3d_aspecto(cam3d_handle* c, float aspecto);

FB_API void  cam3d_chao(cam3d_handle* c, int ativo);
FB_API void  cam3d_definir_chao(cam3d_handle* c, float y);
FB_API void  cam3d_ceu(cam3d_handle* c, int ativo);
FB_API void  cam3d_maximo_ceu(cam3d_handle* c, float y);

FB_API void  cam3d_definir_corpo(cam3d_handle* c, float altura_olhos, float olhos_agachado, float altura_colisao, float raio, int tipo_colisao);
FB_API void  cam3d_fisica(cam3d_handle* c, float gravidade, float forca_pulo);
FB_API void  cam3d_degrau(cam3d_handle* c, float altura_maxima);
FB_API void  cam3d_multiplicadores(cam3d_handle* c, float correr, float agachar);
FB_API void  cam3d_balanco(cam3d_handle* c, int ligado, float amplitude, float frequencia);
FB_API void  cam3d_recuperacao(cam3d_handle* c, float velocidade);

FB_API void  cam3d_mover(cam3d_handle* c, float x, float y, float z);
FB_API void  cam3d_posicionar(cam3d_handle* c, float x, float y, float z);
FB_API void  cam3d_girar(cam3d_handle* c, float x, float y, float z);
FB_API void  cam3d_angulo(cam3d_handle* c, float x, float y, float z);
FB_API void  cam3d_olhar_para(cam3d_handle* c, float x, float y, float z);
FB_API void  cam3d_pular(cam3d_handle* c);
FB_API void  cam3d_agachar(cam3d_handle* c, int ligado);
FB_API void  cam3d_correr(cam3d_handle* c, int ligado);
FB_API void  cam3d_tremer(cam3d_handle* c, float intensidade, float duracao);
FB_API void  cam3d_recuo(cam3d_handle* c, float pitch, float yaw);

FB_API void cam3d_neblina(cam3d_handle* c, int ativo, float inicio, float fim, int no_ceu, uint32_t cor, float intensidade);

FB_API void  cam3d_colisao_mundo(cam3d_handle* c, obj3d_handle* const* objs, int n);
FB_API void  cam3d_atualizar(cam3d_handle* c, float delta);

FB_API float cam3d_x(const cam3d_handle* c);
FB_API float cam3d_y(const cam3d_handle* c);
FB_API float cam3d_z(const cam3d_handle* c);
FB_API float cam3d_pes_y(const cam3d_handle* c);
FB_API float cam3d_angulo_x(const cam3d_handle* c);
FB_API float cam3d_angulo_y(const cam3d_handle* c);
FB_API float cam3d_angulo_z(const cam3d_handle* c);
FB_API float cam3d_altura_corpo(const cam3d_handle* c);
FB_API float cam3d_velocidade(const cam3d_handle* c);
FB_API int   cam3d_no_chao(const cam3d_handle* c);
FB_API int   cam3d_agachado(const cam3d_handle* c);
FB_API void  cam3d_frente(const cam3d_handle* c, float* x, float* y, float* z);

FB_API void cam3d_ambiente(cam3d_handle* c, float valor);
FB_API void cam3d_sombras(cam3d_handle* c, int ligado);

FB_API void cam3d_aura(cam3d_handle* c, int ligada, float desloc_y);
FB_API void cam3d_lanterna(cam3d_handle* c, int ligada, float desloc_y);
FB_API void cam3d_cone_lanterna(cam3d_handle* c, float interno, float externo);

FB_API int  cam3d_luz_criar(cam3d_handle* c, float x, float y, float z, uint32_t cor, float intensidade, float alcance);
FB_API void cam3d_luz_remover(cam3d_handle* c, int id);
FB_API void cam3d_luz_mover(cam3d_handle* c, int id, float x, float y, float z);
FB_API void cam3d_luz_ativa(cam3d_handle* c, int id, int ativa);
FB_API void cam3d_luz_cor(cam3d_handle* c, int id, uint32_t cor);
FB_API void cam3d_luz_intensidade(cam3d_handle* c, int id, float valor);
FB_API void cam3d_luz_alcance(cam3d_handle* c, int id, float valor);
FB_API void cam3d_luz_sombra(cam3d_handle* c, int id, int ligado);
FB_API void cam3d_luz_ponto(cam3d_handle* c, int id);
FB_API void cam3d_luz_spot(cam3d_handle* c, int id, float dx, float dy, float dz, float interno, float externo);

FB_API void cam3d_iluminar(cam3d_handle* c, fb_handle* fb, zb_handle* zb, obj3d_handle* const* objs, int n);

FB_API void    cam3d_vista(const cam3d_handle* c, float* ex, float* ey, float* ez, float* yaw, float* pitch, float* roll);
FB_API zb_mat4 cam3d_matriz_vista(const cam3d_handle* c);
FB_API zb_mat4 cam3d_matriz_projecao(const cam3d_handle* c);
FB_API zb_mat4 cam3d_matriz_vp(const cam3d_handle* c);

FB_API int   cam3d_raio(const cam3d_handle* c, obj3d_handle* const* objs, int n, float alcance, float* distancia);
FB_API void  cam3d_desenhar(cam3d_handle* c, fb_handle* fb, zb_handle* zb, obj3d_handle* const* objs, int n);

#ifdef __cplusplus
}
#endif

#endif
