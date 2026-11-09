#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stddef.h>

#ifndef FB_API
#  if defined(_WIN32)
#    define FB_API __declspec(dllexport)
#  else
#    define FB_API __attribute__((visibility("default")))
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_MAX_VOZES 8

typedef enum {
    AUDIO_MODO_AUTOMATICO = 0,
    AUDIO_MODO_MEMORIA = 1,
    AUDIO_MODO_STREAM = 2
} ModoAudio;

typedef struct audio_stream audio_stream;

typedef struct audio_voz {
    uint64_t pos;
    int      vol;
    uint8_t  ativa;
} audio_voz;

typedef struct audio_handle {
    int16_t*      amostras;
    uint64_t      frames;
    audio_stream* stream;
    audio_voz     voz[AUDIO_MAX_VOZES];
    int           vol;
    uint8_t       repetir;
} audio_handle;

FB_API audio_handle* audio_criar(const char* caminho, int loop, float vol, int modo);
FB_API void  audio_destruir(audio_handle* a);
FB_API void  audio_liberar(audio_handle* a);

FB_API void  audio_tocar(audio_handle* a);
FB_API void  audio_parar(audio_handle* a);
FB_API void  audio_voltar(audio_handle* a);
FB_API void  audio_disparar(audio_handle* a, float vol);
FB_API void  audio_parar_tudo(audio_handle* a);

FB_API void  audio_repetir_definir(audio_handle* a, int repetir);
FB_API int   audio_repetindo(const audio_handle* a);
FB_API void  audio_volume_definir(audio_handle* a, float v);
FB_API float audio_volume(const audio_handle* a);

FB_API int   audio_tocando(const audio_handle* a);
FB_API int   audio_valido(const audio_handle* a);
FB_API int   audio_streaming(const audio_handle* a);
FB_API float audio_duracao(const audio_handle* a);
FB_API float audio_posicao(const audio_handle* a);

#ifdef __cplusplus
}
#endif

#endif