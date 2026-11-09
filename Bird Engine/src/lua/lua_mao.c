#include "lua_comum.h"
#include "../mao/mao.h"

typedef struct { mao_handle* mao; } mao_ud;

static mao_ud* arg_mao(lua_State* L,int i) {
    mao_ud* u=(mao_ud*)luaL_checkudata(L,i,MT_MAO);
    if(!u->mao) luaL_error(L,"mao ja foi destruida");
    return u;
}

static int gc_mao(lua_State* L) {
    mao_ud* u=(mao_ud*)lua_touserdata(L,1);
    if(u && u->mao){mao_destruir(u->mao);u->mao=NULL;}
    return 0;
}
static int l_novo(lua_State* L) {
    mao_ud* u=(mao_ud*)novo_ud(L,MT_MAO,sizeof(*u));
    u->mao=mao_criar();
    if(!u->mao){lua_pop(L,1);lua_pushnil(L);return 1;}
    return 1;
}
static int l_destruir(lua_State* L) { mao_ud* u=arg_mao(L,1);mao_destruir(u->mao);u->mao=NULL;return 0; }
static int l_limpar(lua_State* L) {
    mao_ud* u=arg_mao(L,1);int n=0;
    /* A contagem e os refs compartilham os indices dos itens adicionados. */
    refs_pegar(L,1); if(lua_istable(L,-1)) n=tam_tabela(L,-1); lua_pop(L,1);
    for(int i=1;i<=n;i++)ref_limpar_i(L,1,i);
    mao_limpar(u->mao);return 0;
}
static int l_adicionar3d(lua_State* L) {
    mao_ud* u=arg_mao(L,1);obj3d_handle* o=(obj3d_handle*)arg_ptr_ref(L,2,MT_OBJ3DREF,MT_OBJ3D);
    const int id=mao_adicionar_3d(u->mao,o,(float)luaL_optnumber(L,3,0),(float)luaL_optnumber(L,4,0),(float)luaL_optnumber(L,5,1.5),
                                  (float)luaL_optnumber(L,6,0),(float)luaL_optnumber(L,7,0),(float)luaL_optnumber(L,8,0),(int)luaL_optinteger(L,9,1));
    if(id<0){lua_pushnil(L);return 1;} ref_definir_i(L,1,id+1,2);lua_pushinteger(L,id);return 1;
}
static int l_adicionar2d(lua_State* L) {
    mao_ud* u=arg_mao(L,1);fb_atlas_handle* a=arg_atlas(L,2);
    const int id=mao_adicionar_2d(u->mao,a,(int)luaL_checkinteger(L,3),(int)luaL_checkinteger(L,4),
        (float)luaL_optnumber(L,5,0.65),(float)luaL_optnumber(L,6,0.60),(float)luaL_optnumber(L,7,0.3),(float)luaL_optnumber(L,8,0.35),(int)luaL_optinteger(L,9,1));
    if(id<0){lua_pushnil(L);return 1;}ref_definir_i(L,1,id+1,2);lua_pushinteger(L,id);return 1;
}
static int l_selecionar(lua_State* L){mao_selecionar(arg_mao(L,1)->mao,(int)luaL_checkinteger(L,2));return 0;}
static int l_atualizar(lua_State* L){mao_atualizar(arg_mao(L,1)->mao,(float)luaL_checknumber(L,2),lua_arg_cam3d(L,3));return 0;}
static int l_desenhar(lua_State* L){mao_desenhar(arg_mao(L,1)->mao,lua_arg_cam3d(L,2),arg_fb(L,3),(zb_handle*)arg_ptr(L,4,MT_ZBUFFER));return 0;}
static int l_arma(lua_State* L){mao_arma(arg_mao(L,1)->mao,(int)luaL_checkinteger(L,2),(int)luaL_checkinteger(L,3),
    (float)luaL_checknumber(L,4),(float)luaL_checknumber(L,5),(float)luaL_checknumber(L,6),(int)luaL_optinteger(L,7,0),
    (int)luaL_optinteger(L,8,0),(float)luaL_optnumber(L,9,0.8));return 0;}
static int l_lista_objetos(lua_State* L,int i,obj3d_handle*** saida) {
    if(lua_isnoneornil(L,i)){*saida=NULL;return 0;}
    luaL_checktype(L,i,LUA_TTABLE);const int n=tam_tabela(L,i);
    obj3d_handle** v=n?(obj3d_handle**)memoria_malloc((size_t)n*sizeof(*v)):NULL;
    if(n&&!v)luaL_error(L,"sem memoria");
    const int t=i>0?i:lua_gettop(L)+i+1;
    for(int k=1;k<=n;k++){
        lua_rawgeti(L,t,k);void** p=(void**)testar_ud(L,-1,MT_OBJ3D);if(!p)p=(void**)testar_ud(L,-1,MT_OBJ3DREF);
        if(!p){memoria_free(v);luaL_error(L,"objeto %d da lista nao e um obj3d",k);}
        v[k-1]=(obj3d_handle*)*p;lua_pop(L,1);
    }
    *saida=v;return n;
}
static int l_usar(lua_State* L) {
    mao_handle* m=arg_mao(L,1)->mao;cam3d_handle* c=lua_arg_cam3d(L,2);obj3d_handle** v=NULL;
    const int n=l_lista_objetos(L,3,&v);int alvo=-1;float dist=0;
    const int disparou=mao_usar(m,c,v,n,&alvo,&dist);if(v)memoria_free(v);
    lua_pushboolean(L,disparou);
    if(alvo>=0)lua_pushinteger(L,alvo+1);else lua_pushnil(L);
    lua_pushnumber(L,disparou?mao_dano(m,mao_grupo_ativo(m)):0);
    lua_pushnumber(L,disparou?dist:0);
    int reserva=0;const int atual=mao_municao(m,mao_grupo_ativo(m),&reserva);
    lua_pushinteger(L,atual);lua_pushinteger(L,reserva);return 6;
}
static int l_recarregar(lua_State* L){lua_pushinteger(L,mao_recarregar(arg_mao(L,1)->mao,(int)luaL_checkinteger(L,2)));return 1;}
static int l_municao(lua_State* L){int r=0;int a=mao_municao(arg_mao(L,1)->mao,(int)luaL_checkinteger(L,2),&r);lua_pushinteger(L,a);lua_pushinteger(L,r);return 2;}

static const luaL_Reg metodos[]={
    {"__gc",gc_mao},{"destruir",l_destruir},{"limpar",l_limpar},
    {"adicionar3d",l_adicionar3d},{"adicionar2d",l_adicionar2d},{"selecionar",l_selecionar},
    {"atualizar",l_atualizar},{"desenhar",l_desenhar},{"arma",l_arma},{"usar",l_usar},
    {"recarregar",l_recarregar},{"municao",l_municao},{NULL,NULL}
};
static const luaL_Reg funcoes[]={{"novo",l_novo},{NULL,NULL}};
void bindings_mao(lua_State* L){
    registrar_classe(L,MT_MAO,metodos,NULL);registrar_global(L,"mao",funcoes);
    lua_getglobal(L,"mao");lua_pushinteger(L,MAO_CURTA);lua_setfield(L,-2,"CURTA");
    lua_pushinteger(L,MAO_LONGA);lua_setfield(L,-2,"LONGA");lua_pop(L,1);
}
