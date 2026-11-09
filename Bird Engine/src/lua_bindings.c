#include "lua_bindings.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

void bindings_todos(lua_State* L) {
    bindings_frame_buffer(L);
    bindings_audio_input(L);
    bindings_objetos(L);
    bindings_mapas(L);
    bindings_camera(L);
    bindings_obj3d(L);
    bindings_cam3d(L);
    bindings_mapa3d(L);
    bindings_mao(L);
    bindings_textos(L);
    bindings_janela(L);
    bindings_tempo(L);
    bindings_particulas(L);
}
