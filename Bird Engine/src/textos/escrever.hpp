#ifndef ESCREVER_HPP
#define ESCREVER_HPP

#include "../frame buffer/frame_buffer.hpp"
#include <stddef.h>

class Entrada;

enum Padrao{
    ASCII = 0,
    UTF8  = 1
};

struct LetraTile {
    uint32_t letra;
    uint16_t coluna;
    uint16_t linha;
};

class Fonte{
public:
    void criar(fb_atlas *fonte, Padrao escrita, uint16_t pw, uint16_t ph);
    void criar_proprio(fb_atlas *fonte, uint16_t pw, uint16_t ph, const LetraTile *letras_tiles, size_t quantidade);
    void criar_proprio(fb_atlas *fonte, uint16_t pw, uint16_t ph, const char *letras_tiles);
    void escrever(frame_buffer& fb, const char *frase, int32_t x, int32_t y, fb_cores *cor, uint8_t ampliar);
    void escrever_lento(frame_buffer& fb, const char *frase, int32_t x, int32_t y, fb_cores *cor, uint8_t ampliar, float tempo);
    int32_t largura_texto(const char *frase, uint8_t ampliar) const;
    int32_t altura_texto(const char *frase, uint8_t ampliar) const;

private:
    struct Glyph {
        int16_t coluna = -1;
        int16_t linha = -1;
    };

    fb_atlas *atlas_ = nullptr;
    Padrao padrao_ = ASCII;
    uint16_t tile_w_ = 0;
    uint16_t tile_h_ = 0;
    Glyph glyphs_[256] = {};
    const char *frase_lenta_ = nullptr;
    uint32_t inicio_lento_ = 0;

    void limpar_mapa();
    void mapear(uint32_t letra, uint16_t coluna, uint16_t linha);
    Glyph glyph(uint32_t letra) const;
    void escrever_limite(frame_buffer& fb, const char *frase, int32_t x, int32_t y, fb_cores *cor, uint8_t ampliar, int32_t limite);
};

class Digitar{
public:
    int32_t inteiro(Entrada& entrada, uint8_t numero_maximo = 10);
    float   decimal(Entrada& entrada, uint8_t numero_maximo = 10);
    const char *texto(Entrada& entrada, uint16_t numero_maximo = 255);
    const char *conteudo() const { return buffer_; }
    void desenhar(frame_buffer& fb, Fonte& fonte, int32_t x, int32_t y, fb_cores *cor = nullptr,
                  uint8_t ampliar = 1, bool cursor = true) const;

    bool finalizado = false;
    bool travado = false;

    void limpar();
    void deletar();
    void travar();
    void destravar();
    bool esta_finalizado() const { return finalizado; }
    bool esta_travado() const { return travado; }

private:
    char buffer_[256] = {};
    bool iniciou_ = false;
    bool backspace_repetindo_ = false;
    uint32_t backspace_inicio_ = 0;
    uint32_t backspace_ultimo_ = 0;

    void atualizar(Entrada& entrada, uint16_t maximo, bool numero, bool decimal);
};

extern "C" {

typedef struct fonte_handle   fonte_handle;    /* Fonte   */
typedef struct digitar_handle digitar_handle;  /* Digitar */
typedef struct entrada_handle entrada_handle;  /* definido no input.hpp */

/* mesmo layout do LetraTile */
typedef struct fonte_letra_tile {
    uint32_t letra;
    uint16_t coluna;
    uint16_t linha;
} fonte_letra_tile;

/* Cores em ARGB (0xAARRGGBB). Nas funções de texto, cor = 0 significa "manter a cor original do glyph"
   (no Digitar, cor = 0 significa branco). */

/* ---- Fonte ---- */
FB_API fonte_handle* fonte_novo(void);
FB_API void fonte_destruir(fonte_handle* f);
/* padrao: 0 = ASCII, 1 = UTF8. pw/ph = 0 usa o tamanho do tile do atlas. O atlas deve continuar vivo */
FB_API void fonte_criar(fonte_handle* f, fb_atlas_handle* atlas, int padrao, uint16_t pw, uint16_t ph);
FB_API void fonte_criar_proprio_tiles(fonte_handle* f, fb_atlas_handle* atlas, uint16_t pw, uint16_t ph,
                                      const fonte_letra_tile* letras, size_t quantidade);
/* letras = string UTF-8; a n-ésima letra ocupa o n-ésimo tile do atlas (esquerda->direita, cima->baixo) */
FB_API void fonte_criar_proprio_texto(fonte_handle* f, fb_atlas_handle* atlas, uint16_t pw, uint16_t ph, const char* letras);
FB_API void fonte_escrever(fonte_handle* f, fb_handle* fb, const char* frase, int32_t x, int32_t y, uint32_t cor, uint8_t ampliar);
/* tempo = segundos por letra. Reinicia sozinha quando o texto muda */
FB_API void fonte_escrever_lento(fonte_handle* f, fb_handle* fb, const char* frase, int32_t x, int32_t y,
                                 uint32_t cor, uint8_t ampliar, float tempo);
FB_API void fonte_reiniciar_lento(fonte_handle* f);        /* repete a animação do mesmo texto */
FB_API int32_t fonte_largura_texto(const fonte_handle* f, const char* frase, uint8_t ampliar);
FB_API int32_t fonte_altura_texto(const fonte_handle* f, const char* frase, uint8_t ampliar);

/* ---- Digitar ---- */
FB_API digitar_handle* digitar_novo(void);
FB_API void digitar_destruir(digitar_handle* d);
/* cada uma destas três atualiza a digitação; chame UMA por frame (conforme o tipo do campo) */
FB_API int32_t     digitar_inteiro(digitar_handle* d, entrada_handle* e, uint8_t numero_maximo);
FB_API float       digitar_decimal(digitar_handle* d, entrada_handle* e, uint8_t numero_maximo);
FB_API const char* digitar_texto(digitar_handle* d, entrada_handle* e, uint16_t numero_maximo);
FB_API const char* digitar_conteudo(const digitar_handle* d);   /* só lê; válido até a próxima alteração */
FB_API void digitar_desenhar(const digitar_handle* d, fb_handle* fb, fonte_handle* fonte, int32_t x, int32_t y,
                             uint32_t cor, uint8_t ampliar, int cursor);
FB_API void digitar_limpar(digitar_handle* d);
FB_API void digitar_deletar(digitar_handle* d);                 /* limpa e trava */
FB_API void digitar_travar(digitar_handle* d);
FB_API void digitar_destravar(digitar_handle* d);
FB_API int  digitar_finalizado(const digitar_handle* d);        /* apertou Enter */
FB_API int  digitar_travado(const digitar_handle* d);

}

#endif