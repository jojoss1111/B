#ifndef RENDER_HPP
#define RENDER_HPP

#include "../frame buffer/frame_buffer.hpp"
#include "../inputs/input.hpp"
#include "../textos/escrever.hpp"
#include "../audio/audio.hpp"
#include "../cam/cam.hpp"
#include "../objetos/obj.hpp"
#include "../mapas/mapas.hpp"
#include "../frame buffer/fb_3d.hpp"
#include "../frame buffer/fb_3d_mat.hpp"
#include "../inputs/caixas.hpp"
#include "../cam/cam.hpp"

typedef frame_buffer janela;
typedef fb_cores     Cor;
typedef fb_atlas     Atlas;

enum class ModoTela {
    normal,
    letterbox,
    tela_cheia
};

void iniciar();
void rodar(float delta);
void desenhar(janela *j);

int  iniciar_janela(uint16_t largura, uint16_t altura, uint8_t fps = 60, Cor cor = {0, 0, 0, 255},
        int8_t ampliar = 2, ModoTela modo = ModoTela::letterbox, const char *nome = "bird", Atlas *icone = nullptr);
void fechar_janela();

void     mudar_modo();
void     definir_modo(ModoTela modo);
ModoTela modo_atual();

void     ampliar_janela(int8_t ampliar);
int8_t   ampliar_atual();

Entrada &entrada();
void     titulo_janela(const char *titulo);
void     tela_cheia(bool ativa);
uint16_t largura_janela();
uint16_t altura_janela();
float    fps_atual();


extern "C" {

/* callbacks chamados pelo loop (ctx é devolvido como veio) */
typedef void (*janela_cb_iniciar)(void* ctx);
typedef void (*janela_cb_rodar)(float delta, void* ctx);
typedef void (*janela_cb_desenhar)(fb_handle* fb, void* ctx);

/* modo: 0 = normal, 1 = letterbox, 2 = tela cheia */
FB_API void janela_callbacks(janela_cb_iniciar ini, janela_cb_rodar rod, janela_cb_desenhar des, void* ctx);

/* BLOQUEIA até a janela fechar. 0 = ok, <0 = erro. ampliar <= 0 = escala automática. cor = ARGB */
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

FB_API entrada_handle* janela_entrada(void);   /* entrada da janela (o loop já chama atualizar) */
FB_API uint16_t janela_largura(void);
FB_API uint16_t janela_altura(void);
FB_API float    janela_fps_atual(void);

}
#endif