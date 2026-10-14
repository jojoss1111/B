#ifndef CAM_HPP
#define CAM_HPP

#include "../frame buffer/frame_buffer.hpp"
#include "../objetos/obj.hpp"
#include "../mapas/mapas.hpp"

class Camera {
public:
    Camera() = default;

    bool criar(uint16_t largura, uint16_t altura, int32_t x = 0, int32_t y = 0);
    void destruir();
    bool valida() const { return view_.valido(); }
    int32_t  x() const;
    int32_t  y() const;
    int32_t  largura_mundo() const;
    int32_t  altura_mundo()  const;
    uint16_t largura() const { return w_; }
    uint16_t altura()  const { return h_; }
    float    angulo() const { return ang_; }
    void     angulo(float a);
    void     girar(float delta);
    float zoom() const { return zoom_; }
    void  zoom(float z);
    void  zoom_relativo(float fator);
    void  zoom_em(float z, int32_t tela_x, int32_t tela_y);   // mantem o ponto da tela parado (ex.: scroll do mouse)
    void  zoom_suave(float alvo, uint8_t suavidade);          // anima em atualizar(); 0 = imediato
    void  limites_zoom(float minimo, float maximo);           // padrao: 0.25 .. 8

    void posicionar(int32_t x, int32_t y);                    // canto superior esquerdo da area visivel
    void mover(int32_t dx, int32_t dy);                       // em unidades de mundo
    void centralizar(int32_t mundo_x, int32_t mundo_y);
    void mover_suave(int32_t mundo_x, int32_t mundo_y, uint8_t suavidade, float delta = 1.0f / 60.0f);
    void centralizar_mundo(const Mapa &mapa, uint16_t pw = 0, uint16_t ph = 0);

    void ancorar(const Obj *alvo, int32_t desloc_x = 0, int32_t desloc_y = 0, uint8_t suavidade = 0);
    void soltar();
    void atualizar(float delta = 1.0f / 60.0f);

    void travar_mapa(const Mapa *mapa, uint16_t tile_w = 0, uint16_t tile_h = 0);
    void travar_limites(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
    void destravar();

    void mundo_para_tela(int32_t mundo_x, int32_t mundo_y, int32_t &tela_x, int32_t &tela_y) const;
    void tela_para_mundo(int32_t tela_x, int32_t tela_y, int32_t &mundo_x, int32_t &mundo_y) const;
    void iniciar(fb_cores fundo = {0, 0, 0, 255});
    bool visivel(int32_t x, int32_t y, int32_t w, int32_t h) const {
        return view_.valido() &&
               x < ox_ + view_.largura() && y < oy_ + view_.altura() &&
               x + w > ox_ && y + h > oy_;
    }

    template <class T>
    void desenhar(T &o) {
        if (!view_.valido()) return;
        const int32_t px = o.x(), py = o.y();
        o.posicionar(px - ox_, py - oy_);
        o.desenhar(view_);
        o.posicionar(px, py);
    }

    template <class T>
    void desenhar(const T &o) {
        if (!view_.valido()) return;
        T c = o;
        c.posicionar(c.x() - ox_, c.y() - oy_);
        c.desenhar(view_);
    }

    void apresentar(frame_buffer &janela, int sx = 0, int sy = 0) const;
    void desenhar(frame_buffer &janela, int sx = 0, int sy = 0) { apresentar(janela, sx, sy); }

    frame_buffer &viewport() { return view_; }

private:
    uint16_t ang16() const { return fb_trig::de_graus(ang_); }
    void aplicar();
    void centro_do_alvo(int32_t &cx, int32_t &cy) const;
    void extensao(float &ex, float &ey) const;
    bool reto() const;
    void tamanho_necessario(int32_t &nw, int32_t &nh) const;
    bool garantir_buffer();
    void preparar_quadro();

    frame_buffer view_;                 // buffer 1:1 em unidades de mundo (cobre a area visivel)

    float    cx_ = 0.0f, cy_ = 0.0f;    // centro da camera no mundo
    uint16_t w_ = 0, h_ = 0;            // tamanho na janela
    float    ang_ = 0.0f;
    bool     opaca_ = true;

    float    zoom_ = 1.0f, zoom_min_ = 0.25f, zoom_max_ = 8.0f;
    float    zoom_alvo_ = 1.0f;
    uint8_t  zoom_suave_ = 0;
    bool     zoom_alvo_ativo_ = false;

    // Estado congelado em iniciar(): garante que desenhar e apresentar usem a mesma transformacao.
    int32_t  ox_ = 0, oy_ = 0;          // origem do buffer no mundo
    int64_t  bcx_ = 0, bcy_ = 0;        // centro da camera dentro do buffer (Q16)
    int32_t  ca_ = 32768, sa_ = 0;      // cos/zoom e sen/zoom (Q15)
    bool     reto_ = true;              // zoom 1 e angulo 0: copia direta

    const Obj *alvo_ = nullptr;
    int32_t    alvo_dx_ = 0, alvo_dy_ = 0;
    uint8_t    alvo_suave_ = 0;

    bool    travada_ = false;
    int32_t lx0_ = 0, ly0_ = 0, lx1_ = 0, ly1_ = 0;
};

extern "C" {

typedef struct camera_handle camera_handle;   /* Camera 2D */

FB_API camera_handle* camera_criar(uint16_t largura, uint16_t altura, int32_t x, int32_t y);  /* NULL em falha */
FB_API void     camera_destruir(camera_handle* c);
FB_API int      camera_valida(const camera_handle* c);

FB_API int32_t  camera_x(const camera_handle* c);
FB_API int32_t  camera_y(const camera_handle* c);
FB_API int32_t  camera_largura_mundo(const camera_handle* c);
FB_API int32_t  camera_altura_mundo(const camera_handle* c);
FB_API uint16_t camera_largura(const camera_handle* c);
FB_API uint16_t camera_altura(const camera_handle* c);

/* ângulo e zoom */
FB_API float    camera_angulo(const camera_handle* c);
FB_API void     camera_angulo_definir(camera_handle* c, float graus);
FB_API void     camera_girar(camera_handle* c, float delta);
FB_API float    camera_zoom(const camera_handle* c);
FB_API void     camera_zoom_definir(camera_handle* c, float z);
FB_API void     camera_zoom_relativo(camera_handle* c, float fator);
FB_API void     camera_zoom_em(camera_handle* c, float z, int32_t tela_x, int32_t tela_y);
FB_API void     camera_zoom_suave(camera_handle* c, float alvo, uint8_t suavidade);
FB_API void     camera_limites_zoom(camera_handle* c, float minimo, float maximo);

/* posição */
FB_API void     camera_posicionar(camera_handle* c, int32_t x, int32_t y);
FB_API void     camera_mover(camera_handle* c, int32_t dx, int32_t dy);
FB_API void     camera_centralizar(camera_handle* c, int32_t mundo_x, int32_t mundo_y);
FB_API void     camera_mover_suave(camera_handle* c, int32_t mundo_x, int32_t mundo_y, uint8_t suavidade, float delta);
FB_API void     camera_centralizar_mundo(camera_handle* c, const mapa_handle* mapa, uint16_t pw, uint16_t ph);

/* seguir alvo e limites */
FB_API void     camera_ancorar(camera_handle* c, const obj_handle* alvo, int32_t desloc_x, int32_t desloc_y, uint8_t suavidade);
FB_API void     camera_soltar(camera_handle* c);
FB_API void     camera_atualizar(camera_handle* c, float delta);
FB_API void     camera_travar_mapa(camera_handle* c, const mapa_handle* mapa, uint16_t tile_w, uint16_t tile_h);
FB_API void     camera_travar_limites(camera_handle* c, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
FB_API void     camera_destravar(camera_handle* c);

/* conversões */
FB_API void     camera_mundo_para_tela(const camera_handle* c, int32_t mundo_x, int32_t mundo_y, int32_t* tela_x, int32_t* tela_y);
FB_API void     camera_tela_para_mundo(const camera_handle* c, int32_t tela_x, int32_t tela_y, int32_t* mundo_x, int32_t* mundo_y);

/* desenho: iniciar -> desenhar_* -> apresentar */
FB_API void     camera_iniciar(camera_handle* c, uint32_t fundo);                       /* ARGB */
FB_API int      camera_visivel(const camera_handle* c, int32_t x, int32_t y, int32_t w, int32_t h);
FB_API void     camera_desenhar_obj(camera_handle* c, obj_handle* o);
FB_API void     camera_desenhar_tiles(camera_handle* c, tiles_handle* t);
FB_API void     camera_apresentar(const camera_handle* c, fb_handle* janela, int sx, int sy);

/* buffer interno (somente leitura de pixels, ARGB) */
FB_API uint32_t* camera_viewport_dados(camera_handle* c);
FB_API int      camera_viewport_largura(camera_handle* c);
FB_API int      camera_viewport_altura(camera_handle* c);

}

#endif