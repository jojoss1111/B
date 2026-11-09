#include "lua_comum.h"
#include "../particulas/particula.h"

DEF_ARG(arg_particulas, particulas_handle, MT_PARTICULAS)
DEF_GC(particulas, particulas_handle, particulas_destruir)

static void campo_num(lua_State* L, int t, const char* nome, float* v) {
    lua_getfield(L, t, nome);
    if (lua_isnumber(L, -1)) *v = (float)lua_tonumber(L, -1);
    lua_pop(L, 1);
}

static void campo_faixa(lua_State* L, int t, const char* nome, float* a, float* b) {
    lua_getfield(L, t, nome);
    if (lua_istable(L, -1)) {
        lua_rawgeti(L, -1, 1); if (lua_isnumber(L, -1)) *a = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 2); if (lua_isnumber(L, -1)) *b = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    } else if (lua_isnumber(L, -1)) *a = *b = (float)lua_tonumber(L, -1);
    lua_pop(L, 1);
}

static void preset(const char* nome, particula_config* c) {
    c->n_min=1; c->n_max=1; c->vida_min=.5f; c->vida_max=1; c->vel_min=20; c->vel_max=50;
    c->angulo=90; c->abertura=180; c->quique=.5f; c->atrito=.8f; c->tam_ini_min=1; c->tam_ini_max=3;
    c->tam_fim_min=0; c->tam_fim_max=0; c->rastro=.05f;
    if (!strcmp(nome,"explosao")) { c->n_min=18;c->n_max=26;c->vida_min=.3f;c->vida_max=.7f;c->vel_min=25;c->vel_max=110;c->arrasto=2.5f;c->tam_ini_min=2;c->tam_ini_max=5;c->abertura=180; }
    else if (!strcmp(nome,"fogo")) { c->vida_min=.3f;c->vida_max=.7f;c->vel_min=10;c->vel_max=30;c->angulo=90;c->abertura=18;c->gravidade=-40;c->tam_ini_min=2;c->tam_ini_max=3;c->dispersao=2; }
    else if (!strcmp(nome,"fumaca")) { c->vida_min=.8f;c->vida_max=1.6f;c->vel_min=6;c->vel_max=16;c->angulo=90;c->abertura=25;c->gravidade=-8;c->arrasto=.6f;c->tam_ini_min=2;c->tam_ini_max=3;c->tam_fim_min=5;c->tam_fim_max=7;c->desvanecer=.5f; }
    else if (!strcmp(nome,"fagulhas")) { c->n_min=8;c->n_max=14;c->vida_min=.3f;c->vida_max=.9f;c->vel_min=40;c->vel_max=130;c->angulo=90;c->abertura=60;c->gravidade=220;c->forma=PARTICULA_LINHA;c->rastro=.04f; }
    else if (!strcmp(nome,"poeira")) { c->n_min=4;c->n_max=7;c->vida_min=.2f;c->vida_max=.5f;c->vel_min=8;c->vel_max=25;c->angulo=90;c->abertura=70;c->arrasto=3;c->tam_ini_min=1;c->tam_ini_max=2; }
    else if (!strcmp(nome,"brilho")) { c->n_min=5;c->n_max=8;c->vida_min=.3f;c->vida_max=.7f;c->vel_min=10;c->vel_max=45;c->abertura=180;c->arrasto=3;c->forma=PARTICULA_PIXEL; }
    else if (!strcmp(nome,"sangue")) { c->n_min=8;c->n_max=14;c->vida_min=.4f;c->vida_max=.9f;c->vel_min=30;c->vel_max=90;c->angulo=90;c->abertura=70;c->gravidade=260;c->tam_ini_min=1;c->tam_ini_max=2; }
    else if (!strcmp(nome,"impacto")) { c->n_min=4;c->n_max=6;c->vida_min=.1f;c->vida_max=.25f;c->vel_min=50;c->vel_max=120;c->abertura=180;c->arrasto=4;c->forma=PARTICULA_LINHA;c->rastro=.05f; }
    else return;
    static const uint8_t pal[] = {7,10,9,8,2,5};
    static const uint8_t rgb[16][3]={{0,0,0},{29,43,83},{126,37,83},{0,135,81},{171,82,54},{95,87,79},{194,195,199},{255,241,232},{255,0,77},{255,163,0},{255,236,39},{0,228,54},{41,173,255},{131,118,156},{255,119,168},{255,204,170}};
    c->n_cores=sizeof(pal);
    for (int i=0;i<c->n_cores;i++) c->cores[i]=fb_cor(rgb[pal[i]][0],rgb[pal[i]][1],rgb[pal[i]][2],255);
}

static void ler_config(lua_State* L, int spec, int over, particula_config* c) {
    memset(c,0,sizeof(*c)); c->n_min=c->n_max=1; c->vida_min=c->vida_max=1;
    c->vel_min=c->vel_max=40; c->angulo=90; c->abertura=180; c->quique=.5f; c->atrito=.8f; c->chao=-1;
    c->tam_ini_min=c->tam_ini_max=2; c->rastro=.05f;
    if (lua_type(L,spec)==LUA_TSTRING) {
        const char* nome=lua_tostring(L,spec); preset(nome,c);
        if (!c->n_cores) luaL_error(L,"preset de particulas desconhecido: %s",nome);
    }
    int tab = lua_istable(L,over) ? over : (lua_istable(L,spec) ? spec : 0);
    if (!tab) return;
    campo_faixa(L,tab,"n",&c->n_min,&c->n_max); campo_faixa(L,tab,"vida",&c->vida_min,&c->vida_max);
    campo_faixa(L,tab,"vel",&c->vel_min,&c->vel_max); campo_faixa(L,tab,"tam_ini",&c->tam_ini_min,&c->tam_ini_max);
    campo_faixa(L,tab,"tam_fim",&c->tam_fim_min,&c->tam_fim_max);
    campo_num(L,tab,"angulo",&c->angulo); campo_num(L,tab,"abertura",&c->abertura); campo_num(L,tab,"dispersao",&c->dispersao);
    campo_num(L,tab,"grav",&c->gravidade); campo_num(L,tab,"vento",&c->vento); campo_num(L,tab,"arrasto",&c->arrasto);
    campo_num(L,tab,"quique",&c->quique); campo_num(L,tab,"atrito",&c->atrito); campo_num(L,tab,"rastro",&c->rastro);
    campo_num(L,tab,"desvanecer",&c->desvanecer);
    lua_getfield(L,tab,"chao"); if(lua_isnumber(L,-1)){c->chao=(float)lua_tonumber(L,-1);c->tem_chao=1;} lua_pop(L,1);
    lua_getfield(L,tab,"forma"); if(lua_isstring(L,-1)){const char* f=lua_tostring(L,-1); if(!strcmp(f,"pixel"))c->forma=PARTICULA_PIXEL;else if(!strcmp(f,"quadrado"))c->forma=PARTICULA_QUADRADO;else if(!strcmp(f,"linha"))c->forma=PARTICULA_LINHA;else c->forma=PARTICULA_CIRCULO;} lua_pop(L,1);
    const char* keys[]={"cores","paleta"};
    for(int pass=0;pass<2;pass++){
        lua_getfield(L,tab,keys[pass]);
        if(lua_istable(L,-1)){
            c->n_cores=(uint8_t)tam_tabela(L,-1); if(c->n_cores>16)c->n_cores=16;
            static const uint8_t rgb[16][3]={{0,0,0},{29,43,83},{126,37,83},{0,135,81},{171,82,54},{95,87,79},{194,195,199},{255,241,232},{255,0,77},{255,163,0},{255,236,39},{0,228,54},{41,173,255},{131,118,156},{255,119,168},{255,204,170}};
            for(int i=0;i<c->n_cores;i++){lua_rawgeti(L,-1,i+1);int v=(int)lua_tointeger(L,-1);lua_pop(L,1);c->cores[i]=pass==0?(uint32_t)v:fb_cor(rgb[(v%16+16)%16][0],rgb[(v%16+16)%16][1],rgb[(v%16+16)%16][2],255);}
            lua_pop(L,1); if(pass==0)break;
        } else lua_pop(L,1);
    }
    if(!c->n_cores){c->n_cores=1;c->cores[0]=0xFFFFFFFFu;}
}

static int l_novo(lua_State* L){return ret_ptr(L,MT_PARTICULAS,particulas_criar((uint32_t)luaL_optinteger(L,1,512)));}
static int l_emitir(lua_State* L){particulas_handle*p=arg_particulas(L,1);particula_config c;ler_config(L,2,5,&c);lua_pushinteger(L,particulas_emitir(p,&c,(float)luaL_checknumber(L,3),(float)luaL_checknumber(L,4)));return 1;}
static int l_atualizar(lua_State* L){particulas_atualizar(arg_particulas(L,1),(float)luaL_checknumber(L,2));return 0;}
static int l_desenhar(lua_State* L){
    camera_handle* cam=lua_isnoneornil(L,3)?NULL:(camera_handle*)arg_ptr(L,3,MT_CAMERA);
    particulas_desenhar(arg_particulas(L,1),arg_fb(L,2),cam,(int)luaL_optinteger(L,4,0),(int)luaL_optinteger(L,5,0));return 0;
}
static int l_limpar(lua_State* L){particulas_limpar(arg_particulas(L,1));return 0;}
static int l_quantidade(lua_State* L){lua_pushinteger(L,particulas_quantidade(arg_particulas(L,1)));return 1;}
static const luaL_Reg metodos[]={{"emitir",l_emitir},{"atualizar",l_atualizar},{"desenhar",l_desenhar},{"limpar",l_limpar},{"quantidade",l_quantidade},{"__gc",gc_particulas},{NULL,NULL}};
static const luaL_Reg funcoes[]={{"novo",l_novo},{NULL,NULL}};
void bindings_particulas(lua_State* L){registrar_classe(L,MT_PARTICULAS,metodos,NULL);registrar_global(L,"particulas",funcoes);}
