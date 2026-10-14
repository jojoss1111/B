#include "audio.hpp"
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

namespace {

constexpr int      TAXA = 44100;
constexpr int      MAX_AUDIOS = 32;
constexpr int      BLOCO = 1024;
constexpr int      NAT = 2048;
constexpr size_t   STREAM_LIMITE = 1u << 20;
constexpr uint64_t MAX_FRAMES = (uint64_t)TAXA * 600;

SDL_AudioDeviceID g_dev = 0;
Audio* g_lista[MAX_AUDIOS];
int g_n = 0;

bool dispositivo_ok() {
    return g_dev != 0 && SDL_WasInit(SDL_INIT_AUDIO) != 0;
}

struct trava {
    bool ativa;
    trava() : ativa(dispositivo_ok()) { if (ativa) SDL_LockAudioDevice(g_dev); }
    ~trava() { if (ativa) SDL_UnlockAudioDevice(g_dev); }
};

bool abrir_dispositivo() {
    if (dispositivo_ok()) return true;
    g_dev = 0;

    if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return false;

    SDL_AudioSpec want, have;
    SDL_zero(want);
    SDL_zero(have);
    want.freq = TAXA;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = Audio::callback;

    g_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (g_dev == 0) return false;
    SDL_PauseAudioDevice(g_dev, 0);
    return true;
}

bool registrar(Audio* a) {
    if (!abrir_dispositivo()) return false;
    trava t;
    if (g_n >= MAX_AUDIOS) return false;
    g_lista[g_n++] = a;
    return true;
}

void desregistrar(Audio* a) {
    bool fechar = false;
    {
        trava t;
        for (int i = 0; i < g_n; i++) {
            if (g_lista[i] == a) {
                g_lista[i] = g_lista[g_n - 1];
                g_n--;
                break;
            }
        }
        fechar = (g_n == 0);
    }
    if (fechar && dispositivo_ok()) {
        SDL_CloseAudioDevice(g_dev);
        g_dev = 0;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        if (SDL_WasInit(0) == 0) SDL_Quit();
    }
}

enum class Tipo { nenhum, wav, mp3, ogg };

struct Fonte {
    Tipo        tipo = Tipo::nenhum;
    drwav       wav;
    drmp3       mp3;
    stb_vorbis* ogg = nullptr;
    unsigned    canais = 0, taxa = 0;
    uint64_t    total = 0;

    bool abrir(const uint8_t* d, size_t n) {
        fechar();
        if (n < 4) return false;

        if (memcmp(d, "RIFF", 4) == 0) {
            if (!drwav_init_memory(&wav, d, n, nullptr)) return false;
            if (wav.channels == 0 || wav.sampleRate == 0) { drwav_uninit(&wav); return false; }
            tipo = Tipo::wav;
            canais = wav.channels;
            taxa = wav.sampleRate;
            total = wav.totalPCMFrameCount;
            return true;
        }

        if (memcmp(d, "OggS", 4) == 0) {
            int erro = 0;
            ogg = stb_vorbis_open_memory(d, (int)n, &erro, nullptr);
            if (!ogg) return false;
            stb_vorbis_info info = stb_vorbis_get_info(ogg);
            if (info.channels <= 0 || info.sample_rate == 0) { stb_vorbis_close(ogg); ogg = nullptr; return false; }
            tipo = Tipo::ogg;
            canais = (unsigned)info.channels;
            taxa = info.sample_rate;
            total = stb_vorbis_stream_length_in_samples(ogg);
            return true;
        }

        if (!drmp3_init_memory(&mp3, d, n, nullptr)) return false;
        if (mp3.channels == 0 || mp3.sampleRate == 0) { drmp3_uninit(&mp3); return false; }
        tipo = Tipo::mp3;
        canais = mp3.channels;
        taxa = mp3.sampleRate;
        total = 0;
        return true;
    }

    void fechar() {
        if (tipo == Tipo::wav) drwav_uninit(&wav);
        else if (tipo == Tipo::mp3) drmp3_uninit(&mp3);
        else if (tipo == Tipo::ogg && ogg) stb_vorbis_close(ogg);
        ogg = nullptr;
        tipo = Tipo::nenhum;
        canais = taxa = 0;
        total = 0;
    }

    void reiniciar() {
        if (tipo == Tipo::wav) drwav_seek_to_pcm_frame(&wav, 0);
        else if (tipo == Tipo::mp3) drmp3_seek_to_pcm_frame(&mp3, 0);
        else if (tipo == Tipo::ogg) stb_vorbis_seek_start(ogg);
    }

    uint64_t ler(int16_t* out, uint64_t frames) {
        uint64_t feito = 0;
        while (feito < frames) {
            uint64_t quer = frames - feito, lido = 0;

            if (tipo == Tipo::ogg) {
                int r = stb_vorbis_get_samples_short_interleaved(ogg, 2, (short*)(out + feito * 2), (int)(quer * 2));
                lido = r > 0 ? (uint64_t)r : 0;
            } else if (tipo == Tipo::wav || tipo == Tipo::mp3) {
                int16_t tmp[1024];
                uint64_t max = 1024 / canais;
                if (quer > max) quer = max;
                lido = tipo == Tipo::wav ? drwav_read_pcm_frames_s16(&wav, quer, tmp)
                                         : drmp3_read_pcm_frames_s16(&mp3, quer, tmp);
                for (uint64_t i = 0; i < lido; i++) {
                    int16_t l = tmp[i * canais];
                    int16_t r = canais > 1 ? tmp[i * canais + 1] : l;
                    out[(feito + i) * 2]     = l;
                    out[(feito + i) * 2 + 1] = r;
                }
            }

            if (lido == 0) break;
            feito += lido;
        }
        return feito;
    }
};

int16_t cubica(float p0, float p1, float p2, float p3, float t) {
    float v = p1 + 0.5f * t * (p2 - p0 + t * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3
                                                + t * (3.0f * (p1 - p2) + p3 - p0)));
    if (v > 32767.0f) v = 32767.0f;
    if (v < -32768.0f) v = -32768.0f;
    return (int16_t)(v >= 0 ? v + 0.5f : v - 0.5f);
}

struct Resamp {
    Fonte*  f = nullptr;
    double  passo = 1.0;
    double  pos = 0.0;
    int     n = 0;
    bool    fim = false;
    int16_t buf[NAT * 2];

    void iniciar(Fonte* fonte) {
        f = fonte;
        passo = (double)f->taxa / (double)TAXA;
        zerar();
    }

    void zerar() { pos = 0.0; n = 0; fim = false; }

    void encher() {
        int i = (int)pos;
        int ini = i - 1;
        if (ini < 0) ini = 0;
        if (ini > n) ini = n;

        int resta = n - ini;
        if (ini > 0 && resta > 0) memmove(buf, buf + ini * 2, (size_t)resta * 2 * sizeof(int16_t));
        n = resta;
        pos -= ini;

        uint64_t quer = (uint64_t)(NAT - n);
        uint64_t lidos = f->ler(buf + n * 2, quer);
        if (lidos < quer) fim = true;
        n += (int)lidos;
    }

    bool proximo(int16_t& l, int16_t& r) {
        int i = (int)pos;
        if (!fim && i + 2 >= n) { encher(); i = (int)pos; }
        if (i >= n) return false;

        auto at = [&](int k) {
            if (k < 0) k = 0;
            if (k >= n) k = n - 1;
            return buf + k * 2;
        };

        float t = (float)(pos - (double)i);
        const int16_t* p1 = at(i);
        if (t == 0.0f) {
            l = p1[0];
            r = p1[1];
        } else {
            const int16_t* p0 = at(i - 1);
            const int16_t* p2 = at(i + 1);
            const int16_t* p3 = at(i + 2);
            l = cubica(p0[0], p1[0], p2[0], p3[0], t);
            r = cubica(p0[1], p1[1], p2[1], p3[1], t);
        }
        pos += passo;
        return true;
    }
};

bool decodificar_tudo(Fonte& f, int16_t*& out, uint64_t& frames) {
    Resamp rs;
    rs.iniciar(&f);

    uint64_t cap = TAXA, n = 0;
    int16_t* buf = (int16_t*)memoria_malloc((size_t)(cap * 2 * sizeof(int16_t)));
    if (!buf) return false;

    int16_t l, r;
    while (rs.proximo(l, r)) {
        if (n >= cap) {
            uint64_t nc = cap * 2;
            if (nc > MAX_FRAMES * 2) { memoria_free(buf); return false; }
            int16_t* nb = (int16_t*)memoria_realloc(buf, (size_t)(nc * 2 * sizeof(int16_t)));
            if (!nb) { memoria_free(buf); return false; }
            buf = nb;
            cap = nc;
        }
        buf[n * 2]     = l;
        buf[n * 2 + 1] = r;
        n++;
    }

    if (n == 0) { memoria_free(buf); return false; }
    out = buf;
    frames = n;
    return true;
}

}

struct Audio::Stream {
    uint8_t* dados = nullptr;
    Fonte    fonte;
    Resamp   rs;
    uint64_t total = 0;
    bool     reiniciar = false;
    bool     terminou = false;
};

Audio::~Audio() { liberar(); }

bool Audio::criar(const char* caminho, bool loop, float vol, ModoAudio modo) {
    liberar();

    size_t tam = 0;
    uint8_t* dados = (uint8_t*)arquivo_ler_tudo_tam(caminho, &tam);
    if (!dados) return false;

    if (modo == ModoAudio::automatico)
        modo = tam > STREAM_LIMITE ? ModoAudio::stream : ModoAudio::memoria;

    if (modo == ModoAudio::stream) {
        Stream* s = new Stream;
        s->dados = dados;
        if (!s->fonte.abrir(dados, tam)) {
            memoria_free(dados);
            delete s;
            return false;
        }
        s->rs.iniciar(&s->fonte);
        s->total = s->fonte.total ? s->fonte.total * TAXA / s->fonte.taxa : 0;
        stream_ = s;
    } else {
        Fonte f;
        bool ok = f.abrir(dados, tam) && decodificar_tudo(f, amostras_, frames_);
        f.fechar();
        memoria_free(dados);
        if (!ok) { amostras_ = nullptr; frames_ = 0; return false; }
    }

    for (int i = 0; i < MAX_VOZES; i++) voz_[i] = Voz{};
    repetir_ = loop;
    volume(vol);

    if (!registrar(this)) {
        liberar();
        return false;
    }
    return true;
}

void Audio::liberar() {
    if (!amostras_ && !stream_) return;
    desregistrar(this);

    if (amostras_) {
        memoria_free(amostras_);
        amostras_ = nullptr;
    }
    if (stream_) {
        stream_->fonte.fechar();
        memoria_free(stream_->dados);
        delete stream_;
        stream_ = nullptr;
    }
    frames_ = 0;
    for (int i = 0; i < MAX_VOZES; i++) voz_[i] = Voz{};
}

void Audio::tocar() {
    if (!valido()) return;
    trava t;
    Voz& v = voz_[0];
    if (stream_) {
        if (stream_->terminou) {
            stream_->reiniciar = true;
            stream_->terminou = false;
            v.pos = 0;
        }
    } else if (v.pos >= frames_) {
        v.pos = 0;
    }
    v.ativa = true;
}

void Audio::parar() {
    trava t;
    voz_[0].ativa = false;
}

void Audio::voltar() {
    trava t;
    voz_[0].pos = 0;
    if (stream_) {
        stream_->reiniciar = true;
        stream_->terminou = false;
    }
}

void Audio::disparar(float vol) {
    if (!valido()) return;
    if (stream_) { voltar(); tocar(); return; }

    if (vol < 0.0f) vol = 0.0f;
    if (vol > 2.0f) vol = 2.0f;

    trava t;
    int escolhida = 1;
    uint64_t maior = 0;
    for (int i = 1; i < MAX_VOZES; i++) {
        if (!voz_[i].ativa) { escolhida = i; break; }
        if (voz_[i].pos >= maior) { maior = voz_[i].pos; escolhida = i; }
    }

    Voz& v = voz_[escolhida];
    v.pos = 0;
    v.vol = (int)(vol * 256.0f);
    v.ativa = true;
}

void Audio::parar_tudo() {
    trava t;
    for (int i = 0; i < MAX_VOZES; i++) voz_[i].ativa = false;
}

void Audio::repetir(bool v) {
    trava t;
    repetir_ = v;
}

void Audio::volume(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 2.0f) v = 2.0f;
    trava t;
    vol_ = (int)(v * 256.0f);
}

bool Audio::tocando() const {
    for (int i = 0; i < MAX_VOZES; i++)
        if (voz_[i].ativa) return true;
    return false;
}

float Audio::duracao() const {
    if (stream_) return (float)stream_->total / (float)TAXA;
    return (float)frames_ / (float)TAXA;
}

float Audio::posicao() const { return (float)voz_[0].pos / (float)TAXA; }

void Audio::misturar(int32_t* acc, int frames) {
    if (stream_) {
        Stream& s = *stream_;
        if (s.reiniciar) {
            s.fonte.reiniciar();
            s.rs.zerar();
            s.reiniciar = false;
        }

        Voz& v = voz_[0];
        if (!v.ativa) return;

        for (int i = 0; i < frames; i++) {
            int16_t l, r;
            if (!s.rs.proximo(l, r)) {
                if (!repetir_) { v.ativa = false; s.terminou = true; break; }
                s.fonte.reiniciar();
                s.rs.zerar();
                if (!s.rs.proximo(l, r)) { v.ativa = false; s.terminou = true; break; }
                v.pos = 0;
            }
            acc[i * 2]     += (l * vol_) >> 8;
            acc[i * 2 + 1] += (r * vol_) >> 8;
            v.pos++;
        }
        return;
    }

    if (!amostras_) return;

    for (int k = 0; k < MAX_VOZES; k++) {
        Voz& v = voz_[k];
        if (!v.ativa) continue;
        const int ganho = (vol_ * v.vol) >> 8;

        for (int i = 0; i < frames; i++) {
            if (v.pos >= frames_) {
                if (k == 0 && repetir_) v.pos = 0;
                else { v.ativa = false; break; }
            }
            const int16_t* s = amostras_ + v.pos * 2;
            acc[i * 2]     += (s[0] * ganho) >> 8;
            acc[i * 2 + 1] += (s[1] * ganho) >> 8;
            v.pos++;
        }
    }
}

void Audio::callback(void*, uint8_t* fluxo, int tamanho) {
    static int32_t acc[BLOCO * 2];

    int16_t* out = (int16_t*)fluxo;
    const int total = tamanho / (int)(2 * sizeof(int16_t));
    int feito = 0;

    while (feito < total) {
        int n = total - feito;
        if (n > BLOCO) n = BLOCO;

        memset(acc, 0, (size_t)n * 2 * sizeof(int32_t));
        for (int k = 0; k < g_n; k++) g_lista[k]->misturar(acc, n);

        for (int i = 0; i < n * 2; i++) {
            int32_t v = acc[i];
            if (v > 32767) v = 32767;
            else if (v < -32768) v = -32768;
            out[feito * 2 + i] = (int16_t)v;
        }
        feito += n;
    }
}

/* ---- API C ---- */

namespace {
inline Audio*       A(audio_handle* h)        { return reinterpret_cast<Audio*>(h); }
inline const Audio* A(const audio_handle* h)  { return reinterpret_cast<const Audio*>(h); }
}

extern "C" {

audio_handle* audio_criar(const char* caminho, int loop, float vol, int modo) {
    const ModoAudio m = modo == 1 ? ModoAudio::memoria
                      : modo == 2 ? ModoAudio::stream
                                  : ModoAudio::automatico;
    Audio* a = new Audio;
    if (!a->criar(caminho, loop != 0, vol, m)) { delete a; return nullptr; }
    return reinterpret_cast<audio_handle*>(a);
}
void audio_destruir(audio_handle* a)            { delete A(a); }
void audio_liberar(audio_handle* a)             { if (a) A(a)->liberar(); }

void audio_tocar(audio_handle* a)               { if (a) A(a)->tocar(); }
void audio_parar(audio_handle* a)               { if (a) A(a)->parar(); }
void audio_voltar(audio_handle* a)              { if (a) A(a)->voltar(); }
void audio_disparar(audio_handle* a, float vol) { if (a) A(a)->disparar(vol); }
void audio_parar_tudo(audio_handle* a)          { if (a) A(a)->parar_tudo(); }

void  audio_repetir_definir(audio_handle* a, int r)  { if (a) A(a)->repetir(r != 0); }
int   audio_repetindo(const audio_handle* a)         { return a && A(a)->repetindo(); }
void  audio_volume_definir(audio_handle* a, float v) { if (a) A(a)->volume(v); }
float audio_volume(const audio_handle* a)            { return a ? A(a)->volume() : 0.0f; }

int   audio_tocando(const audio_handle* a)      { return a && A(a)->tocando(); }
int   audio_valido(const audio_handle* a)       { return a && A(a)->valido(); }
int   audio_streaming(const audio_handle* a)    { return a && A(a)->streaming(); }
float audio_duracao(const audio_handle* a)      { return a ? A(a)->duracao() : 0.0f; }
float audio_posicao(const audio_handle* a)      { return a ? A(a)->posicao() : 0.0f; }

}