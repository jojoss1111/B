#ifndef MAPAS_HPP
#define MAPAS_HPP

#include "../objetos/obj.hpp"
#include <crock.h>

#define MAXIMO_SPRITES 128

class Mapa {
public:
    Mapa() = default;
    ~Mapa();
    Mapa(const Mapa &) = delete;
    Mapa &operator=(const Mapa &) = delete;

    bool criar_txt(fb_atlas *sprites, const char *txt, uint16_t pw, uint16_t ph);
    bool criar_chunk(fb_atlas *sprites, uint32_t colunas, uint32_t linhas, uint16_t pw, uint16_t ph, char fundo = ' ');
    void destruir();

    void posicionar(int32_t x, int32_t y) { ox_ = x; oy_ = y; }
    void mover(int32_t x, int32_t y)      { ox_ += x; oy_ += y; }
    int32_t  x() const { return ox_; }
    int32_t  y() const { return oy_; }

    uint32_t colunas()       const { return cols_; }
    uint32_t linhas()        const { return lins_; }
    uint16_t tile_largura()  const { return cw_; }
    uint16_t tile_altura()   const { return ch_; }

    char tile(uint32_t x, uint32_t y) const;
    void definir(uint32_t x, uint32_t y, char t);

    void substituir(char antigo, char novo);
    void substituir(char antigo, char novo, uint32_t x, uint32_t y);

protected:
    bool alocar(uint32_t colunas, uint32_t linhas, char fundo);

    fb_atlas *atlas_ = nullptr;
    char     *g_ = nullptr;
    uint32_t  cols_ = 0, lins_ = 0;
    uint16_t  cw_ = 0, ch_ = 0;
    int32_t   ox_ = 0, oy_ = 0;
};

class Tiles : public Mapa {
public:
    Tiles();
    ~Tiles();

    void sprites(char t, uint16_t coluna, uint16_t linha);
    void ampliar(char t, uint16_t w, uint16_t h);
    void cor(char t, fb_cores c);
    void angulo(char t, float a);
    void alpha(char t, uint8_t a);
    void espelhar_x(char t, bool espelhar);
    void espelhar_y(char t, bool espelhar);
    void colisor(char t, const char *mascara = NULL, uint16_t mascara_w = 0, uint16_t mascara_h = 0);

    bool colidio_x(const AABB_2d *obj, int32_t dx) const;
    bool colidio_y(const AABB_2d *obj, int32_t dy) const;
    bool colidio_x(char t, const AABB_2d *obj, int32_t dx) const;
    bool colidio_y(char t, const AABB_2d *obj, int32_t dy) const;

    void desenhar(frame_buffer &janela) const;

private:
    struct Def {
        uint16_t    col = 0, lin = 0;
        uint16_t    w = 0, h = 0;
        fb_cores    c = {255, 255, 255, 255};
        float       ang = 0.0f;
        uint8_t     a = 255;
        bool        fx = false, fy = false;
        bool        desenha = false;
        bool        solido = false;
        const char *mascara = nullptr;
        uint16_t    mw = 0, mh = 0;
    };

    Def       *obter(char t);
    const Def *achar(char t) const;
    bool       colidir(const AABB_2d *obj, int32_t dx, int32_t dy, bool eixo_x, int filtro) const;

    mutable Vetor defs_;
    int16_t       idx_[256];
};

extern "C" {

typedef struct mapa_handle  mapa_handle;   /* Mapa  */
typedef struct tiles_handle tiles_handle;  /* Tiles (herda de Mapa) */

/* ---- Mapa ---- */
FB_API mapa_handle* mapa_novo(void);
FB_API void     mapa_destruir(mapa_handle* m);                 /* só para mapa_novo(); não use em Tiles */
FB_API int      mapa_criar_txt(mapa_handle* m, fb_atlas_handle* sprites, const char* txt, uint16_t pw, uint16_t ph);
FB_API int      mapa_criar_chunk(mapa_handle* m, fb_atlas_handle* sprites, uint32_t colunas, uint32_t linhas, uint16_t pw, uint16_t ph, int fundo);
FB_API void     mapa_liberar(mapa_handle* m);                  /* libera a grade, mantém o handle */
FB_API void     mapa_posicionar(mapa_handle* m, int32_t x, int32_t y);
FB_API void     mapa_mover(mapa_handle* m, int32_t x, int32_t y);
FB_API int32_t  mapa_x(const mapa_handle* m);
FB_API int32_t  mapa_y(const mapa_handle* m);
FB_API uint32_t mapa_colunas(const mapa_handle* m);
FB_API uint32_t mapa_linhas(const mapa_handle* m);
FB_API uint16_t mapa_tile_largura(const mapa_handle* m);
FB_API uint16_t mapa_tile_altura(const mapa_handle* m);
FB_API int      mapa_tile(const mapa_handle* m, uint32_t x, uint32_t y);   /* 0 fora do mapa */
FB_API void     mapa_definir(mapa_handle* m, uint32_t x, uint32_t y, int t);
FB_API void     mapa_substituir_tudo(mapa_handle* m, int antigo, int novo);
FB_API void     mapa_substituir_em(mapa_handle* m, int antigo, int novo, uint32_t x, uint32_t y);

/* ---- Tiles ---- */
FB_API tiles_handle* tiles_novo(void);
FB_API void  tiles_destruir(tiles_handle* t);
FB_API mapa_handle* tiles_como_mapa(tiles_handle* t);          /* use as funções mapa_* no mesmo objeto */

FB_API void  tiles_sprites(tiles_handle* t, int tile, uint16_t coluna, uint16_t linha);
FB_API void  tiles_ampliar(tiles_handle* t, int tile, uint16_t w, uint16_t h);
FB_API void  tiles_cor(tiles_handle* t, int tile, uint32_t cor);      /* ARGB */
FB_API void  tiles_angulo(tiles_handle* t, int tile, float graus);
FB_API void  tiles_alpha(tiles_handle* t, int tile, uint8_t a);
FB_API void  tiles_espelhar_x(tiles_handle* t, int tile, int espelhar);
FB_API void  tiles_espelhar_y(tiles_handle* t, int tile, int espelhar);
/* mascara pode ser NULL (tile inteiro sólido); deve continuar viva */
FB_API void  tiles_colisor(tiles_handle* t, int tile, const char* mascara, uint16_t mascara_w, uint16_t mascara_h);

FB_API int   tiles_colidio_x(const tiles_handle* t, const aabb_2d_handle* obj, int32_t dx);
FB_API int   tiles_colidio_y(const tiles_handle* t, const aabb_2d_handle* obj, int32_t dy);
FB_API int   tiles_colidio_tile_x(const tiles_handle* t, int tile, const aabb_2d_handle* obj, int32_t dx);
FB_API int   tiles_colidio_tile_y(const tiles_handle* t, int tile, const aabb_2d_handle* obj, int32_t dy);

FB_API void  tiles_desenhar(const tiles_handle* t, fb_handle* fb);

}

#endif