#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <crock.h>
#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32)
#  define FB_API __declspec(dllexport)
#else
#  define FB_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t fx_t;

typedef struct fb_handle {
    uint32_t* px;
    int       w;
    int       h;
} fb_handle;

typedef struct fb_atlas_handle {
    uint32_t* px;
    int       w;
    int       h;
    int       fw;
    int       fh;
} fb_atlas_handle;

/* Cores são passadas empacotadas em ARGB (0xAARRGGBB). Ângulos em graus. */

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

#ifdef __cplusplus
}
#endif

#endif