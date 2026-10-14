#include "fb_3d.hpp"
#include <stdio.h>
#include <stdlib.h>

malha::~malha() { liberar(); }

void malha::liberar() {
    memoria_free(vs_);
    memoria_free(fs_);
    vs_ = nullptr;
    fs_ = nullptr;
    nv_ = nf_ = cap_v_ = cap_f_ = 0;
}

void malha::roubar(malha& o) noexcept {
    vs_ = o.vs_;       fs_ = o.fs_;
    nv_ = o.nv_;       nf_ = o.nf_;
    cap_v_ = o.cap_v_; cap_f_ = o.cap_f_;
    o.vs_ = nullptr;   o.fs_ = nullptr;
    o.nv_ = o.nf_ = o.cap_v_ = o.cap_f_ = 0;
}

malha::malha(malha&& o) noexcept { roubar(o); }

malha& malha::operator=(malha&& o) noexcept {
    if (this != &o) {
        liberar();
        roubar(o);
    }
    return *this;
}

bool malha::reservar(int max_vertices, int max_faces) {
    liberar();

    if (max_vertices <= 0 || max_faces <= 0 || max_vertices > 65535) return false;

    vs_ = (vertice*)memoria_malloc((size_t)max_vertices * sizeof(vertice));
    fs_ = (face*)   memoria_malloc((size_t)max_faces    * sizeof(face));
    if (!vs_ || !fs_) { liberar(); return false; }

    cap_v_ = max_vertices;
    cap_f_ = max_faces;
    return true;
}

int malha::add_vertice(q16_t x, q16_t y, q16_t z) {
    if (nv_ >= cap_v_) return -1;
    vs_[nv_] = { x, y, z };
    return nv_++;
}

bool malha::add_face(int a, int b, int c) {
    if (nf_ >= cap_f_) return false;
    if ((unsigned)a >= (unsigned)nv_ || (unsigned)b >= (unsigned)nv_ || (unsigned)c >= (unsigned)nv_) return false;
    fs_[nf_++] = { (uint16_t)a, (uint16_t)b, (uint16_t)c, 0, 0, 0 };
    return true;
}

void malha::add_quad(int a, int b, int c, int d) {
    add_face(a, b, c);
    add_face(a, c, d);
}

void malha::orientar_para_fora() {

    for (int i = 0; i < nf_; i++) {
        face& f = fs_[i];
        const vertice &a = vs_[f.a], &b = vs_[f.b], &c = vs_[f.c];

        const float ax = q16::para_float(a.x), ay = q16::para_float(a.y), az = q16::para_float(a.z);
        const float ux = q16::para_float(b.x) - ax, uy = q16::para_float(b.y) - ay, uz = q16::para_float(b.z) - az;
        const float vx = q16::para_float(c.x) - ax, vy = q16::para_float(c.y) - ay, vz = q16::para_float(c.z) - az;

        const float nx = uy * vz - uz * vy;
        const float ny = uz * vx - ux * vz;
        const float nz = ux * vy - uy * vx;

        const float gx = ax + (ux + vx) / 3.0f;
        const float gy = ay + (uy + vy) / 3.0f;
        const float gz = az + (uz + vz) / 3.0f;

        if (nx * gx + ny * gy + nz * gz < 0.0f) {
            const uint16_t t = f.b;
            f.b = f.c;
            f.c = t;
        }
    }

    calcular_normais();
}

void malha::calcular_normais() {
    for (int i = 0; i < nf_; i++) {
        face& f = fs_[i];
        const vertice &a = vs_[f.a], &b = vs_[f.b], &c = vs_[f.c];

        const int64_t ux = (int64_t)b.x - a.x, uy = (int64_t)b.y - a.y, uz = (int64_t)b.z - a.z;
        const int64_t vx = (int64_t)c.x - a.x, vy = (int64_t)c.y - a.y, vz = (int64_t)c.z - a.z;

        int64_t nx = uy * vz - uz * vy;
        int64_t ny = uz * vx - ux * vz;
        int64_t nz = ux * vy - uy * vx;

        int64_t mx = nx < 0 ? -nx : nx;
        const int64_t my = ny < 0 ? -ny : ny, mz = nz < 0 ? -nz : nz;
        if (my > mx) mx = my;
        if (mz > mx) mx = mz;

        f.nx = f.ny = f.nz = 0;
        if (mx == 0) continue;

        int sh = 0;
        while ((mx >> sh) > ((int64_t)1 << 28)) sh++;
        nx >>= sh; ny >>= sh; nz >>= sh;

        const int64_t len = (int64_t)lut3d::isqrt64((uint64_t)(nx * nx + ny * ny + nz * nz));
        if (len == 0) continue;

        auto q15 = [&](int64_t v) {
            int64_t r = (v * trig::UM) / len;
            if (r >  32767) r =  32767;
            if (r < -32767) r = -32767;
            return (int16_t)r;
        };
        f.nx = q15(nx);
        f.ny = q15(ny);
        f.nz = q15(nz);
    }
}

namespace {

q16_t escala(q16_t v, int32_t t) {
    return (q16_t)(((int64_t)v * t) >> trig::BITS);
}

uint16_t fatia(int i, int n) {
    return (uint16_t)(((int64_t)65536 * i) / n);
}

}

void Form3d::cubo(float x, float y, float z, float w, float h, float d) {
    posicionar(x, y, z);
    if (!m_.reservar(8, 12)) return;

    const q16_t hw = q16::de_float(w * 0.5f);
    const q16_t hh = q16::de_float(h * 0.5f);
    const q16_t hd = q16::de_float(d * 0.5f);

    for (int i = 0; i < 8; i++)
        m_.add_vertice((i & 1) ? hw : -hw,
                       (i & 2) ? hh : -hh,
                       (i & 4) ? hd : -hd);

    m_.add_quad(0, 2, 6, 4);
    m_.add_quad(1, 3, 7, 5);
    m_.add_quad(0, 1, 5, 4);
    m_.add_quad(2, 3, 7, 6);
    m_.add_quad(0, 1, 3, 2);
    m_.add_quad(4, 5, 7, 6);

    m_.orientar_para_fora();
}

void Form3d::prisma(float x, float y, float z, float w, float h, float d) {
    posicionar(x, y, z);
    if (!m_.reservar(6, 8)) return;

    const q16_t hw = q16::de_float(w * 0.5f);
    const q16_t hh = q16::de_float(h * 0.5f);
    const q16_t hd = q16::de_float(d * 0.5f);

    m_.add_vertice(-hw, -hh, -hd);
    m_.add_vertice( hw, -hh, -hd);
    m_.add_vertice(  0,  hh, -hd);
    m_.add_vertice(-hw, -hh,  hd);
    m_.add_vertice( hw, -hh,  hd);
    m_.add_vertice(  0,  hh,  hd);

    m_.add_face(0, 1, 2);
    m_.add_face(3, 4, 5);
    m_.add_quad(0, 1, 4, 3);
    m_.add_quad(0, 2, 5, 3);
    m_.add_quad(1, 2, 5, 4);

    m_.orientar_para_fora();
}

void Form3d::piramide(float x, float y, float z, float w, float h, float d) {
    posicionar(x, y, z);
    if (!m_.reservar(5, 6)) return;

    const q16_t hw = q16::de_float(w * 0.5f);
    const q16_t hh = q16::de_float(h * 0.5f);
    const q16_t hd = q16::de_float(d * 0.5f);

    m_.add_vertice(-hw, -hh, -hd);
    m_.add_vertice( hw, -hh, -hd);
    m_.add_vertice( hw, -hh,  hd);
    m_.add_vertice(-hw, -hh,  hd);
    m_.add_vertice(  0,  hh,   0);

    m_.add_quad(0, 1, 2, 3);
    m_.add_face(0, 1, 4);
    m_.add_face(1, 2, 4);
    m_.add_face(2, 3, 4);
    m_.add_face(3, 0, 4);

    m_.orientar_para_fora();
}

void Form3d::cilindro(float x, float y, float z, float raio, float h, int segmentos) {
    posicionar(x, y, z);
    if (segmentos < 3) segmentos = 3;
    const int n = segmentos;
    if (!m_.reservar(2 * n + 2, 4 * n)) return;

    const q16_t r  = q16::de_float(raio);
    const q16_t hh = q16::de_float(h * 0.5f);

    for (int i = 0; i < n; i++) {
        const uint16_t a = fatia(i, n);
        m_.add_vertice(escala(r, trig::cosseno(a)), -hh, escala(r, trig::seno(a)));
    }
    for (int i = 0; i < n; i++) {
        const uint16_t a = fatia(i, n);
        m_.add_vertice(escala(r, trig::cosseno(a)),  hh, escala(r, trig::seno(a)));
    }
    const int cb = m_.add_vertice(0, -hh, 0);
    const int ct = m_.add_vertice(0,  hh, 0);

    for (int i = 0; i < n; i++) {
        const int j = (i + 1) % n;
        m_.add_quad(i, j, n + j, n + i);
        m_.add_face(cb, i, j);
        m_.add_face(ct, n + i, n + j);
    }

    m_.orientar_para_fora();
}

void Form3d::esfera(float x, float y, float z, float raio, int aneis, int setores) {
    posicionar(x, y, z);
    if (aneis < 2)   aneis = 2;
    if (setores < 3) setores = 3;

    const int nv = 2 + (aneis - 1) * setores;
    const int nf = 2 * setores + (aneis - 2) * setores * 2;
    if (!m_.reservar(nv, nf)) return;

    const q16_t r = q16::de_float(raio);

    m_.add_vertice(0, -r, 0);
    m_.add_vertice(0,  r, 0);

    for (int j = 1; j < aneis; j++) {
        const uint16_t lat = (uint16_t)(-(int)trig::QUARTO_VOLTA + ((int)trig::MEIA_VOLTA * j) / aneis);
        const q16_t    yy  = escala(r, trig::seno(lat));
        const q16_t    rr  = escala(r, trig::cosseno(lat));

        for (int i = 0; i < setores; i++) {
            const uint16_t a = fatia(i, setores);
            m_.add_vertice(escala(rr, trig::cosseno(a)), yy, escala(rr, trig::seno(a)));
        }
    }

    auto anel = [&](int j, int i) { return 2 + (j - 1) * setores + (i % setores); };

    for (int i = 0; i < setores; i++) {
        m_.add_face(0, anel(1, i),          anel(1, i + 1));
        m_.add_face(1, anel(aneis - 1, i),  anel(aneis - 1, i + 1));
    }
    for (int j = 1; j < aneis - 1; j++)
        for (int i = 0; i < setores; i++)
            m_.add_quad(anel(j, i), anel(j, i + 1), anel(j + 1, i + 1), anel(j + 1, i));

    m_.orientar_para_fora();
}

void Form3d::angulo_x(float x) { ax_ = trig::de_graus(x); }
void Form3d::angulo_y(float y) { ay_ = trig::de_graus(y); }
void Form3d::angulo_z(float z) { az_ = trig::de_graus(z); }

void Form3d::usar_cor(const fb_cores* cor) { if (cor) cor_ = *cor; }

void Form3d::mover_x(float x) { px_ += x; }
void Form3d::mover_y(float y) { py_ += y; }
void Form3d::mover_z(float z) { pz_ += z; }

void Form3d::mover(float x, float y, float z) { px_ += x; py_ += y; pz_ += z; }

void Form3d::girar(float x, float y, float z) {

    ax_ = (uint16_t)(ax_ + trig::de_graus(x));
    ay_ = (uint16_t)(ay_ + trig::de_graus(y));
    az_ = (uint16_t)(az_ + trig::de_graus(z));
}

void Form3d::posicionar(float x, float y, float z) { px_ = x; py_ = y; pz_ = z; }
void Form3d::definir_colisao(Colisao3d colisao) { col_ = colisao; }

namespace {

const char* obj_espacos(const char* p) {
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

const char* obj_proxima_linha(const char* p) {
    while (*p && *p != '\n') p++;
    return *p ? p + 1 : p;
}

bool obj_linha_e(const char* p, char c) {
    return p[0] == c && (p[1] == ' ' || p[1] == '\t');
}

// Lê um índice de face: "12", "12/3", "12//4" ou negativo (relativo).
// Devolve false quando a linha acaba.
bool obj_indice(const char*& p, int vistos, int& idx) {
    p = obj_espacos(p);
    if (*p == '\0' || *p == '\n' || *p == '\r' || *p == '#') return false;

    char* fim;
    const long v = strtol(p, &fim, 10);
    if (fim == p) return false;
    p = fim;

    while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;  // pula /vt/vn

    idx = v > 0 ? (int)v - 1 : vistos + (int)v;
    return true;
}

}

bool Form3d::obj(float x, float y, float z, const char* texto,
                 float tamanho, bool inverter_z, bool orientar) {
    posicionar(x, y, z);
    m_.liberar();
    if (!texto) return false;

    // 1) conta vértices e triângulos (polígonos viram leque de triângulos)
    int nv = 0, nf = 0;
    for (const char* p = texto; *p; p = obj_proxima_linha(p)) {
        const char* l = obj_espacos(p);
        if (obj_linha_e(l, 'v')) {
            nv++;
        } else if (obj_linha_e(l, 'f')) {
            const char* q = l + 1;
            int idx, n = 0;
            while (obj_indice(q, 0, idx)) n++;
            if (n >= 3) nf += n - 2;
        }
    }
    if (nv == 0 || nf == 0) return false;
    if (!m_.reservar(nv, nf)) return false;   // falha acima de 65535 vértices

    // 2) lê vértices em float, acha a caixa para centralizar e escalar
    float* tmp = (float*)memoria_malloc((size_t)nv * 3 * sizeof(float));
    if (!tmp) { m_.liberar(); return false; }

    float mn[3] = {  1e30f,  1e30f,  1e30f };
    float mx[3] = { -1e30f, -1e30f, -1e30f };
    int k = 0;
    for (const char* p = texto; *p && k < nv; p = obj_proxima_linha(p)) {
        const char* l = obj_espacos(p);
        if (!obj_linha_e(l, 'v')) continue;

        const char* q = l + 1;
        for (int c = 0; c < 3; c++) {
            char* e;
            float f = strtof(q, &e);
            q = e;
            if (c == 2 && inverter_z) f = -f;
            tmp[k * 3 + c] = f;
            if (f < mn[c]) mn[c] = f;
            if (f > mx[c]) mx[c] = f;
        }
        k++;
    }

    const float cx = (mn[0] + mx[0]) * 0.5f, cy = (mn[1] + mx[1]) * 0.5f, cz = (mn[2] + mx[2]) * 0.5f;
    float maior = mx[0] - mn[0];
    if (mx[1] - mn[1] > maior) maior = mx[1] - mn[1];
    if (mx[2] - mn[2] > maior) maior = mx[2] - mn[2];
    const float esc = (tamanho > 0.0f && maior > 0.0f) ? tamanho / maior : 1.0f;

    for (int i = 0; i < k; i++)
        m_.add_vertice(q16::de_float((tmp[i * 3 + 0] - cx) * esc),
                       q16::de_float((tmp[i * 3 + 1] - cy) * esc),
                       q16::de_float((tmp[i * 3 + 2] - cz) * esc));
    memoria_free(tmp);

    // 3) faces; ao inverter o Z o sentido dos triângulos também inverte
    int vistos = 0;
    for (const char* p = texto; *p; p = obj_proxima_linha(p)) {
        const char* l = obj_espacos(p);
        if (obj_linha_e(l, 'v')) { vistos++; continue; }
        if (!obj_linha_e(l, 'f')) continue;

        const char* q = l + 1;
        int a, b, c;
        if (!obj_indice(q, vistos, a) || !obj_indice(q, vistos, b)) continue;
        while (obj_indice(q, vistos, c)) {
            if (inverter_z) m_.add_face(a, c, b);
            else            m_.add_face(a, b, c);
            b = c;
        }
    }

    if (m_.num_faces() == 0) { m_.liberar(); return false; }

    if (orientar) m_.orientar_para_fora();
    else          m_.calcular_normais();
    return true;
}

bool Form3d::obj_arquivo(float x, float y, float z, const char* caminho,
                         float tamanho, bool inverter_z, bool orientar) {
    FILE* f = caminho ? fopen(caminho, "rb") : nullptr;
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    const long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return false; }

    char* buf = (char*)memoria_malloc((size_t)n + 1);
    if (!buf) { fclose(f); return false; }

    const size_t lido = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[lido] = '\0';

    const bool ok = obj(x, y, z, buf, tamanho, inverter_z, orientar);
    memoria_free(buf);
    return ok;
}

bool Form3d::carregar_vertices(float x, float y, float z,
                               const Vert3d* verts, int n_verts,
                               const uint16_t* idx, int n_idx,
                               bool orientar) {
    posicionar(x, y, z);
    m_.liberar();

    if (!verts || n_verts < 3) return false;

    const int n_faces = idx ? n_idx / 3 : n_verts / 3;
    if (n_faces <= 0) return false;
    if (!m_.reservar(n_verts, n_faces)) return false;

    for (int i = 0; i < n_verts; i++)
        m_.add_vertice(q16::de_float(verts[i].x),
                       q16::de_float(verts[i].y),
                       q16::de_float(verts[i].z));

    for (int f = 0; f < n_faces; f++) {
        const int a = idx ? idx[f * 3 + 0] : f * 3 + 0;
        const int b = idx ? idx[f * 3 + 1] : f * 3 + 1;
        const int c = idx ? idx[f * 3 + 2] : f * 3 + 2;

        if (!m_.add_face(a, b, c)) {
            m_.liberar();
            return false;
        }
    }

    if (orientar) m_.orientar_para_fora();
    else          m_.calcular_normais();
    return true;
}

/* ---- API C ---- */

namespace {
inline Form3d*       F(f3d_handle* h)             { return reinterpret_cast<Form3d*>(h); }
inline const Form3d* F(const f3d_handle* h)       { return reinterpret_cast<const Form3d*>(h); }
inline fb_cores      cor_de(uint32_t p)           { return fb_cores::desempacotar(p); }
}

extern "C" {

/* ---- Form3d ---- */
f3d_handle* f3d_criar(void)                 { return reinterpret_cast<f3d_handle*>(new Form3d); }
void f3d_destruir(f3d_handle* f)            { delete F(f); }
void f3d_cubo(f3d_handle* f, float x, float y, float z, float w, float h, float d)      { if (f) F(f)->cubo(x, y, z, w, h, d); }
void f3d_esfera(f3d_handle* f, float x, float y, float z, float raio, int aneis, int setores) { if (f) F(f)->esfera(x, y, z, raio, aneis, setores); }
void f3d_prisma(f3d_handle* f, float x, float y, float z, float w, float h, float d)    { if (f) F(f)->prisma(x, y, z, w, h, d); }
void f3d_piramide(f3d_handle* f, float x, float y, float z, float w, float h, float d)  { if (f) F(f)->piramide(x, y, z, w, h, d); }
void f3d_cilindro(f3d_handle* f, float x, float y, float z, float raio, float h, int segmentos) { if (f) F(f)->cilindro(x, y, z, raio, h, segmentos); }
void f3d_angulo_x(f3d_handle* f, float x)   { if (f) F(f)->angulo_x(x); }
void f3d_angulo_y(f3d_handle* f, float y)   { if (f) F(f)->angulo_y(y); }
void f3d_angulo_z(f3d_handle* f, float z)   { if (f) F(f)->angulo_z(z); }
void f3d_usar_cor(f3d_handle* f, uint32_t cor) { if (f) { const fb_cores c = cor_de(cor); F(f)->usar_cor(&c); } }
uint32_t f3d_cor(const f3d_handle* f)       { return f ? F(f)->cor().empacotar() : 0; }
void f3d_mover_x(f3d_handle* f, float x)    { if (f) F(f)->mover_x(x); }
void f3d_mover_y(f3d_handle* f, float y)    { if (f) F(f)->mover_y(y); }
void f3d_mover_z(f3d_handle* f, float z)    { if (f) F(f)->mover_z(z); }
void f3d_mover(f3d_handle* f, float x, float y, float z)      { if (f) F(f)->mover(x, y, z); }
void f3d_girar(f3d_handle* f, float x, float y, float z)      { if (f) F(f)->girar(x, y, z); }
void f3d_posicionar(f3d_handle* f, float x, float y, float z) { if (f) F(f)->posicionar(x, y, z); }
float f3d_x(const f3d_handle* f)            { return f ? F(f)->posicao_atual_x() : 0.0f; }
float f3d_y(const f3d_handle* f)            { return f ? F(f)->posicao_atual_y() : 0.0f; }
float f3d_z(const f3d_handle* f)            { return f ? F(f)->posicao_atual_z() : 0.0f; }
uint16_t f3d_rot_x(const f3d_handle* f)     { return f ? F(f)->rot_x() : 0; }
uint16_t f3d_rot_y(const f3d_handle* f)     { return f ? F(f)->rot_y() : 0; }
uint16_t f3d_rot_z(const f3d_handle* f)     { return f ? F(f)->rot_z() : 0; }
void f3d_definir_colisao(f3d_handle* f, int tipo) { if (f) F(f)->definir_colisao(tipo ? OBB : AABB); }
int  f3d_colisao(const f3d_handle* f)       { return f ? (int)F(f)->colisao() : 0; }
int  f3d_num_vertices(const f3d_handle* f)  { return f ? F(f)->dados().num_vertices() : 0; }
int  f3d_num_faces(const f3d_handle* f)     { return f ? F(f)->dados().num_faces() : 0; }

int f3d_obj(f3d_handle* f, float x, float y, float z, const char* texto, float tamanho, int inverter_z, int orientar) {
    return f && F(f)->obj(x, y, z, texto, tamanho, inverter_z != 0, orientar != 0);
}
int f3d_obj_arquivo(f3d_handle* f, float x, float y, float z, const char* caminho, float tamanho, int inverter_z, int orientar) {
    return f && F(f)->obj_arquivo(x, y, z, caminho, tamanho, inverter_z != 0, orientar != 0);
}
int f3d_carregar_vertices(f3d_handle* f, float x, float y, float z, const float* verts, int n_verts,
                          const uint16_t* idx, int n_idx, int orientar) {
    return f && F(f)->carregar_vertices(x, y, z, reinterpret_cast<const Vert3d*>(verts), n_verts, idx, n_idx, orientar != 0);
}

}