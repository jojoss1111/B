#ifndef FRAME_BUFFER_HPP
#define FRAME_BUFFER_HPP

#include <crock.h>
#include <stdint.h>
#include <stddef.h>

typedef int32_t fx_t;

namespace fx {
    constexpr int  BITS = 4;
    constexpr fx_t UM   = 1 << BITS;

    constexpr fx_t de_int(int v)       { return v * UM; }
    constexpr int  para_int(fx_t v)    { return v >> BITS; }
    constexpr fx_t mul(fx_t a, fx_t b) { return (fx_t)(((int64_t)a * b) >> BITS); }
    constexpr fx_t div(fx_t a, fx_t b) { return (fx_t)(((int64_t)a * UM) / b); }
}

namespace fb_trig {
    constexpr int      BITS_INDICE    = 10;
    constexpr int      BITS_FRACAO    = 16 - BITS_INDICE;
    constexpr int      TAM            = 1 << BITS_INDICE;
    constexpr int      MASCARA_FRACAO = (1 << BITS_FRACAO) - 1;

    constexpr int      BITS = 15;
    constexpr int32_t  UM   = 1 << BITS;

    constexpr uint16_t QUARTO_VOLTA = 16384;
    constexpr uint16_t MEIA_VOLTA   = 32768;

    constexpr double seno_taylor(double x) {
        double x2 = x * x, termo = x, soma = x;
        for (int i = 1; i <= 11; i++) {
            termo *= -x2 / ((2.0 * i) * (2.0 * i + 1.0));
            soma += termo;
        }
        return soma;
    }

    struct tabela {
        int32_t v[TAM];
        constexpr tabela() : v() {
            constexpr double PI = 3.14159265358979323846;
            for (int i = 0; i < TAM; i++) {
                double ang = 2.0 * PI * i / TAM;
                if (ang > PI) ang -= 2.0 * PI;
                double r = seno_taylor(ang) * UM;
                v[i] = (int32_t)(r >= 0 ? r + 0.5 : r - 0.5);
            }
        }
    };

    inline constexpr tabela TAB{};

    constexpr int32_t seno(uint16_t a) {
        const int i = a >> BITS_FRACAO, f = a & MASCARA_FRACAO;
        const int32_t v0 = TAB.v[i], v1 = TAB.v[(i + 1) & (TAM - 1)];
        return v0 + (((v1 - v0) * f) >> BITS_FRACAO);
    }

    constexpr int32_t  cosseno(uint16_t a) { return seno((uint16_t)(a + QUARTO_VOLTA)); }
    constexpr uint16_t graus(int g)        { return (uint16_t)(((int64_t)g * 65536) / 360); }

    constexpr uint16_t de_graus(float g) {
        const float v = g * (65536.0f / 360.0f);
        return (uint16_t)(int32_t)(v >= 0.0f ? v + 0.5f : v - 0.5f);
    }
}

struct fb_cores {
    uint8_t r, g, b, a;

    constexpr uint32_t empacotar() const {
        return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }

    static constexpr fb_cores desempacotar(uint32_t p) {
        return { (uint8_t)(p >> 16), (uint8_t)(p >> 8), (uint8_t)p, (uint8_t)(p >> 24) };
    }

    static constexpr fb_cores misturar(fb_cores dst, fb_cores src) {
        const int a = src.a, ia = 255 - a;
        return { (uint8_t)((src.r * a + dst.r * ia + 127) / 255),
                 (uint8_t)((src.g * a + dst.g * ia + 127) / 255),
                 (uint8_t)((src.b * a + dst.b * ia + 127) / 255),
                 255 };
    }
};

struct fb_neblina {
    bool     ativa  = false;
    fb_cores cor    = {0, 0, 0, 255};
    float    inicio = 5.0f;
    float    fim    = 30.0f;
};

class frame_buffer {
public:
    frame_buffer() = default;
    frame_buffer(int largura, int altura);
    ~frame_buffer();

    frame_buffer(const frame_buffer&) = delete;
    frame_buffer& operator=(const frame_buffer&) = delete;
    frame_buffer(frame_buffer&& o) noexcept;
    frame_buffer& operator=(frame_buffer&& o) noexcept;

    bool   valido()       const { return px_ != nullptr; }
    int    largura()      const { return w_; }
    int    altura()       const { return h_; }
    size_t total_pixels() const { return (size_t)w_ * h_; }
    size_t pitch()        const { return (size_t)w_ * sizeof(uint32_t); }

    uint32_t*       dados()       { return px_; }
    const uint32_t* dados() const { return px_; }

    bool dentro(int x, int y) const { return (unsigned)x < (unsigned)w_ && (unsigned)y < (unsigned)h_; }

    void     limpar(fb_cores c);
    void     pixel(int x, int y, fb_cores c);
    void     misturar(int x, int y, fb_cores c);
    fb_cores ler(int x, int y) const;

    void pixel_fx(fx_t x, fx_t y, fb_cores c) { pixel(fx::para_int(x), fx::para_int(y), c); }

private:
    void liberar();
    void roubar(frame_buffer& o) noexcept;

    uint32_t* px_ = nullptr;
    int       w_  = 0;
    int       h_  = 0;
};

class fb_atlas {
public:
    fb_atlas() = default;
    ~fb_atlas();

    fb_atlas(const fb_atlas&) = delete;
    fb_atlas& operator=(const fb_atlas&) = delete;
    fb_atlas(fb_atlas&& o) noexcept;
    fb_atlas& operator=(fb_atlas&& o) noexcept;

    bool carregar(const char* caminho, uint16_t frame_w = 0, uint16_t frame_h = 0);
    bool carregar_memoria(const uint8_t* dados, size_t tamanho, uint16_t frame_w = 0, uint16_t frame_h = 0);

    bool valido()       const { return px_ != nullptr; }
    int  largura()      const { return w_; }
    int  altura()       const { return h_; }
    int  tile_largura() const { return fw_; }
    int  tile_altura()  const { return fh_; }
    int  colunas()      const { return fw_ ? w_ / fw_ : 0; }
    int  linhas()       const { return fh_ ? h_ / fh_ : 0; }
    int  passo()        const { return w_; }

    const uint32_t* tile_dados(int pw, int ph) const {
        return px_ + (size_t)ph * fh_ * w_ + (size_t)pw * fw_;
    }

    void desenhar(frame_buffer& fb, int pw, int ph, int x, int y, int w = 0, int h = 0) const;
    void desenhar_fx(frame_buffer& fb, int pw, int ph, fx_t x, fx_t y, int w = 0, int h = 0) const {
        desenhar(fb, pw, ph, fx::para_int(x), fx::para_int(y), w, h);
    }

private:
    void liberar();
    void roubar(fb_atlas& o) noexcept;
    bool adotar_rgba(const uint8_t* rgba, int w, int h, uint16_t frame_w, uint16_t frame_h);
    bool tile_valido(int pw, int ph) const;

    uint32_t* px_ = nullptr;
    int       w_  = 0;
    int       h_  = 0;
    int       fw_ = 0;
    int       fh_ = 0;
};

void linha(frame_buffer& fb, int x0, int y0, int x1, int y1, fb_cores c);
void retangulo(frame_buffer& fb, int x, int y, int w, int h, float angulo, bool preenchido, fb_cores c);
void triangulo_retangulo(frame_buffer& fb, int x, int y, int w, int h, float angulo, bool preenchido, fb_cores c);
void circulo(frame_buffer& fb, int cx, int cy, int raio, float angulo, bool preenchido, fb_cores c);
void blit(frame_buffer& dst, const frame_buffer& src, int dx, int dy);
struct fb_handle       { frame_buffer fb; };
struct fb_atlas_handle { fb_atlas at; };

/* ---- API C ----------------------------------------------------------
 * Cores são passadas empacotadas em ARGB (0xAARRGGBB), o mesmo formato
 * de fb_cores::empacotar(). Ângulos em graus.
 * ------------------------------------------------------------------- */
#if defined(_WIN32)
#  define FB_API __declspec(dllexport)
#else
#  define FB_API __attribute__((visibility("default")))
#endif

extern "C" {

typedef struct fb_handle       fb_handle;
typedef struct fb_atlas_handle fb_atlas_handle;

/* utilidades: ponto fixo (fx, 4 bits) e trigonometria (ângulo 0..65535 = 0..360°) */
FB_API int32_t  fb_fx_de_int(int v);
FB_API int      fb_fx_para_int(int32_t v);
FB_API int32_t  fb_fx_mul(int32_t a, int32_t b);
FB_API int32_t  fb_fx_div(int32_t a, int32_t b);
FB_API uint16_t fb_graus(float g);
FB_API int32_t  fb_seno(uint16_t a);       /* escala 1<<15 */
FB_API int32_t  fb_cosseno(uint16_t a);    /* escala 1<<15 */
FB_API uint32_t fb_cor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/* frame_buffer */
FB_API fb_handle*  fb_criar(int largura, int altura);          /* NULL em falha */
FB_API void        fb_destruir(fb_handle* fb);
FB_API int         fb_valido(const fb_handle* fb);
FB_API int         fb_largura(const fb_handle* fb);
FB_API int         fb_altura(const fb_handle* fb);
FB_API uint32_t*   fb_dados(fb_handle* fb);
FB_API size_t      fb_pitch(const fb_handle* fb);              /* bytes por linha */
FB_API size_t      fb_total_pixels(const fb_handle* fb);
FB_API void        fb_pixel_fx(fb_handle* fb, int32_t x, int32_t y, uint32_t cor);

FB_API void      fb_limpar(fb_handle* fb, uint32_t cor);
FB_API void      fb_pixel(fb_handle* fb, int x, int y, uint32_t cor);
FB_API void      fb_misturar(fb_handle* fb, int x, int y, uint32_t cor);
FB_API uint32_t  fb_ler(const fb_handle* fb, int x, int y);

/* primitivas */
FB_API void  fb_linha(fb_handle* fb, int x0, int y0, int x1, int y1, uint32_t cor);
FB_API void  fb_retangulo(fb_handle* fb, int x, int y, int w, int h, float angulo, int preenchido, uint32_t cor);
FB_API void  fb_triangulo_retangulo(fb_handle* fb, int x, int y, int w, int h, float angulo, int preenchido, uint32_t cor);
FB_API void  fb_circulo(fb_handle* fb, int cx, int cy, int raio, float angulo, int preenchido, uint32_t cor);
FB_API void  fb_blit(fb_handle* dst, const fb_handle* src, int dx, int dy);

/* fb_atlas */
FB_API fb_atlas_handle*  fb_atlas_criar(void);
FB_API void  fb_atlas_destruir(fb_atlas_handle* a);
FB_API int   fb_atlas_carregar(fb_atlas_handle* a, const char* caminho, uint16_t frame_w, uint16_t frame_h);
FB_API int   fb_atlas_carregar_memoria(fb_atlas_handle* a, const uint8_t* dados, size_t tamanho, uint16_t frame_w, uint16_t frame_h);
FB_API int   fb_atlas_valido(const fb_atlas_handle* a);
FB_API int   fb_atlas_colunas(const fb_atlas_handle* a);
FB_API int   fb_atlas_linhas(const fb_atlas_handle* a);
FB_API int   fb_atlas_largura(const fb_atlas_handle* a);
FB_API int   fb_atlas_altura(const fb_atlas_handle* a);
FB_API int   fb_atlas_tile_largura(const fb_atlas_handle* a);
FB_API int   fb_atlas_tile_altura(const fb_atlas_handle* a);
FB_API int   fb_atlas_passo(const fb_atlas_handle* a);         /* pixels por linha do atlas */
FB_API const uint32_t*  fb_atlas_tile_dados(const fb_atlas_handle* a, int pw, int ph);
FB_API void  fb_atlas_desenhar_fx(const fb_atlas_handle* a, fb_handle* fb, int pw, int ph, int32_t x, int32_t y, int w, int h);
FB_API void  fb_atlas_desenhar(const fb_atlas_handle* a, fb_handle* fb, int pw, int ph, int x, int y, int w, int h);

}

#endif