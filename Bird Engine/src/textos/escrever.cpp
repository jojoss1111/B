#include "escrever.hpp"
#include "../inputs/input.hpp"

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>

namespace {

uint32_t proximo_codigo(const char*& p, Padrao padrao) {
    const unsigned char c = (unsigned char)*p++;
    if (padrao != UTF8 || c < 0x80) return c;

    if ((c & 0xE0) == 0xC0 && (p[0] & 0xC0) == 0x80) {
        uint32_t r = ((uint32_t)(c & 0x1F) << 6) | (uint32_t)(p[0] & 0x3F);
        p += 1;
        return r;
    }
    if ((c & 0xF0) == 0xE0 && (p[0] & 0xC0) == 0x80 && (p[1] & 0xC0) == 0x80) {
        uint32_t r = ((uint32_t)(c & 0x0F) << 12) |
                     ((uint32_t)(p[0] & 0x3F) << 6) |
                     (uint32_t)(p[1] & 0x3F);
        p += 2;
        return r;
    }
    if ((c & 0xF8) == 0xF0 && (p[0] & 0xC0) == 0x80 &&
        (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
        uint32_t r = ((uint32_t)(c & 0x07) << 18) |
                     ((uint32_t)(p[0] & 0x3F) << 12) |
                     ((uint32_t)(p[1] & 0x3F) << 6) |
                     (uint32_t)(p[2] & 0x3F);
        p += 3;
        return r;
    }
    return '?';
}

void desenhar_glyph(frame_buffer& fb, const fb_atlas& atlas, int col, int lin,
                    int x, int y, int w, int h, const fb_cores* cor) {
    if (w <= 0 || h <= 0) return;
    if (col < 0 || lin < 0 || col >= atlas.colunas() || lin >= atlas.linhas()) return;

    const int fw = atlas.tile_largura();
    const int fh = atlas.tile_altura();
    const int passo = atlas.passo();
    const uint32_t* base = atlas.tile_dados(col, lin);

    const int i0 = x < 0 ? -x : 0;
    const int j0 = y < 0 ? -y : 0;
    const int i1 = x + w > fb.largura() ? fb.largura() - x : w;
    const int j1 = y + h > fb.altura()  ? fb.altura()  - y : h;

    for (int j = j0; j < j1; j++) {
        const int sy = (j * fh) / h;
        for (int i = i0; i < i1; i++) {
            const int sx = (i * fw) / w;
            fb_cores px = fb_cores::desempacotar(base[sy * passo + sx]);
            if (px.a == 0) continue;
            if (cor) {
                const uint8_t alpha = px.a;
                px.r = cor->r;
                px.g = cor->g;
                px.b = cor->b;
                px.a = (uint8_t)((alpha * cor->a) / 255);
            }
            fb.misturar(x + i, y + j, px);
        }
    }
}

bool permitido_numero(unsigned char c, bool decimal) {
    if (c >= '0' && c <= '9') return true;
    if (c == '-') return true;
    return decimal && (c == '.' || c == ',');
}

}

void Fonte::limpar_mapa() {
    for (int i = 0; i < 256; i++) glyphs_[i] = Glyph{};
}

void Fonte::mapear(uint32_t letra, uint16_t coluna, uint16_t linha) {
    if (letra < 256) {
        glyphs_[letra].coluna = (int16_t)coluna;
        glyphs_[letra].linha = (int16_t)linha;
    }
}

Fonte::Glyph Fonte::glyph(uint32_t letra) const {
    if (letra < 256 && glyphs_[letra].coluna >= 0) return glyphs_[letra];
    if (glyphs_[(unsigned)'?'].coluna >= 0) return glyphs_[(unsigned)'?'];
    return Glyph{};
}

void Fonte::criar(fb_atlas *fonte, Padrao escrita, uint16_t pw, uint16_t ph) {
    atlas_ = fonte;
    padrao_ = escrita;
    tile_w_ = pw ? pw : (fonte ? (uint16_t)fonte->tile_largura() : 0);
    tile_h_ = ph ? ph : (fonte ? (uint16_t)fonte->tile_altura() : 0);
    frase_lenta_ = nullptr;
    inicio_lento_ = 0;

    limpar_mapa();
    if (!atlas_ || !atlas_->valido()) return;

    int idx = 0;
    const int cols = atlas_->colunas();
    for (uint32_t c = 32; c <= 126; c++, idx++) {
        mapear(c, (uint16_t)(idx % cols), (uint16_t)(idx / cols));
    }
}

void Fonte::criar_proprio(fb_atlas *fonte, uint16_t pw, uint16_t ph,
                          const LetraTile *letras_tiles, size_t quantidade) {
    atlas_ = fonte;
    padrao_ = UTF8;
    tile_w_ = pw ? pw : (fonte ? (uint16_t)fonte->tile_largura() : 0);
    tile_h_ = ph ? ph : (fonte ? (uint16_t)fonte->tile_altura() : 0);
    frase_lenta_ = nullptr;
    inicio_lento_ = 0;

    limpar_mapa();
    if (!letras_tiles) return;
    for (size_t i = 0; i < quantidade; i++) {
        mapear(letras_tiles[i].letra, letras_tiles[i].coluna, letras_tiles[i].linha);
    }
}

void Fonte::criar_proprio(fb_atlas *fonte, uint16_t pw, uint16_t ph, const char *letras_tiles) {
    atlas_ = fonte;
    padrao_ = UTF8;
    tile_w_ = pw ? pw : (fonte ? (uint16_t)fonte->tile_largura() : 0);
    tile_h_ = ph ? ph : (fonte ? (uint16_t)fonte->tile_altura() : 0);
    frase_lenta_ = nullptr;
    inicio_lento_ = 0;

    limpar_mapa();
    if (!atlas_ || !atlas_->valido() || !letras_tiles) return;

    int idx = 0;
    const int cols = atlas_->colunas();
    const char* p = letras_tiles;
    while (*p) {
        uint32_t c = proximo_codigo(p, UTF8);
        mapear(c, (uint16_t)(idx % cols), (uint16_t)(idx / cols));
        idx++;
    }
}

void Fonte::escrever_limite(frame_buffer& fb, const char *frase, int32_t x, int32_t y, fb_cores *cor,
                            uint8_t ampliar, int32_t limite) {
    if (!atlas_ || !atlas_->valido() || !frase) return;

    if (ampliar == 0) ampliar = 1;
    const int gw = (tile_w_ ? tile_w_ : (uint16_t)atlas_->tile_largura()) * ampliar;
    const int gh = (tile_h_ ? tile_h_ : (uint16_t)atlas_->tile_altura()) * ampliar;
    int cx = x;
    int cy = y;
    int32_t escritos = 0;

    const char* p = frase;
    while (*p && (limite < 0 || escritos < limite)) {
        uint32_t c = proximo_codigo(p, padrao_);
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

        Glyph g = glyph(c);
        if (g.coluna >= 0 && g.linha >= 0) {
            desenhar_glyph(fb, *atlas_, g.coluna, g.linha, cx, cy, gw, gh, cor);
        }
        cx += gw;
        escritos++;
    }
}

void Fonte::escrever(frame_buffer& fb, const char *frase, int32_t x, int32_t y, fb_cores *cor, uint8_t ampliar) {
    escrever_limite(fb, frase, x, y, cor, ampliar, -1);
}

void Fonte::escrever_lento(frame_buffer& fb, const char *frase, int32_t x, int32_t y, fb_cores *cor,
                           uint8_t ampliar, float tempo) {
    if (!frase) return;
    if (frase != frase_lenta_) {
        frase_lenta_ = frase;
        inicio_lento_ = SDL_GetTicks();
    }

    int32_t limite = -1;
    if (tempo > 0.0f) {
        const float passado = (SDL_GetTicks() - inicio_lento_) / 1000.0f;
        limite = (int32_t)(passado / tempo) + 1;
    }
    escrever_limite(fb, frase, x, y, cor, ampliar, limite);
}

int32_t Fonte::largura_texto(const char *frase, uint8_t ampliar) const {
    if (!frase) return 0;
    if (ampliar == 0) ampliar = 1;
    const int gw = (tile_w_ ? tile_w_ : (atlas_ ? (uint16_t)atlas_->tile_largura() : 0)) * ampliar;
    int atual = 0;
    int maior = 0;

    const char* p = frase;
    while (*p) {
        uint32_t c = proximo_codigo(p, padrao_);
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

int32_t Fonte::altura_texto(const char *frase, uint8_t ampliar) const {
    if (!frase) return 0;
    if (ampliar == 0) ampliar = 1;
    const int gh = (tile_h_ ? tile_h_ : (atlas_ ? (uint16_t)atlas_->tile_altura() : 0)) * ampliar;
    int linhas = 1;
    for (const char* p = frase; *p; p++) if (*p == '\n') linhas++;
    return linhas * gh;
}

void Digitar::limpar() {
    buffer_[0] = '\0';
    iniciou_ = true;
    finalizado = false;
    backspace_repetindo_ = false;
    backspace_inicio_ = 0;
    backspace_ultimo_ = 0;
}

void Digitar::deletar() {
    limpar();
    travar();
}

void Digitar::travar() {
    travado = true;
    backspace_repetindo_ = false;
}

void Digitar::destravar() {
    travado = false;
    finalizado = false;
    backspace_repetindo_ = false;
}

void Digitar::atualizar(Entrada& entrada, uint16_t maximo, bool numero, bool decimal) {
    if (!iniciou_) limpar();
    if (maximo >= sizeof(buffer_)) maximo = (uint16_t)(sizeof(buffer_) - 1);
    if (travado || finalizado) return;

    if (entrada.pressionou(Tecla::start)) {
        finalizado = true;
        return;
    }

    const uint32_t agora = SDL_GetTicks();
    bool apagar = entrada.pressionou(Tecla::backspace);
    if (entrada.segurando(Tecla::backspace)) {
        if (!backspace_repetindo_) {
            backspace_repetindo_ = true;
            backspace_inicio_ = agora;
            backspace_ultimo_ = agora;
        } else if (agora - backspace_inicio_ >= 350 && agora - backspace_ultimo_ >= 45) {
            apagar = true;
            backspace_ultimo_ = agora;
        }
    } else {
        backspace_repetindo_ = false;
    }

    if (apagar) {
        const size_t n = strlen(buffer_);
        if (n > 0) buffer_[n - 1] = '\0';
    }

    const char* txt = entrada.texto_digitado();
    size_t n = strlen(buffer_);
    for (size_t i = 0; txt && txt[i] && n < maximo; i++) {
        unsigned char c = (unsigned char)txt[i];
        if (numero && !permitido_numero(c, decimal)) continue;
        if (c == ',' && decimal) c = '.';
        buffer_[n++] = (char)c;
        buffer_[n] = '\0';
    }
}

int32_t Digitar::inteiro(Entrada& entrada, uint8_t numero_maximo) {
    atualizar(entrada, numero_maximo, true, false);
    return (int32_t)strtol(buffer_, nullptr, 10);
}

float Digitar::decimal(Entrada& entrada, uint8_t numero_maximo) {
    atualizar(entrada, numero_maximo, true, true);
    return strtof(buffer_, nullptr);
}

const char *Digitar::texto(Entrada& entrada, uint16_t numero_maximo) {
    atualizar(entrada, numero_maximo, false, false);
    return buffer_;
}

void Digitar::desenhar(frame_buffer& fb, Fonte& fonte, int32_t x, int32_t y, fb_cores *cor,
                       uint8_t ampliar, bool cursor) const {
    if (ampliar == 0) ampliar = 1;

    fb_cores branco = {255, 255, 255, 255};
    fb_cores *c = cor ? cor : &branco;

    if (buffer_[0]) fonte.escrever(fb, buffer_, x, y, c, ampliar);

    if (!cursor || travado || finalizado) return;
    if ((SDL_GetTicks() / 500) % 2) return;

    const int32_t cx = x + fonte.largura_texto(buffer_, ampliar);
    const int32_t ch = fonte.altura_texto(buffer_, ampliar);
    retangulo(fb, cx, y, ampliar, ch, 0, true, *c);
}

/* ---- API C ---- */

/* Fonte com cópia do último texto lento: o Fonte original detecta "texto novo" comparando o PONTEIRO,
   o que quebra quando a outra linguagem passa um buffer diferente (ou o mesmo) a cada chamada.
   Aqui o texto é copiado para um de dois buffers; o ponteiro só muda quando o conteúdo muda. */
struct fonte_handle {
    Fonte  f;
    char*  buf[2] = { nullptr, nullptr };
    size_t cap[2] = { 0, 0 };
    int    atual  = 0;
    bool   tem    = false;
    ~fonte_handle() { memoria_free(buf[0]); memoria_free(buf[1]); }
};

static_assert(sizeof(fonte_letra_tile) == sizeof(LetraTile), "layout diferente de LetraTile");

namespace {

inline Digitar*       D(digitar_handle* h)        { return reinterpret_cast<Digitar*>(h); }
inline const Digitar* D(const digitar_handle* h)  { return reinterpret_cast<const Digitar*>(h); }
inline fb_atlas*      AT(fb_atlas_handle* h)      { return h ? &h->at : nullptr; }

inline fb_cores* cor_opc(uint32_t cor, fb_cores& tmp) {
    if (!cor) return nullptr;
    tmp = fb_cores::desempacotar(cor);
    return &tmp;
}

const char* fixar(fonte_handle* h, const char* frase, bool forcar) {
    if (!forcar && h->tem && strcmp(h->buf[h->atual], frase) == 0) return h->buf[h->atual];

    const int novo = h->tem ? 1 - h->atual : 0;
    const size_t n = strlen(frase) + 1;
    if (h->cap[novo] < n) {
        char* p = (char*)memoria_malloc(n);
        if (!p) return nullptr;
        memoria_free(h->buf[novo]);
        h->buf[novo] = p;
        h->cap[novo] = n;
    }
    memcpy(h->buf[novo], frase, n);
    h->atual = novo;
    h->tem = true;
    return h->buf[novo];
}

}

extern "C" {

/* ---- Fonte ---- */
fonte_handle* fonte_novo(void)                  { return new fonte_handle; }
void fonte_destruir(fonte_handle* f)            { delete f; }

void fonte_criar(fonte_handle* f, fb_atlas_handle* atlas, int padrao, uint16_t pw, uint16_t ph) {
    if (f) f->f.criar(AT(atlas), padrao ? UTF8 : ASCII, pw, ph);
}
void fonte_criar_proprio_tiles(fonte_handle* f, fb_atlas_handle* atlas, uint16_t pw, uint16_t ph,
                               const fonte_letra_tile* letras, size_t quantidade) {
    if (f) f->f.criar_proprio(AT(atlas), pw, ph, reinterpret_cast<const LetraTile*>(letras), quantidade);
}
void fonte_criar_proprio_texto(fonte_handle* f, fb_atlas_handle* atlas, uint16_t pw, uint16_t ph, const char* letras) {
    if (f) f->f.criar_proprio(AT(atlas), pw, ph, letras);
}

void fonte_escrever(fonte_handle* f, fb_handle* fb, const char* frase, int32_t x, int32_t y, uint32_t cor, uint8_t ampliar) {
    if (!f || !fb || !frase) return;
    fb_cores tmp;
    f->f.escrever(fb->fb, frase, x, y, cor_opc(cor, tmp), ampliar);
}

void fonte_escrever_lento(fonte_handle* f, fb_handle* fb, const char* frase, int32_t x, int32_t y,
                          uint32_t cor, uint8_t ampliar, float tempo) {
    if (!f || !fb || !frase) return;
    const char* fixo = fixar(f, frase, false);
    if (!fixo) return;
    fb_cores tmp;
    f->f.escrever_lento(fb->fb, fixo, x, y, cor_opc(cor, tmp), ampliar, tempo);
}

void fonte_reiniciar_lento(fonte_handle* f) {
    if (f && f->tem) fixar(f, f->buf[f->atual], true);   /* troca de buffer -> ponteiro novo -> reinicia */
}

int32_t fonte_largura_texto(const fonte_handle* f, const char* frase, uint8_t ampliar) { return f ? f->f.largura_texto(frase, ampliar) : 0; }
int32_t fonte_altura_texto(const fonte_handle* f, const char* frase, uint8_t ampliar)  { return f ? f->f.altura_texto(frase, ampliar) : 0; }

/* ---- Digitar ---- */
digitar_handle* digitar_novo(void)              { return reinterpret_cast<digitar_handle*>(new Digitar); }
void digitar_destruir(digitar_handle* d)        { delete D(d); }

int32_t digitar_inteiro(digitar_handle* d, entrada_handle* e, uint8_t maximo) { return (d && e) ? D(d)->inteiro(e->e, maximo) : 0; }
float   digitar_decimal(digitar_handle* d, entrada_handle* e, uint8_t maximo) { return (d && e) ? D(d)->decimal(e->e, maximo) : 0.0f; }
const char* digitar_texto(digitar_handle* d, entrada_handle* e, uint16_t maximo) { return (d && e) ? D(d)->texto(e->e, maximo) : ""; }
const char* digitar_conteudo(const digitar_handle* d) { return d ? D(d)->conteudo() : ""; }

void digitar_desenhar(const digitar_handle* d, fb_handle* fb, fonte_handle* fonte, int32_t x, int32_t y,
                      uint32_t cor, uint8_t ampliar, int cursor) {
    if (!d || !fb || !fonte) return;
    fb_cores tmp;
    D(d)->desenhar(fb->fb, fonte->f, x, y, cor_opc(cor, tmp), ampliar, cursor != 0);
}

void digitar_limpar(digitar_handle* d)          { if (d) D(d)->limpar(); }
void digitar_deletar(digitar_handle* d)         { if (d) D(d)->deletar(); }
void digitar_travar(digitar_handle* d)          { if (d) D(d)->travar(); }
void digitar_destravar(digitar_handle* d)       { if (d) D(d)->destravar(); }
int  digitar_finalizado(const digitar_handle* d){ return d && D(d)->esta_finalizado(); }
int  digitar_travado(const digitar_handle* d)   { return d && D(d)->esta_travado(); }

}