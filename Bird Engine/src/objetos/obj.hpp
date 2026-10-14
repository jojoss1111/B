#ifndef OBJ_HPP
#define OBJ_HPP

#include "../frame buffer/frame_buffer.hpp"

class Obj {
public:
    void criar(fb_atlas *sprite, uint16_t pw, uint16_t ph, int32_t x, int32_t y, uint16_t coluna, uint16_t linha);
    void destruir();

    int32_t  x() const { return x_; }
    int32_t  y() const { return y_; }
    uint16_t largura() const { return pw_; }
    uint16_t altura()  const { return ph_; }

    void posicionar(int32_t x, int32_t y);
    void mover(int32_t x, int32_t y);

    void tile(uint16_t coluna, uint16_t linha);
    void cor(const fb_cores &c);
    void colorir(bool substituir);
    void alpha(uint8_t a);
    void ampliar(uint16_t pw, uint16_t ph);
    void angulo(float a);
    void espelhar_x(bool espelhar);
    void espelhar_y(bool espelhar);

    void desenhar(frame_buffer &janela) const;

private:
    fb_atlas *sprite_ = nullptr;
    int32_t   x_ = 0, y_ = 0;
    uint16_t  pw_ = 0, ph_ = 0;
    uint16_t  col_ = 0, lin_ = 0;
    float     ang_ = 0.0f;
    uint8_t   alpha_ = 255;
    bool      flip_x_ = false, flip_y_ = false;
    fb_cores  tint_ = {255, 255, 255, 255};
    bool      subst_ = false;
};

class Animacao {
public:
    void criar(Obj *obj, const int16_t (*quadros)[2], uint8_t total, uint8_t fps, bool loop);
    void destruir();
    void rodar();
    void parar();
    void voltar();
    void atualizar(uint32_t dt_ms);

private:
    void aplicar();

    Obj                *obj_ = nullptr;
    const int16_t     (*arr_)[2] = nullptr;
    uint8_t             total_ = 0, idx_ = 0;
    uint32_t            ms_quadro_ = 1000, acum_ = 0;
    bool                loop_ = true, tocando_ = false;
};

class AABB_2d {
public:
    void criar(Obj *obj, const char *mascara, uint16_t mascara_w, uint16_t mascara_h);
    static bool colidio(const AABB_2d *colisor_1, const AABB_2d *colisor_2);
    void sincronizar_colisao(Obj *obj);
    void posicionar(int32_t x, int32_t y) { x_ = x; y_ = y; }
    int32_t  x() const { return x_; }
    int32_t  y() const { return y_; }
    uint16_t largura() const { return w_; }
    uint16_t altura()  const { return h_; }
    bool colidiu_x(const AABB_2d *outro, int32_t dx) const;
    bool colidiu_y(const AABB_2d *outro, int32_t dy) const;

private:
    static bool testar(const AABB_2d &a, int32_t ax, int32_t ay, const AABB_2d &b, int32_t bx, int32_t by);
    bool solido_local(int32_t lx, int32_t ly) const;

    int32_t     x_ = 0, y_ = 0;
    uint16_t    w_ = 0, h_ = 0;
    const char *mascara_ = nullptr;
    uint16_t    mascara_w_ = 0, mascara_h_ = 0;
};

extern "C" {

typedef struct obj_handle  obj_handle;   /* Obj      */
typedef struct anim_handle anim_handle;  /* Animacao */
typedef struct aabb_2d_handle aabb_2d_handle;  /* AABB_2d     */

/* ---- Obj ---- */
FB_API obj_handle* obj_criar(fb_atlas_handle* sprite, uint16_t pw, uint16_t ph, int32_t x, int32_t y, uint16_t coluna, uint16_t linha);
FB_API void     obj_destruir(obj_handle* o);
FB_API int32_t  obj_x(const obj_handle* o);
FB_API int32_t  obj_y(const obj_handle* o);
FB_API uint16_t obj_largura(const obj_handle* o);
FB_API uint16_t obj_altura(const obj_handle* o);
FB_API void     obj_posicionar(obj_handle* o, int32_t x, int32_t y);
FB_API void     obj_mover(obj_handle* o, int32_t x, int32_t y);
FB_API void     obj_tile(obj_handle* o, uint16_t coluna, uint16_t linha);
FB_API void     obj_cor(obj_handle* o, uint32_t cor);           /* ARGB */
FB_API void     obj_alpha(obj_handle* o, uint8_t a);
FB_API void     obj_ampliar(obj_handle* o, uint16_t pw, uint16_t ph);
FB_API void     obj_angulo(obj_handle* o, float graus);
FB_API void     obj_espelhar_x(obj_handle* o, int espelhar);
FB_API void     obj_espelhar_y(obj_handle* o, int espelhar);
FB_API void     obj_desenhar(const obj_handle* o, fb_handle* fb);

/* ---- Animacao ---- */
/* quadros = total * 2 int16_t: {coluna, linha, coluna, linha, ...}; o array deve continuar vivo */
FB_API anim_handle* anim_criar(obj_handle* o, const int16_t* quadros, uint8_t total, uint8_t fps, int loop);
FB_API void anim_destruir(anim_handle* a);
FB_API void anim_rodar(anim_handle* a);
FB_API void anim_parar(anim_handle* a);
FB_API void anim_voltar(anim_handle* a);
FB_API void anim_atualizar(anim_handle* a, uint32_t dt_ms);

/* ---- AABB_2d ---- */
/* mascara: w*h chars, '#' = sólido (ou NULL = retângulo cheio); deve continuar viva. obj pode ser NULL */
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

}

#endif