#include "obj_3d.h"

#define MAX_V16     65535
#define MAX_MODELOS 1024
#define MAX_GEOS    1024
#define COR_BRANCA  0xFFFFFFFFu

typedef struct {
    zb_vertice* v;
    int         n, cap;
    char        nome[48];
} malha_tmp;

typedef struct {
    malha_tmp* m;
    int        n, cap;
    int        nova;
    char       g[48], mt[48];
} construtor;

static void* crescer(void* p, int* cap, int preciso, size_t tam) {
    if (preciso <= *cap) return p;
    int nc = *cap ? *cap : 64;
    while (nc < preciso) nc *= 2;
    void* q = p ? memoria_realloc(p, (size_t)nc * tam) : memoria_malloc((size_t)nc * tam);
    if (!q) return NULL;
    *cap = nc;
    return q;
}

static void copiar_nome(char* dst, const char* s, const char* e) {
    int i = 0;
    while (s < e && i < 47 && *s && *s != '\r' && *s != '\n') dst[i++] = *s++;
    while (i > 0 && (dst[i - 1] == ' ' || dst[i - 1] == '\t')) i--;
    dst[i] = 0;
}

static int anexar(char* d, int k, const char* s) {
    while (*s && k < 47) d[k++] = *s++;
    d[k] = 0;
    return k;
}

static malha_tmp* cons_nova(construtor* c) {
    int cap = c->cap;
    malha_tmp* q = (malha_tmp*)crescer(c->m, &cap, c->n + 1, sizeof(malha_tmp));
    if (!q) return NULL;
    c->m = q;
    c->cap = cap;
    malha_tmp* m = &c->m[c->n++];
    memoria_zerar(m, sizeof(*m));
    int k = 0;
    if (c->g[0]) k = anexar(m->nome, k, c->g);
    if (c->g[0] && c->mt[0] && k < 46) { m->nome[k++] = '/'; m->nome[k] = 0; }
    if (c->mt[0]) k = anexar(m->nome, k, c->mt);
    if (!k) anexar(m->nome, 0, "parte");
    c->nova = 0;
    return m;
}

static int cons_tri(construtor* c, const zb_vertice* a, const zb_vertice* b, const zb_vertice* d) {
    malha_tmp* m = c->n ? &c->m[c->n - 1] : NULL;
    if (!m || c->nova || m->n + 3 > MAX_V16) {
        m = cons_nova(c);
        if (!m) return 0;
    }
    int cap = m->cap;
    zb_vertice* q = (zb_vertice*)crescer(m->v, &cap, m->n + 3, sizeof(zb_vertice));
    if (!q) return 0;
    m->v = q;
    m->cap = cap;
    m->v[m->n++] = *a;
    m->v[m->n++] = *b;
    m->v[m->n++] = *d;
    return 1;
}

static void cons_liberar(construtor* c) {
    for (int i = 0; i < c->n; i++) if (c->m[i].v) memoria_free(c->m[i].v);
    if (c->m) memoria_free(c->m);
    c->m = NULL;
    c->n = c->cap = 0;
}

static obj3d_handle* cons_finalizar(construtor* c, fb_atlas_handle* atlas, float x, float y, float z, float esc) {
    int np = 0;
    for (int i = 0; i < c->n; i++) if (c->m[i].n > 0) np++;
    if (!np) return NULL;

    obj3d_handle* o = (obj3d_handle*)memoria_calloc(1, sizeof(obj3d_handle));
    if (!o) return NULL;
    o->partes = (obj3d_parte*)memoria_calloc((size_t)np, sizeof(obj3d_parte));
    if (!o->partes) { memoria_free(o); return NULL; }

    o->forma = OBJ3D_MODELO;
    o->atlas = atlas;
    o->x = x; o->y = y; o->z = z;
    o->sx = o->sy = o->sz = esc;
    o->cor = COR_BRANCA;
    o->modo = atlas ? ZB_MODO_TEXTURA : ZB_MODO_GOURAUD;
    o->cull = ZB_CULL_TRAS;
    o->visivel = 1;
    o->colisao = OBJ3D_COL_CAIXA;
    o->cap_partes = np;

    int k = 0;
    for (int i = 0; i < c->n; i++) {
        malha_tmp* m = &c->m[i];
        if (m->n <= 0) continue;
        obj3d_parte* p = &o->partes[k++];
        o->n_partes = k;
        p->indices = (uint16_t*)memoria_malloc((size_t)m->n * sizeof(uint16_t));
        if (!p->indices) { obj3d_destruir(o); return NULL; }
        for (int j = 0; j < m->n; j++) p->indices[j] = (uint16_t)j;
        p->vertices = m->v;
        m->v = NULL;
        p->n_vertices = m->n;
        p->n_indices = m->n;
        p->visivel = 1;
        copiar_nome(p->nome, m->nome, m->nome + 48);
    }
    return o;
}

static const char* pular_esp(const char* s, const char* e) {
    while (s < e && (*s == ' ' || *s == '\t' || *s == '\r')) s++;
    return s;
}

static const char* ler_double(const char* s, const char* e, double* out) {
    s = pular_esp(s, e);
    int neg = 0, dig = 0;
    double v = 0.0;
    if (s < e && (*s == '-' || *s == '+')) { neg = *s == '-'; s++; }
    while (s < e && *s >= '0' && *s <= '9') { v = v * 10.0 + (*s - '0'); s++; dig++; }
    if (s < e && *s == '.') {
        s++;
        double f = 0.1;
        while (s < e && *s >= '0' && *s <= '9') { v += (*s - '0') * f; f *= 0.1; s++; dig++; }
    }
    if (!dig) return NULL;
    if (s < e && (*s == 'e' || *s == 'E')) {
        const char* t = s + 1;
        int en = 0, eneg = 0, ed = 0;
        if (t < e && (*t == '-' || *t == '+')) { eneg = *t == '-'; t++; }
        while (t < e && *t >= '0' && *t <= '9') { if (en < 400) en = en * 10 + (*t - '0'); t++; ed++; }
        if (ed) {
            s = t;
            if (en > 400) en = 400;
            while (en-- > 0) v = eneg ? v * 0.1 : v * 10.0;
        }
    }
    *out = neg ? -v : v;
    return s;
}

static const char* ler_long(const char* s, const char* e, long* out) {
    s = pular_esp(s, e);
    int neg = 0, dig = 0;
    long v = 0;
    if (s < e && (*s == '-' || *s == '+')) { neg = *s == '-'; s++; }
    while (s < e && *s >= '0' && *s <= '9') { if (v < 100000000L) v = v * 10 + (*s - '0'); s++; dig++; }
    if (!dig) return NULL;
    *out = neg ? -v : v;
    return s;
}

static const char* chave(const char* p, const char* e, const char* k) {
    while (*k) {
        if (p >= e || *p != *k) return NULL;
        p++; k++;
    }
    if (p < e && *p != ' ' && *p != '\t' && *p != '\r') return NULL;
    return p;
}

static int obj_face(construtor* c, const char* p, const char* e, const float* pos, int npos, const float* tex, int ntex) {
    zb_vertice fv[128];
    int k = 0;
    for (;;) {
        p = pular_esp(p, e);
        if (p >= e) break;
        long vi = 0, ti = 0;
        int tem_t = 0;
        const char* q = ler_long(p, e, &vi);
        if (!q) break;
        p = q;
        if (p < e && *p == '/') {
            p++;
            if (p < e && *p != '/') {
                q = ler_long(p, e, &ti);
                if (q) { p = q; tem_t = 1; }
            }
            if (p < e && *p == '/') {
                p++;
                long ni;
                q = ler_long(p, e, &ni);
                if (q) p = q;
            }
        }
        const int nv = npos / 3, nt = ntex / 2;
        long iv = vi > 0 ? vi - 1 : nv + vi;
        long it = ti > 0 ? ti - 1 : nt + ti;

        zb_vertice v;
        v.x = v.y = v.z = 0.0f;
        v.u = v.v = 0.0f;
        v.cor = COR_BRANCA;
        if (iv >= 0 && iv < nv) { v.x = pos[iv * 3]; v.y = pos[iv * 3 + 1]; v.z = pos[iv * 3 + 2]; }
        if (tem_t && it >= 0 && it < nt) { v.u = tex[it * 2]; v.v = 1.0f - tex[it * 2 + 1]; }
        if (k < 128) fv[k++] = v;
    }
    for (int i = 1; i + 1 < k; i++)
        if (!cons_tri(c, &fv[0], &fv[i], &fv[i + 1])) return 0;
    return 1;
}

static obj3d_handle* carregar_obj(const char* d, size_t n, fb_atlas_handle* atlas, float x, float y, float z, float esc) {
    construtor c;
    memoria_zerar(&c, sizeof(c));
    float* pos = NULL; int npos = 0, cpos = 0;
    float* tex = NULL; int ntex = 0, ctex = 0;
    int ok = 1;

    const char* s = d;
    const char* fim = d + n;
    while (ok && s < fim) {
        const char* le = s;
        while (le < fim && *le != '\n') le++;
        const char* p = pular_esp(s, le);
        const char* q;

        if ((q = chave(p, le, "vt"))) {
            double a, b;
            const char* r = ler_double(q, le, &a);
            if (r && (r = ler_double(r, le, &b))) {
                float* t = (float*)crescer(tex, &ctex, ntex + 2, sizeof(float));
                if (!t) ok = 0;
                else { tex = t; tex[ntex++] = (float)a; tex[ntex++] = (float)b; }
            }
        } else if ((q = chave(p, le, "vn"))) {
        } else if ((q = chave(p, le, "v"))) {
            double a, b, cc;
            const char* r = ler_double(q, le, &a);
            if (r && (r = ler_double(r, le, &b)) && (r = ler_double(r, le, &cc))) {
                float* t = (float*)crescer(pos, &cpos, npos + 3, sizeof(float));
                if (!t) ok = 0;
                else { pos = t; pos[npos++] = (float)a; pos[npos++] = (float)b; pos[npos++] = (float)cc; }
            }
        } else if ((q = chave(p, le, "f"))) {
            ok = obj_face(&c, q, le, pos, npos, tex, ntex);
        } else if ((q = chave(p, le, "usemtl"))) {
            copiar_nome(c.mt, pular_esp(q, le), le);
            c.nova = 1;
        } else if ((q = chave(p, le, "o")) || (q = chave(p, le, "g"))) {
            copiar_nome(c.g, pular_esp(q, le), le);
            c.nova = 1;
        }
        s = le + 1;
    }

    obj3d_handle* o = ok ? cons_finalizar(&c, atlas, x, y, z, esc) : NULL;
    cons_liberar(&c);
    if (pos) memoria_free(pos);
    if (tex) memoria_free(tex);
    obj3d_recalcular_limites(o);
    return o;
}

typedef struct {
    const uint8_t* src;
    size_t         n, pos;
    uint32_t       bits;
    int            nb, erro;
    uint8_t*       dst;
    size_t         cap, len;
} inflador;

typedef struct {
    uint16_t cont[16];
    uint16_t simb[288];
} huff;

static int inf_bit(inflador* s) {
    if (!s->nb) {
        if (s->pos >= s->n) { s->erro = 1; return 0; }
        s->bits = s->src[s->pos++];
        s->nb = 8;
    }
    int b = (int)(s->bits & 1);
    s->bits >>= 1;
    s->nb--;
    return b;
}

static int inf_bits(inflador* s, int n) {
    int v = 0;
    for (int i = 0; i < n; i++) v |= inf_bit(s) << i;
    return v;
}

static int huff_construir(huff* h, const uint8_t* len, int n) {
    for (int i = 0; i < 16; i++) h->cont[i] = 0;
    for (int i = 0; i < n; i++) h->cont[len[i]]++;
    if (h->cont[0] == n) return 0;
    int left = 1;
    for (int l = 1; l < 16; l++) {
        left <<= 1;
        left -= h->cont[l];
        if (left < 0) return left;
    }
    uint16_t offs[16];
    offs[1] = 0;
    for (int l = 1; l < 15; l++) offs[l + 1] = (uint16_t)(offs[l] + h->cont[l]);
    for (int i = 0; i < n; i++) if (len[i]) h->simb[offs[len[i]]++] = (uint16_t)i;
    return left;
}

static int huff_decodificar(inflador* s, const huff* h) {
    int code = 0, first = 0, index = 0;
    for (int l = 1; l < 16; l++) {
        code |= inf_bit(s);
        int count = h->cont[l];
        if (code - count < first) return h->simb[index + (code - first)];
        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
        if (s->erro) return -1;
    }
    return -1;
}

static const uint16_t LB[29] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
static const uint16_t LE_[29] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
static const uint16_t DB[30] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
static const uint16_t DE[30] = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int inf_codigos(inflador* s, const huff* lc, const huff* dc) {
    for (;;) {
        int sym = huff_decodificar(s, lc);
        if (sym < 0) return -1;
        if (sym < 256) {
            if (s->len >= s->cap) return -1;
            s->dst[s->len++] = (uint8_t)sym;
        } else if (sym == 256) {
            return 0;
        } else {
            sym -= 257;
            if (sym >= 29) return -1;
            size_t len = (size_t)LB[sym] + (size_t)inf_bits(s, LE_[sym]);
            int ds = huff_decodificar(s, dc);
            if (ds < 0 || ds >= 30) return -1;
            size_t dist = (size_t)DB[ds] + (size_t)inf_bits(s, DE[ds]);
            if (dist > s->len || s->len + len > s->cap) return -1;
            for (size_t i = 0; i < len; i++, s->len++) s->dst[s->len] = s->dst[s->len - dist];
        }
        if (s->erro) return -1;
    }
}

static int inf_fixo(inflador* s) {
    uint8_t len[288];
    huff lc, dc;
    int i = 0;
    for (; i < 144; i++) len[i] = 8;
    for (; i < 256; i++) len[i] = 9;
    for (; i < 280; i++) len[i] = 7;
    for (; i < 288; i++) len[i] = 8;
    huff_construir(&lc, len, 288);
    for (i = 0; i < 30; i++) len[i] = 5;
    huff_construir(&dc, len, 30);
    return inf_codigos(s, &lc, &dc);
}

static int inf_dinamico(inflador* s) {
    static const uint8_t ordem[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
    uint8_t len[320];
    huff lc, dc;
    int nlen = inf_bits(s, 5) + 257;
    int ndist = inf_bits(s, 5) + 1;
    int ncod = inf_bits(s, 4) + 4;
    if (nlen > 286 || ndist > 30) return -1;
    for (int i = 0; i < 19; i++) len[ordem[i]] = 0;
    for (int i = 0; i < ncod; i++) len[ordem[i]] = (uint8_t)inf_bits(s, 3);
    if (huff_construir(&lc, len, 19) != 0) return -1;
    int idx = 0;
    while (idx < nlen + ndist) {
        int sym = huff_decodificar(s, &lc);
        if (sym < 0) return -1;
        if (sym < 16) {
            len[idx++] = (uint8_t)sym;
        } else {
            int prev = 0, rep;
            if (sym == 16) {
                if (idx == 0) return -1;
                prev = len[idx - 1];
                rep = 3 + inf_bits(s, 2);
            } else if (sym == 17) {
                rep = 3 + inf_bits(s, 3);
            } else {
                rep = 11 + inf_bits(s, 7);
            }
            if (idx + rep > nlen + ndist) return -1;
            while (rep--) len[idx++] = (uint8_t)prev;
        }
        if (s->erro) return -1;
    }
    if (huff_construir(&lc, len, nlen) < 0) return -1;
    if (huff_construir(&dc, len + nlen, ndist) < 0) return -1;
    return inf_codigos(s, &lc, &dc);
}

static int inflar(const uint8_t* src, size_t n, uint8_t* dst, size_t cap) {
    if (n < 2) return 0;
    inflador s;
    memoria_zerar(&s, sizeof(s));
    s.src = src; s.n = n; s.pos = 2;
    s.dst = dst; s.cap = cap;
    int ultimo;
    do {
        ultimo = inf_bit(&s);
        int tipo = inf_bits(&s, 2);
        int r;
        if (tipo == 0) {
            s.nb = 0;
            if (s.pos + 4 > s.n) return 0;
            size_t l = (size_t)s.src[s.pos] | ((size_t)s.src[s.pos + 1] << 8);
            s.pos += 4;
            if (s.pos + l > s.n || s.len + l > s.cap) return 0;
            memoria_copia(s.dst + s.len, s.src + s.pos, l);
            s.pos += l;
            s.len += l;
            r = 0;
        } else if (tipo == 1) {
            r = inf_fixo(&s);
        } else if (tipo == 2) {
            r = inf_dinamico(&s);
        } else {
            return 0;
        }
        if (r != 0 || s.erro) return 0;
    } while (!ultimo);
    return s.len == cap;
}

typedef struct { const uint8_t* d; size_t n; int v64; } fbx;

typedef struct {
    const uint8_t* nome;
    int            nome_len;
    uint64_t       nprop;
    size_t         prop_ini, filhos_ini, fim;
} no;

typedef struct {
    char           t;
    int64_t        i;
    double         d;
    const uint8_t* p;
    uint32_t       n, enc, cn;
} prop;

static uint32_t u32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t u64(const uint8_t* p) {
    return (uint64_t)u32(p) | ((uint64_t)u32(p + 4) << 32);
}

static int ler_no(const fbx* f, size_t pos, no* n) {
    const size_t cab = f->v64 ? 25 : 13;
    if (pos + cab > f->n) return 0;
    const uint8_t* b = f->d + pos;
    uint64_t fim = f->v64 ? u64(b) : u32(b);
    uint64_t nprop = f->v64 ? u64(b + 8) : u32(b + 4);
    uint64_t lprop = f->v64 ? u64(b + 16) : u32(b + 8);
    uint8_t nl = b[f->v64 ? 24 : 12];
    if (fim == 0) return 0;
    if (fim > f->n || lprop > f->n || fim < pos + cab + nl + lprop) return 0;
    n->nome = b + cab;
    n->nome_len = nl;
    n->nprop = nprop;
    n->prop_ini = pos + cab + nl;
    n->filhos_ini = n->prop_ini + (size_t)lprop;
    n->fim = (size_t)fim;
    return 1;
}

static int nome_igual(const no* n, const char* s) {
    const int l = (int)txt_tam(s);
    if (l != n->nome_len) return 0;
    for (int i = 0; i < l; i++) if (n->nome[i] != (uint8_t)s[i]) return 0;
    return 1;
}

static size_t ler_prop(const fbx* f, size_t pos, prop* p) {
    if (pos >= f->n) return 0;
    const uint8_t* d = f->d;
    p->t = (char)d[pos++];
    p->i = 0; p->d = 0.0; p->p = NULL; p->n = p->enc = p->cn = 0;
    size_t resta = f->n - pos;
    switch (p->t) {
        case 'C': if (resta < 1) return 0; p->i = d[pos]; p->d = (double)p->i; return pos + 1;
        case 'Y': if (resta < 2) return 0; p->i = (int16_t)(d[pos] | (d[pos + 1] << 8)); p->d = (double)p->i; return pos + 2;
        case 'I': if (resta < 4) return 0; p->i = (int32_t)u32(d + pos); p->d = (double)p->i; return pos + 4;
        case 'F': {
            if (resta < 4) return 0;
            uint32_t b = u32(d + pos);
            float v;
            memoria_copia(&v, &b, 4);
            p->d = (double)v;
            return pos + 4;
        }
        case 'D': {
            if (resta < 8) return 0;
            uint64_t b = u64(d + pos);
            double v;
            memoria_copia(&v, &b, 8);
            p->d = v;
            return pos + 8;
        }
        case 'L': if (resta < 8) return 0; p->i = (int64_t)u64(d + pos); p->d = (double)p->i; return pos + 8;
        case 'S':
        case 'R': {
            if (resta < 4) return 0;
            uint32_t l = u32(d + pos);
            if (l > resta - 4) return 0;
            p->p = d + pos + 4;
            p->n = l;
            return pos + 4 + l;
        }
        case 'f': case 'd': case 'l': case 'i': case 'b': {
            if (resta < 12) return 0;
            p->n = u32(d + pos);
            p->enc = u32(d + pos + 4);
            p->cn = u32(d + pos + 8);
            if (p->cn > resta - 12) return 0;
            p->p = d + pos + 12;
            return pos + 12 + p->cn;
        }
        default: return 0;
    }
}

static int prop_k(const fbx* f, const no* n, uint64_t k, prop* p) {
    if (k >= n->nprop) return 0;
    size_t pos = n->prop_ini;
    for (uint64_t i = 0; i <= k; i++) {
        pos = ler_prop(f, pos, p);
        if (!pos) return 0;
    }
    return 1;
}

static int prop_str_igual(const prop* p, const char* s) {
    if (p->t != 'S') return 0;
    const uint32_t l = (uint32_t)txt_tam(s);
    if (l != p->n) return 0;
    for (uint32_t i = 0; i < l; i++) if (p->p[i] != (uint8_t)s[i]) return 0;
    return 1;
}

static void* ler_array(const prop* p, size_t el) {
    size_t total = (size_t)p->n * el;
    uint8_t* out = (uint8_t*)memoria_malloc(total ? total : 1);
    if (!out) return NULL;
    if (p->enc == 0) {
        if (p->cn != total) { memoria_free(out); return NULL; }
        memoria_copia(out, p->p, total);
    } else if (p->enc == 1) {
        if (!inflar(p->p, p->cn, out, total)) { memoria_free(out); return NULL; }
    } else {
        memoria_free(out);
        return NULL;
    }
    return out;
}

static double* ler_doubles(const prop* p, int* cnt) {
    *cnt = 0;
    if (p->t == 'd') {
        double* r = (double*)ler_array(p, 8);
        if (r) *cnt = (int)p->n;
        return r;
    }
    if (p->t == 'f') {
        float* a = (float*)ler_array(p, 4);
        if (!a) return NULL;
        double* r = (double*)memoria_malloc(((size_t)p->n ? (size_t)p->n : 1) * sizeof(double));
        if (!r) { memoria_free(a); return NULL; }
        for (uint32_t i = 0; i < p->n; i++) r[i] = (double)a[i];
        memoria_free(a);
        *cnt = (int)p->n;
        return r;
    }
    return NULL;
}

static int* ler_ints(const prop* p, int* cnt) {
    *cnt = 0;
    if (p->t != 'i') return NULL;
    int* r = (int*)ler_array(p, 4);
    if (r) *cnt = (int)p->n;
    return r;
}

typedef struct { double m[16]; } mat;

static mat mat_id(void) {
    mat r;
    for (int i = 0; i < 16; i++) r.m[i] = 0.0;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0;
    return r;
}

static mat mat_mul(const mat* a, const mat* b) {
    mat r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r.m[i * 4 + j] = a->m[i * 4] * b->m[j] + a->m[i * 4 + 1] * b->m[4 + j]
                           + a->m[i * 4 + 2] * b->m[8 + j] + a->m[i * 4 + 3] * b->m[12 + j];
    return r;
}

static double graus_norm(double g) {
    if (g > 1e6 || g < -1e6) return 0.0;
    while (g >= 360.0) g -= 360.0;
    while (g < 0.0) g += 360.0;
    return g;
}

static double seno_d(double g)    { return (double)fb_seno(fb_graus((float)graus_norm(g))) * (1.0 / 32768.0); }
static double cosseno_d(double g) { return (double)fb_cosseno(fb_graus((float)graus_norm(g))) * (1.0 / 32768.0); }

static mat mat_rot(const double* r) {
    const double cx = cosseno_d(r[0]), sx = seno_d(r[0]);
    const double cy = cosseno_d(r[1]), sy = seno_d(r[1]);
    const double cz = cosseno_d(r[2]), sz = seno_d(r[2]);
    mat x = mat_id(), y = mat_id(), z = mat_id();
    x.m[5] = cx; x.m[6] = -sx; x.m[9] = sx; x.m[10] = cx;
    y.m[0] = cy; y.m[2] = sy;  y.m[8] = -sy; y.m[10] = cy;
    z.m[0] = cz; z.m[1] = -sz; z.m[4] = sz; z.m[5] = cz;
    mat zy = mat_mul(&z, &y);
    return mat_mul(&zy, &x);
}

static double mat_det3(const mat* a) {
    const double* m = a->m;
    return m[0] * (m[5] * m[10] - m[6] * m[9])
         - m[1] * (m[4] * m[10] - m[6] * m[8])
         + m[2] * (m[4] * m[9] - m[5] * m[8]);
}

typedef struct {
    int64_t id, pai;
    char    nome[48];
    double  lt[3], lr[3], ls[3], pre[3];
    mat     local;
} modelo;

typedef struct {
    int64_t id, modelo_id;
    no      nd;
} geo;

static double prop_num(const prop* p) { return p->d; }

static void ler_vec3(const fbx* f, const no* n, double* out) {
    prop p;
    for (int i = 0; i < 3; i++)
        if (prop_k(f, n, 4 + i, &p)) out[i] = prop_num(&p);
}

static void ler_modelo(const fbx* f, const no* n, modelo* md) {
    prop p;
    md->ls[0] = md->ls[1] = md->ls[2] = 1.0;
    if (prop_k(f, n, 0, &p)) md->id = p.i;
    if (prop_k(f, n, 1, &p) && p.t == 'S') copiar_nome(md->nome, (const char*)p.p, (const char*)p.p + p.n);

    size_t pos = n->filhos_ini;
    no c;
    while (pos < n->fim && ler_no(f, pos, &c)) {
        pos = c.fim;
        if (!nome_igual(&c, "Properties70")) continue;
        size_t pp = c.filhos_ini;
        no q;
        while (pp < c.fim && ler_no(f, pp, &q)) {
            pp = q.fim;
            if (!nome_igual(&q, "P") || !prop_k(f, &q, 0, &p)) continue;
            if (prop_str_igual(&p, "Lcl Translation")) ler_vec3(f, &q, md->lt);
            else if (prop_str_igual(&p, "Lcl Rotation")) ler_vec3(f, &q, md->lr);
            else if (prop_str_igual(&p, "Lcl Scaling")) ler_vec3(f, &q, md->ls);
            else if (prop_str_igual(&p, "PreRotation")) ler_vec3(f, &q, md->pre);
        }
    }

    mat t = mat_id();
    t.m[3] = md->lt[0]; t.m[7] = md->lt[1]; t.m[11] = md->lt[2];
    mat pr = mat_rot(md->pre);
    mat r = mat_rot(md->lr);
    mat s = mat_id();
    s.m[0] = md->ls[0]; s.m[5] = md->ls[1]; s.m[10] = md->ls[2];
    mat a = mat_mul(&t, &pr);
    a = mat_mul(&a, &r);
    md->local = mat_mul(&a, &s);
}

typedef struct {
    double* ver; int nver;
    double* uv;  int nuv;
    int*    uvi; int nuvi;
    int     mapa, indexado;
    mat     M;
} ctx;

static zb_vertice fbx_vert(const ctx* k, int slot, int vi) {
    zb_vertice o;
    double x = 0.0, y = 0.0, z = 0.0;
    if (vi >= 0 && vi * 3 + 2 < k->nver) {
        x = k->ver[vi * 3]; y = k->ver[vi * 3 + 1]; z = k->ver[vi * 3 + 2];
    }
    const double* m = k->M.m;
    o.x = (float)(m[0] * x + m[1] * y + m[2] * z + m[3]);
    o.y = (float)(m[4] * x + m[5] * y + m[6] * z + m[7]);
    o.z = (float)(m[8] * x + m[9] * y + m[10] * z + m[11]);
    o.u = o.v = 0.0f;
    o.cor = COR_BRANCA;
    int idx = -1;
    if (k->mapa == 1) idx = slot;
    else if (k->mapa == 2) idx = vi;
    if (idx >= 0 && k->indexado) idx = idx < k->nuvi ? k->uvi[idx] : -1;
    if (idx >= 0 && idx * 2 + 1 < k->nuv) {
        o.u = (float)k->uv[idx * 2];
        o.v = (float)(1.0 - k->uv[idx * 2 + 1]);
    }
    return o;
}

static int fbx_geometria(const fbx* f, const no* g, const mat* M, construtor* c) {
    ctx k;
    memoria_zerar(&k, sizeof(k));
    k.M = *M;
    int* pv = NULL;
    int npv = 0, ok = 1;
    int uv_lido = 0;
    prop p;
    no n;
    size_t pos = g->filhos_ini;
    while (pos < g->fim && ler_no(f, pos, &n)) {
        pos = n.fim;
        if (nome_igual(&n, "Vertices") && !k.ver) {
            if (prop_k(f, &n, 0, &p)) k.ver = ler_doubles(&p, &k.nver);
        } else if (nome_igual(&n, "PolygonVertexIndex") && !pv) {
            if (prop_k(f, &n, 0, &p)) pv = ler_ints(&p, &npv);
        } else if (nome_igual(&n, "LayerElementUV") && !uv_lido) {
            uv_lido = 1;
            size_t cp = n.filhos_ini;
            no q;
            while (cp < n.fim && ler_no(f, cp, &q)) {
                cp = q.fim;
                if (nome_igual(&q, "MappingInformationType") && prop_k(f, &q, 0, &p)) {
                    if (prop_str_igual(&p, "ByPolygonVertex")) k.mapa = 1;
                    else if (prop_str_igual(&p, "ByVertice") || prop_str_igual(&p, "ByVertex")) k.mapa = 2;
                } else if (nome_igual(&q, "ReferenceInformationType") && prop_k(f, &q, 0, &p)) {
                    if (prop_str_igual(&p, "IndexToDirect") || prop_str_igual(&p, "Index")) k.indexado = 1;
                } else if (nome_igual(&q, "UV") && prop_k(f, &q, 0, &p)) {
                    k.uv = ler_doubles(&p, &k.nuv);
                } else if (nome_igual(&q, "UVIndex") && prop_k(f, &q, 0, &p)) {
                    k.uvi = ler_ints(&p, &k.nuvi);
                }
            }
        }
    }

    if (k.indexado && !k.uvi) k.indexado = 0;
    const int inverter = mat_det3(M) < 0.0;

    if (k.ver && pv) {
        int ini = 0;
        for (int i = 0; i < npv && ok; i++) {
            if (pv[i] >= 0) continue;
            const int cnt = i - ini + 1;
            if (cnt >= 3) {
                const zb_vertice a = fbx_vert(&k, ini, pv[ini]);
                for (int t = 1; t + 1 < cnt && ok; t++) {
                    const int s1 = ini + t, s2 = ini + t + 1;
                    const int v1 = pv[s1] < 0 ? ~pv[s1] : pv[s1];
                    const int v2 = pv[s2] < 0 ? ~pv[s2] : pv[s2];
                    const zb_vertice b = fbx_vert(&k, s1, v1);
                    const zb_vertice d = fbx_vert(&k, s2, v2);
                    ok = inverter ? cons_tri(c, &a, &d, &b) : cons_tri(c, &a, &b, &d);
                }
            }
            ini = i + 1;
        }
    }

    if (k.ver) memoria_free(k.ver);
    if (k.uv) memoria_free(k.uv);
    if (k.uvi) memoria_free(k.uvi);
    if (pv) memoria_free(pv);
    return ok;
}

static int achar_modelo(const modelo* m, int n, int64_t id) {
    for (int i = 0; i < n; i++) if (m[i].id == id) return i;
    return -1;
}

static obj3d_handle* carregar_fbx(const uint8_t* d, size_t n, fb_atlas_handle* atlas, float x, float y, float z, float esc) {
    if (n < 27) return NULL;
    fbx f;
    f.d = d;
    f.n = n;
    f.v64 = u32(d + 23) >= 7500;

    modelo* md = (modelo*)memoria_calloc(MAX_MODELOS, sizeof(modelo));
    geo* gs = (geo*)memoria_calloc(MAX_GEOS, sizeof(geo));
    construtor c;
    memoria_zerar(&c, sizeof(c));
    obj3d_handle* res = NULL;
    int nm = 0, ng = 0;
    no top;
    size_t pos = 27;
    if (!md || !gs) goto fim;

    while (ler_no(&f, pos, &top)) {
        pos = top.fim;
        if (nome_igual(&top, "Objects")) {
            size_t cp = top.filhos_ini;
            no q;
            while (cp < top.fim && ler_no(&f, cp, &q)) {
                cp = q.fim;
                if (nome_igual(&q, "Model") && nm < MAX_MODELOS) {
                    modelo* m = &md[nm++];
                    memoria_zerar(m, sizeof(*m));
                    ler_modelo(&f, &q, m);
                } else if (nome_igual(&q, "Geometry") && ng < MAX_GEOS) {
                    prop p;
                    if (!prop_k(&f, &q, 0, &p)) continue;
                    geo* g = &gs[ng++];
                    g->id = p.i;
                    g->modelo_id = 0;
                    g->nd = q;
                }
            }
        }
    }

    pos = 27;
    while (ler_no(&f, pos, &top)) {
        pos = top.fim;
        if (!nome_igual(&top, "Connections")) continue;
        size_t cp = top.filhos_ini;
        no q;
        while (cp < top.fim && ler_no(&f, cp, &q)) {
            cp = q.fim;
            if (!nome_igual(&q, "C")) continue;
            prop a, b, e;
            if (!prop_k(&f, &q, 0, &a) || !prop_str_igual(&a, "OO")) continue;
            if (!prop_k(&f, &q, 1, &b) || !prop_k(&f, &q, 2, &e)) continue;
            const int im = achar_modelo(md, nm, b.i);
            if (im >= 0) md[im].pai = e.i;
            for (int i = 0; i < ng; i++) if (gs[i].id == b.i) gs[i].modelo_id = e.i;
        }
    }

    for (int i = 0; i < ng; i++) {
        mat w = mat_id();
        int im = achar_modelo(md, nm, gs[i].modelo_id);
        c.g[0] = 0;
        c.mt[0] = 0;
        if (im >= 0) {
            w = md[im].local;
            copiar_nome(c.g, md[im].nome, md[im].nome + 48);
            int64_t cur = md[im].pai;
            for (int dp = 0; dp < 32; dp++) {
                const int j = achar_modelo(md, nm, cur);
                if (j < 0) break;
                w = mat_mul(&md[j].local, &w);
                cur = md[j].pai;
            }
        } else {
            prop p;
            if (prop_k(&f, &gs[i].nd, 1, &p) && p.t == 'S') copiar_nome(c.g, (const char*)p.p, (const char*)p.p + p.n);
        }
        c.nova = 1;
        if (!fbx_geometria(&f, &gs[i].nd, &w, &c)) goto fim;
    }

    res = cons_finalizar(&c, atlas, x, y, z, esc);

fim:
    cons_liberar(&c);
    if (md) memoria_free(md);
    if (gs) memoria_free(gs);
    return res;
}

obj3d_handle* obj3d_carregar_memoria(fb_atlas_handle* atlas, const uint8_t* dados, size_t tamanho, float x, float y, float z, float escala) {
    if (!dados || !tamanho) return NULL;
    static const char magia[] = "Kaydara FBX Binary";
    int fbx_bin = tamanho >= 27;
    for (int i = 0; fbx_bin && magia[i]; i++) if (dados[i] != (uint8_t)magia[i]) fbx_bin = 0;
    if (fbx_bin) return carregar_fbx(dados, tamanho, atlas, x, y, z, escala);
    return carregar_obj((const char*)dados, tamanho, atlas, x, y, z, escala);
}

obj3d_handle* obj3d_carregar(fb_atlas_handle* atlas, const char* caminho, float x, float y, float z, float escala) {
    if (!caminho) return NULL;
    size_t tam = 0;
    char* dados = arquivo_ler_tudo_tam(caminho, &tam);
    if (!dados) return NULL;
    obj3d_handle* o = obj3d_carregar_memoria(atlas, (const uint8_t*)dados, tam, x, y, z, escala);
    memoria_free(dados);
    return o;
}

void obj3d_normalizar(obj3d_handle* o) {
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
    if (!tem) return;
    float ext = mx[0] - mn[0];
    if (mx[1] - mn[1] > ext) ext = mx[1] - mn[1];
    if (mx[2] - mn[2] > ext) ext = mx[2] - mn[2];
    if (ext <= 0.0f) return;
    const float cx = (mn[0] + mx[0]) * 0.5f, cy = (mn[1] + mx[1]) * 0.5f, cz = (mn[2] + mx[2]) * 0.5f;
    const float inv = 1.0f / ext;
    for (int i = 0; i < o->n_partes; i++) {
        obj3d_parte* p = &o->partes[i];
        for (int k = 0; k < p->n_vertices; k++) {
            p->vertices[k].x = (p->vertices[k].x - cx) * inv;
            p->vertices[k].y = (p->vertices[k].y - cy) * inv;
            p->vertices[k].z = (p->vertices[k].z - cz) * inv;
        }
    }
    obj3d_recalcular_limites(o);
}
