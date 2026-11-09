#include "render.h"

#include <SDL2/SDL.h>

static SDL_Window    *g_janela = NULL;
static SDL_Renderer  *g_render = NULL;
static SDL_Texture   *g_tela   = NULL;   /* textura do framebuffer */

static fb_handle      *g_fb      = NULL;
static entrada_handle *g_entrada = NULL;
static Cor             g_cor     = 0xFF000000u; /* ARGB: preto opaco */

static ModoTela g_modo      = MODO_LETTERBOX;
static ModoTela g_modo_base = MODO_LETTERBOX;
static int      g_ampliar   = 1;

static uint16_t g_largura = 0, g_altura = 0;
static bool     g_em_loop = false;
static bool     g_sair = false;
static float    g_fps_atual = 0.0f;

typedef struct destino { float escala, ox, oy; } destino;

/* A entrada precisa existir antes de iniciar_janela (o Lua pode pedir janela.entrada() antes). */
static entrada_handle *obter_entrada(void) {
    if (!g_entrada) g_entrada = entrada_criar();
    return g_entrada;
}

static destino calcular_destino(int ww, int wh) {
    const float ex = (float)ww / (float)g_largura;
    const float ey = (float)wh / (float)g_altura;
    const float e = ex < ey ? ex : ey;
    destino d = { e, ((float)ww - g_largura * e) * 0.5f, ((float)wh - g_altura * e) * 0.5f };
    return d;
}

static int piso(float v) {
    const int i = (int)v;
    return (v < 0.0f && (float)i != v) ? i - 1 : i;
}

static int escala_inicial(int w, int h) {
    SDL_Rect r;
    int ex, ey, e;
    if (SDL_GetDisplayUsableBounds(0, &r) != 0 || r.w <= 0 || r.h <= 0) return 1;
    ex = (r.w * 8 / 10) / w;
    ey = (r.h * 8 / 10) / h;
    e = ex < ey ? ex : ey;
    return e < 1 ? 1 : e;
}

static int resolver_ampliar(int ampliar, int w, int h) {
    return ampliar > 0 ? ampliar : escala_inicial(w, h);
}

static void aplicar_icone(Atlas *icone) {
    SDL_Surface *s;
    if (!icone || !fb_atlas_valido(icone) || !g_janela) return;

    s = SDL_CreateRGBSurfaceFrom((void *)fb_atlas_tile_dados(icone, 0, 0),
                                 fb_atlas_tile_largura(icone), fb_atlas_tile_altura(icone), 32,
                                 fb_atlas_passo(icone) * 4,
                                 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!s) return;
    SDL_SetWindowIcon(g_janela, s);
    SDL_FreeSurface(s);
}

static void aplicar_tamanho(void) {
    if (!g_janela) return;

    SDL_SetWindowSize(g_janela, g_largura * g_ampliar, g_altura * g_ampliar);
    if (g_modo != MODO_TELA_CHEIA)
        SDL_SetWindowPosition(g_janela, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

static void aplicar_modo(ModoTela modo) {
    g_modo = modo;
    if (modo != MODO_TELA_CHEIA) g_modo_base = modo;
    if (!g_janela) return;

    if (modo == MODO_TELA_CHEIA) {
        SDL_SetWindowFullscreen(g_janela, SDL_WINDOW_FULLSCREEN_DESKTOP);
        return;
    }

    SDL_SetWindowFullscreen(g_janela, 0);
    SDL_SetWindowResizable(g_janela, modo == MODO_LETTERBOX ? SDL_TRUE : SDL_FALSE);

    if (modo == MODO_NORMAL) aplicar_tamanho();
}

/* ---- Renderer SDL: criação/destruição dos recursos ---- */

static int criar_renderer(uint16_t largura, uint16_t altura, uint8_t fps) {
    Uint32 flags = SDL_RENDERER_ACCELERATED;
    if (fps == 0) flags |= SDL_RENDERER_PRESENTVSYNC;

    g_render = SDL_CreateRenderer(g_janela, -1, flags);
    if (!g_render) g_render = SDL_CreateRenderer(g_janela, -1, SDL_RENDERER_SOFTWARE);
    if (!g_render) return 0;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");   /* nearest */
    g_tela = SDL_CreateTexture(g_render, SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING, largura, altura);
    if (!g_tela) return 0;

    SDL_SetTextureBlendMode(g_tela, SDL_BLENDMODE_NONE);   /* ignora alpha */
    return 1;
}

static void destruir_janela(void) {
    if (g_fb) {
        fb_destruir(g_fb);
        g_fb = NULL;
    }
    if (g_tela)   SDL_DestroyTexture(g_tela);
    if (g_render) SDL_DestroyRenderer(g_render);
    if (g_janela) SDL_DestroyWindow(g_janela);
    g_tela = NULL;
    g_render = NULL;
    g_janela = NULL;
    g_largura = g_altura = 0;
    g_fps_atual = 0.0f;
    SDL_Quit();
}

static int falhar(const char *mensagem) {
    saida_erro("%s: %s", mensagem, SDL_GetError());
    destruir_janela();
    return -1;
}

static void atualizar_mouse(void) {
    int mx, my, ww, wh;
    destino d;
    entrada_mouse_janela(g_entrada, &mx, &my);
    SDL_GetWindowSize(g_janela, &ww, &wh);

    d = calcular_destino(ww, wh);
    if (d.escala <= 0.0f) return;

    entrada_definir_mouse(g_entrada,
                          piso(((float)mx - d.ox) / d.escala),
                          piso(((float)my - d.oy) / d.escala));
}

static void apresentar(void) {
    int ow, oh;
    SDL_Rect dst;
    destino d;

    SDL_GetRendererOutputSize(g_render, &ow, &oh);
    if (ow <= 0 || oh <= 0) return;   /* janela minimizada */

    d = calcular_destino(ow, oh);
    dst.x = (int)(d.ox + 0.5f);
    dst.y = (int)(d.oy + 0.5f);
    dst.w = (int)(g_largura * d.escala + 0.5f);
    dst.h = (int)(g_altura * d.escala + 0.5f);

    SDL_UpdateTexture(g_tela, NULL, fb_dados(g_fb), (int)g_largura * 4);

    /* barras pretas do letterbox */
    SDL_SetRenderDrawColor(g_render, 0, 0, 0, 255);
    SDL_RenderClear(g_render);
    SDL_RenderCopy(g_render, g_tela, NULL, &dst);
    SDL_RenderPresent(g_render);
}

static double segundos_desde(uint64_t inicio, double freq) {
    return (double)(SDL_GetPerformanceCounter() - inicio) / freq;
}

/* ---- Callbacks (API C) ---- */

static janela_cb_iniciar  g_cb_ini = NULL;
static janela_cb_rodar    g_cb_rod = NULL;
static janela_cb_desenhar g_cb_des = NULL;
static void*              g_cb_ctx = NULL;

static ModoTela modo_de(int m) {
    return m <= 0 ? MODO_NORMAL : (m == 1 ? MODO_LETTERBOX : MODO_TELA_CHEIA);
}

#ifdef FB_BIBLIOTECA
/* Compilando como biblioteca para outras linguagens: o jogo NAO deve definir estas tres */
void iniciar(void)               { if (g_cb_ini) g_cb_ini(g_cb_ctx); }
void rodar(float delta)          { if (g_cb_rod) g_cb_rod(delta, g_cb_ctx); }
void desenhar(janela* j)         { (void)j; if (g_cb_des) g_cb_des(g_fb, g_cb_ctx); }
#endif

/* ---- Janela ---- */

int iniciar_janela(uint16_t largura, uint16_t altura, uint8_t fps, Cor cor,
                   int8_t ampliar, ModoTela modo, const char *nome, Atlas *icone) {
    Uint32 flags_janela = 0;
    int escala;
    double freq, alvo;
    float delta;
    uint64_t anterior;

    if (largura == 0 || altura == 0) {
        saida_erro("iniciar_janela: tamanho invalido");
        return -1;
    }
    if (g_janela) destruir_janela();

    if (!obter_entrada()) {
        saida_erro("iniciar_janela: sem memoria para a entrada (init_heap foi chamado?)");
        return -2;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        saida_erro("SDL_Init falhou: %s", SDL_GetError());
        return -1;
    }

    if (modo != MODO_NORMAL)     flags_janela |= SDL_WINDOW_RESIZABLE;
    if (modo == MODO_TELA_CHEIA) flags_janela |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    g_modo = modo;
    g_modo_base = modo == MODO_TELA_CHEIA ? MODO_LETTERBOX : modo;

    g_ampliar = resolver_ampliar(ampliar, largura, altura);
    escala = g_ampliar;
    g_janela = SDL_CreateWindow(nome ? nome : "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                largura * escala, altura * escala, flags_janela);
    if (!g_janela) return falhar("SDL_CreateWindow falhou");

    g_largura = largura;   /* apresentar depende destes */
    g_altura = altura;
    if (!criar_renderer(largura, altura, fps)) return falhar("SDL_CreateRenderer/Texture falhou");

    g_fb = fb_criar(largura, altura);
    if (!g_fb || !fb_valido(g_fb)) {
        saida_erro("iniciar_janela: sem memoria para o frame buffer (init_heap foi chamado?)");
        destruir_janela();
        return -2;
    }

    g_cor = cor;
    g_sair = false;
    aplicar_icone(icone);
    entrada_iniciar(g_entrada);

    g_em_loop = true;
    iniciar();

    freq = (double)SDL_GetPerformanceFrequency();
    alvo = fps ? 1.0 / fps : 0.0;
    delta = (float)(alvo > 0.0 ? alvo : 1.0 / 60.0);
    anterior = SDL_GetPerformanceCounter();

    while (!g_sair) {
        const uint64_t inicio = SDL_GetPerformanceCounter();
        uint64_t agora;
        double dt;

        entrada_atualizar(g_entrada);
        if (entrada_pediu_fechar(g_entrada)) break;
        atualizar_mouse();
        tempo_atualizar(delta);
        rodar(delta);
        if (g_sair) break;

        fb_limpar(g_fb, g_cor);
        desenhar(g_fb);
        apresentar();

        if (alvo > 0.0) {
            double falta = alvo - segundos_desde(inicio, freq);
            if (falta > 0.002) SDL_Delay((Uint32)((falta - 0.001) * 1000.0));
            while (segundos_desde(inicio, freq) < alvo) {}
        }

        agora = SDL_GetPerformanceCounter();
        dt = (double)(agora - anterior) / freq;
        anterior = agora;
        if (dt > 0.1) dt = 0.1;

        delta = (float)dt;
        if (dt > 0.0) {
            const float inst = (float)(1.0 / dt);
            g_fps_atual = g_fps_atual == 0.0f ? inst : g_fps_atual * 0.9f + inst * 0.1f;
        }
    }

    g_em_loop = false;
    destruir_janela();
    return 0;
}

void fechar_janela(void) {
    if (g_em_loop) { g_sair = true; return; }
    destruir_janela();
}

void mudar_modo(void) {
    switch (g_modo) {
        case MODO_NORMAL:     aplicar_modo(MODO_LETTERBOX);  break;
        case MODO_LETTERBOX:  aplicar_modo(MODO_TELA_CHEIA); break;
        case MODO_TELA_CHEIA: aplicar_modo(MODO_NORMAL);     break;
        default:              aplicar_modo(MODO_NORMAL);     break;
    }
}

void definir_modo(ModoTela modo) { aplicar_modo(modo); }
ModoTela modo_atual(void)        { return g_modo; }

void ampliar_janela(int8_t ampliar) {
    if (!g_janela) return;
    g_ampliar = resolver_ampliar(ampliar, g_largura, g_altura);
    aplicar_tamanho();
}

int8_t ampliar_atual(void) { return (int8_t)g_ampliar; }

entrada_handle *entrada(void) { return obter_entrada(); }

void titulo_janela(const char *titulo) {
    if (g_janela && titulo) SDL_SetWindowTitle(g_janela, titulo);
}

void tela_cheia(int ativa) {
    aplicar_modo(ativa ? MODO_TELA_CHEIA : g_modo_base);
}

uint16_t largura_janela(void) { return g_largura;   }
uint16_t altura_janela(void)  { return g_altura;    }
float    fps_atual(void)      { return g_fps_atual; }

/* ---- API C exportada ---- */

void janela_callbacks(janela_cb_iniciar ini, janela_cb_rodar rod, janela_cb_desenhar des, void* ctx) {
    g_cb_ini = ini;
    g_cb_rod = rod;
    g_cb_des = des;
    g_cb_ctx = ctx;
}

int janela_iniciar(uint16_t largura, uint16_t altura, uint8_t fps, uint32_t cor,
                   int8_t ampliar, int modo, const char* nome, fb_atlas_handle* icone) {
    return iniciar_janela(largura, altura, fps, cor, ampliar, modo_de(modo), nome, icone);
}

void janela_fechar(void)                    { fechar_janela(); }
void janela_mudar_modo(void)                { mudar_modo(); }
void janela_definir_modo(int modo)          { definir_modo(modo_de(modo)); }
int  janela_modo_atual(void)                { return (int)modo_atual(); }
void janela_ampliar(int8_t ampliar)         { ampliar_janela(ampliar); }
int  janela_ampliar_atual(void)             { return (int)ampliar_atual(); }
void janela_tela_cheia(int ativa)           { tela_cheia(ativa != 0); }
void janela_titulo(const char* titulo)      { titulo_janela(titulo); }

entrada_handle* janela_entrada(void)        { return obter_entrada(); }
uint16_t janela_largura(void)               { return largura_janela(); }
uint16_t janela_altura(void)                { return altura_janela(); }
float    janela_fps_atual(void)             { return fps_atual(); }