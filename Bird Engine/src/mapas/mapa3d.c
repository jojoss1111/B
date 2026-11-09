#include "mapa3d.h"

static float absf(float v)                        { return v < 0.0f ? -v : v; }
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

static void copiar_bytes(void* d, const void* s, size_t n) {
    unsigned char* a = (unsigned char*)d;
    const unsigned char* b = (const unsigned char*)s;
    for (size_t i = 0; i < n; i++) a[i] = b[i];
}

static void* realocar(void* ant, size_t tam_ant, size_t tam_novo) {
    void* p = memoria_malloc(tam_novo);
    if (!p) return NULL;
    memoria_zerar(p, tam_novo);
    if (ant) {
        copiar_bytes(p, ant, tam_ant < tam_novo ? tam_ant : tam_novo);
        memoria_free(ant);
    }
    return p;
}

static int texto_igual(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static void texto_copiar(char* d, const char* s, int cap) {
    int i = 0;
    for (; i < cap - 1 && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static float curva_valor(int curva, float t) {
    if (curva == MAPA3D_CURVA_SUAVE) return t * t * (3.0f - 2.0f * t);
    return t;
}

mapa3d_handle* mapa3d_criar(void) {
    mapa3d_handle* m = (mapa3d_handle*)memoria_malloc(sizeof(mapa3d_handle));
    if (!m) return NULL;
    memoria_zerar(m, sizeof(*m));
    m->foco = -1;
    return m;
}

void mapa3d_limpar(mapa3d_handle* m) {
    if (!m) return;
    for (int i = 0; i < m->n_objetos; i++) obj3d_destruir(m->objetos[i]);
    m->n_objetos = 0;
    m->n_atuadores = 0;
    m->n_interativos = 0;
    m->n_giros = 0;
    m->foco = -1;
}

void mapa3d_destruir(mapa3d_handle* m) {
    if (!m) return;
    mapa3d_limpar(m);
    if (m->objetos) memoria_free(m->objetos);
    if (m->atuadores) memoria_free(m->atuadores);
    if (m->interativos) memoria_free(m->interativos);
    if (m->giros) memoria_free(m->giros);
    memoria_free(m);
}

void mapa3d_atlas(mapa3d_handle* m, fb_atlas_handle* atlas) {
    if (m) m->atlas = atlas;
}

int mapa3d_tile_definir(mapa3d_handle* m, const char* nome, int coluna, int linha) {
    if (!m || !nome || !nome[0] || coluna < 0 || linha < 0) return 0;
    for (int i = 0; i < m->n_tiles; i++) {
        if (texto_igual(m->tiles[i].nome, nome)) {
            m->tiles[i].coluna = (uint16_t)coluna;
            m->tiles[i].linha = (uint16_t)linha;
            return 1;
        }
    }
    if (m->n_tiles >= MAPA3D_MAX_TILES) return 0;
    mapa3d_tile* t = &m->tiles[m->n_tiles++];
    texto_copiar(t->nome, nome, (int)sizeof(t->nome));
    t->coluna = (uint16_t)coluna;
    t->linha = (uint16_t)linha;
    return 1;
}

int mapa3d_tile_buscar(const mapa3d_handle* m, const char* nome, int* coluna, int* linha) {
    if (!m || !nome) return 0;
    for (int i = 0; i < m->n_tiles; i++) {
        if (!texto_igual(m->tiles[i].nome, nome)) continue;
        if (coluna) *coluna = m->tiles[i].coluna;
        if (linha) *linha = m->tiles[i].linha;
        return 1;
    }
    return 0;
}

int mapa3d_adicionar(mapa3d_handle* m, obj3d_handle* o) {
    if (!m || !o) return 0;
    if (m->n_objetos >= m->cap_objetos) {
        const int cap = m->cap_objetos ? m->cap_objetos * 2 : 32;
        obj3d_handle** novo = (obj3d_handle**)realocar(m->objetos, (size_t)m->cap_objetos * sizeof(obj3d_handle*), (size_t)cap * sizeof(obj3d_handle*));
        if (!novo) return 0;
        m->objetos = novo;
        m->cap_objetos = cap;
    }
    m->objetos[m->n_objetos++] = o;
    return 1;
}

obj3d_handle* const* mapa3d_objetos(const mapa3d_handle* m, int* n) {
    if (n) *n = m ? m->n_objetos : 0;
    return m ? m->objetos : NULL;
}

static void centro_do_objeto(const obj3d_handle* o, float* p) {
    obj3d_caixa b;
    if (obj3d_caixa_mundo(o, &b)) {
        p[0] = b.c[0]; p[1] = b.c[1]; p[2] = b.c[2];
    } else {
        p[0] = o->x; p[1] = o->y; p[2] = o->z;
    }
}

int mapa3d_remover(mapa3d_handle* m, obj3d_handle* o) {
    if (!m || !o) return 0;

    int idx = -1;
    for (int i = 0; i < m->n_objetos; i++) if (m->objetos[i] == o) { idx = i; break; }
    if (idx < 0) return 0;

    for (int i = idx; i < m->n_objetos - 1; i++) m->objetos[i] = m->objetos[i + 1];
    m->n_objetos--;

    for (int i = 0; i < m->n_atuadores; i++) {
        mapa3d_atuador_info* a = &m->atuadores[i];
        if (!a->usado) continue;
        if (a->obj == o) { a->usado = 0; continue; }
        for (int k = 0; k < a->n_anexos; k++) {
            if (a->anexos[k] != o) continue;
            for (int j = k; j < a->n_anexos - 1; j++) {
                a->anexos[j] = a->anexos[j + 1];
                a->anexo_base[j][0] = a->anexo_base[j + 1][0];
                a->anexo_base[j][1] = a->anexo_base[j + 1][1];
                a->anexo_base[j][2] = a->anexo_base[j + 1][2];
            }
            a->n_anexos--;
            k--;
        }
    }

    for (int i = 0; i < m->n_giros; i++) {
        if (m->giros[i].obj != o) continue;
        m->giros[i] = m->giros[m->n_giros - 1];
        m->n_giros--;
        i--;
    }

    for (int i = 0; i < m->n_interativos; i++) {
        mapa3d_interativo_info* it = &m->interativos[i];
        if (!it->usado || it->obj != o) continue;
        float p[3];
        centro_do_objeto(o, p);
        it->x = p[0]; it->y = p[1]; it->z = p[2];
        it->obj = NULL;
    }

    obj3d_destruir(o);
    return 1;
}

static obj3d_handle* registrar(mapa3d_handle* m, obj3d_handle* o, int coluna, int linha) {
    if (!o) return NULL;
    if (!mapa3d_adicionar(m, o)) {
        obj3d_destruir(o);
        return NULL;
    }
    if (coluna >= 0 && linha >= 0) obj3d_tile(o, (uint16_t)coluna, (uint16_t)linha);
    return o;
}

obj3d_handle* mapa3d_cubo(mapa3d_handle* m, float x, float y, float z, float largura, float altura, float profundidade, int coluna, int linha) {
    if (!m) return NULL;
    return registrar(m, obj3d_criar_cubo(m->atlas, x, y, z, largura, altura, profundidade, 1.0f, 1.0f, 1), coluna, linha);
}

obj3d_handle* mapa3d_cilindro(mapa3d_handle* m, float x, float y, float z, float raio, float altura, int fatias, int coluna, int linha) {
    if (!m) return NULL;
    return registrar(m, obj3d_criar_cilindro(m->atlas, x, y, z, raio, altura, fatias > 0 ? fatias : 16, 1.0f, 1.0f, 1), coluna, linha);
}

obj3d_handle* mapa3d_cone(mapa3d_handle* m, float x, float y, float z, float raio, float altura, int fatias, int coluna, int linha) {
    if (!m) return NULL;
    return registrar(m, obj3d_criar_cone(m->atlas, x, y, z, raio, altura, fatias > 0 ? fatias : 16, 1.0f, 1.0f, 1), coluna, linha);
}

obj3d_handle* mapa3d_esfera(mapa3d_handle* m, float x, float y, float z, float raio, int fatias, int aneis, int coluna, int linha) {
    if (!m) return NULL;
    return registrar(m, obj3d_criar_esfera(m->atlas, x, y, z, raio, fatias > 0 ? fatias : 16, aneis > 0 ? aneis : 12, 1.0f, 1.0f, 1), coluna, linha);
}

obj3d_handle* mapa3d_poligono(mapa3d_handle* m, float x, float y, float z, const float* pontos, int n, float altura, int coluna, int linha) {
    if (!m) return NULL;
    return registrar(m, obj3d_criar_poligono(m->atlas, x, y, z, pontos, n, altura > 0.0f ? altura : 1.0f, 1.0f, 1.0f, 1), coluna, linha);
}

obj3d_handle* mapa3d_billboard(mapa3d_handle* m, float x, float y, float z, float largura, float altura, int coluna, int linha) {
    if (!m) return NULL;
    return registrar(m, obj3d_criar_billboard(m->atlas, x, y, z, largura, altura, 1.0f, 1.0f, 1), coluna, linha);
}

void mapa3d_chao(mapa3d_handle* m, float tamanho, int divisao, int coluna, int linha) {
    if (!m || divisao < 1) return;
    const float lado = tamanho / (float)divisao;
    const float meio = tamanho * 0.5f;
    for (int i = 0; i < divisao; i++)
        for (int j = 0; j < divisao; j++)
            mapa3d_cubo(m, -meio + lado * ((float)i + 0.5f), -0.5f, -meio + lado * ((float)j + 0.5f), lado, 1.0f, lado, coluna, linha);
}

void mapa3d_paredes(mapa3d_handle* m, float tamanho, float altura, int coluna, int linha) {
    if (!m) return;
    const float k = tamanho * 0.5f + 0.5f;
    mapa3d_cubo(m, 0.0f, altura * 0.5f, -k, tamanho + 2.0f, altura, 1.0f, coluna, linha);
    mapa3d_cubo(m, 0.0f, altura * 0.5f,  k, tamanho + 2.0f, altura, 1.0f, coluna, linha);
    mapa3d_cubo(m, -k, altura * 0.5f, 0.0f, 1.0f, altura, tamanho, coluna, linha);
    mapa3d_cubo(m,  k, altura * 0.5f, 0.0f, 1.0f, altura, tamanho, coluna, linha);
}

void mapa3d_escada(mapa3d_handle* m, float x, float z, float largura, int degraus, float altura_degrau, float profundidade, int coluna, int linha) {
    if (!m) return;
    for (int n = 1; n <= degraus; n++) {
        const float h = altura_degrau * (float)n;
        mapa3d_cubo(m, x, h * 0.5f, z - (float)(n - 1) * profundidade, largura, h, profundidade, coluna, linha);
    }
}

void mapa3d_girar(mapa3d_handle* m, obj3d_handle* o, float rx, float ry, float rz) {
    if (!m || !o) return;
    for (int i = 0; i < m->n_giros; i++) {
        if (m->giros[i].obj != o) continue;
        m->giros[i].rx = rx; m->giros[i].ry = ry; m->giros[i].rz = rz;
        return;
    }
    if (m->n_giros >= m->cap_giros) {
        const int cap = m->cap_giros ? m->cap_giros * 2 : 8;
        mapa3d_giro* novo = (mapa3d_giro*)realocar(m->giros, (size_t)m->cap_giros * sizeof(mapa3d_giro), (size_t)cap * sizeof(mapa3d_giro));
        if (!novo) return;
        m->giros = novo;
        m->cap_giros = cap;
    }
    mapa3d_giro* g = &m->giros[m->n_giros++];
    g->obj = o; g->rx = rx; g->ry = ry; g->rz = rz;
}

static mapa3d_atuador_info* atuador_de(const mapa3d_handle* m, int id) {
    if (!m || id < 0 || id >= m->n_atuadores || !m->atuadores[id].usado) return NULL;
    return &m->atuadores[id];
}

static mapa3d_interativo_info* interativo_de(const mapa3d_handle* m, int id) {
    if (!m || id < 0 || id >= m->n_interativos || !m->interativos[id].usado) return NULL;
    return &m->interativos[id];
}

static void atuador_recalcular(mapa3d_atuador_info* a) {
    a->vel = maxf(a->vel, 0.001f);
    const float len = raiz(a->desloc[0] * a->desloc[0] + a->desloc[1] * a->desloc[1] + a->desloc[2] * a->desloc[2]);
    if (len > 1e-4f) a->dur = len / a->vel;
    else if (a->graus != 0.0f) a->dur = absf(a->graus) / a->vel;
    else a->dur = 1.0f;
    if (a->dur < 1e-3f) a->dur = 1e-3f;
}

static void atuador_aplicar(mapa3d_atuador_info* a) {
    const float s = curva_valor(a->curva, a->t);
    float px = a->base[0] + a->desloc[0] * s;
    const float py = a->base[1] + a->desloc[1] * s;
    float pz = a->base[2] + a->desloc[2] * s;

    if (a->graus != 0.0f) {
        const float th = a->graus * s;
        const float rx = a->base[0] - a->pivo_x, rz = a->base[2] - a->pivo_z;
        const float ct = cs(th), st = sn(th);
        px = a->pivo_x + rx * ct + rz * st + a->desloc[0] * s;
        pz = a->pivo_z - rx * st + rz * ct + a->desloc[2] * s;
        obj3d_rotacionar_y(a->obj, a->ang0 + th);
    }
    obj3d_posicionar(a->obj, px, py, pz);

    for (int i = 0; i < a->n_anexos; i++) {
        float ax = a->anexo_base[i][0] + a->desloc[0] * s;
        const float ay = a->anexo_base[i][1] + a->desloc[1] * s;
        float az = a->anexo_base[i][2] + a->desloc[2] * s;
        if (a->graus != 0.0f) {
            const float th = a->graus * s;
            const float rx = a->anexo_base[i][0] - a->pivo_x;
            const float rz = a->anexo_base[i][2] - a->pivo_z;
            ax = a->pivo_x + rx * cs(th) + rz * sn(th) + a->desloc[0] * s;
            az = a->pivo_z - rx * sn(th) + rz * cs(th) + a->desloc[2] * s;
            obj3d_rotacionar_y(a->anexos[i], a->anexo_ang0[i] + th);
        }
        obj3d_posicionar(a->anexos[i], ax, ay, az);
    }
}

int mapa3d_atuador(mapa3d_handle* m, obj3d_handle* o) {
    if (!m || !o) return -1;

    int id = -1;
    for (int i = 0; i < m->n_atuadores; i++) if (!m->atuadores[i].usado) { id = i; break; }
    if (id < 0) {
        if (m->n_atuadores >= m->cap_atuadores) {
            const int cap = m->cap_atuadores ? m->cap_atuadores * 2 : 16;
            mapa3d_atuador_info* novo = (mapa3d_atuador_info*)realocar(m->atuadores, (size_t)m->cap_atuadores * sizeof(mapa3d_atuador_info), (size_t)cap * sizeof(mapa3d_atuador_info));
            if (!novo) return -1;
            m->atuadores = novo;
            m->cap_atuadores = cap;
        }
        id = m->n_atuadores++;
    }

    mapa3d_atuador_info* a = &m->atuadores[id];
    memoria_zerar(a, sizeof(*a));
    a->usado = 1;
    a->obj = o;
    a->base[0] = o->x; a->base[1] = o->y; a->base[2] = o->z;
    a->pivo_x = o->x; a->pivo_z = o->z;
    a->ang0 = o->ry;
    a->vel = 2.0f;
    a->modo = MAPA3D_MOV_ALTERNAR;
    a->curva = MAPA3D_CURVA_SUAVE;
    a->dir = 1;
    atuador_recalcular(a);
    return id;
}

void mapa3d_atuador_deslocar(mapa3d_handle* m, int id, float dx, float dy, float dz) {
    mapa3d_atuador_info* a = atuador_de(m, id);
    if (!a) return;
    a->desloc[0] = dx; a->desloc[1] = dy; a->desloc[2] = dz;
    atuador_recalcular(a);
    atuador_aplicar(a);
}

void mapa3d_atuador_giro(mapa3d_handle* m, int id, float pivo_x, float pivo_z, float graus) {
    mapa3d_atuador_info* a = atuador_de(m, id);
    if (!a) return;
    a->pivo_x = pivo_x; a->pivo_z = pivo_z; a->graus = graus;
    atuador_recalcular(a);
    atuador_aplicar(a);
}

void mapa3d_atuador_velocidade(mapa3d_handle* m, int id, float velocidade) {
    mapa3d_atuador_info* a = atuador_de(m, id);
    if (!a) return;
    a->vel = velocidade;
    atuador_recalcular(a);
}

void mapa3d_atuador_modo(mapa3d_handle* m, int id, int modo) {
    mapa3d_atuador_info* a = atuador_de(m, id);
    if (a) a->modo = modo == MAPA3D_MOV_VAIVEM ? MAPA3D_MOV_VAIVEM : MAPA3D_MOV_ALTERNAR;
}

void mapa3d_atuador_curva(mapa3d_handle* m, int id, int curva) {
    mapa3d_atuador_info* a = atuador_de(m, id);
    if (!a) return;
    a->curva = curva == MAPA3D_CURVA_LINEAR ? MAPA3D_CURVA_LINEAR : MAPA3D_CURVA_SUAVE;
    atuador_aplicar(a);
}

void mapa3d_atuador_pausa(mapa3d_handle* m, int id, float segundos) {
    mapa3d_atuador_info* a = atuador_de(m, id);
    if (a) a->pausa = maxf(segundos, 0.0f);
}

void mapa3d_atuador_auto_fechar(mapa3d_handle* m, int id, float segundos) {
    mapa3d_atuador_info* a = atuador_de(m, id);
    if (!a) return;
    a->auto_fechar = maxf(segundos, 0.0f);
    a->tempo_auto_fechar = 0.0f;
}

void mapa3d_atuador_carregar(mapa3d_handle* m, int id, int ligado) {
    mapa3d_atuador_info* a = atuador_de(m, id);
    if (a) a->carregar = ligado != 0;
}

void mapa3d_atuador_ativo(mapa3d_handle* m, int id, int ligado) {
    mapa3d_atuador_info* a = atuador_de(m, id);
    if (a) a->ativo = ligado != 0;
}

int mapa3d_atuador_anexar(mapa3d_handle* m, int id, obj3d_handle* o) {
    mapa3d_atuador_info* a = atuador_de(m, id);
    if (!a || !o || o == a->obj || a->n_anexos >= MAPA3D_MAX_ANEXOS) return 0;
    for (int i = 0; i < a->n_anexos; i++) if (a->anexos[i] == o) return 1;

    const float s = curva_valor(a->curva, a->t);
    a->anexos[a->n_anexos] = o;
    if (a->graus != 0.0f) {
        const float th = a->graus * s;
        const float dx = o->x - a->pivo_x - a->desloc[0] * s;
        const float dz = o->z - a->pivo_z - a->desloc[2] * s;
        a->anexo_base[a->n_anexos][0] = a->pivo_x + dx * cs(th) - dz * sn(th);
        a->anexo_base[a->n_anexos][2] = a->pivo_z + dx * sn(th) + dz * cs(th);
        a->anexo_ang0[a->n_anexos] = o->ry - th;
    } else {
        a->anexo_base[a->n_anexos][0] = o->x - a->desloc[0] * s;
        a->anexo_base[a->n_anexos][2] = o->z - a->desloc[2] * s;
        a->anexo_ang0[a->n_anexos] = o->ry;
    }
    a->anexo_base[a->n_anexos][1] = o->y - a->desloc[1] * s;
    a->n_anexos++;
    return 1;
}

static void atuador_comando(mapa3d_atuador_info* a, int acao, float valor) {
    if (acao == MAPA3D_ACAO_TRAVAR)    { a->travado = 1; return; }
    if (acao == MAPA3D_ACAO_DESTRAVAR) { a->travado = 0; return; }
    if (a->travado) return;

    if (a->modo == MAPA3D_MOV_VAIVEM) {
        if (acao == MAPA3D_ACAO_ABRIR) a->ativo = 1;
        else if (acao == MAPA3D_ACAO_FECHAR) a->ativo = 0;
        else if (acao == MAPA3D_ACAO_ALTERNAR) a->ativo = !a->ativo;
        else if (acao == MAPA3D_ACAO_IR) { a->t = limitar(valor, 0.0f, 1.0f); atuador_aplicar(a); }
        return;
    }

    if (acao == MAPA3D_ACAO_ABRIR) a->alvo = 1.0f;
    else if (acao == MAPA3D_ACAO_FECHAR) a->alvo = 0.0f;
    else if (acao == MAPA3D_ACAO_ALTERNAR) a->alvo = a->alvo > 0.5f ? 0.0f : 1.0f;
    else if (acao == MAPA3D_ACAO_IR) a->alvo = limitar(valor, 0.0f, 1.0f);
    if (a->alvo < 1.0f) a->tempo_auto_fechar = 0.0f;
    else if (a->t >= 1.0f) a->tempo_auto_fechar = a->auto_fechar;
}

void mapa3d_atuador_acao(mapa3d_handle* m, int id, int acao, float valor) {
    mapa3d_atuador_info* a = atuador_de(m, id);
    if (a) atuador_comando(a, acao, valor);
}

float mapa3d_atuador_t(const mapa3d_handle* m, int id) {
    const mapa3d_atuador_info* a = atuador_de(m, id);
    return a ? a->t : 0.0f;
}

int mapa3d_atuador_aberto(const mapa3d_handle* m, int id) {
    const mapa3d_atuador_info* a = atuador_de(m, id);
    if (!a) return 0;
    return a->modo == MAPA3D_MOV_VAIVEM ? a->ativo : a->alvo > 0.5f;
}

int mapa3d_atuador_movendo(const mapa3d_handle* m, int id) {
    const mapa3d_atuador_info* a = atuador_de(m, id);
    return a ? a->movendo : 0;
}

int mapa3d_atuador_travado(const mapa3d_handle* m, int id) {
    const mapa3d_atuador_info* a = atuador_de(m, id);
    return a ? a->travado : 0;
}

int mapa3d_porta_deslizar(mapa3d_handle* m, obj3d_handle* o, float dx, float dy, float dz, float velocidade) {
    const int id = mapa3d_atuador(m, o);
    if (id < 0) return -1;
    mapa3d_atuador_deslocar(m, id, dx, dy, dz);
    mapa3d_atuador_velocidade(m, id, velocidade > 0.0f ? velocidade : 2.0f);
    return id;
}

int mapa3d_porta_subir(mapa3d_handle* m, obj3d_handle* o, float altura, float velocidade) {
    return mapa3d_porta_deslizar(m, o, 0.0f, absf(altura), 0.0f, velocidade);
}

int mapa3d_porta_descer(mapa3d_handle* m, obj3d_handle* o, float altura, float velocidade) {
    return mapa3d_porta_deslizar(m, o, 0.0f, -absf(altura), 0.0f, velocidade);
}

int mapa3d_porta_girar(mapa3d_handle* m, obj3d_handle* o, float pivo_x, float pivo_z, float graus, float velocidade) {
    const int id = mapa3d_atuador(m, o);
    if (id < 0) return -1;
    mapa3d_atuador_giro(m, id, pivo_x, pivo_z, graus);
    mapa3d_atuador_velocidade(m, id, velocidade > 0.0f ? velocidade : 120.0f);
    return id;
}

int mapa3d_elevador(mapa3d_handle* m, obj3d_handle* o, float altura, float velocidade) {
    const int id = mapa3d_porta_deslizar(m, o, 0.0f, altura, 0.0f, velocidade);
    if (id >= 0) mapa3d_atuador_carregar(m, id, 1);
    return id;
}

int mapa3d_andador(mapa3d_handle* m, obj3d_handle* o, float dx, float dy, float dz, float velocidade, float pausa, int ativo) {
    const int id = mapa3d_porta_deslizar(m, o, dx, dy, dz, velocidade);
    if (id < 0) return -1;
    mapa3d_atuador_modo(m, id, MAPA3D_MOV_VAIVEM);
    mapa3d_atuador_pausa(m, id, pausa);
    mapa3d_atuador_carregar(m, id, 1);
    mapa3d_atuador_ativo(m, id, ativo);
    return id;
}

int mapa3d_interativo(mapa3d_handle* m, obj3d_handle* ancora, float x, float y, float z, float alcance) {
    if (!m) return -1;

    int id = -1;
    for (int i = 0; i < m->n_interativos; i++) if (!m->interativos[i].usado) { id = i; break; }
    if (id < 0) {
        if (m->n_interativos >= m->cap_interativos) {
            const int cap = m->cap_interativos ? m->cap_interativos * 2 : 16;
            mapa3d_interativo_info* novo = (mapa3d_interativo_info*)realocar(m->interativos, (size_t)m->cap_interativos * sizeof(mapa3d_interativo_info), (size_t)cap * sizeof(mapa3d_interativo_info));
            if (!novo) return -1;
            m->interativos = novo;
            m->cap_interativos = cap;
        }
        id = m->n_interativos++;
    }

    mapa3d_interativo_info* it = &m->interativos[id];
    memoria_zerar(it, sizeof(*it));
    it->usado = 1;
    it->ativo = 1;
    it->obj = ancora;
    it->x = x; it->y = y; it->z = z;
    it->alcance = alcance > 0.0f ? alcance : 2.0f;
    it->entrada = MAPA3D_ENTRADA_APERTAR;
    it->acao = MAPA3D_ACAO_ALTERNAR;
    return id;
}

void mapa3d_interativo_entrada(mapa3d_handle* m, int id, int entrada) {
    mapa3d_interativo_info* it = interativo_de(m, id);
    if (!it) return;
    it->entrada = (entrada == MAPA3D_ENTRADA_SEGURAR || entrada == MAPA3D_ENTRADA_PROXIMIDADE) ? entrada : MAPA3D_ENTRADA_APERTAR;
}

void mapa3d_interativo_acao(mapa3d_handle* m, int id, int acao, float valor) {
    mapa3d_interativo_info* it = interativo_de(m, id);
    if (!it) return;
    it->acao = acao;
    it->valor = valor;
}

int mapa3d_interativo_ligar(mapa3d_handle* m, int id, int atuador) {
    mapa3d_interativo_info* it = interativo_de(m, id);
    if (!it || !atuador_de(m, atuador) || it->n_ligados >= MAPA3D_MAX_LIGADOS) return 0;
    for (int i = 0; i < it->n_ligados; i++) if (it->ligados[i] == atuador) return 1;
    it->ligados[it->n_ligados++] = atuador;
    return 1;
}

void mapa3d_interativo_alcance(mapa3d_handle* m, int id, float alcance) {
    mapa3d_interativo_info* it = interativo_de(m, id);
    if (it && alcance > 0.0f) it->alcance = alcance;
}

void mapa3d_interativo_olhar(mapa3d_handle* m, int id, float graus) {
    mapa3d_interativo_info* it = interativo_de(m, id);
    if (it) it->angulo = limitar(graus, 0.0f, 180.0f);
}

void mapa3d_interativo_recarga(mapa3d_handle* m, int id, float segundos) {
    mapa3d_interativo_info* it = interativo_de(m, id);
    if (it) it->recarga = maxf(segundos, 0.0f);
}

void mapa3d_interativo_uma_vez(mapa3d_handle* m, int id, int ligado) {
    mapa3d_interativo_info* it = interativo_de(m, id);
    if (it) it->uma_vez = ligado != 0;
}

void mapa3d_interativo_ativar(mapa3d_handle* m, int id, int ligado) {
    mapa3d_interativo_info* it = interativo_de(m, id);
    if (it) it->ativo = ligado != 0;
}

int mapa3d_interativo_disparou(const mapa3d_handle* m, int id) {
    const mapa3d_interativo_info* it = interativo_de(m, id);
    return it ? it->disparou : 0;
}

int mapa3d_interativo_disparos(const mapa3d_handle* m, int id) {
    const mapa3d_interativo_info* it = interativo_de(m, id);
    return it ? it->disparos : 0;
}

int mapa3d_interativo_dentro(const mapa3d_handle* m, int id) {
    const mapa3d_interativo_info* it = interativo_de(m, id);
    return it ? it->dentro : 0;
}

int mapa3d_foco(const mapa3d_handle* m) {
    return m ? m->foco : -1;
}

static float dist2_interativo(const mapa3d_interativo_info* it, const float* olho, float* q) {
    obj3d_caixa b;
    if (it->obj && obj3d_caixa_mundo(it->obj, &b)) {
        float d[3] = { olho[0] - b.c[0], olho[1] - b.c[1], olho[2] - b.c[2] };
        q[0] = b.c[0]; q[1] = b.c[1]; q[2] = b.c[2];
        for (int k = 0; k < 3; k++) {
            const float t = limitar(d[0] * b.u[k][0] + d[1] * b.u[k][1] + d[2] * b.u[k][2], -b.e[k], b.e[k]);
            q[0] += b.u[k][0] * t; q[1] += b.u[k][1] * t; q[2] += b.u[k][2] * t;
        }
    } else if (it->obj) {
        q[0] = it->obj->x; q[1] = it->obj->y; q[2] = it->obj->z;
    } else {
        q[0] = it->x; q[1] = it->y; q[2] = it->z;
    }
    const float dx = olho[0] - q[0], dy = olho[1] - q[1], dz = olho[2] - q[2];
    return dx * dx + dy * dy + dz * dz;
}

static int olhando_para(const mapa3d_interativo_info* it, const float* olho, const float* frente, const float* q) {
    if (it->angulo <= 0.0f) return 1;
    const float v[3] = { q[0] - olho[0], q[1] - olho[1], q[2] - olho[2] };
    const float l = raiz(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (l < 1e-3f) return 1;
    return (v[0] * frente[0] + v[1] * frente[1] + v[2] * frente[2]) / l >= cs(it->angulo);
}

static void interativo_disparar(mapa3d_handle* m, mapa3d_interativo_info* it, int acao, float valor, int contar) {
    for (int i = 0; i < it->n_ligados; i++) {
        mapa3d_atuador_info* a = atuador_de(m, it->ligados[i]);
        if (a) atuador_comando(a, acao, valor);
    }
    if (!contar) return;
    it->disparou = 1;
    it->disparos++;
    it->espera = it->recarga;
}

static int acao_inversa(int acao) {
    switch (acao) {
        case MAPA3D_ACAO_ABRIR:     return MAPA3D_ACAO_FECHAR;
        case MAPA3D_ACAO_FECHAR:    return MAPA3D_ACAO_ABRIR;
        case MAPA3D_ACAO_TRAVAR:    return MAPA3D_ACAO_DESTRAVAR;
        case MAPA3D_ACAO_DESTRAVAR: return MAPA3D_ACAO_TRAVAR;
        default:                    return -1;
    }
}

static void interativo_reverter(mapa3d_handle* m, mapa3d_interativo_info* it) {
    const int inv = acao_inversa(it->acao);
    if (inv >= 0) interativo_disparar(m, it, inv, it->valor, 0);
}

static void atualizar_interativos(mapa3d_handle* m, float dt, cam3d_handle* cam, int apertou, int segurando) {
    m->foco = -1;
    for (int i = 0; i < m->n_interativos; i++) {
        mapa3d_interativo_info* it = &m->interativos[i];
        it->disparou = 0;
        if (it->espera > 0.0f) it->espera -= dt;
    }
    if (!cam) return;

    const float olho[3] = { cam3d_x(cam), cam3d_y(cam), cam3d_z(cam) };
    float frente[3];
    cam3d_frente(cam, &frente[0], &frente[1], &frente[2]);

    float melhor = 1e30f;
    for (int i = 0; i < m->n_interativos; i++) {
        mapa3d_interativo_info* it = &m->interativos[i];
        if (!it->usado || !it->ativo || it->entrada == MAPA3D_ENTRADA_PROXIMIDADE) continue;
        if (it->uma_vez && it->disparos > 0) continue;
        float q[3];
        const float d2 = dist2_interativo(it, olho, q);
        if (d2 > it->alcance * it->alcance || !olhando_para(it, olho, frente, q)) continue;
        if (d2 < melhor) { melhor = d2; m->foco = i; }
    }

    for (int i = 0; i < m->n_interativos; i++) {
        mapa3d_interativo_info* it = &m->interativos[i];
        if (!it->usado) continue;

        if (!it->ativo) {
            if (it->segurando_ativo) { it->segurando_ativo = 0; interativo_reverter(m, it); }
            it->dentro = 0;
            continue;
        }

        if (it->entrada == MAPA3D_ENTRADA_APERTAR) {
            if (m->foco == i && apertou && it->espera <= 0.0f) interativo_disparar(m, it, it->acao, it->valor, 1);
        } else if (it->entrada == MAPA3D_ENTRADA_SEGURAR) {
            if (m->foco == i && segurando) {
                if (!it->segurando_ativo) {
                    it->segurando_ativo = 1;
                    interativo_disparar(m, it, it->acao, it->valor, 1);
                }
            } else if (it->segurando_ativo) {
                it->segurando_ativo = 0;
                interativo_reverter(m, it);
            }
        } else {
            float q[3];
            const int dentro = dist2_interativo(it, olho, q) <= it->alcance * it->alcance;
            if (dentro && !it->dentro) {
                if (!(it->uma_vez && it->disparos > 0) && it->espera <= 0.0f)
                    interativo_disparar(m, it, it->acao, it->valor, 1);
            } else if (!dentro && it->dentro) {
                interativo_reverter(m, it);
            }
            it->dentro = dentro;
        }
    }
}

static int jogador_em_cima(const cam3d_handle* c, const obj3d_handle* o) {
    obj3d_caixa b;
    if (!c->no_chao || !obj3d_caixa_mundo(o, &b)) return 0;

    const float topo = b.c[1] + absf(b.u[0][1]) * b.e[0] + absf(b.u[1][1]) * b.e[1] + absf(b.u[2][1]) * b.e[2];
    if (absf(c->py - topo) > 0.3f) return 0;

    const float d[3] = { c->px - b.c[0], c->py - b.c[1], c->pz - b.c[2] };
    const float lx = d[0] * b.u[0][0] + d[1] * b.u[0][1] + d[2] * b.u[0][2];
    const float lz = d[0] * b.u[2][0] + d[1] * b.u[2][1] + d[2] * b.u[2][2];
    return absf(lx) <= b.e[0] + c->raio && absf(lz) <= b.e[2] + c->raio;
}

static void atuador_passo(mapa3d_atuador_info* a, float dt) {
    const float t0 = a->t;
    const float passo = dt / a->dur;

    if (a->modo == MAPA3D_MOV_VAIVEM) {
        if (a->ativo) {
            if (a->espera > 0.0f) {
                a->espera -= dt;
            } else {
                a->t += (float)a->dir * passo;
                if (a->t >= 1.0f)      { a->t = 1.0f; a->dir = -1; a->espera = a->pausa; }
                else if (a->t <= 0.0f) { a->t = 0.0f; a->dir = 1;  a->espera = a->pausa; }
            }
        }
    } else if (a->t < a->alvo) {
        a->t += passo;
        if (a->t > a->alvo) a->t = a->alvo;
    } else if (a->t > a->alvo) {
        a->t -= passo;
        if (a->t < a->alvo) a->t = a->alvo;
    }

    if (a->modo == MAPA3D_MOV_ALTERNAR && a->auto_fechar > 0.0f) {
        if (a->t >= 1.0f && a->alvo >= 1.0f) {
            if (a->tempo_auto_fechar <= 0.0f) a->tempo_auto_fechar = a->auto_fechar;
            a->tempo_auto_fechar -= dt;
            if (a->tempo_auto_fechar <= 0.0f) {
                a->tempo_auto_fechar = 0.0f;
                a->alvo = 0.0f;
            }
        } else if (a->alvo < 1.0f) a->tempo_auto_fechar = 0.0f;
    }

    a->movendo = absf(a->t - t0) > 1e-7f;
}

static void atualizar_atuadores(mapa3d_handle* m, float dt, cam3d_handle* cam) {
    for (int i = 0; i < m->n_atuadores; i++) {
        mapa3d_atuador_info* a = &m->atuadores[i];
        if (!a->usado) continue;

        const int passageiro = cam && a->carregar && jogador_em_cima(cam, a->obj);
        const float ox = a->obj->x, oy = a->obj->y, oz = a->obj->z;

        atuador_passo(a, dt);
        a->delta[0] = a->delta[1] = a->delta[2] = 0.0f;
        if (!a->movendo) continue;

        atuador_aplicar(a);
        a->delta[0] = a->obj->x - ox;
        a->delta[1] = a->obj->y - oy;
        a->delta[2] = a->obj->z - oz;

        if (passageiro) {
            cam->px += a->delta[0];
            cam->py += a->delta[1];
            cam->pz += a->delta[2];
        }
    }
}

void mapa3d_atualizar(mapa3d_handle* m, float dt, cam3d_handle* cam, int apertou, int segurando) {
    if (!m) return;
    if (dt < 0.0f) dt = 0.0f;

    if (cam) cam3d_colisao_mundo(cam, m->objetos, m->n_objetos);

    atualizar_interativos(m, dt, cam, apertou, segurando);

    for (int i = 0; i < m->n_giros; i++) {
        const mapa3d_giro* g = &m->giros[i];
        obj3d_girar(g->obj, g->rx * dt, g->ry * dt, g->rz * dt);
    }

    atualizar_atuadores(m, dt, cam);
}

void mapa3d_desenhar(mapa3d_handle* m, cam3d_handle* cam, fb_handle* fb, zb_handle* zb) {
    if (!m || !cam) return;
    cam3d_desenhar(cam, fb, zb, m->objetos, m->n_objetos);
}
