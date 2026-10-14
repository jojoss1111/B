#ifndef CAIXAS_HPP
#define CAIXAS_HPP

#include <stdint.h>
#include "input.hpp"

class Caixa {
public:
    Caixa() = default;
    static void usar_entrada(const Entrada* e) { entrada_ = e; }

    // ---- criação / destruição ----
    void criar(int x, int y, int w, int h);
    void criar(int x, int y, int raio);
    void destruir() { ativa_ = false; }

    // ---- manipulação ----
    void mover(int x, int y) { x_ = x; y_ = y; }
    void tamanho(int w, int h) { w_ = w; h_ = h; }
    void raio(int r) { raio_ = r; }

    // ---- consulta ----
    bool ativa() const { return ativa_; }
    bool dentro(int px, int py) const;
    bool mouse_em_cima() const;       

    // ---- eventos do mouse (só valem com o  em cima da caixa) ----
    bool pressionou(MouseBotao b) const;
    bool segurou(MouseBotao b) const;
    bool soltou(MouseBotao b) const;

    // Versões que recebem a Entrada explicitamente (sem precisar de usar_entrada)
    bool pressionou(const Entrada& in, MouseBotao b) const;
    bool segurou(const Entrada& in, MouseBotao b) const;
    bool soltou(const Entrada& in, MouseBotao b) const;

private:
    enum class Forma : uint8_t { retangulo, circulo };

    static const Entrada* entrada_;

    Forma forma_ = Forma::retangulo;
    bool  ativa_ = false;
    int   x_ = 0, y_ = 0;
    int   w_ = 0, h_ = 0;
    int   raio_ = 0;
};

extern "C" {

typedef struct caixa_handle caixa_handle;   /* Caixa (retângulo ou círculo de clique) */

FB_API caixa_handle* caixa_novo(void);
FB_API void  caixa_destruir(caixa_handle* c);                 /* libera o handle */
FB_API void  caixa_criar_retangulo(caixa_handle* c, int x, int y, int w, int h);
FB_API void  caixa_criar_circulo(caixa_handle* c, int x, int y, int raio);
FB_API void  caixa_desativar(caixa_handle* c);                /* é o Caixa::destruir(): só desativa */

FB_API void  caixa_mover(caixa_handle* c, int x, int y);
FB_API void  caixa_tamanho(caixa_handle* c, int w, int h);
FB_API void  caixa_raio(caixa_handle* c, int r);

FB_API int   caixa_ativa(const caixa_handle* c);
FB_API int   caixa_dentro(const caixa_handle* c, int px, int py);

/* usa a entrada global definida em caixa_usar_entrada */
FB_API void  caixa_usar_entrada(const entrada_handle* e);     /* NULL desfaz */
FB_API int   caixa_mouse_em_cima(const caixa_handle* c);
FB_API int   caixa_pressionou(const caixa_handle* c, int botao);
FB_API int   caixa_segurou(const caixa_handle* c, int botao);
FB_API int   caixa_soltou(const caixa_handle* c, int botao);

/* versões com entrada explícita */
FB_API int   caixa_pressionou_em(const caixa_handle* c, const entrada_handle* e, int botao);
FB_API int   caixa_segurou_em(const caixa_handle* c, const entrada_handle* e, int botao);
FB_API int   caixa_soltou_em(const caixa_handle* c, const entrada_handle* e, int botao);

}

#endif