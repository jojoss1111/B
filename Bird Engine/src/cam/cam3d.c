#include "cam3d.h"

#define PI_F 3.14159265f

static float absf(float v)                        { return v < 0.0f ? -v : v; }
static float minf(float a, float b)               { return a < b ? a : b; }
static float maxf(float a, float b)               { return a > b ? a : b; }
static float limitar(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static float raiz(float x) {
    if (x <= 0.0f) return 0.0f;
    union { float f; uint32_t i; } u;
    u.f = x;
    u.i = 0x1FBD1DF5u + (u.i >> 1);
    float y = u.f;
    y = 0.5f * (y + x / y);
    y = 0.5f * (y + x / y);
    return y;
}

static float atan_poly(float z) {
    const float z2 = z * z;
    return z * (0.9998660f + z2 * (-0.3302995f + z2 * (0.1801410f + z2 * (-0.0851330f + z2 * 0.0208351f))));
}

static float atan2_graus(float y, float x) {
    if (x == 0.0f && y == 0.0f) return 0.0f;
    const float ax = absf(x), ay = absf(y);
    float a = ax >= ay ? atan_poly(ay / ax) : (PI_F * 0.5f - atan_poly(ax / ay));
    if (x < 0.0f) a = PI_F - a;
    if (y < 0.0f) a = -a;
    return a * (180.0f / PI_F);
}

static float envolver(float g) {
    if (g > 1e6f || g < -1e6f) return 0.0f;
    if (g >= 360.0f || g < 0.0f) {
        g -= 360.0f * (float)(int)(g / 360.0f);
        if (g < 0.0f) g += 360.0f;
    }
    return g;
}

static float sn(float g) { return (float)fb_seno(fb_graus(envolver(g))) * (1.0f / 32768.0f); }
static float cs(float g) { return (float)fb_cosseno(fb_graus(envolver(g))) * (1.0f / 32768.0f); }

static float aleatorio(uint32_t* s) {
    *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5;
    return (float)(int32_t)*s / 2147483648.0f;
}

static float dot3(const float* a, const float* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

static void cruz3(const float* a, const float* b, float* o) {
    o[0] = a[1] * b[2] - a[2] * b[1];
    o[1] = a[2] * b[0] - a[0] * b[2];
    o[2] = a[0] * b[1] - a[1] * b[0];
}

static int col_sat(const obj3d_caixa* a, const obj3d_caixa* b, float* n, float* prof) {
    const float d[3] = { a->c[0] - b->c[0], a->c[1] - b->c[1], a->c[2] - b->c[2] };
    float melhor = 1e30f;
    *prof = 0.0f;
    n[0] = 0.0f; n[1] = 1.0f; n[2] = 0.0f;

    for (int k = 0; k < 15; k++) {
        float L[3];
        int cruzado = 0;
        if (k < 3) {
            L[0] = a->u[k][0]; L[1] = a->u[k][1]; L[2] = a->u[k][2];
        } else if (k < 6) {
            L[0] = b->u[k - 3][0]; L[1] = b->u[k - 3][1]; L[2] = b->u[k - 3][2];
        } else {
            cruz3(a->u[(k - 6) / 3], b->u[(k - 6) % 3], L);
            const float l2 = dot3(L, L);
            if (l2 < 1e-4f) continue;
            const float inv = 1.0f / raiz(l2);
            L[0] *= inv; L[1] *= inv; L[2] *= inv;
            cruzado = 1;
        }

        const float ra = absf(dot3(a->u[0], L)) * a->e[0] + absf(dot3(a->u[1], L)) * a->e[1] + absf(dot3(a->u[2], L)) * a->e[2];
        const float rb = absf(dot3(b->u[0], L)) * b->e[0] + absf(dot3(b->u[1], L)) * b->e[1] + absf(dot3(b->u[2], L)) * b->e[2];
        const float dist = dot3(d, L);
        const float ov = ra + rb - absf(dist);
        if (ov <= 0.0f) return 0;

        const float peso = cruzado ? ov * 1.05f + 0.002f : ov;
        if (peso < melhor) {
            melhor = peso;
            *prof = ov;
            const float sg = dist >= 0.0f ? 1.0f : -1.0f;
            n[0] = L[0] * sg; n[1] = L[1] * sg; n[2] = L[2] * sg;
        }
    }
    return 1;
}

static float topo_da_caixa(const obj3d_caixa* b) {
    return b->c[1] + absf(b->u[0][1]) * b->e[0] + absf(b->u[1][1]) * b->e[1] + absf(b->u[2][1]) * b->e[2];
}

static void luz_padrao(cam3d_luz* l, int tipo, float intensidade, float alcance) {
    l->usada = 1; l->ativa = 0; l->tipo = tipo; l->sombra = 0;
    l->x = l->y = l->z = 0.0f;
    l->dx = 0.0f; l->dy = -1.0f; l->dz = 0.0f;
    l->cor_r = l->cor_g = l->cor_b = 1.0f;
    l->intensidade = intensidade;
    l->alcance = alcance;
    l->cone_interno = 20.0f;
    l->cone_externo = 35.0f;
    l->desloc_y = 0.0f;
}

void cam3d_iniciar(cam3d_handle* c, float x, float y, float z, float angulo_x, float angulo_y, float angulo_z) {
    memoria_zerar(c, sizeof(*c));
    c->olho = 1.6f;
    c->olho_agach = 1.0f;
    c->olho_atual = c->olho;
    c->alt = 1.8f;
    c->raio = 0.3f;
    c->tipo_col = CAM3D_COL_AABB;
    c->grav = 25.0f;
    c->pulo = 8.0f;
    c->degrau = 0.4f;
    c->mult_correr = 1.6f;
    c->mult_agachar = 0.5f;
    c->mult_atual = 1.0f;
    c->bob_amp = 0.05f;
    c->bob_freq = 10.0f;
    c->rec_vel = 8.0f;
    c->rng = 2463534242u;
    c->chao_ativo = 1;
    c->chao_y = 0.0f;
    c->ceu_y = 100.0f;
    c->fov = 70.0f;
    c->perto = 0.05f;
    c->longe = 1000.0f;
    c->aspecto = 4.0f / 3.0f;
    c->pitch = limitar(angulo_x, -89.0f, 89.0f);
    c->yaw = envolver(angulo_y);
    c->roll = angulo_z;
    c->ambiente = 1.0f;
    c->sombras = 1;
    luz_padrao(&c->luzes[CAM3D_LUZ_AURA], CAM3D_LUZ_PONTO, 1.0f, 8.0f);
    luz_padrao(&c->luzes[CAM3D_LUZ_LANTERNA], CAM3D_LUZ_SPOT, 1.0f, 20.0f);
    c->luzes[CAM3D_LUZ_LANTERNA].cone_interno = 12.0f;
    c->luzes[CAM3D_LUZ_LANTERNA].cone_externo = 25.0f;
    cam3d_posicionar(c, x, y, z);
}

void cam3d_liberar(cam3d_handle* c) {
    if (!c) return;
    if (c->caixas) memoria_free(c->caixas);
    c->caixas = NULL;
    c->n_caixas = c->cap_caixas = 0;
    c->mundo = NULL;
    c->n_mundo = 0;
    if (c->sombristas) memoria_free(c->sombristas);
    if (c->cand) memoria_free(c->cand);
    c->sombristas = NULL;
    c->cand = NULL;
    c->n_sombristas = c->cap_sombristas = c->cap_cand = 0;
}

cam3d_handle* cam3d_criar(float x, float y, float z, float angulo_x, float angulo_y, float angulo_z) {
    cam3d_handle* c = (cam3d_handle*)memoria_malloc(sizeof(cam3d_handle));
    if (!c) return NULL;
    cam3d_iniciar(c, x, y, z, angulo_x, angulo_y, angulo_z);
    return c;
}

void cam3d_destruir(cam3d_handle* c) {
    if (!c) return;
    cam3d_liberar(c);
    memoria_free(c);
}

void cam3d_lente(cam3d_handle* c, float fov, float perto, float longe) {
    if (!c) return;
    c->fov = limitar(fov, 10.0f, 150.0f);
    c->perto = maxf(perto, 0.01f);
    c->longe = maxf(longe, c->perto + 0.01f);
}

void cam3d_fov(cam3d_handle* c, float graus)       { if (c) c->fov = limitar(graus, 10.0f, 150.0f); }
void cam3d_aspecto(cam3d_handle* c, float aspecto) { if (c && aspecto > 0.0f) c->aspecto = aspecto; }

void cam3d_chao(cam3d_handle* c, int ativo)        { if (c) c->chao_ativo = ativo != 0; }
void cam3d_definir_chao(cam3d_handle* c, float y)  { if (c) c->chao_y = y; }
void cam3d_ceu(cam3d_handle* c, int ativo)         { if (c) c->ceu_ativo = ativo != 0; }
void cam3d_maximo_ceu(cam3d_handle* c, float y)    { if (c) { c->ceu_y = y; c->ceu_ativo = 1; } }

void cam3d_definir_corpo(cam3d_handle* c, float altura_olhos, float olhos_agachado, float altura_colisao, float raio, int tipo_colisao) {
    if (!c) return;
    c->olho = maxf(altura_olhos, 0.05f);
    c->olho_agach = limitar(olhos_agachado, 0.05f, c->olho);
    c->alt = maxf(altura_colisao, 0.1f);
    c->raio = maxf(raio, 0.02f);
    c->tipo_col = tipo_colisao ? CAM3D_COL_OBB : CAM3D_COL_AABB;
    c->olho_atual = c->agachado_real ? c->olho_agach : c->olho;
}

void cam3d_fisica(cam3d_handle* c, float gravidade, float forca_pulo) { if (c) { c->grav = gravidade; c->pulo = forca_pulo; } }
void cam3d_degrau(cam3d_handle* c, float altura_maxima)               { if (c) c->degrau = maxf(altura_maxima, 0.0f); }
void cam3d_multiplicadores(cam3d_handle* c, float correr, float agachar) { if (c) { c->mult_correr = correr; c->mult_agachar = agachar; } }

void cam3d_balanco(cam3d_handle* c, int ligado, float amplitude, float frequencia) {
    if (!c) return;
    c->bob_ativo = ligado != 0;
    c->bob_amp = amplitude;
    c->bob_freq = frequencia;
}

void cam3d_recuperacao(cam3d_handle* c, float velocidade) { if (c) c->rec_vel = maxf(velocidade, 0.0f); }

static float altura_olho(const cam3d_handle* c) { return c->olho_atual; }

void cam3d_posicionar(cam3d_handle* c, float x, float y, float z) {
    if (!c) return;
    c->px = x;
    c->pz = z;
    c->py = y - altura_olho(c);
    c->vy = 0.0f;
}

void cam3d_mover(cam3d_handle* c, float x, float y, float z) {
    if (!c) return;
    c->in_x += x; c->in_y += y; c->in_z += z;
}

void cam3d_girar(cam3d_handle* c, float x, float y, float z) {
    if (!c) return;
    c->pitch = limitar(c->pitch + x, -89.0f, 89.0f);
    c->yaw = envolver(c->yaw + y);
    c->roll = envolver(c->roll + z);
    if (c->roll > 180.0f) c->roll -= 360.0f;
}

void cam3d_angulo(cam3d_handle* c, float x, float y, float z) {
    if (!c) return;
    c->pitch = limitar(x, -89.0f, 89.0f);
    c->yaw = envolver(y);
    c->roll = z;
}

void cam3d_pular(cam3d_handle* c) {
    if (!c) return;
    if (c->grav != 0.0f && c->no_chao) {
        c->vy = c->pulo;
        c->no_chao = 0;
    }
}

void cam3d_agachar(cam3d_handle* c, int ligado) { if (c) c->agachado = ligado != 0; }
void cam3d_correr(cam3d_handle* c, int ligado)  { if (c) c->correndo = ligado != 0; }

void cam3d_olhar_para(cam3d_handle* c, float x, float y, float z) {
    if (!c) return;
    const float dx = x - cam3d_x(c), dy = y - cam3d_y(c), dz = z - cam3d_z(c);
    const float h = raiz(dx * dx + dz * dz);
    if (h < 1e-5f && absf(dy) < 1e-5f) return;
    if (h > 1e-5f) c->yaw = envolver(atan2_graus(dx, -dz));
    c->pitch = limitar(atan2_graus(dy, h), -89.0f, 89.0f);
}

void cam3d_tremer(cam3d_handle* c, float intensidade, float duracao) {
    if (!c || duracao <= 0.0f) return;
    c->trem_int = intensidade;
    c->trem_dur = c->trem_t = duracao;
}

void cam3d_recuo(cam3d_handle* c, float pitch, float yaw) {
    if (!c) return;
    c->rec_pitch += pitch;
    c->rec_yaw += yaw;
}

void cam3d_colisao_mundo(cam3d_handle* c, obj3d_handle* const* objs, int n) {
    if (!c) return;
    c->mundo = objs;
    c->n_mundo = n < 0 ? 0 : n;
}

static void reunir_caixas(cam3d_handle* c) {
    c->n_caixas = 0;
    if (!c->mundo || c->n_mundo <= 0) return;

    if (c->n_mundo > c->cap_caixas) {
        if (c->caixas) memoria_free(c->caixas);
        c->caixas = (obj3d_caixa*)memoria_malloc((size_t)c->n_mundo * sizeof(obj3d_caixa));
        c->cap_caixas = c->caixas ? c->n_mundo : 0;
        if (!c->caixas) return;
    }

    for (int i = 0; i < c->n_mundo; i++) {
        const obj3d_handle* o = c->mundo[i];
        if (!o || !o->visivel || o->colisao == OBJ3D_COL_NENHUMA) continue;
        
        if (o->forma == OBJ3D_POLIGONO || o->forma == OBJ3D_BILLBOARD) continue;

        if (obj3d_caixa_mundo(o, &c->caixas[c->n_caixas])) c->n_caixas++;
    }
}

static float altura_corpo(const cam3d_handle* c) {
    const float h = c->agachado_real ? c->alt - (c->olho - c->olho_agach) : c->alt;
    return maxf(h, 0.1f);
}

static void montar_corpo(const cam3d_handle* c, float x, float y, float z, float h, float folga, obj3d_caixa* o) {
    const float hh = maxf(h - folga, 0.02f) * 0.5f;
    const float r = maxf(c->raio - folga, 0.005f);
    o->c[0] = x; o->c[1] = y + folga + hh; o->c[2] = z;
    o->e[0] = r; o->e[1] = hh; o->e[2] = r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) o->u[i][j] = i == j ? 1.0f : 0.0f;
    if (c->tipo_col == CAM3D_COL_OBB) {
        const float s = sn(c->yaw), co = cs(c->yaw);
        o->u[0][0] = co;  o->u[0][2] = s;
        o->u[2][0] = -s;  o->u[2][2] = co;
    }
}

static int colide_em(const cam3d_handle* c, float x, float y, float z, float h, float folga) {
    obj3d_caixa corpo;
    float n[3], prof;
    montar_corpo(c, x, y, z, h, folga, &corpo);
    for (int i = 0; i < c->n_caixas; i++)
        if (col_sat(&corpo, &c->caixas[i], n, &prof)) return 1;
    return 0;
}

static void poli_ponto_mundo(const zb_mat4* m, const obj3d_handle* o, int i, float* x, float* z) {
    float lx, lz;
    obj3d_poligono_ler(o, i, &lx, &lz);
    *x = m->m[0] * lx + m->m[2] * lz + m->m[3];
    *z = m->m[8] * lx + m->m[10] * lz + m->m[11];
}

static int poli_proximo(const obj3d_handle* o, const zb_mat4* m, int n, float px, float pz, float* qx, float* qz, float* d2) {
    int dentro = 0;
    float melhor = 1e30f;
    float x2, z2;
    poli_ponto_mundo(m, o, n - 1, &x2, &z2);

    for (int i = 0; i < n; i++) {
        const float x1 = x2, z1 = z2;
        poli_ponto_mundo(m, o, i, &x2, &z2);

        const float vx = x2 - x1, vz = z2 - z1;
        if (((z1 > pz) != (z2 > pz)) && (px < vx * (pz - z1) / (z2 - z1) + x1)) dentro = !dentro;

        const float len2 = vx * vx + vz * vz;
        const float t = len2 > 1e-12f ? limitar(((px - x1) * vx + (pz - z1) * vz) / len2, 0.0f, 1.0f) : 0.0f;
        const float cx = x1 + t * vx, cz = z1 + t * vz;
        const float dx = px - cx, dz = pz - cz;
        const float d = dx * dx + dz * dz;
        if (d < melhor) { melhor = d; *qx = cx; *qz = cz; }
    }
    *d2 = melhor;
    return dentro;
}

static int resolver_poligono(cam3d_handle* c, const obj3d_handle* o, float h) {
    const int n = obj3d_poligono_n(o);
    if (n < 3) return 0;

    float y0 = o->y, y1 = o->y + o->sy;
    if (y0 > y1) { const float t = y0; y0 = y1; y1 = t; }

    const float pb = c->py, pa = c->py + h;
    if (pa <= y0 || pb - y1 > 0.1f) return 0;

    const zb_mat4 mat = obj3d_matriz(o);
    float qx = 0.0f, qz = 0.0f, d2 = 0.0f;
    const int dentro = poli_proximo(o, &mat, n, c->px, c->pz, &qx, &qz, &d2);
    const float r = c->raio;
    if (!dentro && d2 >= r * r) return 0;

    const float sobe = y1 - pb;
    const int cruzou_topo = c->py_antes_passo >= y1 - 0.001f && pb <= y1 + 0.001f;
    /*
     * No primeiro quadro o spawn pode deixar os pes dentro da espessura do
     * poligono. Se o corpo ainda atravessa o topo, pousa nele antes da
     * resolucao lateral empurrar o jogador para fora do contorno.
     */
    const int spawn_embutido = !c->no_chao_antes && pb < y1 && pa > y0;
    if (sobe > -0.1f && c->vy <= 0.5f &&
        (sobe <= 0.3f || cruzou_topo || spawn_embutido ||
         (c->degrau > 0.0f && c->no_chao_antes && sobe <= c->degrau))) {
        const int mudou = sobe > 1e-5f;
        if (mudou && c->no_chao_antes) c->suave_y -= sobe;
        c->py = y1;
        c->no_chao = 1;
        if (c->vy < 0.0f) c->vy = 0.0f;
        return mudou;
    }

    if (dentro && pb < y0 && c->vy > 0.0f) {
        c->py = y0 - h - 0.001f;
        c->vy = 0.0f;
        return 1;
    }

    const float d = raiz(d2);
    float nx, nz;

    if (!dentro && d > 1e-5f) {
        nx = (c->px - qx) / d;
        nz = (c->pz - qz) / d;
        const float p = r - d + 0.001f;
        c->px += nx * p;
        c->pz += nz * p;
        return 1;
    }

    if (d > 1e-5f) {
        nx = (qx - c->px) / d;
        nz = (qz - c->pz) / d;
    } else {
        float mx = 0.0f, mz = 0.0f;
        for (int i = 0; i < n; i++) {
            float vx, vz;
            poli_ponto_mundo(&mat, o, i, &vx, &vz);
            mx += vx; mz += vz;
        }
        mx /= (float)n; mz /= (float)n;
        nx = c->px - mx;
        nz = c->pz - mz;
        const float l = raiz(nx * nx + nz * nz);
        if (l > 1e-5f) { nx /= l; nz /= l; }
        else { nx = 1.0f; nz = 0.0f; }
    }
    c->px = qx + nx * (r + 0.001f);
    c->pz = qz + nz * (r + 0.001f);
    return 1;
}

static int resolver_billboard(cam3d_handle* c, const obj3d_handle* o, float h) {
    obj3d_cil s;
    if (!obj3d_cilindro_mundo(o, &s)) return 0;

    const float pb = c->py, pa = c->py + h;
    if (pa <= s.y0 || pb >= s.y1) return 0;

    const float R = s.r + c->raio;
    const float dx = c->px - s.x, dz = c->pz - s.z;
    const float d2 = dx * dx + dz * dz;
    if (d2 >= R * R) return 0;

    const float d = raiz(d2);
    float nx = 1.0f, nz = 0.0f;
    if (d > 1e-5f) { nx = dx / d; nz = dz / d; }

    const float p = R - d + 0.001f;
    c->px += nx * p;
    c->pz += nz * p;
    return 1;
}

static void resolver(cam3d_handle* c, float h) {
    for (int it = 0; it < 4; it++) {
        int mexeu = 0;

        for (int i = 0; i < c->n_caixas; i++) {
            obj3d_caixa corpo;
            float n[3], prof;
            montar_corpo(c, c->px, c->py, c->pz, h, 0.0f, &corpo);
            if (!col_sat(&corpo, &c->caixas[i], n, &prof)) continue;

            if (c->degrau > 0.0f && c->no_chao_antes && absf(n[1]) < 0.5f) {
                const float topo = topo_da_caixa(&c->caixas[i]);
                const float sobe = topo - c->py;
                if (sobe > 0.0f && sobe <= c->degrau && !colide_em(c, c->px, topo + 0.01f, c->pz, h, 0.03f)) {
                    c->py = topo + 0.01f;
                    c->suave_y -= sobe;
                    c->no_chao = 1;
                    if (c->vy < 0.0f) c->vy = 0.0f;
                    mexeu = 1;
                    continue;
                }
            }

            const float p = prof + 0.001f;
            c->px += n[0] * p;
            c->py += n[1] * p;
            c->pz += n[2] * p;

            if (n[1] > 0.5f)       { c->no_chao = 1; if (c->vy < 0.0f) c->vy = 0.0f; }
            else if (n[1] < -0.5f) { if (c->vy > 0.0f) c->vy = 0.0f; }
            mexeu = 1;
        }

        if (c->mundo) {
            for (int i = 0; i < c->n_mundo; i++) {
                const obj3d_handle* o = c->mundo[i];
                if (!o || !o->visivel || o->colisao == OBJ3D_COL_NENHUMA) continue;
                if (o->forma == OBJ3D_POLIGONO) { if (resolver_poligono(c, o, h)) mexeu = 1; }
                else if (o->forma == OBJ3D_BILLBOARD) { if (resolver_billboard(c, o, h)) mexeu = 1; }
            }
        }

        if (!mexeu) break;
    }
}

static void atualizar_corpo(cam3d_handle* c, float dt) {
    reunir_caixas(c);

    if (c->agachado) c->agachado_real = 1;
    else if (c->agachado_real && !colide_em(c, c->px, c->py, c->pz, c->alt, 0.03f)) c->agachado_real = 0;

    const float h = altura_corpo(c);

    c->mult_atual = c->agachado_real ? c->mult_agachar : (c->correndo ? c->mult_correr : 1.0f);

    const float sy = sn(c->yaw), cy = cs(c->yaw);
    const float dx = (c->in_x * cy + c->in_z * sy) * c->mult_atual * dt;
    const float dz = (c->in_x * sy - c->in_z * cy) * c->mult_atual * dt;
    float dy;

    if (c->grav != 0.0f) {
        c->vy -= c->grav * dt;
        if (c->vy < -60.0f) c->vy = -60.0f;
        dy = c->vy * dt;
    } else {
        c->vy = 0.0f;
        dy = c->in_y * c->mult_atual * dt;
    }
    c->vel_h = raiz(dx * dx + dz * dz) / dt;

    const float desl = maxf(maxf(absf(dx), absf(dz)), absf(dy));
    const float passo = maxf(c->raio * 0.5f, 0.05f);
    int passos = 1 + (int)(desl / passo);
    if (passos > 32) passos = 32;

    c->no_chao_antes = c->no_chao;
    c->no_chao = 0;

    for (int s = 0; s < passos; s++) {
        c->py_antes_passo = c->py;
        c->px += dx / (float)passos;
        c->py += dy / (float)passos;
        c->pz += dz / (float)passos;

        resolver(c, h);

        if (c->chao_ativo && c->py < c->chao_y) {
            c->py = c->chao_y;
            if (c->vy < 0.0f) c->vy = 0.0f;
            c->no_chao = 1;
        }
        if (c->ceu_ativo && c->py + h > c->ceu_y) {
            c->py = c->ceu_y - h;
            if (c->vy > 0.0f) c->vy = 0.0f;
        }
    }
}

static void efeitos(cam3d_handle* c, float dt) {
    const float alvo = c->agachado_real ? c->olho_agach : c->olho;
    c->olho_atual += limitar(alvo - c->olho_atual, -6.0f * dt, 6.0f * dt);

    if (c->suave_y < 0.0f) { c->suave_y += 4.0f * dt; if (c->suave_y > 0.0f) c->suave_y = 0.0f; }
    else if (c->suave_y > 0.0f) { c->suave_y -= 4.0f * dt; if (c->suave_y < 0.0f) c->suave_y = 0.0f; }

    const int andando = c->bob_ativo && c->no_chao && c->vel_h > 0.1f;
    c->bob_k += ((andando ? 1.0f : 0.0f) - c->bob_k) * minf(1.0f, 10.0f * dt);
    if (andando) {
        c->bob_fase += c->bob_freq * c->mult_atual * dt;
        if (c->bob_fase > 2.0f * PI_F) c->bob_fase -= 2.0f * PI_F;
    }
    c->bob_y = sn(c->bob_fase * (180.0f / PI_F)) * c->bob_amp * c->bob_k;

    const float k = minf(1.0f, c->rec_vel * dt);
    c->rec_pitch -= c->rec_pitch * k;
    c->rec_yaw -= c->rec_yaw * k;
    if (absf(c->rec_pitch) < 0.001f) c->rec_pitch = 0.0f;
    if (absf(c->rec_yaw) < 0.001f) c->rec_yaw = 0.0f;

    if (c->trem_t > 0.0f) {
        c->trem_t -= dt;
        const float f = c->trem_t > 0.0f ? (c->trem_t / c->trem_dur) * c->trem_int : 0.0f;
        c->trem_yaw = aleatorio(&c->rng) * f;
        c->trem_pitch = aleatorio(&c->rng) * f;
        c->trem_roll = aleatorio(&c->rng) * f * 0.5f;
    } else {
        c->trem_yaw = c->trem_pitch = c->trem_roll = 0.0f;
    }
}

void cam3d_atualizar(cam3d_handle* c, float delta) {
    if (!c || !(delta > 0.0f)) return;
    if (delta > 0.1f) delta = 0.1f;
    atualizar_corpo(c, delta);
    efeitos(c, delta);
    c->in_x = c->in_y = c->in_z = 0.0f;
}

float cam3d_x(const cam3d_handle* c)        { return c ? c->px : 0.0f; }
float cam3d_y(const cam3d_handle* c)        { return c ? c->py + altura_olho(c) : 0.0f; }
float cam3d_z(const cam3d_handle* c)        { return c ? c->pz : 0.0f; }
float cam3d_pes_y(const cam3d_handle* c)    { return c ? c->py : 0.0f; }
float cam3d_angulo_x(const cam3d_handle* c) { return c ? c->pitch : 0.0f; }
float cam3d_angulo_y(const cam3d_handle* c) { return c ? c->yaw : 0.0f; }
float cam3d_angulo_z(const cam3d_handle* c) { return c ? c->roll : 0.0f; }
float cam3d_altura_corpo(const cam3d_handle* c) { return c ? altura_corpo(c) : 0.0f; }
float cam3d_velocidade(const cam3d_handle* c)   { return c ? c->vel_h : 0.0f; }
int   cam3d_no_chao(const cam3d_handle* c)      { return c && c->no_chao; }
int   cam3d_agachado(const cam3d_handle* c)     { return c && c->agachado_real; }

void cam3d_frente(const cam3d_handle* c, float* x, float* y, float* z) {
    float fx = 0.0f, fy = 0.0f, fz = -1.0f;
    if (c) {
        const float cp = cs(c->pitch);
        fx = sn(c->yaw) * cp;
        fy = sn(c->pitch);
        fz = -cs(c->yaw) * cp;
    }
    if (x) *x = fx;
    if (y) *y = fy;
    if (z) *z = fz;
}

void cam3d_vista(const cam3d_handle* c, float* ex, float* ey, float* ez, float* yaw, float* pitch, float* roll) {
    if (!c) return;
    if (ex) *ex = c->px;
    if (ey) *ey = c->py + altura_olho(c) + c->suave_y + c->bob_y;
    if (ez) *ez = c->pz;
    if (yaw) *yaw = c->yaw + c->rec_yaw + c->trem_yaw;
    if (pitch) *pitch = limitar(c->pitch + c->rec_pitch + c->trem_pitch, -89.0f, 89.0f);
    if (roll) *roll = c->roll + c->trem_roll;
}

zb_mat4 cam3d_matriz_vista(const cam3d_handle* c) {
    if (!c) return zb_mat4_identidade();

    float ex, ey, ez, yw, pt, rl;
    cam3d_vista(c, &ex, &ey, &ez, &yw, &pt, &rl);

    const float sy = sn(yw), cy = cs(yw), sp = sn(pt), cp = cs(pt), sr = sn(rl), cr = cs(rl);
    const float f[3] = { sy * cp, sp, -cy * cp };
    const float r0[3] = { cy, 0.0f, sy };
    const float u0[3] = { -sy * sp, cp, cy * sp };
    const float r[3] = { r0[0] * cr + u0[0] * sr, r0[1] * cr + u0[1] * sr, r0[2] * cr + u0[2] * sr };
    const float u[3] = { u0[0] * cr - r0[0] * sr, u0[1] * cr - r0[1] * sr, u0[2] * cr - r0[2] * sr };
    const float e[3] = { ex, ey, ez };

    zb_mat4 m = zb_mat4_identidade();
    for (int k = 0; k < 3; k++) {
        m.m[0 * 4 + k] = r[k];
        m.m[1 * 4 + k] = u[k];
        m.m[2 * 4 + k] = -f[k];
    }
    m.m[3]  = -dot3(r, e);
    m.m[7]  = -dot3(u, e);
    m.m[11] = dot3(f, e);
    return m;
}

zb_mat4 cam3d_matriz_projecao(const cam3d_handle* c) {
    if (!c) return zb_mat4_identidade();
    return zb_mat4_perspectiva(c->fov, c->aspecto, c->perto, c->longe);
}

zb_mat4 cam3d_matriz_vp(const cam3d_handle* c) {
    const zb_mat4 p = cam3d_matriz_projecao(c);
    const zb_mat4 v = cam3d_matriz_vista(c);
    return zb_mat4_multiplicar(&p, &v);
}

int cam3d_raio(const cam3d_handle* c, obj3d_handle* const* objs, int n, float alcance, float* distancia) {
    if (!c || !objs || n <= 0) return -1;

    float ex, ey, ez, yw, pt, rl;
    cam3d_vista(c, &ex, &ey, &ez, &yw, &pt, &rl);
    const float cp = cs(pt);
    const float d[3] = { sn(yw) * cp, sn(pt), -cs(yw) * cp };
    const float org[3] = { ex, ey, ez };

    int achou = -1;
    float melhor = alcance;

    for (int i = 0; i < n; i++) {
        const obj3d_handle* o = objs[i];
        obj3d_caixa b;
        if (!o || !o->visivel || !obj3d_caixa_mundo(o, &b)) continue;

        const float oc[3] = { org[0] - b.c[0], org[1] - b.c[1], org[2] - b.c[2] };
        float tn = -1e30f, tf = 1e30f;
        int fora = 0;
        for (int k = 0; k < 3; k++) {
            const float ol = dot3(oc, b.u[k]);
            const float dl = dot3(d, b.u[k]);
            if (absf(dl) < 1e-6f) {
                if (absf(ol) > b.e[k]) { fora = 1; break; }
                continue;
            }
            float t1 = (-b.e[k] - ol) / dl, t2 = (b.e[k] - ol) / dl;
            if (t1 > t2) { const float t = t1; t1 = t2; t2 = t; }
            if (t1 > tn) tn = t1;
            if (t2 < tf) tf = t2;
            if (tn > tf) { fora = 1; break; }
        }
        if (fora || tf < 0.0f) continue;
        const float t = tn < 0.0f ? 0.0f : tn;
        if (t <= melhor) {
            melhor = t;
            achou = i;
        }
    }

    if (achou >= 0 && distancia) *distancia = melhor;
    return achou;
}

void cam3d_neblina(cam3d_handle* c, int ativo, float inicio, float fim, int no_ceu, uint32_t cor, float intensidade) {
    if (!c) return;
    c->neb_ativa = ativo != 0;
    c->neb_ini = maxf(inicio, 0.0f);
    c->neb_fim = maxf(fim, c->neb_ini + 0.01f);
    c->neb_ceu = no_ceu != 0;
    c->neb_cor = cor | 0xFF000000u;
    c->neb_int = limitar(intensidade, 0.0f, 1.0f);
}

static void aplicar_neblina(const cam3d_handle* c, fb_handle* fb, const zb_handle* zb) {
    if (!c->neb_ativa || fb->w <= 0 || fb->h <= 0 || fb->w != zb->w || fb->h != zb->h) return;

    const zb_mat4 P = cam3d_matriz_projecao(c);
    const float m10 = P.m[10], m11 = P.m[11];
    const float ini = c->neb_ini;
    const float inv = 1.0f / (c->neb_fim - ini);
    const float forca = c->neb_int * 256.0f;
    const int   t_ceu = (int)forca;

    const int nr = (int)((c->neb_cor >> 16) & 255u);
    const int ng = (int)((c->neb_cor >> 8) & 255u);
    const int nb = (int)(c->neb_cor & 255u);

    uint32_t* cores = fb_dados(fb);
    const size_t total = (size_t)fb->w * fb->h;

    for (size_t i = 0; i < total; i++) {
        const float z = zb->z[i];
        int t;

        if (z > 1.0f) {
            if (!c->neb_ceu) continue;
            t = t_ceu;
        } else {
            const float den = -z - m10;
            if (den < 1e-7f) continue;

            float f = (-m11 / den - ini) * inv;
            if (f <= 0.0f) continue;
            if (f > 1.0f) f = 1.0f;
            f = f * f * (3.0f - 2.0f * f);
            t = (int)(f * forca);
        }

        if (t <= 0) continue;

        const uint32_t px = cores[i];
        const int r = (int)((px >> 16) & 255u);
        const int g = (int)((px >> 8) & 255u);
        const int b = (int)(px & 255u);

        const uint32_t rr = (uint32_t)(r + (nr - r) * t / 256);
        const uint32_t gg = (uint32_t)(g + (ng - g) * t / 256);
        const uint32_t bb = (uint32_t)(b + (nb - b) * t / 256);
        cores[i] = (px & 0xFF000000u) | (rr << 16) | (gg << 8) | bb;
    }
}

void cam3d_desenhar(cam3d_handle* c, fb_handle* fb, zb_handle* zb, obj3d_handle* const* objs, int n) {
    if (!c || !fb || !zb) return;
    if (fb->w > 0 && fb->h > 0) c->aspecto = (float)fb->w / (float)fb->h;
    zb_limpar(zb);
    if (objs) {
        const zb_mat4 vp = cam3d_matriz_vp(c);
        for (int i = 0; i < n; i++)
            if (objs[i]) obj3d_desenhar(objs[i], fb, zb, &vp);
        cam3d_iluminar(c, fb, zb, objs, n);
    }
    aplicar_neblina(c, fb, zb);
}

/* ======================= luz, aura e sombras ======================= */

#define SOMBRA_FOLGA 0.03f

static cam3d_luz* luz_id(cam3d_handle* c, int id) {
    if (!c || id < 0 || id >= CAM3D_MAX_LUZES || !c->luzes[id].usada) return NULL;
    return &c->luzes[id];
}

static void luz_rgb(cam3d_luz* l, uint32_t cor) {
    l->cor_r = (float)((cor >> 16) & 255u) * (1.0f / 255.0f);
    l->cor_g = (float)((cor >> 8) & 255u) * (1.0f / 255.0f);
    l->cor_b = (float)(cor & 255u) * (1.0f / 255.0f);
}

void cam3d_ambiente(cam3d_handle* c, float valor) { if (c) c->ambiente = maxf(valor, 0.0f); }
void cam3d_sombras(cam3d_handle* c, int ligado)   { if (c) c->sombras = ligado != 0; }

void cam3d_aura(cam3d_handle* c, int ligada, float desloc_y) {
    if (!c) return;
    c->luzes[CAM3D_LUZ_AURA].ativa = ligada != 0;
    c->luzes[CAM3D_LUZ_AURA].desloc_y = desloc_y;
}

void cam3d_lanterna(cam3d_handle* c, int ligada, float desloc_y) {
    if (!c) return;
    c->luzes[CAM3D_LUZ_LANTERNA].ativa = ligada != 0;
    c->luzes[CAM3D_LUZ_LANTERNA].desloc_y = desloc_y;
}

void cam3d_cone_lanterna(cam3d_handle* c, float interno, float externo) {
    if (!c) return;
    cam3d_luz* l = &c->luzes[CAM3D_LUZ_LANTERNA];
    l->cone_externo = limitar(externo, 1.0f, 89.0f);
    l->cone_interno = limitar(interno, 0.0f, l->cone_externo);
}

int cam3d_luz_criar(cam3d_handle* c, float x, float y, float z, uint32_t cor, float intensidade, float alcance) {
    if (!c) return -1;
    for (int i = 2; i < CAM3D_MAX_LUZES; i++) {
        if (c->luzes[i].usada) continue;
        cam3d_luz* l = &c->luzes[i];
        luz_padrao(l, CAM3D_LUZ_PONTO, maxf(intensidade, 0.0f), maxf(alcance, 0.1f));
        l->ativa = 1;
        l->x = x; l->y = y; l->z = z;
        luz_rgb(l, cor);
        return i;
    }
    return -1;
}

void cam3d_luz_remover(cam3d_handle* c, int id) {
    cam3d_luz* l = luz_id(c, id);
    if (!l || id < 2) return;
    l->usada = 0;
    l->ativa = 0;
}

void cam3d_luz_mover(cam3d_handle* c, int id, float x, float y, float z) {
    cam3d_luz* l = luz_id(c, id);
    if (!l || id < 2) return;   /* aura e lanterna seguem a câmera */
    l->x = x; l->y = y; l->z = z;
}

void cam3d_luz_ativa(cam3d_handle* c, int id, int ativa)       { cam3d_luz* l = luz_id(c, id); if (l) l->ativa = ativa != 0; }
void cam3d_luz_cor(cam3d_handle* c, int id, uint32_t cor)      { cam3d_luz* l = luz_id(c, id); if (l) luz_rgb(l, cor); }
void cam3d_luz_intensidade(cam3d_handle* c, int id, float v)   { cam3d_luz* l = luz_id(c, id); if (l) l->intensidade = maxf(v, 0.0f); }
void cam3d_luz_alcance(cam3d_handle* c, int id, float v)       { cam3d_luz* l = luz_id(c, id); if (l) l->alcance = maxf(v, 0.1f); }
void cam3d_luz_sombra(cam3d_handle* c, int id, int ligado)     { cam3d_luz* l = luz_id(c, id); if (l) l->sombra = ligado != 0; }
void cam3d_luz_ponto(cam3d_handle* c, int id)                  { cam3d_luz* l = luz_id(c, id); if (l) l->tipo = CAM3D_LUZ_PONTO; }

void cam3d_luz_spot(cam3d_handle* c, int id, float dx, float dy, float dz, float interno, float externo) {
    cam3d_luz* l = luz_id(c, id);
    if (!l) return;
    const float len2 = dx * dx + dy * dy + dz * dz;
    if (len2 > 1e-8f) {
        const float inv = 1.0f / raiz(len2);
        l->dx = dx * inv; l->dy = dy * inv; l->dz = dz * inv;
    }
    l->cone_externo = limitar(externo, 1.0f, 89.0f);
    l->cone_interno = limitar(interno, 0.0f, l->cone_externo);
    l->tipo = CAM3D_LUZ_SPOT;
}

static void base_vista(const cam3d_handle* c, float* e, float* r, float* u, float* f) {
    float yw, pt, rl;
    cam3d_vista(c, &e[0], &e[1], &e[2], &yw, &pt, &rl);
    const float sy = sn(yw), cy = cs(yw), sp = sn(pt), cp = cs(pt), sr = sn(rl), cr = cs(rl);
    f[0] = sy * cp; f[1] = sp; f[2] = -cy * cp;
    const float r0[3] = { cy, 0.0f, sy };
    const float u0[3] = { -sy * sp, cp, cy * sp };
    for (int k = 0; k < 3; k++) {
        r[k] = r0[k] * cr + u0[k] * sr;
        u[k] = u0[k] * cr - r0[k] * sr;
    }
}

static void atualizar_luzes(cam3d_handle* c) {
    float e[3], r[3], u[3], f[3];
    base_vista(c, e, r, u, f);

    cam3d_luz* a = &c->luzes[CAM3D_LUZ_AURA];
    a->x = c->px;
    a->y = c->py + altura_corpo(c) * 0.5f + a->desloc_y;
    a->z = c->pz;

    cam3d_luz* l = &c->luzes[CAM3D_LUZ_LANTERNA];
    l->x = e[0]; l->y = e[1] + l->desloc_y; l->z = e[2];
    l->dx = f[0]; l->dy = f[1]; l->dz = f[2];
}

static void reunir_sombristas(cam3d_handle* c, obj3d_handle* const* objs, int n) {
    c->n_sombristas = 0;
    if (!objs || n <= 0) return;

    if (n > c->cap_sombristas) {
        if (c->sombristas) memoria_free(c->sombristas);
        c->sombristas = (cam3d_sombrista*)memoria_malloc((size_t)n * sizeof(cam3d_sombrista));
        c->cap_sombristas = c->sombristas ? n : 0;
        if (!c->sombristas) return;
    }

    for (int i = 0; i < n; i++) {
        const obj3d_handle* o = objs[i];
        if (!o || !o->visivel) continue;
        cam3d_sombrista* s = &c->sombristas[c->n_sombristas];
        if (!obj3d_caixa_mundo(o, &s->b)) continue;
        s->forma = (int)o->forma;
        for (int k = 0; k < 3; k++) s->b.e[k] = maxf(s->b.e[k] - SOMBRA_FOLGA, 0.001f);
        s->raio = raiz(s->b.e[0] * s->b.e[0] + s->b.e[1] * s->b.e[1] + s->b.e[2] * s->b.e[2]);
        c->n_sombristas++;
    }
}

static int ocluido(const cam3d_sombrista* s, const float* o, const float* d, float dist) {
    const obj3d_caixa* b = &s->b;
    const float oc[3] = { o[0] - b->c[0], o[1] - b->c[1], o[2] - b->c[2] };
    float ol[3], dl[3];
    for (int k = 0; k < 3; k++) {
        const float inv = 1.0f / b->e[k];
        ol[k] = dot3(oc, b->u[k]) * inv;
        dl[k] = dot3(d, b->u[k]) * inv;
    }

    float t0, t1;
    if (s->forma == OBJ3D_ESFERA) {
        const float a = dot3(dl, dl), bb = dot3(ol, dl), cc = dot3(ol, ol) - 1.0f;
        if (a < 1e-12f) return 0;
        const float disc = bb * bb - a * cc;
        if (disc <= 0.0f) return 0;
        const float sq = raiz(disc);
        t0 = (-bb - sq) / a;
        t1 = (-bb + sq) / a;
    } else {
        const int cil = s->forma == OBJ3D_CILINDRO;
        t0 = -1e30f; t1 = 1e30f;
        for (int k = 0; k < 3; k++) {
            if (cil && k != 1) continue;
            if (absf(dl[k]) < 1e-6f) {
                if (absf(ol[k]) > 1.0f) return 0;
                continue;
            }
            float ta = (-1.0f - ol[k]) / dl[k], tb = (1.0f - ol[k]) / dl[k];
            if (ta > tb) { const float t = ta; ta = tb; tb = t; }
            if (ta > t0) t0 = ta;
            if (tb < t1) t1 = tb;
            if (t0 > t1) return 0;
        }
        if (cil) {
            const float a = dl[0] * dl[0] + dl[2] * dl[2];
            const float bb = ol[0] * dl[0] + ol[2] * dl[2];
            const float cc = ol[0] * ol[0] + ol[2] * ol[2] - 1.0f;
            if (a < 1e-12f) {
                if (cc > 0.0f) return 0;
            } else {
                const float disc = bb * bb - a * cc;
                if (disc <= 0.0f) return 0;
                const float sq = raiz(disc);
                const float ta = (-bb - sq) / a, tb = (-bb + sq) / a;
                if (ta > t0) t0 = ta;
                if (tb < t1) t1 = tb;
            }
        }
    }
    return t0 < t1 && t1 > 0.02f && t0 < dist - 0.05f;
}

typedef struct {
    float x, y, z, dx, dy, dz;
    float r, g, b;
    float inv_alc, alc2, ce, inv_c;
    int   spot, sombra, n_cand;
    const int* cand;
} luz_frame;

static uint32_t canal255(float v) {
    return (uint32_t)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v + 0.5f));
}

void cam3d_iluminar(cam3d_handle* c, fb_handle* fb, zb_handle* zb, obj3d_handle* const* objs, int n) {
    if (!c || !fb || !zb || fb->w <= 0 || fb->h <= 0 || fb->w != zb->w || fb->h != zb->h) return;

    atualizar_luzes(c);

    int precisa_sombra = 0;
    if (c->sombras)
        for (int i = 0; i < CAM3D_MAX_LUZES; i++) {
            const cam3d_luz* l = &c->luzes[i];
            if (l->usada && l->ativa && l->sombra) { precisa_sombra = 1; break; }
        }

    if (precisa_sombra) reunir_sombristas(c, objs, n);
    else c->n_sombristas = 0;
    const int ns = c->n_sombristas;

    if (ns > 0 && ns * CAM3D_MAX_LUZES > c->cap_cand) {
        if (c->cand) memoria_free(c->cand);
        c->cand = (int*)memoria_malloc((size_t)ns * CAM3D_MAX_LUZES * sizeof(int));
        c->cap_cand = c->cand ? ns * CAM3D_MAX_LUZES : 0;
    }

    luz_frame lf[CAM3D_MAX_LUZES];
    int nl = 0;
    for (int i = 0; i < CAM3D_MAX_LUZES; i++) {
        const cam3d_luz* l = &c->luzes[i];
        if (!l->usada || !l->ativa || l->intensidade <= 0.0f || l->alcance <= 0.0f) continue;

        luz_frame* L = &lf[nl];
        L->x = l->x; L->y = l->y; L->z = l->z;
        L->dx = l->dx; L->dy = l->dy; L->dz = l->dz;
        L->r = l->cor_r * l->intensidade;
        L->g = l->cor_g * l->intensidade;
        L->b = l->cor_b * l->intensidade;
        L->inv_alc = 1.0f / l->alcance;
        L->alc2 = l->alcance * l->alcance;
        L->spot = l->tipo == CAM3D_LUZ_SPOT;
        L->ce = cs(l->cone_externo);
        L->inv_c = 1.0f / maxf(cs(l->cone_interno) - L->ce, 1e-4f);

        L->sombra = 0;
        L->n_cand = 0;
        L->cand = NULL;
        if (precisa_sombra && l->sombra && ns > 0 && c->cand) {
            int* lista = c->cand + nl * ns;
            int k = 0;
            for (int s = 0; s < ns; s++) {
                const cam3d_sombrista* sb = &c->sombristas[s];
                const float ax = sb->b.c[0] - l->x, ay = sb->b.c[1] - l->y, az = sb->b.c[2] - l->z;
                const float lim = l->alcance + sb->raio;
                if (ax * ax + ay * ay + az * az < lim * lim) lista[k++] = s;
            }
            L->sombra = k > 0;
            L->n_cand = k;
            L->cand = lista;
        }
        nl++;
    }

    const float amb = c->ambiente;
    if (nl == 0 && amb >= 0.999f) return;

    float e[3], r[3], u[3], f[3];
    base_vista(c, e, r, u, f);
    c->aspecto = (float)fb->w / (float)fb->h;
    const zb_mat4 P = cam3d_matriz_projecao(c);
    const float sx = 1.0f / P.m[0], sy = 1.0f / P.m[5];
    const float m10 = P.m[10], m11 = P.m[11];

    const int W = fb->w, H = fb->h;
    uint32_t* cores = fb_dados(fb);

    for (int y = 0; y < H; y++) {
        const float ny = (1.0f - 2.0f * ((float)y + 0.5f) / (float)H) * sy;

        for (int x = 0; x < W; x++) {
            const size_t idx = (size_t)y * W + x;
            const float z = zb->z[idx];
            if (z > 1.0f) continue;

            float lr = amb, lg = amb, lb = amb;

            if (nl > 0) {
                const float den = -z - m10;
                if (den < 1e-7f) continue;
                const float prof = -m11 / den;
                const float nx = (2.0f * ((float)x + 0.5f) / (float)W - 1.0f) * sx;
                const float xe = nx * prof, ye = ny * prof;
                const float p[3] = { e[0] + xe * r[0] + ye * u[0] + prof * f[0],
                                     e[1] + xe * r[1] + ye * u[1] + prof * f[1],
                                     e[2] + xe * r[2] + ye * u[2] + prof * f[2] };

                for (int i = 0; i < nl; i++) {
                    const luz_frame* L = &lf[i];
                    const float vx = p[0] - L->x, vy = p[1] - L->y, vz = p[2] - L->z;
                    const float d2 = vx * vx + vy * vy + vz * vz;
                    if (d2 >= L->alc2) continue;

                    float dist = raiz(d2);
                    if (dist < 1e-4f) dist = 1e-4f;

                    float att = 1.0f - dist * L->inv_alc;
                    att *= att;

                    if (L->spot) {
                        const float cosd = (vx * L->dx + vy * L->dy + vz * L->dz) / dist;
                        float s = (cosd - L->ce) * L->inv_c;
                        if (s <= 0.0f) continue;
                        if (s > 1.0f) s = 1.0f;
                        att *= s * s * (3.0f - 2.0f * s);
                    }

                    if (L->sombra && att > 0.01f) {
                        const float inv = 1.0f / dist;
                        const float d[3] = { -vx * inv, -vy * inv, -vz * inv };
                        for (int k = 0; k < L->n_cand; k++)
                            if (ocluido(&c->sombristas[L->cand[k]], p, d, dist)) { att = 0.0f; break; }
                    }

                    lr += L->r * att;
                    lg += L->g * att;
                    lb += L->b * att;
                }
            }

            if (lr == 1.0f && lg == 1.0f && lb == 1.0f) continue;

            const uint32_t cor = cores[idx];
            const uint32_t rr = canal255((float)((cor >> 16) & 255u) * lr);
            const uint32_t gg = canal255((float)((cor >> 8) & 255u) * lg);
            const uint32_t bb = canal255((float)(cor & 255u) * lb);
            cores[idx] = (cor & 0xFF000000u) | (rr << 16) | (gg << 8) | bb;
        }
    }
}
