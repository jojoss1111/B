#ifndef RENDER_H
#define RENDER_H

#include "../frame buffer/frame_buffer.h"
#include "../inputs/input.h"
#include "../textos/escrever.h"
#include "../audio/audio.h"
#include "../cam/cam.h"
#include "../objetos/obj.h"
#include "../mapas/mapas.h"
#include "../inputs/caixas.h"
#include "../tempo/tempo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef fb_handle       janela;
typedef uint32_t        Cor;
typedef fb_atlas_handle Atlas;

typedef enum {
    MODO_NORMAL = 0,
    MODO_LETTERBOX = 1,
    MODO_TELA_CHEIA = 2
} ModoTela;

void iniciar(void);
void rodar(float delta);
void desenhar(janela* j);

int  iniciar_janela(uint16_t largura, uint16_t altura, uint8_t fps, Cor cor,
                    int8_t ampliar, ModoTela modo, const char* nome, Atlas* icone);
void fechar_janela(void);

void     mudar_modo(void);
void     definir_modo(ModoTela modo);
ModoTela modo_atual(void);

void     ampliar_janela(int8_t ampliar);
int8_t   ampliar_atual(void);

entrada_handle* entrada(void);
void     titulo_janela(const char* titulo);
void     tela_cheia(int ativa);
uint16_t largura_janela(void);
uint16_t altura_janela(void);
float    fps_atual(void);

typedef void (*janela_cb_iniciar)(void* ctx);
typedef void (*janela_cb_rodar)(float delta, void* ctx);
typedef void (*janela_cb_desenhar)(fb_handle* fb, void* ctx);

FB_API void janela_callbacks(janela_cb_iniciar ini, janela_cb_rodar rod, janela_cb_desenhar des, void* ctx);

FB_API int  janela_iniciar(uint16_t largura, uint16_t altura, uint8_t fps, uint32_t cor,
                           int8_t ampliar, int modo, const char* nome, fb_atlas_handle* icone);
FB_API void janela_fechar(void);

FB_API void janela_mudar_modo(void);
FB_API void janela_definir_modo(int modo);
FB_API int  janela_modo_atual(void);
FB_API void janela_ampliar(int8_t ampliar);
FB_API int  janela_ampliar_atual(void);
FB_API void janela_tela_cheia(int ativa);
FB_API void janela_titulo(const char* titulo);

FB_API entrada_handle* janela_entrada(void);
FB_API uint16_t janela_largura(void);
FB_API uint16_t janela_altura(void);
FB_API float    janela_fps_atual(void);

#ifdef __cplusplus
}
#endif

#endif