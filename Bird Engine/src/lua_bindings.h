#ifndef LUA_BINDINGS_H
#define LUA_BINDINGS_H

#ifdef __cplusplus
extern "C" {
#endif

struct lua_State;

void bindings_frame_buffer(struct lua_State* L);
void bindings_audio_input(struct lua_State* L);
void bindings_objetos(struct lua_State* L);
void bindings_mapas(struct lua_State* L);
void bindings_camera(struct lua_State* L);
void bindings_textos(struct lua_State* L);
void bindings_janela(struct lua_State* L);
void bindings_tempo(struct lua_State* L);
void bindings_particulas(struct lua_State* L);
void bindings_obj3d(struct lua_State* L);
void bindings_cam3d(struct lua_State* L);
void bindings_mapa3d(struct lua_State* L);
void bindings_mao(struct lua_State* L);
void bindings_todos(struct lua_State* L);

#ifdef __cplusplus
}
#endif

#endif
