#include "obj_3d.h"
#include <math.h>

#define OBJ3D_COR_PADRAO 0xFFFFFFFFu

static float sn(float g) { return (float)fb_seno(fb_graus(g)) * (1.0f / 32768.0f); }
static float cs(float g) { return (float)fb_cosseno(fb_graus(g)) * (1.0f / 32768.0f); }

static float envolver(float a) {
    if (a > 1e6f || a < -1e6f) return 0.0f;
    while (a >= 360.0f) a -= 360.0f;
    while (a < 0.0f) a += 360.0f;
    return a;
}

static int limitar_int(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static float rep_ok(float r)    { return r > 0.0f ? r : 1.0f; }
static float valor_abs(float v) { return v < 0.0f ? -v : v; }

static float ciclo(float v) {
    const float r = (float)(int)(v + 0.5f);
    return r < 1.0f ? 1.0f : r;
}

static void rep_parte(obj3d_parte* p, float u, float v) {
    if (!p->repetir) { p->rep_u = p->rep_v = 1.0f; return; }
    p->rep_u = u * rep_ok(p->dens_u);
    p->rep_v = v * rep_ok(p->dens_v);
}

static void rep_parte_ciclo(obj3d_parte* p, float arco, float v) {
    if (!p->repetir) { p->rep_u = p->rep_v = 1.0f; return; }
    p->rep_u = ciclo(arco * rep_ok(p->dens_u));
    p->rep_v = v * rep_ok(p->dens_v);
}

static float poligono_perimetro(const obj3d_parte* lado);

static void atualizar_rep(obj3d_handle* o) {
    if (!o) return;
    const float sx = valor_abs(o->sx), sy = valor_abs(o->sy), sz = valor_abs(o->sz);
    const float arco = 3.14159265f * (sx + sz) * 0.5f;

    switch (o->forma) {
    case OBJ3D_CUBO:
        rep_parte(&o->partes[0], sx, sy);
        rep_parte(&o->partes[1], sx, sy);
        rep_parte(&o->partes[2], sx, sz);
        rep_parte(&o->partes[3], sx, sz);
        rep_parte(&o->partes[4], sz, sy);
        rep_parte(&o->partes[5], sz, sy);
        break;
    case OBJ3D_CILINDRO:
        rep_parte_ciclo(&o->partes[0], arco, sy);
        rep_parte(&o->partes[1], sx, sz);
        rep_parte(&o->partes[2], sx, sz);
        break;
    case OBJ3D_CONE:
        rep_parte_ciclo(&o->partes[0], arco, sy);
        rep_parte(&o->partes[1], sx, sz);
        break;
    case OBJ3D_ESFERA:
        rep_parte_ciclo(&o->partes[0], arco, arco * 0.5f);
        break;
    case OBJ3D_POLIGONO:
        rep_parte_ciclo(&o->partes[0], poligono_perimetro(&o->partes[0]) * (sx + sz) * 0.5f, sy);
        rep_parte(&o->partes[1], (o->bmax[0] - o->bmin[0]) * sx, (o->bmax[2] - o->bmin[2]) * sz);
        rep_parte(&o->partes[2], (o->bmax[0] - o->bmin[0]) * sx, (o->bmax[2] - o->bmin[2]) * sz);
        break;
    default:
        for (int i = 0; i < o->n_partes; i++) rep_parte(&o->partes[i], 1.0f, 1.0f);
        break;
    }
}

static void parte_liberar(obj3d_parte* p) {
    if (p->vertices) memoria_free(p->vertices);
    if (p->indices) memoria_free(p->indices);
    p->vertices = NULL;
    p->indices = NULL;
    p->n_vertices = p->n_indices = 0;
}

static int parte_alocar(obj3d_parte* p, int nv, int ni) {
    p->vertices = (zb_vertice*)memoria_malloc((size_t)nv * sizeof(zb_vertice));
    p->indices = (uint16_t*)memoria_malloc((size_t)ni * sizeof(uint16_t));
    p->n_vertices = nv;
    p->n_indices = ni;
    p->col = p->lin = 0;
    p->visivel = 1;
    p->nome[0] = 0;
    p->rep_u = p->rep_v = 1.0f;
    p->dens_u = p->dens_v = 1.0f;
    p->repetir = 1;
    if (!p->vertices || !p->indices) {
        parte_liberar(p);
        return 0;
    }
    return 1;
}

static void vert(zb_vertice* v, float x, float y, float z, float u, float w, uint32_t cor) {
    v->x = x; v->y = y; v->z = z;
    v->u = u; v->v = w;
    v->cor = cor;
}

static const float CUBO_F[6][4][3] = {
    { {-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f} },
    { {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f} },
    { {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f} },
    { { 0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f} },
    { { 0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f} },
    { {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f} }
};

static void quad(obj3d_parte* p, int f, const float q[4][3]) {
    static const float uv[4][2] = { {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f} };
    zb_vertice* v = p->vertices + f * 4;
    uint16_t* ix = p->indices + f * 6;
    const int b = f * 4;
    for (int k = 0; k < 4; k++) vert(&v[k], q[k][0], q[k][1], q[k][2], uv[k][0], uv[k][1], OBJ3D_COR_PADRAO);
    ix[0] = (uint16_t)b;       ix[1] = (uint16_t)(b + 1); ix[2] = (uint16_t)(b + 2);
    ix[3] = (uint16_t)b;       ix[4] = (uint16_t)(b + 2); ix[5] = (uint16_t)(b + 3);
}

static int montar_cubo(obj3d_handle* o) {
    for (int i = 0; i < 6; i++)
        if (!parte_alocar(&o->partes[i], 4, 6)) return 0;
    /* Frente (-Z), tras (+Z), cima, baixo, esquerda (-X), direita (+X). */
    quad(&o->partes[0], 0, CUBO_F[3]);
    quad(&o->partes[1], 0, CUBO_F[2]);
    quad(&o->partes[2], 0, CUBO_F[0]);
    quad(&o->partes[3], 0, CUBO_F[1]);
    quad(&o->partes[4], 0, CUBO_F[5]);
    quad(&o->partes[5], 0, CUBO_F[4]);
    o->n_partes = 6;
    return 1;
}

static int montar_lado_cilindro(obj3d_parte* p, int n) {
    if (!parte_alocar(p, 2 * (n + 1), 6 * n)) return 0;
    for (int i = 0; i <= n; i++) {
        const float a = 360.0f * (float)(i % n) / (float)n;
        const float c = cs(a) * 0.5f, s = -sn(a) * 0.5f;
        const float u = (float)i / (float)n;
        vert(&p->vertices[2 * i],     c, -0.5f, s, u, 1.0f, OBJ3D_COR_PADRAO);
        vert(&p->vertices[2 * i + 1], c,  0.5f, s, u, 0.0f, OBJ3D_COR_PADRAO);
    }
    for (int i = 0; i < n; i++) {
        const int b = 2 * i;
        uint16_t* ix = p->indices + 6 * i;
        ix[0] = (uint16_t)b; ix[1] = (uint16_t)(b + 2); ix[2] = (uint16_t)(b + 3);
        ix[3] = (uint16_t)b; ix[4] = (uint16_t)(b + 3); ix[5] = (uint16_t)(b + 1);
    }
    return 1;
}

static int montar_tampa(obj3d_parte* p, int n, float y, int cima) {
    if (!parte_alocar(p, n + 2, 3 * n)) return 0;
    vert(&p->vertices[0], 0.0f, y, 0.0f, 0.5f, 0.5f, OBJ3D_COR_PADRAO);
    for (int i = 0; i <= n; i++) {
        const float a = 360.0f * (float)(i % n) / (float)n;
        const float c = cs(a), s = -sn(a);
        vert(&p->vertices[1 + i], c * 0.5f, y, s * 0.5f, 0.5f + 0.5f * c, 0.5f + 0.5f * s, OBJ3D_COR_PADRAO);
    }
    for (int i = 0; i < n; i++) {
        uint16_t* ix = p->indices + 3 * i;
        ix[0] = 0;
        if (cima) { ix[1] = (uint16_t)(1 + i); ix[2] = (uint16_t)(2 + i); }
        else      { ix[1] = (uint16_t)(2 + i); ix[2] = (uint16_t)(1 + i); }
    }
    return 1;
}

static int montar_cilindro(obj3d_handle* o, int n) {
    if (!montar_lado_cilindro(&o->partes[0], n)) return 0;
    if (!montar_tampa(&o->partes[1], n, 0.5f, 1)) return 0;
    if (!montar_tampa(&o->partes[2], n, -0.5f, 0)) return 0;
    o->n_partes = 3;
    return 1;
}

static int montar_lado_cone(obj3d_parte* p, int n) {
    if (!parte_alocar(p, 2 * n + 1, 3 * n)) return 0;
    for (int i = 0; i <= n; i++) {
        const float a = 360.0f * (float)(i % n) / (float)n;
        vert(&p->vertices[i], cs(a) * 0.5f, -0.5f, -sn(a) * 0.5f, (float)i / (float)n, 1.0f, OBJ3D_COR_PADRAO);
    }
    for (int i = 0; i < n; i++)
        vert(&p->vertices[n + 1 + i], 0.0f, 0.5f, 0.0f, ((float)i + 0.5f) / (float)n, 0.0f, OBJ3D_COR_PADRAO);
    for (int i = 0; i < n; i++) {
        uint16_t* ix = p->indices + 3 * i;
        ix[0] = (uint16_t)i;
        ix[1] = (uint16_t)(i + 1);
        ix[2] = (uint16_t)(n + 1 + i);
    }
    return 1;
}

static int montar_cone(obj3d_handle* o, int n) {
    if (!montar_lado_cone(&o->partes[0], n)) return 0;
    if (!montar_tampa(&o->partes[1], n, -0.5f, 0)) return 0;
    o->n_partes = 2;
    return 1;
}

static int montar_esfera(obj3d_handle* o, int s, int a) {
    obj3d_parte* p = &o->partes[0];
    if (!parte_alocar(p, (a + 1) * (s + 1), 6 * a * s)) return 0;
    for (int j = 0; j <= a; j++) {
        const float ph = 180.0f * (float)j / (float)a;
        const float y = 0.5f * cs(ph);
        const float rr = 0.5f * sn(ph);
        for (int i = 0; i <= s; i++) {
            const float t = 360.0f * (float)(i % s) / (float)s;
            vert(&p->vertices[j * (s + 1) + i], rr * cs(t), y, -rr * sn(t), (float)i / (float)s, (float)j / (float)a, OBJ3D_COR_PADRAO);
        }
    }
    for (int j = 0; j < a; j++) {
        for (int i = 0; i < s; i++) {
            const int A = j * (s + 1) + i, B = A + 1, C = A + s + 1, D = C + 1;
            uint16_t* ix = p->indices + 6 * (j * s + i);
            ix[0] = (uint16_t)A; ix[1] = (uint16_t)C; ix[2] = (uint16_t)B;
            ix[3] = (uint16_t)B; ix[4] = (uint16_t)C; ix[5] = (uint16_t)D;
        }
    }
    o->n_partes = 1;
    return 1;
}

static obj3d_handle* novo(obj3d_forma f, fb_atlas_handle* atlas, float x, float y, float z, float sx, float sy, float sz) {
    obj3d_handle* o = (obj3d_handle*)memoria_calloc(1, sizeof(obj3d_handle));
    if (!o) return NULL;
    o->forma = f;
    o->atlas = atlas;
    o->x = x; o->y = y; o->z = z;
    o->sx = sx; o->sy = sy; o->sz = sz;
    o->cor = OBJ3D_COR_PADRAO;
    o->modo = atlas ? ZB_MODO_TEXTURA : ZB_MODO_GOURAUD;
    o->cull = ZB_CULL_TRAS;
    o->visivel = 1;
    o->cap_partes = 6;
    o->colisao = OBJ3D_COL_CAIXA;
    o->bmin[0] = o->bmin[1] = o->bmin[2] = -0.5f;
    o->bmax[0] = o->bmax[1] = o->bmax[2] =  0.5f;
    o->partes = (obj3d_parte*)memoria_calloc(6, sizeof(obj3d_parte));
    if (!o->partes) {
        memoria_free(o);
        return NULL;
    }
    return o;
}

static obj3d_handle* finalizar(obj3d_handle* o, int ok) {
    if (!ok) {
        obj3d_destruir(o);
        return NULL;
    }
    return o;
}

obj3d_handle* obj3d_criar_cubo(fb_atlas_handle* atlas, float x, float y, float z, float largura, float altura, float profundidade, float rep_largura, float rep_altura, int repetir) {
    obj3d_handle* o = novo(OBJ3D_CUBO, atlas, x, y, z, largura, altura, profundidade);
    if (!o) return NULL;
    o = finalizar(o, montar_cubo(o));
    if (o) obj3d_repetir(o, rep_largura, rep_altura, repetir);
    return o;
}

obj3d_handle* obj3d_criar_esfera(fb_atlas_handle* atlas, float x, float y, float z, float raio, int fatias, int aneis, float rep_largura, float rep_altura, int repetir) {
    obj3d_handle* o = novo(OBJ3D_ESFERA, atlas, x, y, z, raio * 2.0f, raio * 2.0f, raio * 2.0f);
    if (!o) return NULL;
    o = finalizar(o, montar_esfera(o, limitar_int(fatias, 3, 128), limitar_int(aneis, 2, 128)));
    if (o) obj3d_repetir(o, rep_largura, rep_altura, repetir);
    return o;
}

obj3d_handle* obj3d_criar_cone(fb_atlas_handle* atlas, float x, float y, float z, float raio, float altura, int fatias, float rep_largura, float rep_altura, int repetir) {
    obj3d_handle* o = novo(OBJ3D_CONE, atlas, x, y, z, raio * 2.0f, altura, raio * 2.0f);
    if (!o) return NULL;
    o = finalizar(o, montar_cone(o, limitar_int(fatias, 3, 256)));
    if (o) obj3d_repetir(o, rep_largura, rep_altura, repetir);
    return o;
}

obj3d_handle* obj3d_criar_cilindro(fb_atlas_handle* atlas, float x, float y, float z, float raio, float altura, int fatias, float rep_largura, float rep_altura, int repetir) {
    obj3d_handle* o = novo(OBJ3D_CILINDRO, atlas, x, y, z, raio * 2.0f, altura, raio * 2.0f);
    if (!o) return NULL;
    o = finalizar(o, montar_cilindro(o, limitar_int(fatias, 3, 256)));
    if (o) obj3d_repetir(o, rep_largura, rep_altura, repetir);
    return o;
}

static const float BILL_F[4][3] = {
    {-0.5f, -0.5f, 0.0f}, { 0.5f, -0.5f, 0.0f}, { 0.5f,  0.5f, 0.0f}, {-0.5f,  0.5f, 0.0f}
};

static int montar_billboard(obj3d_handle* o) {
    if (!parte_alocar(&o->partes[0], 4, 6)) return 0;
    quad(&o->partes[0], 0, BILL_F);
    o->n_partes = 1;
    return 1;
}

obj3d_handle* obj3d_criar_billboard(fb_atlas_handle* atlas, float x, float y, float z, float largura, float altura, float rep_largura, float rep_altura, int repetir) {
    obj3d_handle* o = novo(OBJ3D_BILLBOARD, atlas, x, y, z, largura, altura, 1.0f);
    if (!o) return NULL;
    o = finalizar(o, montar_billboard(o));
    if (o) {
        o->billboard = OBJ3D_BB_CILINDRICO;
        o->colisao = OBJ3D_COL_NENHUMA;
        obj3d_repetir(o, rep_largura, rep_altura, repetir);
    }
    return o;
}

void obj3d_billboard(obj3d_handle* o, int tipo) {
    if (!o) return;
    o->billboard = (tipo == OBJ3D_BB_ESFERICO || tipo == OBJ3D_BB_CILINDRICO) ? tipo : OBJ3D_BB_NENHUM;
}

int obj3d_billboard_tipo(const obj3d_handle* o) {
    return o ? o->billboard : OBJ3D_BB_NENHUM;
}

#define OBJ3D_POLI_MAX 512

static float cruz_xz(float ax, float az, float bx, float bz, float cx, float cz) {
    return (bx - ax) * (cz - az) - (bz - az) * (cx - ax);
}

static float cruz_poli(const float* p, int a, int b, int c) {
    return cruz_xz(p[2 * a], p[2 * a + 1], p[2 * b], p[2 * b + 1], p[2 * c], p[2 * c + 1]);
}

static int poligono_limpar_pontos(const float* in, int n, float* out) {
    int m = 0;
    for (int i = 0; i < n; i++) {
        const float x = in[2 * i], z = in[2 * i + 1];
        if (m > 0 && out[2 * (m - 1)] == x && out[2 * (m - 1) + 1] == z) continue;
        out[2 * m] = x;
        out[2 * m + 1] = z;
        m++;
    }
    if (m > 1 && out[0] == out[2 * (m - 1)] && out[1] == out[2 * (m - 1) + 1]) m--;
    return m;
}

static float poligono_perimetro(const obj3d_parte* lado) {
    if (!lado || !lado->vertices || lado->n_vertices < 4) return 0.0f;
    const int n = lado->n_vertices / 2 - 1;
    float soma = 0.0f;
    for (int i = 0; i < n; i++) {
        const float dx = lado->vertices[2 * (i + 1)].x - lado->vertices[2 * i].x;
        const float dz = lado->vertices[2 * (i + 1)].z - lado->vertices[2 * i].z;
        soma += sqrtf(dx * dx + dz * dz);
    }
    return soma;
}

static int poligono_triangular(const float* p, int n, int inv, uint16_t* ix) {
    uint16_t* ord = (uint16_t*)memoria_malloc((size_t)n * sizeof(uint16_t));
    if (!ord) return 0;
    for (int i = 0; i < n; i++) ord[i] = (uint16_t)(inv ? n - 1 - i : i);

    int resto = n, k = 0;
    while (resto > 3) {
        int orelha = -1, qualquer = -1;
        for (int i = 0; i < resto && orelha < 0; i++) {
            const int a = ord[(i + resto - 1) % resto], b = ord[i], c = ord[(i + 1) % resto];
            if (cruz_poli(p, a, b, c) >= 0.0f) continue;
            if (qualquer < 0) qualquer = i;

            int livre = 1;
            for (int j = 0; j < resto && livre; j++) {
                const int q = ord[j];
                if (q == a || q == b || q == c) continue;
                if (cruz_poli(p, a, b, q) < 0.0f && cruz_poli(p, b, c, q) < 0.0f && cruz_poli(p, c, a, q) < 0.0f)
                    livre = 0;
            }
            if (livre) orelha = i;
        }
        if (orelha < 0) orelha = qualquer >= 0 ? qualquer : 0;

        ix[k++] = ord[(orelha + resto - 1) % resto];
        ix[k++] = ord[orelha];
        ix[k++] = ord[(orelha + 1) % resto];
        for (int j = orelha; j < resto - 1; j++) ord[j] = ord[j + 1];
        resto--;
    }
    ix[k++] = ord[0];
    ix[k++] = ord[1];
    ix[k++] = ord[2];

    memoria_free(ord);
    return 1;
}

static int poligono_falha(obj3d_parte* pt) {
    for (int i = 0; i < 3; i++) parte_liberar(&pt[i]);
    return 0;
}

static int poligono_construir(obj3d_parte* pt, const float* p, int n) {
    if (n < 3 || n > OBJ3D_POLI_MAX) return 0;

    float S = 0.0f, per = 0.0f;
    float mnx = p[0], mxx = p[0], mnz = p[1], mxz = p[1];
    for (int i = 0; i < n; i++) {
        const int j = (i + 1) % n;
        S += p[2 * i] * p[2 * j + 1] - p[2 * j] * p[2 * i + 1];
        const float dx = p[2 * j] - p[2 * i], dz = p[2 * j + 1] - p[2 * i + 1];
        per += sqrtf(dx * dx + dz * dz);
        if (p[2 * i] < mnx) mnx = p[2 * i];
        if (p[2 * i] > mxx) mxx = p[2 * i];
        if (p[2 * i + 1] < mnz) mnz = p[2 * i + 1];
        if (p[2 * i + 1] > mxz) mxz = p[2 * i + 1];
    }
    if (S == 0.0f || per <= 0.0f) return 0;

    const int   inv = S > 0.0f;
    const float lw = mxx - mnx > 0.0f ? mxx - mnx : 1.0f;
    const float ld = mxz - mnz > 0.0f ? mxz - mnz : 1.0f;

    obj3d_parte* L = &pt[0];
    if (!parte_alocar(L, 2 * (n + 1), 6 * n)) return poligono_falha(pt);
    float acum = 0.0f;
    for (int i = 0; i <= n; i++) {
        const int k = i % n;
        if (i > 0) {
            const int a = (i - 1) % n;
            const float dx = p[2 * k] - p[2 * a], dz = p[2 * k + 1] - p[2 * a + 1];
            acum += sqrtf(dx * dx + dz * dz);
        }
        float u = acum / per;
        if (inv) u = 1.0f - u;
        vert(&L->vertices[2 * i],     p[2 * k], 0.0f, p[2 * k + 1], u, 1.0f, OBJ3D_COR_PADRAO);
        vert(&L->vertices[2 * i + 1], p[2 * k], 1.0f, p[2 * k + 1], u, 0.0f, OBJ3D_COR_PADRAO);
    }
    for (int i = 0; i < n; i++) {
        const int b = 2 * i;
        uint16_t* ix = L->indices + 6 * i;
        if (!inv) {
            ix[0] = (uint16_t)b; ix[1] = (uint16_t)(b + 2); ix[2] = (uint16_t)(b + 3);
            ix[3] = (uint16_t)b; ix[4] = (uint16_t)(b + 3); ix[5] = (uint16_t)(b + 1);
        } else {
            ix[0] = (uint16_t)b; ix[1] = (uint16_t)(b + 3); ix[2] = (uint16_t)(b + 2);
            ix[3] = (uint16_t)b; ix[4] = (uint16_t)(b + 1); ix[5] = (uint16_t)(b + 3);
        }
    }

    if (!parte_alocar(&pt[1], n, 3 * (n - 2))) return poligono_falha(pt);
    if (!parte_alocar(&pt[2], n, 3 * (n - 2))) return poligono_falha(pt);
    for (int i = 0; i < n; i++) {
        const float u = (p[2 * i] - mnx) / lw, v = (p[2 * i + 1] - mnz) / ld;
        vert(&pt[1].vertices[i], p[2 * i], 1.0f, p[2 * i + 1], u, v, OBJ3D_COR_PADRAO);
        vert(&pt[2].vertices[i], p[2 * i], 0.0f, p[2 * i + 1], u, v, OBJ3D_COR_PADRAO);
    }
    if (!poligono_triangular(p, n, inv, pt[1].indices)) return poligono_falha(pt);
    for (int t = 0; t < n - 2; t++) {
        pt[2].indices[3 * t]     = pt[1].indices[3 * t];
        pt[2].indices[3 * t + 1] = pt[1].indices[3 * t + 2];
        pt[2].indices[3 * t + 2] = pt[1].indices[3 * t + 1];
    }
    return 1;
}

obj3d_handle* obj3d_criar_poligono(fb_atlas_handle* atlas, float x, float y, float z, const float* pontos, int n, float altura, float rep_largura, float rep_altura, int repetir) {
    if (!pontos || n < 3 || n > OBJ3D_POLI_MAX + 1) return NULL;

    float* p = (float*)memoria_malloc((size_t)n * 2 * sizeof(float));
    if (!p) return NULL;
    const int m = poligono_limpar_pontos(pontos, n, p);
    if (m < 3) { memoria_free(p); return NULL; }

    obj3d_handle* o = novo(OBJ3D_POLIGONO, atlas, x, y, z, 1.0f, altura > 0.0f ? altura : 1.0f, 1.0f);
    if (!o) { memoria_free(p); return NULL; }

    o = finalizar(o, poligono_construir(o->partes, p, m));
    memoria_free(p);
    if (o) {
        o->n_partes = 3;
        obj3d_recalcular_limites(o);
        obj3d_repetir(o, rep_largura, rep_altura, repetir);
    }
    return o;
}

static int poligono_reconstruir(obj3d_handle* o, const float* pts, int n) {
    if (!o || o->forma != OBJ3D_POLIGONO || !pts || n < 3 || n > OBJ3D_POLI_MAX + 1) return 0;

    float* p = (float*)memoria_malloc((size_t)n * 2 * sizeof(float));
    if (!p) return 0;
    const int m = poligono_limpar_pontos(pts, n, p);

    obj3d_parte novas[3] = { {0}, {0}, {0} };
    if (m < 3 || !poligono_construir(novas, p, m)) { memoria_free(p); return 0; }
    memoria_free(p);

    for (int i = 0; i < 3; i++) {
        novas[i].col = o->partes[i].col;
        novas[i].lin = o->partes[i].lin;
        novas[i].visivel = o->partes[i].visivel;
        novas[i].dens_u = o->partes[i].dens_u;
        novas[i].dens_v = o->partes[i].dens_v;
        novas[i].repetir = o->partes[i].repetir;
        for (int k = 0; k < 48; k++) novas[i].nome[k] = o->partes[i].nome[k];
        parte_liberar(&o->partes[i]);
        o->partes[i] = novas[i];
    }
    obj3d_cor(o, o->cor);
    obj3d_recalcular_limites(o);
    atualizar_rep(o);
    return 1;
}

int obj3d_poligono_n(const obj3d_handle* o) {
    if (!o || o->forma != OBJ3D_POLIGONO || o->n_partes < 1) return 0;
    return o->partes[0].n_vertices / 2 - 1;
}

int obj3d_poligono_ler(const obj3d_handle* o, int i, float* x, float* z) {
    const int n = obj3d_poligono_n(o);
    if (i < 0 || i >= n) return 0;
    if (x) *x = o->partes[0].vertices[2 * i].x;
    if (z) *z = o->partes[0].vertices[2 * i].z;
    return 1;
}

static float* poligono_copiar_pontos(const obj3d_handle* o, int* n_saida) {
    const int n = obj3d_poligono_n(o);
    *n_saida = n;
    if (n < 3) return NULL;
    float* p = (float*)memoria_malloc((size_t)n * 2 * sizeof(float));
    if (!p) return NULL;
    for (int i = 0; i < n; i++) {
        p[2 * i]     = o->partes[0].vertices[2 * i].x;
        p[2 * i + 1] = o->partes[0].vertices[2 * i].z;
    }
    return p;
}

int obj3d_poligono_pontos(obj3d_handle* o, const float* pontos, int n) {
    return poligono_reconstruir(o, pontos, n);
}

int obj3d_poligono_ponto(obj3d_handle* o, int i, float x, float z) {
    int n;
    float* p = poligono_copiar_pontos(o, &n);
    if (!p) return 0;
    if (i < 0 || i >= n) { memoria_free(p); return 0; }
    p[2 * i] = x;
    p[2 * i + 1] = z;
    const int ok = poligono_reconstruir(o, p, n);
    memoria_free(p);
    return ok;
}

int obj3d_poligono_girar_pontos(obj3d_handle* o, float graus, float cx, float cz) {
    int n;
    float* p = poligono_copiar_pontos(o, &n);
    if (!p) return 0;
    const float c = cs(graus), s = sn(graus);
    for (int i = 0; i < n; i++) {
        const float x = p[2 * i] - cx, z = p[2 * i + 1] - cz;
        p[2 * i]     = cx + x * c + z * s;
        p[2 * i + 1] = cz - x * s + z * c;
    }
    const int ok = poligono_reconstruir(o, p, n);
    memoria_free(p);
    return ok;
}

int obj3d_poligono_pivo(obj3d_handle* o, float px, float pz) {
    int n;
    float* p = poligono_copiar_pontos(o, &n);
    if (!p) return 0;

    const zb_mat4 m = obj3d_matriz(o);
    const float wx = m.m[0] * px + m.m[2]  * pz + m.m[3];
    const float wy = m.m[4] * px + m.m[6]  * pz + m.m[7];
    const float wz = m.m[8] * px + m.m[10] * pz + m.m[11];

    for (int i = 0; i < n; i++) { p[2 * i] -= px; p[2 * i + 1] -= pz; }
    const int ok = poligono_reconstruir(o, p, n);
    memoria_free(p);
    if (ok) { o->x = wx; o->y = wy; o->z = wz; }
    return ok;
}

void obj3d_poligono_altura(obj3d_handle* o, float altura) {
    if (!o || o->forma != OBJ3D_POLIGONO || altura <= 0.0f) return;
    obj3d_escalar(o, o->sx, altura, o->sz);
}

void obj3d_destruir(obj3d_handle* o) {
    if (!o) return;
    if (o->partes) {
        for (int i = 0; i < o->cap_partes; i++) parte_liberar(&o->partes[i]);
        memoria_free(o->partes);
    }
    memoria_free(o);
}

obj3d_forma obj3d_forma_de(const obj3d_handle* o) { return o ? o->forma : OBJ3D_CUBO; }
float obj3d_x(const obj3d_handle* o)        { return o ? o->x : 0.0f; }
float obj3d_y(const obj3d_handle* o)        { return o ? o->y : 0.0f; }
float obj3d_z(const obj3d_handle* o)        { return o ? o->z : 0.0f; }
float obj3d_rot_x(const obj3d_handle* o)    { return o ? o->rx : 0.0f; }
float obj3d_rot_y(const obj3d_handle* o)    { return o ? o->ry : 0.0f; }
float obj3d_rot_z(const obj3d_handle* o)    { return o ? o->rz : 0.0f; }
float obj3d_escala_x(const obj3d_handle* o) { return o ? o->sx : 0.0f; }
float obj3d_escala_y(const obj3d_handle* o) { return o ? o->sy : 0.0f; }
float obj3d_escala_z(const obj3d_handle* o) { return o ? o->sz : 0.0f; }

void obj3d_posicionar(obj3d_handle* o, float x, float y, float z) { if (o) { o->x = x; o->y = y; o->z = z; } }
void obj3d_mover(obj3d_handle* o, float x, float y, float z)       { if (o) { o->x += x; o->y += y; o->z += z; } }

void obj3d_rotacionar(obj3d_handle* o, float rx, float ry, float rz) {
    if (!o) return;
    o->rx = envolver(rx);
    o->ry = envolver(ry);
    o->rz = envolver(rz);
}

void obj3d_girar(obj3d_handle* o, float rx, float ry, float rz) {
    if (!o) return;
    o->rx = envolver(o->rx + rx);
    o->ry = envolver(o->ry + ry);
    o->rz = envolver(o->rz + rz);
}

void obj3d_rotacionar_x(obj3d_handle* o, float graus) { if (o) o->rx = envolver(graus); }
void obj3d_rotacionar_y(obj3d_handle* o, float graus) { if (o) o->ry = envolver(graus); }
void obj3d_rotacionar_z(obj3d_handle* o, float graus) { if (o) o->rz = envolver(graus); }

void obj3d_escalar(obj3d_handle* o, float sx, float sy, float sz) {
    if (!o) return;
    o->sx = sx; o->sy = sy; o->sz = sz;
    atualizar_rep(o);
}

void obj3d_escalar_uniforme(obj3d_handle* o, float s) {
    if (!o) return;
    o->sx = o->sy = o->sz = s;
    atualizar_rep(o);
}

void obj3d_ampliar(obj3d_handle* o, float sx, float sy, float sz) {
    if (!o) return;
    o->sx *= sx; o->sy *= sy; o->sz *= sz;
    atualizar_rep(o);
}

void obj3d_atlas(obj3d_handle* o, fb_atlas_handle* atlas) {
    if (!o) return;
    o->atlas = atlas;
    o->modo = atlas ? ZB_MODO_TEXTURA : ZB_MODO_GOURAUD;
}

void obj3d_tile(obj3d_handle* o, uint16_t coluna, uint16_t linha) {
    if (!o) return;
    for (int i = 0; i < o->n_partes; i++) { o->partes[i].col = coluna; o->partes[i].lin = linha; }
}

static int tem_tres_partes(const obj3d_handle* o) {
    return o->forma == OBJ3D_CUBO || o->forma == OBJ3D_CILINDRO || o->forma == OBJ3D_POLIGONO;
}

void obj3d_tile_lado(obj3d_handle* o, uint16_t coluna, uint16_t linha) {
    if (!o || !tem_tres_partes(o)) return;
    if (o->forma == OBJ3D_CUBO) {
        const int lados[] = {0, 1, 4, 5};
        for (int i = 0; i < 4; i++) { o->partes[lados[i]].col = coluna; o->partes[lados[i]].lin = linha; }
    } else { o->partes[0].col = coluna; o->partes[0].lin = linha; }
}

void obj3d_tile_cima(obj3d_handle* o, uint16_t coluna, uint16_t linha) {
    if (!o || !tem_tres_partes(o)) return;
    const int i = o->forma == OBJ3D_CUBO ? 2 : 1;
    o->partes[i].col = coluna; o->partes[i].lin = linha;
}

void obj3d_tile_baixo(obj3d_handle* o, uint16_t coluna, uint16_t linha) {
    if (!o || !tem_tres_partes(o)) return;
    const int i = o->forma == OBJ3D_CUBO ? 3 : 2;
    o->partes[i].col = coluna; o->partes[i].lin = linha;
}

static void tile_face_cubo(obj3d_handle* o, int parte, uint16_t coluna, uint16_t linha) {
    if (!o || o->forma != OBJ3D_CUBO) return;
    o->partes[parte].col = coluna;
    o->partes[parte].lin = linha;
}

void obj3d_tile_frente(obj3d_handle* o, uint16_t coluna, uint16_t linha) { tile_face_cubo(o, 0, coluna, linha); }
void obj3d_tile_tras(obj3d_handle* o, uint16_t coluna, uint16_t linha) { tile_face_cubo(o, 1, coluna, linha); }
void obj3d_tile_esquerda(obj3d_handle* o, uint16_t coluna, uint16_t linha) { tile_face_cubo(o, 4, coluna, linha); }
void obj3d_tile_direita(obj3d_handle* o, uint16_t coluna, uint16_t linha) { tile_face_cubo(o, 5, coluna, linha); }

static void dens_parte(obj3d_parte* p, float u, float v, int repetir) {
    p->dens_u = rep_ok(u);
    p->dens_v = rep_ok(v);
    p->repetir = repetir ? 1 : 0;
}

void obj3d_repetir(obj3d_handle* o, float rep_largura, float rep_altura, int repetir) {
    if (!o) return;
    for (int i = 0; i < o->n_partes; i++) dens_parte(&o->partes[i], rep_largura, rep_altura, repetir);
    atualizar_rep(o);
}

void obj3d_repetir_lado(obj3d_handle* o, float rep_largura, float rep_altura, int repetir) {
    if (!o || !tem_tres_partes(o)) return;
    if (o->forma == OBJ3D_CUBO) {
        const int lados[] = {0, 1, 4, 5};
        for (int i = 0; i < 4; i++) dens_parte(&o->partes[lados[i]], rep_largura, rep_altura, repetir);
    } else dens_parte(&o->partes[0], rep_largura, rep_altura, repetir);
    atualizar_rep(o);
}

void obj3d_repetir_cima(obj3d_handle* o, float rep_largura, float rep_altura, int repetir) {
    if (!o || !tem_tres_partes(o)) return;
    const int i = o->forma == OBJ3D_CUBO ? 2 : 1;
    dens_parte(&o->partes[i], rep_largura, rep_altura, repetir);
    atualizar_rep(o);
}

void obj3d_repetir_baixo(obj3d_handle* o, float rep_largura, float rep_altura, int repetir) {
    if (!o || !tem_tres_partes(o)) return;
    const int i = o->forma == OBJ3D_CUBO ? 3 : 2;
    dens_parte(&o->partes[i], rep_largura, rep_altura, repetir);
    atualizar_rep(o);
}

void obj3d_parte_repetir(obj3d_handle* o, int i, float rep_largura, float rep_altura, int repetir) {
    if (!o || i < 0 || i >= o->n_partes) return;
    dens_parte(&o->partes[i], rep_largura, rep_altura, repetir);
    atualizar_rep(o);
}

void obj3d_cor(obj3d_handle* o, uint32_t cor) {
    if (!o) return;
    o->cor = cor;
    for (int i = 0; i < o->n_partes; i++) {
        obj3d_parte* p = &o->partes[i];
        for (int k = 0; k < p->n_vertices; k++) p->vertices[k].cor = cor;
    }
}

void obj3d_modo(obj3d_handle* o, int modo)       { if (o) o->modo = modo; }
void obj3d_cull(obj3d_handle* o, int cull)       { if (o) o->cull = cull; }
void obj3d_visivel(obj3d_handle* o, int visivel) { if (o) o->visivel = visivel != 0; }

int obj3d_partes(const obj3d_handle* o) { return o ? o->n_partes : 0; }

const char* obj3d_parte_nome(const obj3d_handle* o, int i) {
    if (!o || i < 0 || i >= o->n_partes) return "";
    return o->partes[i].nome;
}

int obj3d_parte_buscar(const obj3d_handle* o, const char* nome) {
    if (!o || !nome) return -1;
    for (int i = 0; i < o->n_partes; i++)
        if (txt_comp(o->partes[i].nome, nome) == 0) return i;
    return -1;
}

void obj3d_parte_visivel(obj3d_handle* o, int i, int visivel) {
    if (!o || i < 0 || i >= o->n_partes) return;
    o->partes[i].visivel = visivel != 0;
}

void obj3d_parte_tile(obj3d_handle* o, int i, uint16_t coluna, uint16_t linha) {
    if (!o || i < 0 || i >= o->n_partes) return;
    o->partes[i].col = coluna;
    o->partes[i].lin = linha;
}

zb_mat4 obj3d_matriz(const obj3d_handle* o) {
    if (!o) return zb_mat4_identidade();
    const zb_mat4 t  = zb_mat4_translacao(o->x, o->y, o->z);
    const zb_mat4 rx = zb_mat4_rotacao_x(o->rx);
    const zb_mat4 ry = zb_mat4_rotacao_y(o->ry);
    const zb_mat4 rz = zb_mat4_rotacao_z(o->rz);
    const zb_mat4 s  = zb_mat4_escala(o->sx, o->sy, o->sz);
    zb_mat4 r = zb_mat4_multiplicar(&rz, &ry);
    r = zb_mat4_multiplicar(&r, &rx);
    zb_mat4 m = zb_mat4_multiplicar(&t, &r);
    return zb_mat4_multiplicar(&m, &s);
}

static int camera_de_vp(const zb_mat4* vp, float c[3]) {
    const float* m = vp->m;
    const float a[3] = { m[0],  m[1],  m[2]  };
    const float b[3] = { m[4],  m[5],  m[6]  };
    const float d[3] = { m[12], m[13], m[14] };

    const float bd[3] = { b[1] * d[2] - b[2] * d[1], b[2] * d[0] - b[0] * d[2], b[0] * d[1] - b[1] * d[0] };
    const float da[3] = { d[1] * a[2] - d[2] * a[1], d[2] * a[0] - d[0] * a[2], d[0] * a[1] - d[1] * a[0] };
    const float ab[3] = { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };

    const float det = a[0] * bd[0] + a[1] * bd[1] + a[2] * bd[2];
    if (valor_abs(det) < 1e-12f) return 0;

    const float ka = -m[3], kb = -m[7], kd = -m[15];
    const float inv = 1.0f / det;
    for (int i = 0; i < 3; i++) c[i] = (ka * bd[i] + kb * da[i] + kd * ab[i]) * inv;
    return 1;
}

static zb_mat4 matriz_billboard(const obj3d_handle* o, const zb_mat4* vp) {
    float c[3];
    if (!camera_de_vp(vp, c)) return obj3d_matriz(o);

    float f[3] = { c[0] - o->x, c[1] - o->y, c[2] - o->z };
    if (o->billboard == OBJ3D_BB_CILINDRICO) f[1] = 0.0f;
    const float fl = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    if (fl < 1e-6f) return obj3d_matriz(o);
    f[0] /= fl; f[1] /= fl; f[2] /= fl;

    float r[3] = { f[2], 0.0f, -f[0] };
    const float rl = sqrtf(r[0] * r[0] + r[2] * r[2]);
    if (rl < 1e-6f) { r[0] = 1.0f; r[2] = 0.0f; }
    else { r[0] /= rl; r[2] /= rl; }

    float u[3] = {
        f[1] * r[2] - f[2] * r[1],
        f[2] * r[0] - f[0] * r[2],
        f[0] * r[1] - f[1] * r[0]
    };

    if (o->rz != 0.0f) {
        const float cr = cs(o->rz), sr = sn(o->rz);
        for (int i = 0; i < 3; i++) {
            const float ri = r[i] * cr + u[i] * sr;
            const float ui = u[i] * cr - r[i] * sr;
            r[i] = ri; u[i] = ui;
        }
    }

    zb_mat4 m = zb_mat4_identidade();
    for (int i = 0; i < 3; i++) {
        m.m[i * 4 + 0] = r[i] * o->sx;
        m.m[i * 4 + 1] = u[i] * o->sy;
        m.m[i * 4 + 2] = f[i] * o->sz;
    }
    m.m[3] = o->x; m.m[7] = o->y; m.m[11] = o->z;
    return m;
}

void obj3d_desenhar(const obj3d_handle* o, fb_handle* fb, zb_handle* zb, const zb_mat4* vp) {
    if (!o || !fb || !zb || !vp || !o->visivel) return;

    const zb_mat4 modelo = o->billboard ? matriz_billboard(o, vp) : obj3d_matriz(o);
    const zb_mat4 mvp = zb_mat4_multiplicar(vp, &modelo);

    zb_estado e = zb_estado_padrao();
    e.modo = o->modo;
    e.cull = o->cull;
    e.atlas = o->atlas;
    
    for (int i = 0; i < o->n_partes; i++) {
        const obj3d_parte* p = &o->partes[i];
        if (!p->visivel) continue;
        zb_malha malha;
        malha.vertices = p->vertices;
        malha.n_vertices = p->n_vertices;
        malha.indices = p->indices;
        malha.n_indices = p->n_indices;
        e.tile_x = p->col;
        e.tile_y = p->lin;
        e.rep_u = p->rep_u > 0.0f ? p->rep_u : 1.0f;
        e.rep_v = p->rep_v > 0.0f ? p->rep_v : 1.0f;
        zb_malha_desenhar(fb, zb, &e, &mvp, &malha);
    }
}

void obj3d_recalcular_limites(obj3d_handle* o) {
    if (!o) return;
    float mn[3] = { 1e30f, 1e30f, 1e30f }, mx[3] = { -1e30f, -1e30f, -1e30f };
    int tem = 0;
    for (int i = 0; i < o->n_partes; i++) {
        const obj3d_parte* p = &o->partes[i];
        for (int k = 0; k < p->n_vertices; k++) {
            const float v[3] = { p->vertices[k].x, p->vertices[k].y, p->vertices[k].z };
            for (int a = 0; a < 3; a++) {
                if (v[a] < mn[a]) mn[a] = v[a];
                if (v[a] > mx[a]) mx[a] = v[a];
            }
            tem = 1;
        }
    }
    for (int a = 0; a < 3; a++) {
        o->bmin[a] = tem ? mn[a] : -0.5f;
        o->bmax[a] = tem ? mx[a] :  0.5f;
    }
}

static float absf(float v) { return v < 0.0f ? -v : v; }

void obj3d_colisao(obj3d_handle* o, int tipo) {
    if (o) o->colisao = tipo ? OBJ3D_COL_CAIXA : OBJ3D_COL_NENHUMA;
}

void obj3d_colisao_raio(obj3d_handle* o, float raio) {
    if (o) o->col_raio = raio > 0.0f ? raio : 0.0f;
}

int obj3d_cilindro_mundo(const obj3d_handle* o, obj3d_cil* s) {
    if (!o || !s || o->forma != OBJ3D_BILLBOARD) return 0;
    const float meia = 0.5f * valor_abs(o->sy);
    s->x  = o->x;
    s->z  = o->z;
    s->y0 = o->y - meia;
    s->y1 = o->y + meia;
    s->r  = o->col_raio > 0.0f ? o->col_raio : 0.5f * valor_abs(o->sx);
    return 1;
}

int obj3d_caixa_mundo(const obj3d_handle* o, obj3d_caixa* b) {
    if (!o || !b) return 0;

    obj3d_cil cil;
    if (obj3d_cilindro_mundo(o, &cil)) {
        b->c[0] = cil.x;
        b->c[1] = 0.5f * (cil.y0 + cil.y1);
        b->c[2] = cil.z;
        b->e[0] = cil.r;
        b->e[1] = 0.5f * (cil.y1 - cil.y0);
        b->e[2] = cil.r;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) b->u[i][j] = i == j ? 1.0f : 0.0f;
        return 1;
    }

    const zb_mat4 rx = zb_mat4_rotacao_x(o->rx);
    const zb_mat4 ry = zb_mat4_rotacao_y(o->ry);
    const zb_mat4 rz = zb_mat4_rotacao_z(o->rz);
    zb_mat4 r = zb_mat4_multiplicar(&rz, &ry);
    r = zb_mat4_multiplicar(&r, &rx);

    const float s[3] = { o->sx, o->sy, o->sz };

    float cl[3];
    for (int i = 0; i < 3; i++) {
        cl[i]   = 0.5f * (o->bmin[i] + o->bmax[i]) * s[i];
        b->e[i] = 0.5f * (o->bmax[i] - o->bmin[i]) * absf(s[i]);
    }

    for (int j = 0; j < 3; j++)
        b->c[j] = (&o->x)[j] + r.m[j * 4] * cl[0] + r.m[j * 4 + 1] * cl[1] + r.m[j * 4 + 2] * cl[2];

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            b->u[i][j] = r.m[j * 4 + i];

    return 1;
}
