#ifndef AUDIO_HPP
#define AUDIO_HPP

#include <stdint.h>
#include <stddef.h>

enum class ModoAudio {
    automatico,
    memoria,
    stream
};

class Audio {
public:
    static constexpr int MAX_VOZES = 8;

    Audio() = default;
    ~Audio();
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    bool criar(const char* caminho, bool loop = false, float vol = 1.0f,
               ModoAudio modo = ModoAudio::automatico);
    void liberar();

    void tocar();
    void parar();
    void voltar();

    void disparar(float vol = 1.0f);
    void parar_tudo();

    void  repetir(bool v);
    bool  repetindo() const { return repetir_; }
    void  volume(float v);
    float volume() const { return vol_ / 256.0f; }

    bool  tocando() const;
    bool  valido() const { return amostras_ != nullptr || stream_ != nullptr; }
    bool  streaming() const { return stream_ != nullptr; }
    float duracao() const;
    float posicao() const;

    static void callback(void* usuario, uint8_t* fluxo, int tamanho);

private:
    struct Voz {
        uint64_t pos = 0;
        int      vol = 256;
        bool     ativa = false;
    };
    struct Stream;

    void misturar(int32_t* acc, int frames);

    int16_t* amostras_ = nullptr;
    uint64_t frames_ = 0;
    Stream*  stream_ = nullptr;
    Voz      voz_[MAX_VOZES];
    int      vol_ = 256;
    bool     repetir_ = false;
};

/* O audio.hpp não inclui o frame_buffer.hpp, então define a macro de exportação (idêntica à de lá) */
#ifndef FB_API
#  if defined(_WIN32)
#    define FB_API __declspec(dllexport)
#  else
#    define FB_API __attribute__((visibility("default")))
#  endif
#endif

extern "C" {

typedef struct audio_handle audio_handle;   /* Audio */

/* modo: 0 = automático (stream se o arquivo > 1 MiB), 1 = memória, 2 = stream. NULL em falha */
FB_API audio_handle* audio_criar(const char* caminho, int loop, float vol, int modo);
FB_API void  audio_destruir(audio_handle* a);
FB_API void  audio_liberar(audio_handle* a);              /* libera o som, mantém o handle */

FB_API void  audio_tocar(audio_handle* a);
FB_API void  audio_parar(audio_handle* a);
FB_API void  audio_voltar(audio_handle* a);
FB_API void  audio_disparar(audio_handle* a, float vol);  /* efeito sonoro: toca sobreposto */
FB_API void  audio_parar_tudo(audio_handle* a);

FB_API void  audio_repetir_definir(audio_handle* a, int repetir);
FB_API int   audio_repetindo(const audio_handle* a);
FB_API void  audio_volume_definir(audio_handle* a, float v);   /* 0.0 .. 2.0 */
FB_API float audio_volume(const audio_handle* a);

FB_API int   audio_tocando(const audio_handle* a);
FB_API int   audio_valido(const audio_handle* a);
FB_API int   audio_streaming(const audio_handle* a);
FB_API float audio_duracao(const audio_handle* a);        /* segundos */
FB_API float audio_posicao(const audio_handle* a);        /* segundos */

}

#endif