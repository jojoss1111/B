#ifndef INPUT_HPP
#define INPUT_HPP

#include <stdint.h>

union SDL_Event;

enum class Tecla : uint8_t {
    nenhuma = 0,
    seta_cima = 1, seta_baixo = 2, seta_esquerda = 3, seta_direita = 4,
    ctrl = 5,
    shift = 6,
    backspace = 8, tab = 9, start = '\r', sair = 27, espaco = ' ',
    f1 = 14, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12,
    n0 = '0', n1, n2, n3, n4, n5, n6, n7, n8, n9,
    a = 'a', b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
    cima = 'w', baixo = 's', esquerda = 'a', direita = 'd',
};

enum class MouseBotao : uint8_t { esquerdo = 0, direito = 1, meio = 2 };

class Entrada {
public:
    typedef void (*Callback)(const SDL_Event& ev, void* ctx);

    // máximo de dedos ao mesmo tempo; o índice (0..MAX_TOQUES-1) é o "slot" do dedo
    static constexpr int MAX_TOQUES = 10;

    Entrada() = default;

    void iniciar();
    void atualizar();
    void callback_evento(Callback cb, void* ctx) { cb_ = cb; cb_ctx_ = ctx; }

    bool        pediu_fechar() const { return pediu_fechar_; }
    const char* texto_digitado() const { return texto_; }
    bool        capslock() const;
    static const char* layout_teclado();

    // ---- teclado ----
    bool   pressionou(Tecla t) const { return atual_[(uint8_t)t] && !anterior_[(uint8_t)t]; }  // só no frame
    bool   soltou(Tecla t)     const { return !atual_[(uint8_t)t] && anterior_[(uint8_t)t]; }
    bool   segurando(Tecla t)  const { return atual_[(uint8_t)t]; }
    Tecla  ultima()            const { return ultima_; }

    // ---- mouse ----
    bool mouse_pressionou(MouseBotao b) const { return mouse_atual_[(uint8_t)b] && !mouse_anterior_[(uint8_t)b]; }
    bool mouse_soltou(MouseBotao b)     const { return !mouse_atual_[(uint8_t)b] && mouse_anterior_[(uint8_t)b]; }
    bool mouse_segurando(MouseBotao b)  const { return mouse_atual_[(uint8_t)b]; }
    int  mouse_x()    const { return fb_x_; }
    int  mouse_y()    const { return fb_y_; }
    int  mouse_roda() const { return roda_; }

    void mouse_janela(int& x, int& y) const { x = x_; y = y_; }
    void definir_mouse(int x, int y) { fb_x_ = x; fb_y_ = y; }

    // ---- touch screen (mesma lógica do mouse, mas um conjunto por dedo) ----
    // i = slot do dedo (0..MAX_TOQUES-1). O slot é fixo enquanto o dedo estiver na tela.
    // Um toque rápido (apertou e soltou no mesmo frame) ainda conta como pressionou + soltou.
    bool toque_pressionou(int i) const { return toque_valido(i) && toques_[i].caiu; }   // só no frame
    bool toque_soltou(int i)     const { return toque_valido(i) && toques_[i].subiu; }  // só no frame
    bool toque_segurando(int i)  const { return toque_valido(i) && (toques_[i].ativo || toques_[i].caiu); }
    int  toque_x(int i)          const { return toque_valido(i) ? toques_[i].x : 0; }   // coords do framebuffer
    int  toque_y(int i)          const { return toque_valido(i) ? toques_[i].y : 0; }
    int  toques_ativos() const;                    // quantos dedos estão na tela agora

    // Tamanho do framebuffer para converter o toque (0..1) em pixels.
    // Se não definir, usa o tamanho da janela (igual ao mouse sem definir_mouse).
    void definir_area_toque(int w, int h) { area_w_ = w; area_h_ = h; }

private:
    static constexpr int TECLAS = 256;
    static constexpr int BOTOES = 3;

    struct Toque {
        int64_t id = -1;     // SDL_FingerID
        int     x = 0, y = 0;
        bool    usado = false;  // slot ocupado (vive até o frame seguinte ao soltar)
        bool    ativo = false;  // dedo ainda na tela
        bool    caiu  = false;  // apertou neste frame
        bool    subiu = false;  // soltou neste frame
    };

    static bool toque_valido(int i) { return i >= 0 && i < MAX_TOQUES; }
    int  achar_toque(int64_t id) const;
    void normalizado_para_fb(float nx, float ny, int& x, int& y) const;

    bool  atual_[TECLAS]    = {};
    bool  anterior_[TECLAS] = {};
    Tecla ultima_ = Tecla::nenhuma;
    bool  pediu_fechar_ = false;
    char  texto_[32] = {};

    bool mouse_atual_[BOTOES]    = {};
    bool mouse_anterior_[BOTOES] = {};
    int  x_ = 0, y_ = 0;
    int  fb_x_ = 0, fb_y_ = 0;
    int  roda_ = 0;

    Toque toques_[MAX_TOQUES];
    int   area_w_ = 0, area_h_ = 0;

    Callback cb_ = nullptr;
    void*    cb_ctx_ = nullptr;
};

/* O input.hpp não inclui o frame_buffer.hpp, então define a macro de exportação (idêntica à de lá) */
#ifndef FB_API
#  if defined(_WIN32)
#    define FB_API __declspec(dllexport)
#  else
#    define FB_API __attribute__((visibility("default")))
#  endif
#endif

/* handle público (completo aqui porque o caixas.cpp também usa) */
struct entrada_handle {
    Entrada e;
    void  (*cb)(const void*, void*) = nullptr;
    void*   ctx = nullptr;
};

extern "C" {

typedef struct entrada_handle entrada_handle;

/* callback de eventos: sdl_event aponta para um SDL_Event */
typedef void (*entrada_callback)(const void* sdl_event, void* ctx);

/* Teclas (valores de Tecla): 0 nenhuma, 1 cima, 2 baixo, 3 esquerda, 4 direita (setas), 5 ctrl, 6 shift,
   8 backspace, 9 tab, 13 start/enter, 27 sair/esc, 32 espaço, 14..25 F1..F12,
   '0'..'9' (48..57) e 'a'..'z' (97..122).  Botões do mouse: 0 esquerdo, 1 direito, 2 meio. */

FB_API entrada_handle* entrada_criar(void);
FB_API void  entrada_destruir(entrada_handle* e);
FB_API void  entrada_iniciar(entrada_handle* e);
FB_API void  entrada_atualizar(entrada_handle* e);
FB_API void  entrada_callback_evento(entrada_handle* e, entrada_callback cb, void* ctx);

FB_API int         entrada_pediu_fechar(const entrada_handle* e);
FB_API const char* entrada_texto_digitado(const entrada_handle* e);   /* válido até o próximo atualizar */
FB_API int         entrada_capslock(const entrada_handle* e);
FB_API const char* entrada_layout_teclado(void);

/* teclado */
FB_API int  entrada_pressionou(const entrada_handle* e, int tecla);
FB_API int  entrada_soltou(const entrada_handle* e, int tecla);
FB_API int  entrada_segurando(const entrada_handle* e, int tecla);
FB_API int  entrada_ultima(const entrada_handle* e);

/* mouse */
FB_API int  entrada_mouse_pressionou(const entrada_handle* e, int botao);
FB_API int  entrada_mouse_soltou(const entrada_handle* e, int botao);
FB_API int  entrada_mouse_segurando(const entrada_handle* e, int botao);
FB_API int  entrada_mouse_x(const entrada_handle* e);
FB_API int  entrada_mouse_y(const entrada_handle* e);
FB_API int  entrada_mouse_roda(const entrada_handle* e);
FB_API void entrada_mouse_janela(const entrada_handle* e, int* x, int* y);
FB_API void entrada_definir_mouse(entrada_handle* e, int x, int y);

/* toque (i = slot do dedo, 0..9) */
FB_API int  entrada_toque_pressionou(const entrada_handle* e, int i);
FB_API int  entrada_toque_soltou(const entrada_handle* e, int i);
FB_API int  entrada_toque_segurando(const entrada_handle* e, int i);
FB_API int  entrada_toque_x(const entrada_handle* e, int i);
FB_API int  entrada_toque_y(const entrada_handle* e, int i);
FB_API int  entrada_toques_ativos(const entrada_handle* e);
FB_API void entrada_definir_area_toque(entrada_handle* e, int w, int h);

}


#endif
