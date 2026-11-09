#ifndef LUA_COMUM_H
#define LUA_COMUM_H

#include <lua.h>
#include <lauxlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../lua_bindings.h"
#include "../frame buffer/frame_buffer.h"
#include "../frame buffer/z_buffer.h"
#include "../audio/audio.h"
#include "../inputs/input.h"
#include "../inputs/caixas.h"
#include "../objetos/obj.h"
#include "../objetos/obj_3d.h"
#include "../mapas/mapas.h"
#include "../mapas/mapa3d.h"
#include "../cam/cam.h"
#include "../cam/cam3d.h"
#include "../textos/escrever.h"
#include "../renderizador/render.h"
#include "../tempo/tempo.h"

#define PADRAO_ASCII 0
#define PADRAO_UTF8  1
#define COR_NENHUMA  0u

enum {
    REF_MAPA_COLISOR = 1000,
    REF_MAPA_ATLAS   = 2000,
};

extern const char* const MT_FB;
extern const char* const MT_FBREF;
extern const char* const MT_ATLAS;
extern const char* const MT_AUDIO;
extern const char* const MT_ENTRADA;
extern const char* const MT_ENTRADAREF;
extern const char* const MT_CAIXA;
extern const char* const MT_OBJ;
extern const char* const MT_ANIM;
extern const char* const MT_AABB;
extern const char* const MT_MAPA;
extern const char* const MT_CAMERA;
extern const char* const MT_CAM25D;
extern const char* const MT_FONTE;
extern const char* const MT_DIGITAR;
extern const char* const MT_TEMPO;
extern const char* const MT_PARTICULAS;
extern const char* const MT_OBJ3D;
extern const char* const MT_CAM3D;
extern const char* const MT_ZBUFFER;
extern const char* const MT_MAPA3D;
extern const char* const MT_OBJ3DREF;
extern const char* const MT_MAO;

#define DEF_GC(nome, tipo, destruir_fn)                          \
    static int gc_##nome(lua_State* L) {                         \
        tipo** p = (tipo**)lua_touserdata(L, 1);                 \
        if (p && *p) { destruir_fn(*p); *p = NULL; }             \
        return 0;                                                \
    }

#define DEF_DESTRUIR(nome, tipo, mt, destruir_fn)                \
    static int nome(lua_State* L) {                              \
        tipo** p = (tipo**)luaL_checkudata(L, 1, mt);            \
        if (*p) { destruir_fn(*p); *p = NULL; }                  \
        return 0;                                                \
    }

#define DEF_ARG(nome, tipo, mt) \
    static tipo* nome(lua_State* L, int i) { return (tipo*)arg_ptr(L, i, mt); }

extern const luaL_Reg metodos_vazios[];
extern const luaL_Reg fb_comum[];
extern const luaL_Reg entrada_comum[];

void definir_funcoes(lua_State* L, const luaL_Reg* funcs);
void registrar_classe(lua_State* L, const char* nome, const luaL_Reg* metodos, const luaL_Reg* base);
void registrar_global(lua_State* L, const char* nome, const luaL_Reg* funcs);
void campo(lua_State* L, const char* nome, int v);
int  tam_tabela(lua_State* L, int i);

void refs_pegar(lua_State* L, int dono);
void ref_definir(lua_State* L, int dono, const char* chave, int valor);
void ref_limpar(lua_State* L, int dono, const char* chave);
void ref_definir_i(lua_State* L, int dono, int chave, int valor);
void ref_limpar_i(lua_State* L, int dono, int chave);

void* testar_ud(lua_State* L, int i, const char* nome);
void* novo_ud(lua_State* L, const char* mt, size_t tam);
int   ret_ptr(lua_State* L, const char* mt, void* ptr);
int   ret_ref(lua_State* L, const char* mt, void* ptr, int dono);
void* arg_ptr(lua_State* L, int i, const char* mt);
void* arg_ptr_ref(lua_State* L, int i, const char* mt_ref, const char* mt);

bool     arg_bool(lua_State* L, int i, bool padrao);
int      arg_tile(lua_State* L, int i);
int      ret_tile(lua_State* L, int c);
uint32_t arg_cor(lua_State* L, int i);
uint32_t arg_cor_opcional(lua_State* L, int i);
int      ret_cor(lua_State* L, uint32_t c);
void     ler_par(lua_State* L, int tabela, int indice, int* a, int* b);

fb_handle*       arg_fb(lua_State* L, int i);
entrada_handle*  arg_entrada(lua_State* L, int i);
fb_atlas_handle* arg_atlas(lua_State* L, int i);
obj_handle*      arg_obj(lua_State* L, int i);
aabb_2d_handle*  arg_aabb(lua_State* L, int i);
mapa_handle*     arg_mapa(lua_State* L, int i);
cam3d_handle*    lua_arg_cam3d(lua_State* L, int i);


#endif
