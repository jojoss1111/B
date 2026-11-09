#include "escrever.h"

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>

static uint32_t proximo_codigo(const char** pp, int utf8) {
    const char* p = *pp;
    const unsigned char c = (unsigned char)*p++;
    *pp = p;
    if (!utf8 || c < 0x80) return c;

    if ((c & 0xE0) == 0xC0 && (p[0] & 0xC0) == 0x80) {
        uint32_t r = ((uint32_t)(c & 0x1F) << 6) | (uint32_t)(p[0] & 0x3F);
        *pp = p + 1;
        return r;
    }
    if ((c & 0xF0) == 0xE0 && (p[0] & 0xC0) == 0x80 && (p[1] & 0xC0) == 0x80) {
        uint32_t r = ((uint32_t)(c & 0x0F) << 12) |
                     ((uint32_t)(p[0] & 0x3F) << 6) |
                     (uint32_t)(p[1] & 0x3F);
        *pp = p + 2;
        return r;
    }
    if ((c & 0xF8) == 0xF0 && (p[0] & 0xC0) == 0x80 &&
        (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
        uint32_t r = ((uint32_t)(c & 0x07) << 18) |
                     ((uint32_t)(p[0] & 0x3F) << 12) |
                     ((uint32_t)(p[1] & 0x3F) << 6) |
                     (uint32_t)(p[2] & 0x3F);
        *pp = p + 3;
        return r;
    }
    return '?';
}

static void desenhar_glyph(fb_handle* fb, const fb_atlas_handle* atlas, int col, int lin,
                           int x, int y, int w, int h, uint32_t cor) {
    if (w <= 0 || h <= 0) return;
    if (col < 0 || lin < 0 || col >= fb_atlas_colunas(atlas) || lin >= fb_atlas_linhas(atlas)) return;

    const int fw = fb_atlas_tile_largura(atlas);
    const int fh = fb_atlas_tile_altura(atlas);
    const int passo = fb_atlas_passo(atlas);
    const uint32_t* base = fb_atlas_tile_dados(atlas, col, lin);

    const int i0 = x < 0 ? -x : 0;
    const int j0 = y < 0 ? -y : 0;
    const int i1 = x + w > fb_largura(fb) ? fb_largura(fb) - x : w;
    const int j1 = y + h > fb_altura(fb)  ? fb_altura(fb)  - y : h;

    for (int j = j0; j < j1; j++) {
        const int sy = (j * fh) / h;
        for (int i = i0; i < i1; i++) {
            const int sx = (i * fw) / w;
            uint32_t px = base[sy * passo + sx];
            const uint32_t alpha = px >> 24;
            if (alpha == 0) continue;
            if (cor) {
                const uint32_t a = (alpha * (cor >> 24)) / 255;
                px = (a << 24) | (cor & 0x00FFFFFFu);
            }
            fb_misturar(fb, x + i, y + j, px);
        }
    }
}

static int permitido_numero(unsigned char c, int decimal) {
    if (c >= '0' && c <= '9') return 1;
    if (c == '-') return 1;
    return decimal && (c == '.' || c == ',');
}

static void limpar_mapa(fonte_handle* f) {
    for (int i = 0; i < 256; i++) { f->glyphs[i].coluna = -1; f->glyphs[i].linha = -1; }
}

static void mapear(fonte_handle* f, uint32_t letra, uint16_t coluna, uint16_t linha) {
    if (letra < 256) {
        f->glyphs[letra].coluna = (int16_t)coluna;
        f->glyphs[letra].linha = (int16_t)linha;
    }
}

static fonte_glyph achar_glyph(const fonte_handle* f, uint32_t letra) {
    fonte_glyph vazio = { -1, -1 };
    if (letra < 256 && f->glyphs[letra].coluna >= 0) return f->glyphs[letra];
    if (f->glyphs[(unsigned)'?'].coluna >= 0) return f->glyphs['?'];
    return vazio;
}

static void fonte_preparar(fonte_handle* f, fb_atlas_handle* atlas, int utf8, uint16_t pw, uint16_t ph) {
    f->atlas = atlas;
    f->utf8 = utf8 != 0;
    f->tile_w = pw ? pw : (atlas ? (uint16_t)fb_atlas_tile_largura(atlas) : 0);
    f->tile_h = ph ? ph : (atlas ? (uint16_t)fb_atlas_tile_altura(atlas) : 0);
    f->frase_lenta = NULL;
    f->inicio_lento = 0;
    limpar_mapa(f);
}

fonte_handle* fonte_novo(void) {
    fonte_handle* f = (fonte_handle*)memoria_malloc(sizeof(fonte_handle));
    if (!f) return NULL;
    f->atlas = NULL;
    f->utf8 = 0;
    f->tile_w = f->tile_h = 0;
    f->frase_lenta = NULL;
    f->inicio_lento = 0;
    f->buf[0] = f->buf[1] = NULL;
    f->cap[0] = f->cap[1] = 0;
    f->atual = 0;
    f->tem = 0;
    limpar_mapa(f);
    return f;
}

void fonte_destruir(fonte_handle* f) {
    if (!f) return;
    memoria_free(f->buf[0]);
    memoria_free(f->buf[1]);
    memoria_free(f);
}

void fonte_criar(fonte_handle* f, fb_atlas_handle* atlas, int padrao, uint16_t pw, uint16_t ph) {
    if (!f) return;
    fonte_preparar(f, atlas, padrao, pw, ph);
    if (!atlas || !fb_atlas_valido(atlas)) return;

    int idx = 0;
    const int cols = fb_atlas_colunas(atlas);
    for (uint32_t c = 32; c <= 126; c++, idx++) {
        mapear(f, c, (uint16_t)(idx % cols), (uint16_t)(idx / cols));
    }
}

void fonte_criar_proprio_tiles(fonte_handle* f, fb_atlas_handle* atlas, uint16_t pw, uint16_t ph,
                               const fonte_letra_tile* letras, size_t quantidade) {
    if (!f) return;
    fonte_preparar(f, atlas, 1, pw, ph);
    if (!letras) return;
    for (size_t i = 0; i < quantidade; i++) {
        mapear(f, letras[i].letra, letras[i].coluna, letras[i].linha);
    }
}

void fonte_criar_proprio_texto(fonte_handle* f, fb_atlas_handle* atlas, uint16_t pw, uint16_t ph, const char* letras) {
    if (!f) return;
    fonte_preparar(f, atlas, 1, pw, ph);
    if (!atlas || !fb_atlas_valido(atlas) || !letras) return;

    int idx = 0;
    const int cols = fb_atlas_colunas(atlas);
    const char* p = letras;
    while (*p) {
        uint32_t c = proximo_codigo(&p, 1);
        mapear(f, c, (uint16_t)(idx % cols), (uint16_t)(idx / cols));
        idx++;
    }
}

static void escrever_limite(fonte_handle* f, fb_handle* fb, const char* frase, int32_t x, int32_t y,
                            uint32_t cor, uint8_t ampliar, int32_t limite) {
    if (!f->atlas || !fb_atlas_valido(f->atlas) || !frase) return;

    if (ampliar == 0) ampliar = 1;
    const int gw = (f->tile_w ? f->tile_w : (uint16_t)fb_atlas_tile_largura(f->atlas)) * ampliar;
    const int gh = (f->tile_h ? f->tile_h : (uint16_t)fb_atlas_tile_altura(f->atlas)) * ampliar;
    int cx = x;
    int cy = y;
    int32_t escritos = 0;

    const char* p = frase;
    while (*p && (limite < 0 || escritos < limite)) {
        uint32_t c = proximo_codigo(&p, f->utf8);
        if (c == '\n') {
            cx = x;
            cy += gh;
            continue;
        }
        if (c == '\t') {
            cx += gw * 4;
            escritos++;
            continue;
        }
        if (c == ' ') {
            cx += gw;
            escritos++;
            continue;
        }

        fonte_glyph g = achar_glyph(f, c);
        if (g.coluna >= 0 && g.linha >= 0) {
            desenhar_glyph(fb, f->atlas, g.coluna, g.linha, cx, cy, gw, gh, cor);
        }
        cx += gw;
        escritos++;
    }
}

void fonte_escrever(fonte_handle* f, fb_handle* fb, const char* frase, int32_t x, int32_t y, uint32_t cor, uint8_t ampliar) {
    if (!f || !fb || !frase) return;
    escrever_limite(f, fb, frase, x, y, cor, ampliar, -1);
}

static const char* fixar(fonte_handle* h, const char* frase, int forcar) {
    if (!forcar && h->tem && strcmp(h->buf[h->atual], frase) == 0) return h->buf[h->atual];

    const int novo = h->tem ? 1 - h->atual : 0;
    const size_t n = strlen(frase) + 1;
    if (h->cap[novo] < n) {
        char* p = (char*)memoria_malloc(n);
        if (!p) return NULL;
        memoria_free(h->buf[novo]);
        h->buf[novo] = p;
        h->cap[novo] = n;
    }
    memcpy(h->buf[novo], frase, n);
    h->atual = novo;
    h->tem = 1;
    return h->buf[novo];
}

void fonte_escrever_lento(fonte_handle* f, fb_handle* fb, const char* frase, int32_t x, int32_t y,
                          uint32_t cor, uint8_t ampliar, float tempo) {
    if (!f || !fb || !frase) return;
    const char* fixo = fixar(f, frase, 0);
    if (!fixo) return;

    if (fixo != f->frase_lenta) {
        f->frase_lenta = fixo;
        f->inicio_lento = SDL_GetTicks();
    }

    int32_t limite = -1;
    if (tempo > 0.0f) {
        const float passado = (SDL_GetTicks() - f->inicio_lento) / 1000.0f;
        limite = (int32_t)(passado / tempo) + 1;
    }
    escrever_limite(f, fb, fixo, x, y, cor, ampliar, limite);
}

void fonte_reiniciar_lento(fonte_handle* f) {
    if (f && f->tem) fixar(f, f->buf[f->atual], 1);
}

int32_t fonte_largura_texto(const fonte_handle* f, const char* frase, uint8_t ampliar) {
    if (!f || !frase) return 0;
    if (ampliar == 0) ampliar = 1;
    const int gw = (f->tile_w ? f->tile_w : (f->atlas ? (uint16_t)fb_atlas_tile_largura(f->atlas) : 0)) * ampliar;
    int atual = 0;
    int maior = 0;

    const char* p = frase;
    while (*p) {
        uint32_t c = proximo_codigo(&p, f->utf8);
        if (c == '\n') {
            if (atual > maior) maior = atual;
            atual = 0;
        } else if (c == '\t') {
            atual += gw * 4;
        } else {
            atual += gw;
        }
    }
    return atual > maior ? atual : maior;
}

int32_t fonte_altura_texto(const fonte_handle* f, const char* frase, uint8_t ampliar) {
    if (!f || !frase) return 0;
    if (ampliar == 0) ampliar = 1;
    const int gh = (f->tile_h ? f->tile_h : (f->atlas ? (uint16_t)fb_atlas_tile_altura(f->atlas) : 0)) * ampliar;
    int linhas = 1;
    for (const char* p = frase; *p; p++) if (*p == '\n') linhas++;
    return linhas * gh;
}

digitar_handle* digitar_novo(void) {
    digitar_handle* d = (digitar_handle*)memoria_malloc(sizeof(digitar_handle));
    if (!d) return NULL;
    for (size_t i = 0; i < sizeof(d->buffer); i++) d->buffer[i] = '\0';
    d->iniciou = 0;
    d->backspace_repetindo = 0;
    d->finalizado = 0;
    d->travado = 0;
    d->backspace_inicio = 0;
    d->backspace_ultimo = 0;
    return d;
}

void digitar_destruir(digitar_handle* d) { if (d) memoria_free(d); }

void digitar_limpar(digitar_handle* d) {
    if (!d) return;
    d->buffer[0] = '\0';
    d->iniciou = 1;
    d->finalizado = 0;
    d->backspace_repetindo = 0;
    d->backspace_inicio = 0;
    d->backspace_ultimo = 0;
}

void digitar_travar(digitar_handle* d) {
    if (!d) return;
    d->travado = 1;
    d->backspace_repetindo = 0;
}

void digitar_deletar(digitar_handle* d) {
    digitar_limpar(d);
    digitar_travar(d);
}

void digitar_destravar(digitar_handle* d) {
    if (!d) return;
    d->travado = 0;
    d->finalizado = 0;
    d->backspace_repetindo = 0;
}

int digitar_finalizado(const digitar_handle* d) { return d && d->finalizado; }
int digitar_travado(const digitar_handle* d)    { return d && d->travado; }

static void digitar_atualizar(digitar_handle* d, entrada_handle* e, uint16_t maximo, int numero, int decimal) {
    if (!d->iniciou) digitar_limpar(d);
    if (maximo >= sizeof(d->buffer)) maximo = (uint16_t)(sizeof(d->buffer) - 1);
    if (d->travado || d->finalizado) return;

    if (entrada_pressionou(e, TECLA_START)) {
        d->finalizado = 1;
        return;
    }

    const uint32_t agora = SDL_GetTicks();
    int apagar = entrada_pressionou(e, TECLA_BACKSPACE);
    if (entrada_segurando(e, TECLA_BACKSPACE)) {
        if (!d->backspace_repetindo) {
            d->backspace_repetindo = 1;
            d->backspace_inicio = agora;
            d->backspace_ultimo = agora;
        } else if (agora - d->backspace_inicio >= 350 && agora - d->backspace_ultimo >= 45) {
            apagar = 1;
            d->backspace_ultimo = agora;
        }
    } else {
        d->backspace_repetindo = 0;
    }

    if (apagar) {
        const size_t n = strlen(d->buffer);
        if (n > 0) d->buffer[n - 1] = '\0';
    }

    const char* txt = entrada_texto_digitado(e);
    size_t n = strlen(d->buffer);
    for (size_t i = 0; txt && txt[i] && n < maximo; i++) {
        unsigned char c = (unsigned char)txt[i];
        if (numero && !permitido_numero(c, decimal)) continue;
        if (c == ',' && decimal) c = '.';
        d->buffer[n++] = (char)c;
        d->buffer[n] = '\0';
    }
}

int32_t digitar_inteiro(digitar_handle* d, entrada_handle* e, uint8_t numero_maximo) {
    if (!d || !e) return 0;
    digitar_atualizar(d, e, numero_maximo, 1, 0);
    return (int32_t)strtol(d->buffer, NULL, 10);
}

float digitar_decimal(digitar_handle* d, entrada_handle* e, uint8_t numero_maximo) {
    if (!d || !e) return 0.0f;
    digitar_atualizar(d, e, numero_maximo, 1, 1);
    return strtof(d->buffer, NULL);
}

const char* digitar_texto(digitar_handle* d, entrada_handle* e, uint16_t numero_maximo) {
    if (!d || !e) return "";
    digitar_atualizar(d, e, numero_maximo, 0, 0);
    return d->buffer;
}

const char* digitar_conteudo(const digitar_handle* d) { return d ? d->buffer : ""; }

void digitar_desenhar(const digitar_handle* d, fb_handle* fb, fonte_handle* fonte, int32_t x, int32_t y,
                      uint32_t cor, uint8_t ampliar, int cursor) {
    if (!d || !fb || !fonte) return;
    if (ampliar == 0) ampliar = 1;

    const uint32_t c = cor ? cor : 0xFFFFFFFFu;

    if (d->buffer[0]) fonte_escrever(fonte, fb, d->buffer, x, y, c, ampliar);

    if (!cursor || d->travado || d->finalizado) return;
    if ((SDL_GetTicks() / 500) % 2) return;

    const int32_t cx = x + fonte_largura_texto(fonte, d->buffer, ampliar);
    const int32_t ch = fonte_altura_texto(fonte, d->buffer, ampliar);
    fb_retangulo(fb, cx, y, ampliar, ch, 0, 1, c);
}