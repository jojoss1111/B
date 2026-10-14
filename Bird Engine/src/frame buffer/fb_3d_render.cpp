#include "fb_3d.hpp"
#include "fb_3d_mat.hpp"

z_buffer::~z_buffer() { liberar(); }

void z_buffer::liberar() {
    memoria_free(d_);
    d_ = nullptr;
    w_ = h_ = 0;
}

bool z_buffer::criar(int largura, int altura) {
    if (d_ && largura == w_ && altura == h_) return true;

    liberar();
    if (largura <= 0 || altura <= 0) return false;

    d_ = (uint32_t*)memoria_malloc((size_t)largura * altura * sizeof(uint32_t));
    if (!d_) return false;

    w_ = largura;
    h_ = altura;
    limpar();
    return true;
}

void z_buffer::limpar() {
    if (!d_) return;
    for (size_t i = 0, n = (size_t)w_ * h_; i < n; i++) d_[i] = 0;
}

namespace {

using mat3d::rot_x;
using mat3d::rot_y;
using mat3d::rot_z;
using mat3d::mul;

struct v3 { int32_t x, y, z; };

v3 aplicar(const mat3& m, int32_t x, int32_t y, int32_t z) {
    return {
        (int32_t)(((int64_t)m.m[0][0] * x + (int64_t)m.m[0][1] * y + (int64_t)m.m[0][2] * z) >> trig::BITS),
        (int32_t)(((int64_t)m.m[1][0] * x + (int64_t)m.m[1][1] * y + (int64_t)m.m[1][2] * z) >> trig::BITS),
        (int32_t)(((int64_t)m.m[2][0] * x + (int64_t)m.m[2][1] * y + (int64_t)m.m[2][2] * z) >> trig::BITS),
    };
}

int recortar_near(const v3 in[3], v3 out[4], int32_t nq) {
    int n = 0;
    for (int i = 0; i < 3; i++) {
        const v3 &p = in[i], &q = in[(i + 1) % 3];
        const bool pd = p.z >= nq, qd = q.z >= nq;
        if (pd) out[n++] = p;
        if (pd != qd) {
            const double t = ((double)nq - (double)p.z) / ((double)q.z - (double)p.z);
            out[n++] = { (int32_t)((double)p.x + ((double)q.x - (double)p.x) * t),
                         (int32_t)((double)p.y + ((double)q.y - (double)p.y) * t),
                         nq };
        }
    }
    return n;
}

inline int indice_neblina(uint32_t d) {
    const int msb = lut3d::msb32(d);
    const uint32_t mant = msb >= 4 ? (d >> (msb - 4)) & 15u : (d << (4 - msb)) & 15u;
    return (msb << 4) | (int)mant;
}

inline int32_t q16_limitado(float v) {
    if (v >  30000.0f) v =  30000.0f;
    if (v < -30000.0f) v = -30000.0f;
    return q16::de_float(v);
}

inline int64_t min3(int64_t a, int64_t b, int64_t c) { int64_t m = a < b ? a : b; return m < c ? m : c; }
inline int64_t max3(int64_t a, int64_t b, int64_t c) { int64_t m = a > b ? a : b; return m > c ? m : c; }

constexpr int64_t LIMITE_TELA = (int64_t)1 << 27;

}

render3d::render3d() {
    luz(-0.35f, 0.75f, -0.55f);
}

render3d::~render3d() {
    memoria_free(vp_);
    vp_ = nullptr;
    cap_vp_ = 0;
}

void render3d::fov(float graus) {
    if (graus < 10.0f)  graus = 10.0f;
    if (graus > 150.0f) graus = 150.0f;
    fov_ = graus;
}

void render3d::plano_proximo(float unidades) {
    if (unidades < 0.05f) unidades = 0.05f;
    near_q_ = q16::de_float(unidades);
}

void render3d::camera(float x, float y, float z) {
    cam_x_ = x; cam_y_ = y; cam_z_ = z;
}

void render3d::camera_angulo(float yaw, float pitch, float roll) {
    yaw_   = trig::de_graus(yaw);
    pitch_ = trig::de_graus(pitch);
    roll_  = trig::de_graus(roll);
}

void render3d::luz(float x, float y, float z) {
    const int64_t a = (int64_t)(x * 65536.0f), b = (int64_t)(y * 65536.0f), c = (int64_t)(z * 65536.0f);
    const int64_t len = (int64_t)lut3d::isqrt64((uint64_t)(a * a + b * b + c * c));
    if (len == 0) return;
    luz_[0] = (int32_t)((a * trig::UM) / len);
    luz_[1] = (int32_t)((b * trig::UM) / len);
    luz_[2] = (int32_t)((c * trig::UM) / len);
}

void render3d::ambiente(int v) {
    amb_ = v < 0 ? 0 : (v > 255 ? 255 : v);
}

void render3d::neblina_usar(const neblina& n) {
    neb_ = n;
    if (!n.ativa) return;

    float vao = n.fim - n.inicio;
    if (vao < 0.001f) vao = 0.001f;

    for (int i = 0; i < 512; i++) {
        const int msb = i >> 4, mant = i & 15;
        float d = 16.0f + (float)mant + 0.5f;
        for (int k = msb; k < 4; k++) d *= 0.5f;
        for (int k = 4; k < msb; k++) d *= 2.0f;

        const float z = 16777216.0f / d;
        float f = (z - n.inicio) / vao;
        if (f < 0.0f) f = 0.0f;
        if (f > 1.0f) f = 1.0f;
        fog_lut_[i] = (uint8_t)(f * 255.0f + 0.5f);
    }
}

int render3d::luz_nova(const luz3d& l) {
    for (int i = 0; i < LUZES_MAX; i++) {
        if (usada_[i]) continue;
        usada_[i] = true;
        luzes_[i] = l;
        return i;
    }
    return -1;
}

luz3d* render3d::luz_obter(int id) {
    return (id >= 0 && id < LUZES_MAX && usada_[id]) ? &luzes_[id] : nullptr;
}

void render3d::luz_remover(int id) {
    if (id >= 0 && id < LUZES_MAX) usada_[id] = false;
}

void render3d::luzes_limpar() {
    for (int i = 0; i < LUZES_MAX; i++) usada_[i] = false;
    n_lz_ = 0;
}

void render3d::preparar_luzes() {
    n_lz_ = 0;

    for (int i = 0; i < LUZES_MAX; i++) {
        if (!usada_[i] || !luzes_[i].ativa) continue;

        const luz3d& L = luzes_[i];
        luz_cam& o = lz_[n_lz_];

        float alc = L.alcance;
        if (alc < 0.05f)   alc = 0.05f;
        if (alc > 8000.0f) alc = 8000.0f;
        o.alc = (int64_t)(alc * 65536.0f);
        o.r2  = o.alc * o.alc;
        o.sh  = 0;
        while ((o.r2 >> o.sh) >= ((int64_t)1 << 15)) o.sh++;
        o.inv = ((int64_t)1 << 30) / (o.r2 >> o.sh);
        float sv = L.suavidade;
        if (sv < 0.0f) sv = 0.0f;
        if (sv > 1.0f) sv = 1.0f;
        o.wrap = (int32_t)(sv * 32768.0f + 0.5f);
        const v3 p = aplicar(mv_, q16_limitado(L.x - cam_x_), q16_limitado(L.y - cam_y_), q16_limitado(L.z - cam_z_));
        o.x = p.x; o.y = p.y; o.z = p.z;

        int inten = (int)(L.intensidade * 256.0f + 0.5f);
        if (inten < 0)    inten = 0;
        if (inten > 2048) inten = 2048;
        o.c[0] = ((L.cor.r + (L.cor.r >> 7)) * inten) >> 8;
        o.c[1] = ((L.cor.g + (L.cor.g >> 7)) * inten) >> 8;
        o.c[2] = ((L.cor.b + (L.cor.b >> 7)) * inten) >> 8;

        o.spot = false;
        o.sx = o.sy = o.sz = o.cos_ext = 0;
        o.inv_cone = 0;

        if (L.spot) {
            const int64_t a = (int64_t)(L.dx * 65536.0f), b = (int64_t)(L.dy * 65536.0f), c = (int64_t)(L.dz * 65536.0f);
            const int64_t len = (int64_t)lut3d::isqrt64((uint64_t)(a * a + b * b + c * c));
            if (len > 0) {
                const v3 sd = aplicar(mv_, (int32_t)((a * trig::UM) / len),
                                           (int32_t)((b * trig::UM) / len),
                                           (int32_t)((c * trig::UM) / len));
                o.sx = sd.x; o.sy = sd.y; o.sz = sd.z;

                float ce = L.cone_externo;
                if (ce < 1.0f)  ce = 1.0f;
                if (ce > 89.0f) ce = 89.0f;
                float ci = L.cone_interno;
                if (ci < 0.0f) ci = 0.0f;
                if (ci > ce)   ci = ce;

                o.cos_ext = trig::cosseno(trig::de_graus(ce));
                int32_t cint = trig::cosseno(trig::de_graus(ci));
                if (cint <= o.cos_ext) cint = o.cos_ext + 1;
                o.inv_cone = ((int64_t)1 << 30) / (cint - o.cos_ext);
                o.spot = true;
            }
        }

        n_lz_++;
    }
}

void render3d::comecar(frame_buffer& fb) {
    tris_ = 0;
    n_lz_ = 0;
    if (!fb.valido() || !z_.criar(fb.largura(), fb.altura())) { w_ = h_ = 0; return; }

    z_.limpar();
    w_ = fb.largura();
    h_ = fb.altura();
    cx4_ = w_ * 8;
    cy4_ = h_ * 8;

    const uint16_t meio = trig::de_graus(fov_ * 0.5f);
    const int32_t  s = trig::seno(meio), c = trig::cosseno(meio);
    focal4_ = s > 0 ? ((int64_t)w_ * 8 * c) / s : 0;
    ifocal_ = focal4_ > 0 ? ((int64_t)1 << 30) / focal4_ : 0;

    mv_ = mul(rot_z(roll_), mul(rot_x(pitch_), rot_y((uint16_t)(65536 - yaw_))));
    preparar_luzes();
}

render3d::ponto render3d::projetar(int32_t vx, int32_t vy, int32_t vz) const {
    const uint32_t d = lut3d::recip24((uint32_t)vz);

    const int64_t rx = ((int64_t)vx * d) >> 24;
    const int64_t ry = ((int64_t)vy * d) >> 24;

    int64_t sx = cx4_ + ((rx * focal4_) >> 16);
    int64_t sy = cy4_ - ((ry * focal4_) >> 16);

    if (sx >  LIMITE_TELA) sx =  LIMITE_TELA;
    if (sx < -LIMITE_TELA) sx = -LIMITE_TELA;
    if (sy >  LIMITE_TELA) sy =  LIMITE_TELA;
    if (sy < -LIMITE_TELA) sy = -LIMITE_TELA;

    return { (int32_t)sx, (int32_t)sy, d };
}

int render3d::sombrear(const mat3& mm, const face& fc) const {
    const int64_t nx = ((int64_t)mm.m[0][0] * fc.nx + (int64_t)mm.m[0][1] * fc.ny + (int64_t)mm.m[0][2] * fc.nz) >> trig::BITS;
    const int64_t ny = ((int64_t)mm.m[1][0] * fc.nx + (int64_t)mm.m[1][1] * fc.ny + (int64_t)mm.m[1][2] * fc.nz) >> trig::BITS;
    const int64_t nz = ((int64_t)mm.m[2][0] * fc.nx + (int64_t)mm.m[2][1] * fc.ny + (int64_t)mm.m[2][2] * fc.nz) >> trig::BITS;

    int64_t dot = (nx * luz_[0] + ny * luz_[1] + nz * luz_[2]) >> trig::BITS;
    if (dot < 0) dot = 0;

    int64_t inten = amb_ + (((int64_t)(255 - amb_) * dot) >> trig::BITS);
    if (inten > 255) inten = 255;

    return (int)inten + ((int)inten >> 7);
}

void render3d::preparar_face(face_ctx& f, const mat3& c, const face& fc, int32_t ax, int32_t ay, int32_t az) const {
    f.nluz = 0;
    if (n_lz_ == 0) return;

    const v3 n = aplicar(c, fc.nx, fc.ny, fc.nz);
    f.nx = n.x; f.ny = n.y; f.nz = n.z;

    for (int j = 0; j < n_lz_; j++) {
        const luz_cam& l = lz_[j];
        const int64_t sd = (int64_t)n.x * (l.x - ax) + (int64_t)n.y * (l.y - ay) + (int64_t)n.z * (l.z - az);
        if (sd <= 0 || sd >= (l.alc << 15)) continue;
        f.luz[f.nluz++] = (uint8_t)j;
    }
}

void render3d::iluminar(const face_ctx& f, int x, int y, uint32_t dv, int& r, int& g, int& b) const {
    if (dv < 4096) return;

    const int64_t z16 = lut3d::recip24(dv);
    const int64_t px = (z16 * (((int64_t)x * 16 + 8 - cx4_) * ifocal_)) >> 30;
    const int64_t py = (z16 * (((int64_t)cy4_ - ((int64_t)y * 16 + 8)) * ifocal_)) >> 30;
    const int64_t pz = z16;

    int64_t ar = f.k, ag = f.k, ab = f.k;

    for (int j = 0; j < f.nluz; j++) {
        const luz_cam& l = lz_[f.luz[j]];

        const int64_t dx = l.x - px, dy = l.y - py, dz = l.z - pz;
        if (dx > l.alc || dx < -l.alc || dy > l.alc || dy < -l.alc || dz > l.alc || dz < -l.alc) continue;

        const int64_t d2 = dx * dx + dy * dy + dz * dz;
        if (d2 >= l.r2) continue;

        const int64_t nd = f.nx * dx + f.ny * dy + f.nz * dz;
        if (nd <= 0) continue;

        uint32_t rs;
        int e2;
        lut3d::rsqrt64((uint64_t)d2, rs, e2);

        int64_t ndl = (nd * rs) >> (16 + e2);
        if (ndl > 32768) ndl = 32768;

        int64_t att = 32768 - (((d2 >> l.sh) * l.inv) >> 15);
        if (att <= 0) continue;
        att = (att * (32768 + att)) >> 16;

        ndl += ((32768 - ndl) * l.wrap) >> 15;

        int64_t inten = (att * ndl) >> 15;
        if (l.spot) {
            const int64_t sd = -(l.sx * dx + l.sy * dy + l.sz * dz);
            if (sd <= 0) continue;
            int64_t cs = ((sd * rs) >> (16 + e2)) - l.cos_ext;
            if (cs <= 0) continue;
            int64_t cone = (cs * l.inv_cone) >> 15;
            if (cone > 32768) cone = 32768;
            cone = (cone * cone) >> 15;
            inten = (inten * cone) >> 15;
        }

        ar += (inten * l.c[0]) >> 15;
        ag += (inten * l.c[1]) >> 15;
        ab += (inten * l.c[2]) >> 15;
    }

    if (ar > 512) ar = 512;
    if (ag > 512) ag = 512;
    if (ab > 512) ab = 512;

    const int rr = (int)((f.br * ar) >> 8), gg = (int)((f.bg * ag) >> 8), bb = (int)((f.bb * ab) >> 8);
    r = rr > 255 ? 255 : rr;
    g = gg > 255 ? 255 : gg;
    b = bb > 255 ? 255 : bb;
}

void render3d::desenhar(frame_buffer& fb, const Form3d& f) {
    const malha& m = f.dados();
    if (!m.valida() || !fb.valido() || !z_.valido()) return;
    if (fb.largura() != w_ || fb.altura() != h_) return;

    const int nv = m.num_vertices();
    if (nv > cap_vp_) {
        memoria_free(vp_);
        vp_ = (vproj*)memoria_malloc((size_t)nv * sizeof(vproj));
        cap_vp_ = vp_ ? nv : 0;
        if (!vp_) return;
    }

    const mat3 mm = mat3d::modelo(f);
    const mat3& mv = mv_;
    const mat3 c  = mul(mv, mm);

    const v3 t = aplicar(mv, q16::de_float(f.posicao_atual_x() - cam_x_),
                             q16::de_float(f.posicao_atual_y() - cam_y_),
                             q16::de_float(f.posicao_atual_z() - cam_z_));

    const vertice* vs = m.vertices();
    for (int i = 0; i < nv; i++) {
        const vertice& v = vs[i];
        vproj& p = vp_[i];

        p.vx = (int32_t)(((int64_t)c.m[0][0] * v.x + (int64_t)c.m[0][1] * v.y + (int64_t)c.m[0][2] * v.z) >> trig::BITS) + t.x;
        p.vy = (int32_t)(((int64_t)c.m[1][0] * v.x + (int64_t)c.m[1][1] * v.y + (int64_t)c.m[1][2] * v.z) >> trig::BITS) + t.y;
        p.vz = (int32_t)(((int64_t)c.m[2][0] * v.x + (int64_t)c.m[2][1] * v.y + (int64_t)c.m[2][2] * v.z) >> trig::BITS) + t.z;

        if (p.vz >= near_q_) {
            const ponto q = projetar(p.vx, p.vy, p.vz);
            p.sx = q.x; p.sy = q.y; p.d = q.d;
        } else {
            p.d = 0;
        }
    }

    const face* fs = m.faces();
    const fb_cores base = f.cor();

    for (int i = 0, nf = m.num_faces(); i < nf; i++) {
        const face& fc = fs[i];
        const vproj &a = vp_[fc.a], &b = vp_[fc.b], &cc = vp_[fc.c];

        const bool oa = a.vz >= near_q_, ob = b.vz >= near_q_, oc = cc.vz >= near_q_;
        if (!oa && !ob && !oc) continue;

        if (oa && ob && oc) {
            const ponto pa = { a.sx, a.sy, a.d }, pb = { b.sx, b.sy, b.d }, pc = { cc.sx, cc.sy, cc.d };

            const int64_t area = ((int64_t)pb.x - pa.x) * ((int64_t)pc.y - pa.y)
                               - ((int64_t)pb.y - pa.y) * ((int64_t)pc.x - pa.x);
            if (area <= 0) continue;

            face_ctx fx;
            fx.k = sombrear(mm, fc);
            fx.br = base.r; fx.bg = base.g; fx.bb = base.b;
            fx.r = (base.r * fx.k) >> 8; fx.g = (base.g * fx.k) >> 8; fx.b = (base.b * fx.k) >> 8;
            preparar_face(fx, c, fc, a.vx, a.vy, a.vz);

            rasterizar(fb, pa, pb, pc, area, fx);
            tris_++;
            continue;
        }

        const v3 in[3] = { { a.vx, a.vy, a.vz }, { b.vx, b.vy, b.vz }, { cc.vx, cc.vy, cc.vz } };
        v3 poly[4];
        const int n = recortar_near(in, poly, near_q_);
        if (n < 3) continue;

        ponto pp[4];
        for (int k = 0; k < n; k++) pp[k] = projetar(poly[k].x, poly[k].y, poly[k].z);

        face_ctx fx;
        fx.k = sombrear(mm, fc);
        fx.br = base.r; fx.bg = base.g; fx.bb = base.b;
        fx.r = (base.r * fx.k) >> 8; fx.g = (base.g * fx.k) >> 8; fx.b = (base.b * fx.k) >> 8;
        preparar_face(fx, c, fc, poly[0].x, poly[0].y, poly[0].z);

        for (int k = 1; k + 1 < n; k++) {
            const int64_t area = ((int64_t)pp[k].x - pp[0].x) * ((int64_t)pp[k + 1].y - pp[0].y)
                               - ((int64_t)pp[k].y - pp[0].y) * ((int64_t)pp[k + 1].x - pp[0].x);
            if (area <= 0) continue;

            rasterizar(fb, pp[0], pp[k], pp[k + 1], area, fx);
            tris_++;
        }
    }
}

void render3d::rasterizar(frame_buffer& fb, const ponto& a, const ponto& b, const ponto& c,
                          int64_t area, const face_ctx& fx) {
    const int w = fb.largura(), h = fb.altura();

    int x0 = (int)(min3(a.x, b.x, c.x) >> 4), x1 = (int)(max3(a.x, b.x, c.x) >> 4);
    int y0 = (int)(min3(a.y, b.y, c.y) >> 4), y1 = (int)(max3(a.y, b.y, c.y) >> 4);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > w - 1) x1 = w - 1;
    if (y1 > h - 1) y1 = h - 1;
    if (x0 > x1 || y0 > y1) return;

    const int64_t ax = a.x, ay = a.y, bx = b.x, by = b.y, cx = c.x, cy = c.y;

    const int64_t px = (int64_t)x0 * 16 + 8, py = (int64_t)y0 * 16 + 8;

    int64_t e0 = (cx - bx) * (py - by) - (cy - by) * (px - bx);
    int64_t e1 = (ax - cx) * (py - cy) - (ay - cy) * (px - cx);
    int64_t e2 = (bx - ax) * (py - ay) - (by - ay) * (px - ax);

    const int64_t e0dx = -(cy - by) * 16, e0dy = (cx - bx) * 16;
    const int64_t e1dx = -(ay - cy) * 16, e1dy = (ax - cx) * 16;
    const int64_t e2dx = -(by - ay) * 16, e2dy = (bx - ax) * 16;

    auto vies = [](int64_t dx, int64_t dy) { return (dy < 0 || (dy == 0 && dx > 0)) ? 0 : 1; };
    e0 -= vies(cx - bx, cy - by);
    e1 -= vies(ax - cx, ay - cy);
    e2 -= vies(bx - ax, by - ay);

    const double dA = (double)area;
    const double d0 = (double)a.d, d1 = (double)b.d, d2 = (double)c.d;

    const double l1 = (double)((px - ax) * (cy - ay) - (py - ay) * (cx - ax));
    const double l2 = (double)((bx - ax) * (py - ay) - (by - ay) * (px - ax));
    const double dstart = d0 + ((d1 - d0) * l1 + (d2 - d0) * l2) / dA;

    const double gxd = (((d1 - d0) * (double)(cy - ay) - (d2 - d0) * (double)(by - ay)) / dA) * 16.0;
    const double gyd = (((d2 - d0) * (double)(bx - ax) - (d1 - d0) * (double)(cx - ax)) / dA) * 16.0;

    int64_t dd0 = (int64_t)(dstart * 65536.0);
    const int64_t gx = (int64_t)(gxd * 65536.0);
    const int64_t gy = (int64_t)(gyd * 65536.0);

    uint32_t* zrow = z_.dados() + (size_t)y0 * w;
    uint32_t* prow = fb.dados() + (size_t)y0 * w;

    const uint32_t cor = fb_cores{ (uint8_t)fx.r, (uint8_t)fx.g, (uint8_t)fx.b, 255 }.empacotar();
    const bool     fog = neb_.ativa;
    const bool     luz = fx.nluz > 0;

    for (int y = y0; y <= y1; y++, zrow += w, prow += w, e0 += e0dy, e1 += e1dy, e2 += e2dy, dd0 += gy) {
        int64_t w0 = e0, w1 = e1, w2 = e2, dd = dd0;
        bool entrou = false;

        for (int x = x0; x <= x1; x++, w0 += e0dx, w1 += e1dx, w2 += e2dx, dd += gx) {
            if ((w0 | w1 | w2) < 0) {
                if (entrou) break;
                continue;
            }
            entrou = true;

            const int64_t dv = dd >> 16;
            if (dv <= (int64_t)zrow[x]) continue;
            zrow[x] = (uint32_t)dv;

            if (!fog && !luz) { prow[x] = cor; continue; }

            int r = fx.r, g = fx.g, bl = fx.b;
            if (luz) iluminar(fx, x, y, (uint32_t)dv, r, g, bl);

            if (fog) {
                const uint32_t fv = fog_lut_[indice_neblina((uint32_t)dv)];
                if (fv != 0) {
                    const int f2 = (int)fv + ((int)fv >> 7);
                    r  += (((int)neb_.cor.r - r)  * f2) >> 8;
                    g  += (((int)neb_.cor.g - g)  * f2) >> 8;
                    bl += (((int)neb_.cor.b - bl) * f2) >> 8;
                }
            }

            prow[x] = fb_cores{ (uint8_t)r, (uint8_t)g, (uint8_t)bl, 255 }.empacotar();
        }
    }
}

/* ---- API C ---- */

namespace {
inline z_buffer*       Z(z3d_handle* h)             { return reinterpret_cast<z_buffer*>(h); }
inline const z_buffer* Z(const z3d_handle* h)       { return reinterpret_cast<const z_buffer*>(h); }
inline render3d*       R(render3d_handle* h)        { return reinterpret_cast<render3d*>(h); }
inline const render3d* R(const render3d_handle* h)  { return reinterpret_cast<const render3d*>(h); }
inline const Form3d*   FF(const f3d_handle* h)      { return reinterpret_cast<const Form3d*>(h); }
inline fb_cores        cor3(uint32_t p)             { return fb_cores::desempacotar(p); }

inline void  sai_mat(const mat3& m, int32_t* o)     { for (int i = 0; i < 9; i++) o[i] = m.m[i / 3][i % 3]; }
inline mat3  le_mat(const int32_t* p)               { mat3 m; for (int i = 0; i < 9; i++) m.m[i / 3][i % 3] = p[i]; return m; }

luz3d monta_luz(int ativa, int spot, float x, float y, float z, float dx, float dy, float dz,
                uint32_t cor, float inten, float alc, float ci, float ce, float suav) {
    luz3d l;
    l.ativa = ativa != 0;  l.spot = spot != 0;
    l.x = x;  l.y = y;  l.z = z;
    l.dx = dx; l.dy = dy; l.dz = dz;
    l.cor = cor3(cor);
    l.intensidade = inten;  l.alcance = alc;
    l.cone_interno = ci;    l.cone_externo = ce;
    l.suavidade = suav;
    return l;
}
}

extern "C" {

/* ---- mat3d ---- */
void mat3d_rot_x(uint16_t a, int32_t* out9) { if (out9) sai_mat(mat3d::rot_x(a), out9); }
void mat3d_rot_y(uint16_t a, int32_t* out9) { if (out9) sai_mat(mat3d::rot_y(a), out9); }
void mat3d_rot_z(uint16_t a, int32_t* out9) { if (out9) sai_mat(mat3d::rot_z(a), out9); }
void mat3d_mul(const int32_t* a9, const int32_t* b9, int32_t* out9) {
    if (a9 && b9 && out9) sai_mat(mat3d::mul(le_mat(a9), le_mat(b9)), out9);
}
void mat3d_modelo(const f3d_handle* forma, int32_t* out9) {
    if (forma && out9) sai_mat(mat3d::modelo(*FF(forma)), out9);
}

/* ---- z_buffer ---- */
z3d_handle* z3d_criar(int largura, int altura) {
    z_buffer* z = new z_buffer;
    if (!z->criar(largura, altura)) { delete z; return nullptr; }
    return reinterpret_cast<z3d_handle*>(z);
}
void     z3d_destruir(z3d_handle* z)         { delete Z(z); }
void     z3d_limpar(z3d_handle* z)           { if (z) Z(z)->limpar(); }
int      z3d_valido(const z3d_handle* z)     { return z && Z(z)->valido(); }
int      z3d_largura(const z3d_handle* z)    { return z ? Z(z)->largura() : 0; }
int      z3d_altura(const z3d_handle* z)     { return z ? Z(z)->altura()  : 0; }
uint32_t* z3d_dados(z3d_handle* z)           { return z ? Z(z)->dados() : nullptr; }

/* ---- render3d ---- */
render3d_handle* render3d_criar(void)        { return reinterpret_cast<render3d_handle*>(new render3d); }
void render3d_destruir(render3d_handle* r)   { delete R(r); }
void render3d_fov(render3d_handle* r, float g)             { if (r) R(r)->fov(g); }
void render3d_plano_proximo(render3d_handle* r, float u)   { if (r) R(r)->plano_proximo(u); }
void render3d_camera(render3d_handle* r, float x, float y, float z)            { if (r) R(r)->camera(x, y, z); }
void render3d_camera_angulo(render3d_handle* r, float yaw, float pitch, float roll) { if (r) R(r)->camera_angulo(yaw, pitch, roll); }
void render3d_luz(render3d_handle* r, float x, float y, float z)               { if (r) R(r)->luz(x, y, z); }
void render3d_ambiente(render3d_handle* r, int v)          { if (r) R(r)->ambiente(v); }

void render3d_neblina_usar(render3d_handle* r, int ativa, uint32_t cor, float inicio, float fim) {
    if (!r) return;
    neblina n;
    n.ativa = ativa != 0;
    n.cor = cor3(cor);
    n.inicio = inicio;
    n.fim = fim;
    R(r)->neblina_usar(n);
}

int render3d_luz_nova(render3d_handle* r, int ativa, int spot,
                      float x, float y, float z, float dx, float dy, float dz,
                      uint32_t cor, float inten, float alc, float ci, float ce, float suav) {
    if (!r) return -1;
    return R(r)->luz_nova(monta_luz(ativa, spot, x, y, z, dx, dy, dz, cor, inten, alc, ci, ce, suav));
}

int render3d_luz_atualizar(render3d_handle* r, int id, int ativa, int spot,
                           float x, float y, float z, float dx, float dy, float dz,
                           uint32_t cor, float inten, float alc, float ci, float ce, float suav) {
    if (!r) return 0;
    luz3d* l = R(r)->luz_obter(id);
    if (!l) return 0;
    *l = monta_luz(ativa, spot, x, y, z, dx, dy, dz, cor, inten, alc, ci, ce, suav);
    return 1;
}

void render3d_luz_remover(render3d_handle* r, int id)      { if (r) R(r)->luz_remover(id); }
void render3d_luzes_limpar(render3d_handle* r)             { if (r) R(r)->luzes_limpar(); }

void render3d_comecar(render3d_handle* r, fb_handle* fb)   { if (r && fb) R(r)->comecar(fb->fb); }
void render3d_desenhar(render3d_handle* r, fb_handle* fb, const f3d_handle* forma) {
    if (r && fb && forma) R(r)->desenhar(fb->fb, *FF(forma));
}
int  render3d_triangulos(const render3d_handle* r)         { return r ? R(r)->triangulos() : 0; }

}