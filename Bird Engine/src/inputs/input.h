#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

#ifndef FB_API
#  if defined(_WIN32)
#    define FB_API __declspec(dllexport)
#  else
#    define FB_API __attribute__((visibility("default")))
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ENTRADA_MAX_TOQUES 10
#define ENTRADA_TECLAS     256
#define ENTRADA_BOTOES     3

typedef enum {
    TECLA_NENHUMA = 0,
    TECLA_SETA_CIMA = 1, TECLA_SETA_BAIXO = 2, TECLA_SETA_ESQUERDA = 3, TECLA_SETA_DIREITA = 4,
    TECLA_CTRL = 5,
    TECLA_SHIFT = 6,
    TECLA_BACKSPACE = 8, TECLA_TAB = 9, TECLA_START = '\r', TECLA_SAIR = 27, TECLA_ESPACO = ' ',
    TECLA_F1 = 14, TECLA_F2, TECLA_F3, TECLA_F4, TECLA_F5, TECLA_F6, TECLA_F7, TECLA_F8, TECLA_F9, TECLA_F10, TECLA_F11, TECLA_F12,
    TECLA_N0 = '0', TECLA_N1, TECLA_N2, TECLA_N3, TECLA_N4, TECLA_N5, TECLA_N6, TECLA_N7, TECLA_N8, TECLA_N9,
    TECLA_A = 'a', TECLA_B, TECLA_C, TECLA_D, TECLA_E, TECLA_F, TECLA_G, TECLA_H, TECLA_I, TECLA_J, TECLA_K, TECLA_L, TECLA_M,
    TECLA_N, TECLA_O, TECLA_P, TECLA_Q, TECLA_R, TECLA_S, TECLA_T, TECLA_U, TECLA_V, TECLA_W, TECLA_X, TECLA_Y, TECLA_Z,
    TECLA_CIMA = 'w', TECLA_BAIXO = 's', TECLA_ESQUERDA = 'a', TECLA_DIREITA = 'd'
} Tecla;

typedef enum { MOUSE_ESQUERDO = 0, MOUSE_DIREITO = 1, MOUSE_MEIO = 2 } MouseBotao;

typedef void (*entrada_callback)(const void* sdl_event, void* ctx);

typedef struct entrada_toque {
    int64_t id;
    int     x, y;
    uint8_t usado, ativo, caiu, subiu;
} entrada_toque;

typedef struct entrada_handle {
    uint8_t atual[ENTRADA_TECLAS];
    uint8_t anterior[ENTRADA_TECLAS];
    uint8_t ultima;
    uint8_t pediu_fechar;
    char    texto[32];

    uint8_t mouse_atual[ENTRADA_BOTOES];
    uint8_t mouse_anterior[ENTRADA_BOTOES];
    int     x, y;
    int     fb_x, fb_y;
    int     roda;

    entrada_toque toques[ENTRADA_MAX_TOQUES];
    int           area_w, area_h;

    entrada_callback cb;
    void*            ctx;
} entrada_handle;

FB_API entrada_handle* entrada_criar(void);
FB_API void  entrada_destruir(entrada_handle* e);
FB_API void  entrada_iniciar(entrada_handle* e);
FB_API void  entrada_atualizar(entrada_handle* e);
FB_API void  entrada_callback_evento(entrada_handle* e, entrada_callback cb, void* ctx);

FB_API int         entrada_pediu_fechar(const entrada_handle* e);
FB_API const char* entrada_texto_digitado(const entrada_handle* e);
FB_API int         entrada_capslock(const entrada_handle* e);
FB_API const char* entrada_layout_teclado(void);

FB_API int  entrada_pressionou(const entrada_handle* e, int tecla);
FB_API int  entrada_soltou(const entrada_handle* e, int tecla);
FB_API int  entrada_segurando(const entrada_handle* e, int tecla);
FB_API int  entrada_ultima(const entrada_handle* e);

FB_API int  entrada_mouse_pressionou(const entrada_handle* e, int botao);
FB_API int  entrada_mouse_soltou(const entrada_handle* e, int botao);
FB_API int  entrada_mouse_segurando(const entrada_handle* e, int botao);
FB_API int  entrada_mouse_x(const entrada_handle* e);
FB_API int  entrada_mouse_y(const entrada_handle* e);
FB_API int  entrada_mouse_roda(const entrada_handle* e);
FB_API void entrada_mouse_janela(const entrada_handle* e, int* x, int* y);
FB_API void entrada_definir_mouse(entrada_handle* e, int x, int y);

FB_API int  entrada_toque_pressionou(const entrada_handle* e, int i);
FB_API int  entrada_toque_soltou(const entrada_handle* e, int i);
FB_API int  entrada_toque_segurando(const entrada_handle* e, int i);
FB_API int  entrada_toque_x(const entrada_handle* e, int i);
FB_API int  entrada_toque_y(const entrada_handle* e, int i);
FB_API int  entrada_toques_ativos(const entrada_handle* e);
FB_API void entrada_definir_area_toque(entrada_handle* e, int w, int h);

#ifdef __cplusplus
}
#endif

#endif