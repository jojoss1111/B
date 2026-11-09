#include "mao.h"

#define MAO_GRUPOS 32
enum { ITEM_3D, ITEM_2D };

typedef struct {
    int tipo, grupo, coluna, linha;
    obj3d_handle* obj;
    fb_atlas_handle* atlas;
    float x, y, z, rx, ry, rz, w, h;
} mao_item;

typedef struct {
    int configurada, tipo, pente_max, pente, reserva;
    float dano, alcance, intervalo, espera, recuo;
} mao_arma_info;

struct mao_handle {
    mao_item* itens;
    int n, cap, grupo;
    mao_arma_info armas[MAO_GRUPOS];
    float fase, balanco, recuo;
};

static float seno(float g) { return (float)fb_seno(fb_graus(g)) / 32768.0f; }
static float cosseno(float g) { return (float)fb_cosseno(fb_graus(g)) / 32768.0f; }
static float maxf(float a, float b) { return a > b ? a : b; }

mao_handle* mao_criar(void) {
    mao_handle* m = (mao_handle*)memoria_calloc(1, sizeof(*m));
    if (m) m->grupo = 1;
    return m;
}

void mao_limpar(mao_handle* m) { if (m) m->n = 0; }
void mao_destruir(mao_handle* m) {
    if (!m) return;
    if (m->itens) memoria_free(m->itens);
    memoria_free(m);
}

static int reservar(mao_handle* m) {
    if (m->n < m->cap) return 1;
    const int nc = m->cap ? m->cap * 2 : 8;
    mao_item* p = (mao_item*)memoria_realloc(m->itens, (size_t)nc * sizeof(*p));
    if (!p) return 0;
    m->itens = p; m->cap = nc;
    return 1;
}

int mao_adicionar_3d(mao_handle* m, obj3d_handle* o, float x, float y, float z, float rx, float ry, float rz, int grupo) {
    if (!m || !o || !reservar(m)) return -1;
    mao_item* i = &m->itens[m->n]; memoria_zerar(i, sizeof(*i));
    i->tipo = ITEM_3D; i->grupo = grupo < 0 ? 0 : (grupo >= MAO_GRUPOS ? MAO_GRUPOS - 1 : grupo); i->obj = o;
    i->x=x; i->y=y; i->z=z; i->rx=rx; i->ry=ry; i->rz=rz;
    return m->n++;
}

int mao_adicionar_2d(mao_handle* m, fb_atlas_handle* a, int c, int l, float x, float y, float w, float h, int grupo) {
    if (!m || !a || !reservar(m)) return -1;
    mao_item* i = &m->itens[m->n]; memoria_zerar(i, sizeof(*i));
    i->tipo=ITEM_2D; i->grupo=grupo<0?0:(grupo>=MAO_GRUPOS?MAO_GRUPOS-1:grupo); i->atlas=a; i->coluna=c; i->linha=l;
    i->x=x; i->y=y; i->w=w; i->h=h;
    return m->n++;
}

void mao_selecionar(mao_handle* m, int g) { if (m) m->grupo = g < 0 ? 0 : (g >= MAO_GRUPOS ? MAO_GRUPOS - 1 : g); }
int mao_grupo_ativo(const mao_handle* m) { return m ? m->grupo : 0; }

void mao_atualizar(mao_handle* m, float dt, cam3d_handle* cam) {
    if (!m) return;
    if (dt < 0.0f) dt=0.0f; if (dt>0.1f) dt=0.1f;
    if (cam && cam3d_velocidade(cam) > 0.08f) {
        m->fase += dt * (cam3d_velocidade(cam) > 3.0f ? 13.0f : 9.0f);
        m->balanco = maxf(m->balanco, 0.0f);
        if (m->balanco < 1.0f) m->balanco += dt * 4.0f;
    } else m->balanco = maxf(0.0f, m->balanco - dt * 4.0f);
    m->recuo = maxf(0.0f, m->recuo - dt * 5.0f);
    for (int i=0;i<MAO_GRUPOS;i++) if (m->armas[i].espera>0) m->armas[i].espera=maxf(0,m->armas[i].espera-dt);
}

static int visivel(const mao_handle* m, const mao_item* i) { return i->grupo==0 || i->grupo==m->grupo; }

void mao_desenhar(mao_handle* m, cam3d_handle* cam, fb_handle* fb, zb_handle* zb) {
    if (!m || !cam || !fb || !zb) return;
    float ex,ey,ez,yaw,pitch,roll;
    cam3d_vista(cam,&ex,&ey,&ez,&yaw,&pitch,&roll);
    const float sy=seno(yaw), cy=cosseno(yaw), sp=seno(pitch), cp=cosseno(pitch), sr=seno(roll), cr=cosseno(roll);
    const float r0x=cy,r0y=0,r0z=sy,u0x=-sy*sp,u0y=cp,u0z=cy*sp;
    const float rx=r0x*cr+u0x*sr, ry=r0y*cr+u0y*sr, rz=r0z*cr+u0z*sr;
    const float ux=u0x*cr-r0x*sr, uy=u0y*cr-r0y*sr, uz=u0z*cr-r0z*sr;
    const float fx=sy*cp, fy=sp, fz=-cy*cp;
    const float sway=seno(m->fase*2.0f)*0.025f*m->balanco;
    const float bob=cosseno(m->fase*4.0f)*0.025f*m->balanco;
    const zb_mat4 vp=cam3d_matriz_vp(cam);
    for (int k=0;k<m->n;k++) {
        mao_item* i=&m->itens[k]; if (i->tipo!=ITEM_3D || !visivel(m,i) || !i->obj) continue;
        const float ox=i->obj->x, oy=i->obj->y, oz=i->obj->z;
        const float orx=i->obj->rx, ory=i->obj->ry, orz=i->obj->rz;
        const float offy=i->y+bob-m->recuo*0.08f;
        obj3d_posicionar(i->obj,ex+rx*(i->x+sway)+ux*offy+fx*i->z,
                         ey+ry*(i->x+sway)+uy*offy+fy*i->z,
                         ez+rz*(i->x+sway)+uz*offy+fz*i->z);
        obj3d_rotacionar(i->obj,i->rx+pitch,i->ry+yaw,i->rz+roll);
        obj3d_desenhar(i->obj,fb,zb,&vp);
        obj3d_posicionar(i->obj,ox,oy,oz); obj3d_rotacionar(i->obj,orx,ory,orz);
    }
    for (int k=0;k<m->n;k++) {
        mao_item* i=&m->itens[k]; if (i->tipo!=ITEM_2D || !visivel(m,i) || !i->atlas) continue;
        fb_atlas_desenhar(i->atlas,fb,i->coluna,i->linha,(int)(i->x*fb->w),(int)(i->y*fb->h),(int)(i->w*fb->w),(int)(i->h*fb->h));
    }
}

void mao_arma(mao_handle* m,int g,int tipo,float dano,float alcance,float intervalo,int pente,int reserva,float recuo) {
    if (!m || g<0 || g>=MAO_GRUPOS) return;
    mao_arma_info* a=&m->armas[g]; a->configurada=1; a->tipo=tipo==MAO_CURTA?MAO_CURTA:MAO_LONGA;
    a->dano=maxf(0,dano); a->alcance=maxf(0,alcance); a->intervalo=maxf(0.01f,intervalo);
    a->pente_max=pente<0?0:pente; a->pente=a->pente_max; a->reserva=reserva<0?0:reserva; a->recuo=maxf(0,recuo);
}

int mao_usar(mao_handle* m,cam3d_handle* cam,obj3d_handle* const* objs,int n,int* alvo,float* distancia) {
    if (alvo) *alvo=-1; if (distancia) *distancia=0;
    if (!m || m->grupo<0 || m->grupo>=MAO_GRUPOS) return 0;
    mao_arma_info* a=&m->armas[m->grupo];
    if (!a->configurada || a->espera>0 || (a->tipo==MAO_LONGA && a->pente_max>0 && a->pente<=0)) return 0;
    a->espera=a->intervalo; if (a->tipo==MAO_LONGA && a->pente_max>0) a->pente--;
    m->recuo=1.0f; if (cam && a->recuo>0) cam3d_recuo(cam,a->recuo,0);
    float d=0; const int hit=cam?cam3d_raio(cam,objs,n,a->alcance,&d):-1;
    if (hit>=0 && alvo) *alvo=hit;
    if (distancia) *distancia=d;
    return 1;
}

int mao_recarregar(mao_handle* m,int g) {
    if (!m || g<0 || g>=MAO_GRUPOS) return 0;
    mao_arma_info* a=&m->armas[g]; if (!a->configurada || a->tipo!=MAO_LONGA || a->pente_max<=0 || a->reserva<=0) return 0;
    const int falta=a->pente_max-a->pente, q=falta<a->reserva?falta:a->reserva;
    a->pente+=q; a->reserva-=q; return q;
}
int mao_municao(const mao_handle* m,int g,int* reserva) {
    if (!m || g<0 || g>=MAO_GRUPOS || !m->armas[g].configurada) { if(reserva)*reserva=0; return 0; }
    if(reserva)*reserva=m->armas[g].reserva; return m->armas[g].pente;
}
float mao_dano(const mao_handle* m,int g) { return m&&g>=0&&g<MAO_GRUPOS&&m->armas[g].configurada?m->armas[g].dano:0.0f; }
