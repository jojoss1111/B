#include "audio.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <crock.h>

#define DR_WAV_NO_STDIO
#include "dr_wav.h"
#define DR_MP3_NO_STDIO
#include "dr_mp3.h"
#define STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_NO_STDIO
#include "stb_vorbis.c"

#define TAXA          44100
#define MAX_AUDIOS    32
#define BLOCO         1024
#define NAT           2048
#define STREAM_LIMITE ((size_t)1 << 20)
#define MAX_FRAMES    ((uint64_t)TAXA * 600)

static SDL_AudioDeviceID g_dev = 0;
static audio_handle* g_lista[MAX_AUDIOS];
static int g_n = 0;

static int dispositivo_ok(void) {
    return g_dev != 0 && SDL_WasInit(SDL_INIT_AUDIO) != 0;
}

static int trava_entrar(void) {
    const int ativa = dispositivo_ok();
    if (ativa) SDL_LockAudioDevice(g_dev);
    return ativa;
}

static void trava_sair(int ativa) {
    if (ativa) SDL_UnlockAudioDevice(g_dev);
}

static void audio_callback(void* usuario, uint8_t* fluxo, int tamanho);

static int abrir_dispositivo(void) {
    if (dispositivo_ok()) return 1;
    g_dev = 0;

    if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return 0;

    SDL_AudioSpec want, have;
    SDL_zero(want);
    SDL_zero(have);
    want.freq = TAXA;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = audio_callback;

    g_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (g_dev == 0) return 0;
    SDL_PauseAudioDevice(g_dev, 0);
    return 1;
}

static int registrar(audio_handle* a) {
    if (!abrir_dispositivo()) return 0;
    const int t = trava_entrar();
    if (g_n >= MAX_AUDIOS) { trava_sair(t); return 0; }
    g_lista[g_n++] = a;
    trava_sair(t);
    return 1;
}

static void desregistrar(audio_handle* a) {
    int fechar = 0;
    const int t = trava_entrar();
    for (int i = 0; i < g_n; i++) {
        if (g_lista[i] == a) {
            g_lista[i] = g_lista[g_n - 1];
            g_n--;
            break;
        }
    }
    fechar = (g_n == 0);
    trava_sair(t);

    if (fechar && dispositivo_ok()) {
        SDL_CloseAudioDevice(g_dev);
        g_dev = 0;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        if (SDL_WasInit(0) == 0) SDL_Quit();
    }
}

typedef enum { AF_NENHUM, AF_WAV, AF_MP3, AF_OGG } afonte_tipo;

typedef struct {
    afonte_tipo tipo;
    drwav       wav;
    drmp3       mp3;
    stb_vorbis* ogg;
    unsigned    canais, taxa;
    uint64_t    total;
} afonte;

static void afonte_iniciar(afonte* f) {
    f->tipo = AF_NENHUM;
    f->ogg = NULL;
    f->canais = f->taxa = 0;
    f->total = 0;
}

static void afonte_fechar(afonte* f) {
    if (f->tipo == AF_WAV) drwav_uninit(&f->wav);
    else if (f->tipo == AF_MP3) drmp3_uninit(&f->mp3);
    else if (f->tipo == AF_OGG && f->ogg) stb_vorbis_close(f->ogg);
    f->ogg = NULL;
    f->tipo = AF_NENHUM;
    f->canais = f->taxa = 0;
    f->total = 0;
}

static int afonte_abrir(afonte* f, const uint8_t* d, size_t n) {
    afonte_fechar(f);
    if (n < 4) return 0;

    if (memcmp(d, "RIFF", 4) == 0) {
        if (!drwav_init_memory(&f->wav, d, n, NULL)) return 0;
        if (f->wav.channels == 0 || f->wav.sampleRate == 0) { drwav_uninit(&f->wav); return 0; }
        f->tipo = AF_WAV;
        f->canais = f->wav.channels;
        f->taxa = f->wav.sampleRate;
        f->total = f->wav.totalPCMFrameCount;
        return 1;
    }

    if (memcmp(d, "OggS", 4) == 0) {
        int erro = 0;
        f->ogg = stb_vorbis_open_memory(d, (int)n, &erro, NULL);
        if (!f->ogg) return 0;
        stb_vorbis_info info = stb_vorbis_get_info(f->ogg);
        if (info.channels <= 0 || info.sample_rate == 0) { stb_vorbis_close(f->ogg); f->ogg = NULL; return 0; }
        f->tipo = AF_OGG;
        f->canais = (unsigned)info.channels;
        f->taxa = info.sample_rate;
        f->total = stb_vorbis_stream_length_in_samples(f->ogg);
        return 1;
    }

    if (!drmp3_init_memory(&f->mp3, d, n, NULL)) return 0;
    if (f->mp3.channels == 0 || f->mp3.sampleRate == 0) { drmp3_uninit(&f->mp3); return 0; }
    f->tipo = AF_MP3;
    f->canais = f->mp3.channels;
    f->taxa = f->mp3.sampleRate;
    f->total = 0;
    return 1;
}

static void afonte_reiniciar(afonte* f) {
    if (f->tipo == AF_WAV) drwav_seek_to_pcm_frame(&f->wav, 0);
    else if (f->tipo == AF_MP3) drmp3_seek_to_pcm_frame(&f->mp3, 0);
    else if (f->tipo == AF_OGG) stb_vorbis_seek_start(f->ogg);
}

static uint64_t afonte_ler(afonte* f, int16_t* out, uint64_t frames) {
    uint64_t feito = 0;
    while (feito < frames) {
        uint64_t quer = frames - feito, lido = 0;

        if (f->tipo == AF_OGG) {
            int r = stb_vorbis_get_samples_short_interleaved(f->ogg, 2, (short*)(out + feito * 2), (int)(quer * 2));
            lido = r > 0 ? (uint64_t)r : 0;
        } else if (f->tipo == AF_WAV || f->tipo == AF_MP3) {
            int16_t tmp[1024];
            uint64_t max = 1024 / f->canais;
            if (quer > max) quer = max;
            lido = f->tipo == AF_WAV ? drwav_read_pcm_frames_s16(&f->wav, quer, tmp)
                                     : drmp3_read_pcm_frames_s16(&f->mp3, quer, tmp);
            for (uint64_t i = 0; i < lido; i++) {
                int16_t l = tmp[i * f->canais];
                int16_t r = f->canais > 1 ? tmp[i * f->canais + 1] : l;
                out[(feito + i) * 2]     = l;
                out[(feito + i) * 2 + 1] = r;
            }
        }

        if (lido == 0) break;
        feito += lido;
    }
    return feito;
}

static int16_t cubica(float p0, float p1, float p2, float p3, float t) {
    float v = p1 + 0.5f * t * (p2 - p0 + t * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3
                                                + t * (3.0f * (p1 - p2) + p3 - p0)));
    if (v > 32767.0f) v = 32767.0f;
    if (v < -32768.0f) v = -32768.0f;
    return (int16_t)(v >= 0 ? v + 0.5f : v - 0.5f);
}

typedef struct {
    afonte* f;
    double  passo;
    double  pos;
    int     n;
    uint8_t fim;
    int16_t buf[NAT * 2];
} resamp;

static void resamp_zerar(resamp* rs) {
    rs->pos = 0.0;
    rs->n = 0;
    rs->fim = 0;
}

static void resamp_iniciar(resamp* rs, afonte* fonte) {
    rs->f = fonte;
    rs->passo = (double)fonte->taxa / (double)TAXA;
    resamp_zerar(rs);
}

static void resamp_encher(resamp* rs) {
    int i = (int)rs->pos;
    int ini = i - 1;
    if (ini < 0) ini = 0;
    if (ini > rs->n) ini = rs->n;

    int resta = rs->n - ini;
    if (ini > 0 && resta > 0) memmove(rs->buf, rs->buf + ini * 2, (size_t)resta * 2 * sizeof(int16_t));
    rs->n = resta;
    rs->pos -= ini;

    uint64_t quer = (uint64_t)(NAT - rs->n);
    uint64_t lidos = afonte_ler(rs->f, rs->buf + rs->n * 2, quer);
    if (lidos < quer) rs->fim = 1;
    rs->n += (int)lidos;
}

static const int16_t* resamp_at(const resamp* rs, int k) {
    if (k < 0) k = 0;
    if (k >= rs->n) k = rs->n - 1;
    return rs->buf + k * 2;
}

static int resamp_proximo(resamp* rs, int16_t* l, int16_t* r) {
    int i = (int)rs->pos;
    if (!rs->fim && i + 2 >= rs->n) { resamp_encher(rs); i = (int)rs->pos; }
    if (i >= rs->n) return 0;

    float t = (float)(rs->pos - (double)i);
    const int16_t* p1 = resamp_at(rs, i);
    if (t == 0.0f) {
        *l = p1[0];
        *r = p1[1];
    } else {
        const int16_t* p0 = resamp_at(rs, i - 1);
        const int16_t* p2 = resamp_at(rs, i + 1);
        const int16_t* p3 = resamp_at(rs, i + 2);
        *l = cubica(p0[0], p1[0], p2[0], p3[0], t);
        *r = cubica(p0[1], p1[1], p2[1], p3[1], t);
    }
    rs->pos += rs->passo;
    return 1;
}

static int decodificar_tudo(afonte* f, int16_t** out, uint64_t* frames) {
    resamp rs;
    resamp_iniciar(&rs, f);

    uint64_t cap = TAXA, n = 0;
    int16_t* buf = (int16_t*)memoria_malloc((size_t)(cap * 2 * sizeof(int16_t)));
    if (!buf) return 0;

    int16_t l, r;
    while (resamp_proximo(&rs, &l, &r)) {
        if (n >= cap) {
            uint64_t nc = cap * 2;
            if (nc > MAX_FRAMES * 2) { memoria_free(buf); return 0; }
            int16_t* nb = (int16_t*)memoria_realloc(buf, (size_t)(nc * 2 * sizeof(int16_t)));
            if (!nb) { memoria_free(buf); return 0; }
            buf = nb;
            cap = nc;
        }
        buf[n * 2]     = l;
        buf[n * 2 + 1] = r;
        n++;
    }

    if (n == 0) { memoria_free(buf); return 0; }
    *out = buf;
    *frames = n;
    return 1;
}

struct audio_stream {
    uint8_t* dados;
    afonte   fonte;
    resamp   rs;
    uint64_t total;
    uint8_t  reiniciar;
    uint8_t  terminou;
};

static void voz_zerar(audio_voz* v) {
    v->pos = 0;
    v->vol = 256;
    v->ativa = 0;
}

void audio_liberar(audio_handle* a) {
    if (!a) return;
    if (!a->amostras && !a->stream) return;
    desregistrar(a);

    if (a->amostras) {
        memoria_free(a->amostras);
        a->amostras = NULL;
    }
    if (a->stream) {
        afonte_fechar(&a->stream->fonte);
        memoria_free(a->stream->dados);
        memoria_free(a->stream);
        a->stream = NULL;
    }
    a->frames = 0;
    for (int i = 0; i < AUDIO_MAX_VOZES; i++) voz_zerar(&a->voz[i]);
}

static int audio_carregar(audio_handle* a, const char* caminho, int loop, float vol, int modo) {
    audio_liberar(a);

    size_t tam = 0;
    uint8_t* dados = (uint8_t*)arquivo_ler_tudo_tam(caminho, &tam);
    if (!dados) return 0;

    if (modo != AUDIO_MODO_MEMORIA && modo != AUDIO_MODO_STREAM)
        modo = tam > STREAM_LIMITE ? AUDIO_MODO_STREAM : AUDIO_MODO_MEMORIA;

    if (modo == AUDIO_MODO_STREAM) {
        audio_stream* s = (audio_stream*)memoria_malloc(sizeof(audio_stream));
        if (!s) { memoria_free(dados); return 0; }
        s->dados = dados;
        afonte_iniciar(&s->fonte);
        s->reiniciar = 0;
        s->terminou = 0;
        s->total = 0;
        if (!afonte_abrir(&s->fonte, dados, tam)) {
            memoria_free(dados);
            memoria_free(s);
            return 0;
        }
        resamp_iniciar(&s->rs, &s->fonte);
        s->total = s->fonte.total ? s->fonte.total * TAXA / s->fonte.taxa : 0;
        a->stream = s;
    } else {
        afonte f;
        afonte_iniciar(&f);
        int ok = afonte_abrir(&f, dados, tam) && decodificar_tudo(&f, &a->amostras, &a->frames);
        afonte_fechar(&f);
        memoria_free(dados);
        if (!ok) { a->amostras = NULL; a->frames = 0; return 0; }
    }

    for (int i = 0; i < AUDIO_MAX_VOZES; i++) voz_zerar(&a->voz[i]);
    a->repetir = loop != 0;
    audio_volume_definir(a, vol);

    if (!registrar(a)) {
        audio_liberar(a);
        return 0;
    }
    return 1;
}

audio_handle* audio_criar(const char* caminho, int loop, float vol, int modo) {
    audio_handle* a = (audio_handle*)memoria_malloc(sizeof(audio_handle));
    if (!a) return NULL;

    a->amostras = NULL;
    a->frames = 0;
    a->stream = NULL;
    for (int i = 0; i < AUDIO_MAX_VOZES; i++) voz_zerar(&a->voz[i]);
    a->vol = 256;
    a->repetir = 0;

    if (!audio_carregar(a, caminho, loop, vol, modo)) { memoria_free(a); return NULL; }
    return a;
}

void audio_destruir(audio_handle* a) {
    if (!a) return;
    audio_liberar(a);
    memoria_free(a);
}

int audio_valido(const audio_handle* a)     { return a && (a->amostras != NULL || a->stream != NULL); }
int audio_streaming(const audio_handle* a)  { return a && a->stream != NULL; }
int audio_repetindo(const audio_handle* a)  { return a && a->repetir; }
float audio_volume(const audio_handle* a)   { return a ? a->vol / 256.0f : 0.0f; }

void audio_tocar(audio_handle* a) {
    if (!audio_valido(a)) return;
    const int t = trava_entrar();
    audio_voz* v = &a->voz[0];
    if (a->stream) {
        if (a->stream->terminou) {
            a->stream->reiniciar = 1;
            a->stream->terminou = 0;
            v->pos = 0;
        }
    } else if (v->pos >= a->frames) {
        v->pos = 0;
    }
    v->ativa = 1;
    trava_sair(t);
}

void audio_parar(audio_handle* a) {
    if (!a) return;
    const int t = trava_entrar();
    a->voz[0].ativa = 0;
    trava_sair(t);
}

void audio_voltar(audio_handle* a) {
    if (!a) return;
    const int t = trava_entrar();
    a->voz[0].pos = 0;
    if (a->stream) {
        a->stream->reiniciar = 1;
        a->stream->terminou = 0;
    }
    trava_sair(t);
}

void audio_disparar(audio_handle* a, float vol) {
    if (!audio_valido(a)) return;
    if (a->stream) { audio_voltar(a); audio_tocar(a); return; }

    if (vol < 0.0f) vol = 0.0f;
    if (vol > 2.0f) vol = 2.0f;

    const int t = trava_entrar();
    int escolhida = 1;
    uint64_t maior = 0;
    for (int i = 1; i < AUDIO_MAX_VOZES; i++) {
        if (!a->voz[i].ativa) { escolhida = i; break; }
        if (a->voz[i].pos >= maior) { maior = a->voz[i].pos; escolhida = i; }
    }

    audio_voz* v = &a->voz[escolhida];
    v->pos = 0;
    v->vol = (int)(vol * 256.0f);
    v->ativa = 1;
    trava_sair(t);
}

void audio_parar_tudo(audio_handle* a) {
    if (!a) return;
    const int t = trava_entrar();
    for (int i = 0; i < AUDIO_MAX_VOZES; i++) a->voz[i].ativa = 0;
    trava_sair(t);
}

void audio_repetir_definir(audio_handle* a, int repetir) {
    if (!a) return;
    const int t = trava_entrar();
    a->repetir = repetir != 0;
    trava_sair(t);
}

void audio_volume_definir(audio_handle* a, float v) {
    if (!a) return;
    if (v < 0.0f) v = 0.0f;
    if (v > 2.0f) v = 2.0f;
    const int t = trava_entrar();
    a->vol = (int)(v * 256.0f);
    trava_sair(t);
}

int audio_tocando(const audio_handle* a) {
    if (!a) return 0;
    for (int i = 0; i < AUDIO_MAX_VOZES; i++)
        if (a->voz[i].ativa) return 1;
    return 0;
}

float audio_duracao(const audio_handle* a) {
    if (!a) return 0.0f;
    if (a->stream) return (float)a->stream->total / (float)TAXA;
    return (float)a->frames / (float)TAXA;
}

float audio_posicao(const audio_handle* a) { return a ? (float)a->voz[0].pos / (float)TAXA : 0.0f; }

static void audio_misturar(audio_handle* a, int32_t* acc, int frames) {
    if (a->stream) {
        audio_stream* s = a->stream;
        if (s->reiniciar) {
            afonte_reiniciar(&s->fonte);
            resamp_zerar(&s->rs);
            s->reiniciar = 0;
        }

        audio_voz* v = &a->voz[0];
        if (!v->ativa) return;

        for (int i = 0; i < frames; i++) {
            int16_t l, r;
            if (!resamp_proximo(&s->rs, &l, &r)) {
                if (!a->repetir) { v->ativa = 0; s->terminou = 1; break; }
                afonte_reiniciar(&s->fonte);
                resamp_zerar(&s->rs);
                if (!resamp_proximo(&s->rs, &l, &r)) { v->ativa = 0; s->terminou = 1; break; }
                v->pos = 0;
            }
            acc[i * 2]     += (l * a->vol) >> 8;
            acc[i * 2 + 1] += (r * a->vol) >> 8;
            v->pos++;
        }
        return;
    }

    if (!a->amostras) return;

    for (int k = 0; k < AUDIO_MAX_VOZES; k++) {
        audio_voz* v = &a->voz[k];
        if (!v->ativa) continue;
        const int ganho = (a->vol * v->vol) >> 8;

        for (int i = 0; i < frames; i++) {
            if (v->pos >= a->frames) {
                if (k == 0 && a->repetir) v->pos = 0;
                else { v->ativa = 0; break; }
            }
            const int16_t* s = a->amostras + v->pos * 2;
            acc[i * 2]     += (s[0] * ganho) >> 8;
            acc[i * 2 + 1] += (s[1] * ganho) >> 8;
            v->pos++;
        }
    }
}

static void audio_callback(void* usuario, uint8_t* fluxo, int tamanho) {
    (void)usuario;
    static int32_t acc[BLOCO * 2];

    int16_t* out = (int16_t*)fluxo;
    const int total = tamanho / (int)(2 * sizeof(int16_t));
    int feito = 0;

    while (feito < total) {
        int n = total - feito;
        if (n > BLOCO) n = BLOCO;

        memset(acc, 0, (size_t)n * 2 * sizeof(int32_t));
        for (int k = 0; k < g_n; k++) audio_misturar(g_lista[k], acc, n);

        for (int i = 0; i < n * 2; i++) {
            int32_t v = acc[i];
            if (v > 32767) v = 32767;
            else if (v < -32768) v = -32768;
            out[feito * 2 + i] = (int16_t)v;
        }
        feito += n;
    }
}