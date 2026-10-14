#include "render.hpp"

#include <SDL2/SDL.h>

namespace {

SDL_Window   *g_janela = nullptr;
SDL_Renderer *g_render = nullptr;
SDL_Texture  *g_tela   = nullptr;

fb_handle      g_fb_h;
entrada_handle g_entrada_h;
frame_buffer&  g_fb      = g_fb_h.fb;
Entrada&       g_entrada = g_entrada_h.e;
Cor          g_cor = {0, 0, 0, 255};

ModoTela g_modo = ModoTela::letterbox;
ModoTela g_modo_base = ModoTela::letterbox;
int      g_ampliar = 1;

uint16_t g_largura = 0, g_altura = 0;
bool     g_em_loop = false;
bool     g_sair = false;
float    g_fps_atual = 0.0f;

struct destino { float escala, ox, oy; };

destino calcular_destino(int ww, int wh) {
    const float ex = (float)ww / (float)g_largura;
    const float ey = (float)wh / (float)g_altura;
    const float e = ex < ey ? ex : ey;
    return { e, ((float)ww - g_largura * e) * 0.5f, ((float)wh - g_altura * e) * 0.5f };
}

int piso(float v) {
    const int i = (int)v;
    return (v < 0.0f && (float)i != v) ? i - 1 : i;
}

int escala_inicial(int w, int h) {
    SDL_Rect r;
    if (SDL_GetDisplayUsableBounds(0, &r) != 0 || r.w <= 0 || r.h <= 0) return 1;
    const int ex = (r.w * 8 / 10) / w;
    const int ey = (r.h * 8 / 10) / h;
    const int e = ex < ey ? ex : ey;
    return e < 1 ? 1 : e;
}

int resolver_ampliar(int ampliar, int w, int h) {
    return ampliar > 0 ? ampliar : escala_inicial(w, h);
}

void aplicar_icone(Atlas *icone) {
    if (!icone || !icone->valido() || !g_janela) return;

    SDL_Surface *s = SDL_CreateRGBSurfaceFrom((void *)icone->tile_dados(0, 0),
                                              icone->tile_largura(), icone->tile_altura(), 32,
                                              icone->passo() * 4,
                                              0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!s) return;
    SDL_SetWindowIcon(g_janela, s);
    SDL_FreeSurface(s);
}

void aplicar_tamanho() {
    if (!g_janela) return;

    SDL_SetWindowSize(g_janela, g_largura * g_ampliar, g_altura * g_ampliar);
    if (g_modo != ModoTela::tela_cheia)
        SDL_SetWindowPosition(g_janela, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void aplicar_modo(ModoTela modo) {
    g_modo = modo;
    if (modo != ModoTela::tela_cheia) g_modo_base = modo;
    if (!g_janela) return;

    if (modo == ModoTela::tela_cheia) {
        SDL_SetWindowFullscreen(g_janela, SDL_WINDOW_FULLSCREEN_DESKTOP);
        return;
    }

    SDL_SetWindowFullscreen(g_janela, 0);
    SDL_SetWindowResizable(g_janela, modo == ModoTela::letterbox ? SDL_TRUE : SDL_FALSE);

    if (modo == ModoTela::normal) aplicar_tamanho();
}

void destruir() {
    g_fb = frame_buffer();
    if (g_tela)   SDL_DestroyTexture(g_tela);
    if (g_render) SDL_DestroyRenderer(g_render);
    if (g_janela) SDL_DestroyWindow(g_janela);
    g_tela = nullptr;
    g_render = nullptr;
    g_janela = nullptr;
    g_largura = g_altura = 0;
    g_fps_atual = 0.0f;
    SDL_Quit();
}

int falhar(const char *mensagem) {
    saida_erro("%s: %s", mensagem, SDL_GetError());
    destruir();
    return -1;
}

void atualizar_mouse() {
    int mx, my, ww, wh;
    g_entrada.mouse_janela(mx, my);
    SDL_GetWindowSize(g_janela, &ww, &wh);

    const destino d = calcular_destino(ww, wh);
    if (d.escala <= 0.0f) return;

    g_entrada.definir_mouse(piso(((float)mx - d.ox) / d.escala),
                            piso(((float)my - d.oy) / d.escala));
}

void apresentar() {
    SDL_UpdateTexture(g_tela, nullptr, g_fb.dados(), (int)g_fb.pitch());

    SDL_SetRenderDrawColor(g_render, 0, 0, 0, 255);
    SDL_RenderClear(g_render);

    int ow, oh;
    SDL_GetRendererOutputSize(g_render, &ow, &oh);
    const destino d = calcular_destino(ow, oh);
    const SDL_Rect dst = { (int)(d.ox + 0.5f), (int)(d.oy + 0.5f),
                           (int)(g_largura * d.escala + 0.5f), (int)(g_altura * d.escala + 0.5f) };

    SDL_RenderCopy(g_render, g_tela, nullptr, &dst);
    SDL_RenderPresent(g_render);
}

double segundos_desde(uint64_t inicio, double freq) {
    return (double)(SDL_GetPerformanceCounter() - inicio) / freq;
}

}

int iniciar_janela(uint16_t largura, uint16_t altura, uint8_t fps, Cor cor,
                   int8_t ampliar, ModoTela modo, const char *nome, Atlas *icone) {
    if (largura == 0 || altura == 0) {
        saida_erro("iniciar_janela: tamanho invalido");
        return -1;
    }
    if (g_janela) destruir();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        saida_erro("SDL_Init falhou: %s", SDL_GetError());
        return -1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    Uint32 flags_janela = 0;
    if (modo != ModoTela::normal)     flags_janela |= SDL_WINDOW_RESIZABLE;
    if (modo == ModoTela::tela_cheia) flags_janela |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    g_modo = modo;
    g_modo_base = modo == ModoTela::tela_cheia ? ModoTela::letterbox : modo;

    g_ampliar = resolver_ampliar(ampliar, largura, altura);
    const int escala = g_ampliar;
    g_janela = SDL_CreateWindow(nome ? nome : "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                largura * escala, altura * escala, flags_janela);
    if (!g_janela) return falhar("SDL_CreateWindow falhou");

    const Uint32 vsync = fps == 0 ? SDL_RENDERER_PRESENTVSYNC : 0;
    g_render = SDL_CreateRenderer(g_janela, -1, SDL_RENDERER_ACCELERATED | vsync);
    if (!g_render) g_render = SDL_CreateRenderer(g_janela, -1, SDL_RENDERER_SOFTWARE | vsync);
    if (!g_render) return falhar("SDL_CreateRenderer falhou");

    g_tela = SDL_CreateTexture(g_render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                               largura, altura);
    if (!g_tela) return falhar("SDL_CreateTexture falhou");

    g_fb = frame_buffer(largura, altura);
    if (!g_fb.valido()) {
        saida_erro("iniciar_janela: sem memoria para o frame buffer (init_heap foi chamado?)");
        destruir();
        return -2;
    }

    g_largura = largura;
    g_altura = altura;
    g_cor = cor;
    g_sair = false;
    aplicar_icone(icone);
    g_entrada.iniciar();

    g_em_loop = true;
    iniciar();

    const double freq = (double)SDL_GetPerformanceFrequency();
    const double alvo = fps ? 1.0 / fps : 0.0;
    float delta = (float)(alvo > 0.0 ? alvo : 1.0 / 60.0);
    uint64_t anterior = SDL_GetPerformanceCounter();

    while (!g_sair) {
        const uint64_t inicio = SDL_GetPerformanceCounter();
        
        g_entrada.atualizar();
        if (g_entrada.pediu_fechar()) break;
        atualizar_mouse();

        rodar(delta);
        if (g_sair) break;

        g_fb.limpar(g_cor);
        desenhar(&g_fb);
        apresentar();

        if (alvo > 0.0) {
            double falta = alvo - segundos_desde(inicio, freq);
            if (falta > 0.002) SDL_Delay((Uint32)((falta - 0.001) * 1000.0));
            while (segundos_desde(inicio, freq) < alvo) {}
        }

        const uint64_t agora = SDL_GetPerformanceCounter();
        double dt = (double)(agora - anterior) / freq;
        anterior = agora;
        if (dt > 0.1) dt = 0.1;

        delta = (float)dt;
        if (dt > 0.0) {
            const float inst = (float)(1.0 / dt);
            g_fps_atual = g_fps_atual == 0.0f ? inst : g_fps_atual * 0.9f + inst * 0.1f;
        }
    }

    g_em_loop = false;
    destruir();
    return 0;
}

void fechar_janela() {
    if (g_em_loop) { g_sair = true; return; }
    destruir();
}

void mudar_modo() {
    switch (g_modo) {
        case ModoTela::normal:     aplicar_modo(ModoTela::letterbox);  break;
        case ModoTela::letterbox:  aplicar_modo(ModoTela::tela_cheia); break;
        case ModoTela::tela_cheia: aplicar_modo(ModoTela::normal);     break;
    }
}

void definir_modo(ModoTela modo) { aplicar_modo(modo); }
ModoTela modo_atual()            { return g_modo; }

void ampliar_janela(int8_t ampliar) {
    if (!g_janela) return;
    g_ampliar = resolver_ampliar(ampliar, g_largura, g_altura);
    aplicar_tamanho();
}

int8_t ampliar_atual() { return (int8_t)g_ampliar; }

Entrada &entrada() { return g_entrada; }

void titulo_janela(const char *titulo) {
    if (g_janela && titulo) SDL_SetWindowTitle(g_janela, titulo);
}

void tela_cheia(bool ativa) {
    aplicar_modo(ativa ? ModoTela::tela_cheia : g_modo_base);
}

uint16_t largura_janela() { return g_largura;   }
uint16_t altura_janela()  { return g_altura;    }
float    fps_atual()      { return g_fps_atual; }

/* ---- API C ---- */

namespace {

janela_cb_iniciar  g_cb_ini = nullptr;
janela_cb_rodar    g_cb_rod = nullptr;
janela_cb_desenhar g_cb_des = nullptr;
void*              g_cb_ctx = nullptr;

inline ModoTela modo_de(int m) {
    return m <= 0 ? ModoTela::normal : (m == 1 ? ModoTela::letterbox : ModoTela::tela_cheia);
}

}

#ifdef FB_BIBLIOTECA
/* Compilando como biblioteca para outras linguagens: o jogo C++ NÃO deve definir estas três */
void iniciar()                 { if (g_cb_ini) g_cb_ini(g_cb_ctx); }
void rodar(float delta)        { if (g_cb_rod) g_cb_rod(delta, g_cb_ctx); }
void desenhar(janela* /*j*/)   { if (g_cb_des) g_cb_des(&g_fb_h, g_cb_ctx); }
#endif

extern "C" {

void janela_callbacks(janela_cb_iniciar ini, janela_cb_rodar rod, janela_cb_desenhar des, void* ctx) {
    g_cb_ini = ini;
    g_cb_rod = rod;
    g_cb_des = des;
    g_cb_ctx = ctx;
}

int janela_iniciar(uint16_t largura, uint16_t altura, uint8_t fps, uint32_t cor,
                   int8_t ampliar, int modo, const char* nome, fb_atlas_handle* icone) {
    return iniciar_janela(largura, altura, fps, fb_cores::desempacotar(cor), ampliar,
                          modo_de(modo), nome, icone ? &icone->at : nullptr);
}

void janela_fechar(void)                    { fechar_janela(); }
void janela_mudar_modo(void)                { mudar_modo(); }
void janela_definir_modo(int modo)          { definir_modo(modo_de(modo)); }
int  janela_modo_atual(void)                { return (int)modo_atual(); }
void janela_ampliar(int8_t ampliar)         { ampliar_janela(ampliar); }
int  janela_ampliar_atual(void)             { return (int)ampliar_atual(); }
void janela_tela_cheia(int ativa)           { tela_cheia(ativa != 0); }
void janela_titulo(const char* titulo)      { titulo_janela(titulo); }

entrada_handle* janela_entrada(void)        { return &g_entrada_h; }
uint16_t janela_largura(void)               { return largura_janela(); }
uint16_t janela_altura(void)                { return altura_janela(); }
float    janela_fps_atual(void)             { return fps_atual(); }

}