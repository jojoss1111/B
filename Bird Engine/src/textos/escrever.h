#ifndef ESCREVER_H
#define ESCREVER_H

#include "../frame buffer/frame_buffer.h"
#include "../inputs/input.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fonte_letra_tile {
    uint32_t letra;
    uint16_t coluna;
    uint16_t linha;
} fonte_letra_tile;

typedef struct fonte_glyph {
    int16_t coluna;
    int16_t linha;
} fonte_glyph;

typedef struct fonte_handle {
    fb_atlas_handle* atlas;
    uint8_t     utf8;
    uint16_t    tile_w, tile_h;
    fonte_glyph glyphs[256];
    const char* frase_lenta;
    uint32_t    inicio_lento;
    char*       buf[2];
    size_t      cap[2];
    int         atual;
    uint8_t     tem;
} fonte_handle;

typedef struct digitar_handle {
    char     buffer[256];
    uint8_t  iniciou;
    uint8_t  backspace_repetindo;
    uint8_t  finalizado;
    uint8_t  travado;
    uint32_t backspace_inicio;
    uint32_t backspace_ultimo;
} digitar_handle;

FB_API fonte_handle* fonte_novo(void);
FB_API void fonte_destruir(fonte_handle* f);
FB_API void fonte_criar(fonte_handle* f, fb_atlas_handle* atlas, int padrao, uint16_t pw, uint16_t ph);
FB_API void fonte_criar_proprio_tiles(fonte_handle* f, fb_atlas_handle* atlas, uint16_t pw, uint16_t ph,
                                      const fonte_letra_tile* letras, size_t quantidade);
FB_API void fonte_criar_proprio_texto(fonte_handle* f, fb_atlas_handle* atlas, uint16_t pw, uint16_t ph, const char* letras);
FB_API void fonte_escrever(fonte_handle* f, fb_handle* fb, const char* frase, int32_t x, int32_t y, uint32_t cor, uint8_t ampliar);
FB_API void fonte_escrever_lento(fonte_handle* f, fb_handle* fb, const char* frase, int32_t x, int32_t y,
                                 uint32_t cor, uint8_t ampliar, float tempo);
FB_API void fonte_reiniciar_lento(fonte_handle* f);
FB_API int32_t fonte_largura_texto(const fonte_handle* f, const char* frase, uint8_t ampliar);
FB_API int32_t fonte_altura_texto(const fonte_handle* f, const char* frase, uint8_t ampliar);

FB_API digitar_handle* digitar_novo(void);
FB_API void digitar_destruir(digitar_handle* d);
FB_API int32_t     digitar_inteiro(digitar_handle* d, entrada_handle* e, uint8_t numero_maximo);
FB_API float       digitar_decimal(digitar_handle* d, entrada_handle* e, uint8_t numero_maximo);
FB_API const char* digitar_texto(digitar_handle* d, entrada_handle* e, uint16_t numero_maximo);
FB_API const char* digitar_conteudo(const digitar_handle* d);
FB_API void digitar_desenhar(const digitar_handle* d, fb_handle* fb, fonte_handle* fonte, int32_t x, int32_t y,
                             uint32_t cor, uint8_t ampliar, int cursor);
FB_API void digitar_limpar(digitar_handle* d);
FB_API void digitar_deletar(digitar_handle* d);
FB_API void digitar_travar(digitar_handle* d);
FB_API void digitar_destravar(digitar_handle* d);
FB_API int  digitar_finalizado(const digitar_handle* d);
FB_API int  digitar_travado(const digitar_handle* d);

#ifdef __cplusplus
}
#endif

#endif