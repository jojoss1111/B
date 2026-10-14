#include "../frame buffer/fb_3d.hpp"
#include "../frame buffer/fb_3d_mat.hpp"

namespace {

using col3d::vf;
using col3d::caixa;

constexpr float PI_F = 3.14159265f;

inline float absf(float v)                          { return v < 0.0f ? -v : v; }
inline float minf(float a, float b)                 { return a < b ? a : b; }
inline float maxf(float a, float b)                 { return a > b ? a : b; }
inline float limitar(float v, float lo, float hi)   { return v < lo ? lo : (v > hi ? hi : v); }

float raiz(float x) {
    if (x <= 0.0f) return 0.0f;
    union { float f; uint32_t i; } u = { x };
    u.i = 0x1FBD1DF5u + (u.i >> 1);
    float y = u.f;
    y = 0.5f * (y + x / y);
    y = 0.5f * (y + x / y);
    return y;
}

float atan_poly(float z) {
    const float z2 = z * z;
    return z * (0.9998660f + z2 * (-0.3302995f + z2 * (0.1801410f + z2 * (-0.0851330f + z2 * 0.0208351f))));
}

float atan2_graus(float y, float x) {
    if (x == 0.0f && y == 0.0f) return 0.0f;
    const float ax = absf(x), ay = absf(y);
    float a = ax >= ay ? atan_poly(ay / ax) : (PI_F * 0.5f - atan_poly(ax / ay));
    if (x < 0.0f) a = PI_F - a;
    if (y < 0.0f) a = -a;
    return a * (180.0f / PI_F);
}

float envolver(float g) {
    if (g >= 360.0f || g < 0.0f) {
        g -= 360.0f * (float)(int)(g / 360.0f);
        if (g < 0.0f) g += 360.0f;
    }
    return g;
}

float sen_g(float g) { return (float)trig::seno(trig::de_graus(envolver(g)))     / (float)trig::UM; }
float cos_g(float g) { return (float)trig::cosseno(trig::de_graus(envolver(g)))  / (float)trig::UM; }

float aleatorio(uint32_t& s) {
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return (float)(int32_t)s / 2147483648.0f;
}

inline vf    sub (const vf& a, const vf& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline vf    esc (const vf& a, float s)     { return { a.x * s, a.y * s, a.z * s }; }
inline float dot (const vf& a, const vf& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline vf    cruz(const vf& a, const vf& b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }

bool caixa_da_forma(const Form3d& f, caixa& out) {
    const malha& m = f.dados();
    if (!m.valida() || m.num_vertices() == 0) return false;

    const vertice* vs = m.vertices();
    int32_t mn[3] = { vs[0].x, vs[0].y, vs[0].z };
    int32_t mx[3] = { vs[0].x, vs[0].y, vs[0].z };
    for (int i = 1, n = m.num_vertices(); i < n; i++) {
        const int32_t v[3] = { vs[i].x, vs[i].y, vs[i].z };
        for (int k = 0; k < 3; k++) {
            if (v[k] < mn[k]) mn[k] = v[k];
            if (v[k] > mx[k]) mx[k] = v[k];
        }
    }

    const float k = 1.0f / 65536.0f;
    const float cl[3] = { (float)((int64_t)mn[0] + mx[0]) * 0.5f * k,
                          (float)((int64_t)mn[1] + mx[1]) * 0.5f * k,
                          (float)((int64_t)mn[2] + mx[2]) * 0.5f * k };
    out.e = { (float)(mx[0] - mn[0]) * 0.5f * k,
              (float)(mx[1] - mn[1]) * 0.5f * k,
              (float)(mx[2] - mn[2]) * 0.5f * k };

    const mat3 mm = mat3d::modelo(f);
    const float q = 1.0f / (float)trig::UM;
    for (int i = 0; i < 3; i++)
        out.u[i] = { (float)mm.m[0][i] * q, (float)mm.m[1][i] * q, (float)mm.m[2][i] * q };

    out.c = { f.posicao_atual_x() + out.u[0].x * cl[0] + out.u[1].x * cl[1] + out.u[2].x * cl[2],
              f.posicao_atual_y() + out.u[0].y * cl[0] + out.u[1].y * cl[1] + out.u[2].y * cl[2],
              f.posicao_atual_z() + out.u[0].z * cl[0] + out.u[1].z * cl[1] + out.u[2].z * cl[2] };
    if (f.colisao() == AABB) {
        const vf e  = out.e;
        const vf u0 = out.u[0], u1 = out.u[1], u2 = out.u[2];
        out.e = { absf(u0.x) * e.x + absf(u1.x) * e.y + absf(u2.x) * e.z,
                absf(u0.y) * e.x + absf(u1.y) * e.y + absf(u2.y) * e.z,
                absf(u0.z) * e.x + absf(u1.z) * e.y + absf(u2.z) * e.z };
        out.u[0] = { 1.0f, 0.0f, 0.0f };
        out.u[1] = { 0.0f, 1.0f, 0.0f };
        out.u[2] = { 0.0f, 0.0f, 1.0f };
    }
    return true;
}

inline float topo_da_caixa(const caixa& b) {
    return b.c.y + absf(b.u[0].y) * b.e.x + absf(b.u[1].y) * b.e.y + absf(b.u[2].y) * b.e.z;
}

bool col_sat(const caixa& a, const caixa& b, vf& n, float& prof) {
    const vf d = sub(a.c, b.c);
    float melhor = 1e30f;
    prof = 0.0f;
    n = { 0.0f, 1.0f, 0.0f };

    auto testar = [&](vf L, bool cruzado) -> bool {
        if (cruzado) {
            const float l2 = dot(L, L);
            if (l2 < 1e-4f) return true;
            L = esc(L, 1.0f / raiz(l2));
        }
        const float ra = absf(dot(a.u[0], L)) * a.e.x + absf(dot(a.u[1], L)) * a.e.y + absf(dot(a.u[2], L)) * a.e.z;
        const float rb = absf(dot(b.u[0], L)) * b.e.x + absf(dot(b.u[1], L)) * b.e.y + absf(dot(b.u[2], L)) * b.e.z;
        const float dist = dot(d, L);
        const float ov = ra + rb - absf(dist);
        if (ov <= 0.0f) return false;

        const float peso = cruzado ? ov * 1.05f + 0.002f : ov;
        if (peso < melhor) {
            melhor = peso;
            prof = ov;
            n = dist >= 0.0f ? L : esc(L, -1.0f);
        }
        return true;
    };

    for (int i = 0; i < 3; i++) if (!testar(a.u[i], false)) return false;
    for (int i = 0; i < 3; i++) if (!testar(b.u[i], false)) return false;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (!testar(cruz(a.u[i], b.u[j]), true)) return false;
    return true;
}

}

Cam3d::~Cam3d() { destruir(); }

void Cam3d::destruir() {
    memoria_free(caixas_);
    caixas_ = nullptr;
    n_caixas_ = cap_caixas_ = 0;
    mundo_ = nullptr;
    n_mundo_ = 0;
    alvo_ = nullptr;
}

void Cam3d::criar(TipoCam3d tipo, float x, float y, float z, float angulo_x, float angulo_y, float angulo_z) {
    tipo_  = tipo;
    pitch_ = limitar(angulo_x, -89.0f, 89.0f);
    yaw_   = envolver(angulo_y);
    roll_  = angulo_z;

    agachado_real_ = false;
    olho_atual_    = olho_;
    vy_ = vel_h_ = suave_y_ = bob_fase_ = bob_k_ = bob_y_ = 0.0f;
    rec_pitch_ = rec_yaw_ = 0.0f;
    trem_t_ = trem_yaw_ = trem_pitch_ = trem_roll_ = 0.0f;
    in_x_ = in_y_ = in_z_ = 0.0f;
    no_chao_ = no_chao_antes_ = false;

    r_.luzes_limpar();
    luz3d lant;
    lant.ativa = false;
    lant.spot  = true;
    lant_id_   = r_.luz_nova(lant);
    luz3d au;
    au.ativa = false;
    au.spot  = false;
    aura_id_ = r_.luz_nova(au);
    tp_ = false;
    posicionar(x, y, z);
}

void Cam3d::definir_corpo(float altura_olhos, float agachados_olhos, float altura_colisao, float raio, Colisao3d tipo_colisao) {
    olho_      = maxf(altura_olhos, 0.05f);
    olho_agach_ = limitar(agachados_olhos, 0.05f, olho_);
    alt_       = maxf(altura_colisao, 0.1f);
    raio_      = maxf(raio, 0.02f);
    tipo_col_  = tipo_colisao;
    olho_atual_ = agachado_real_ ? olho_agach_ : olho_;
}

void Cam3d::fisica(float gravidade, float forca_pulo) { grav_ = gravidade; pulo_ = forca_pulo; }
void Cam3d::degrau(float altura_maxima)               { degrau_ = maxf(altura_maxima, 0.0f); }

void Cam3d::balanco(bool ligado, float amplitude, float frequencia) {
    bob_ativo_ = ligado;
    bob_amp_   = amplitude;
    bob_freq_  = frequencia;
}

void Cam3d::seguir_corpo(Form3d* forma) {
    alvo_ = forma;
    seg_ok_ = false;
    arm_ = dist_;
}

void Cam3d::distancia(float d, float altura_alvo) {
    dist_ = maxf(d, 0.0f);
    alvo_alt_ = altura_alvo;
    if (arm_ > dist_) arm_ = dist_;
}

void Cam3d::colisao_camera(bool ligada, float margem) {
    col_cam_ = ligada;
    margem_cam_ = maxf(margem, 0.0f);
}

void Cam3d::posicao_alvo(float& x, float& y, float& z) const {
    if (alvo_) { x = alvo_->posicao_atual_x();     y = alvo_->posicao_atual_y();     z = alvo_->posicao_atual_z(); }
    else            { x = y = z = 0.0f; }
    y += alvo_alt_;
}

void Cam3d::colisao(Form3d* const* formas, int n) {
    mundo_   = formas;
    n_mundo_ = n < 0 ? 0 : n;
}

int Cam3d::criar_luz(float x, float y, float z, const fb_cores* cor, float intensidade, float alcance) {
    luz3d l;
    l.x = x; l.y = y; l.z = z;
    if (cor) l.cor = *cor;
    l.intensidade = intensidade;
    l.alcance     = alcance;
    return r_.luz_nova(l);
}

void Cam3d::mover_luz(int id, float x, float y, float z) {
    if (id == lant_id_ || id == aura_id_) return;
    luz3d* l = r_.luz_obter(id);
    if (!l) return;
    l->x = x; l->y = y; l->z = z;
}

void Cam3d::remover_luz(int id) {
    if (id != lant_id_ && id != aura_id_) r_.luz_remover(id);
}

void Cam3d::lanterna(bool ligada, float desloc_y) { lant_.ligada = ligada; lant_.desloc_y = desloc_y; }
void Cam3d::cor_luz(fb_cores cor)                 { lant_.cor = cor; }
void Cam3d::intenciadade_luz(float valor)         { lant_.intensidade = valor; }
void Cam3d::alcance_luz(float alcance)            { lant_.alcance = alcance; }
void Cam3d::cone_lanterna(float interno, float externo) { lant_.cone_interno = interno; lant_.cone_externo = externo; }
void Cam3d::luz_direcional(float dx, float dy, float dz) { r_.luz(dx, dy, dz); }

void Cam3d::neblina_usar(bool ativa, fb_cores cor, float inicio, float fim, bool pintar_fundo) {
    neblina n;
    n.ativa  = ativa;
    n.cor    = cor;
    n.inicio = inicio;
    n.fim    = fim;
    r_.neblina_usar(n);
    fog_ativa_ = ativa;
    fog_cor_   = cor;
    fog_fundo_ = pintar_fundo;
}

float Cam3d::altura_olho() const { return tipo_ == FPS ? olho_atual_ : 0.0f; }

float Cam3d::altura_corpo() const {
    const float h = agachado_real_ ? alt_ - (olho_ - olho_agach_) : alt_;
    return maxf(h, 0.1f);
}

float Cam3d::x() const { return orbita() ? cx_ : px_; }
float Cam3d::y() const { return orbita() ? cy_ : py_ + altura_olho(); }
float Cam3d::z() const { return orbita() ? cz_ : pz_; }

void Cam3d::posicionar(float x, float y, float z) {
    px_ = x;
    pz_ = z;
    py_ = y - altura_olho();
    vy_ = 0.0f;
}

void Cam3d::mover(float x, float y, float z) { in_x_ += x; in_y_ += y; in_z_ += z; }

void Cam3d::girar(float x, float y, float z) {
    pitch_ = limitar(pitch_ + x, -89.0f, 89.0f);
    yaw_   = envolver(yaw_ + y);
    roll_  = envolver(roll_ + z);
    if (roll_ > 180.0f) roll_ -= 360.0f;
}

void Cam3d::pular() {
    if (grav_ != 0.0f && no_chao_) {
        vy_ = pulo_;
        no_chao_ = false;
    }
}

void Cam3d::olhar_para(float x, float y, float z) {
    const float dx = x - this->x(), dy = y - this->y(), dz = z - this->z();
    const float h = raiz(dx * dx + dz * dz);
    if (h < 1e-5f && absf(dy) < 1e-5f) return;
    if (h > 1e-5f) yaw_ = envolver(atan2_graus(dx, dz));
    pitch_ = limitar(atan2_graus(dy, h), -89.0f, 89.0f);
}

void Cam3d::tremer(float intensidade, float duracao) {
    if (duracao <= 0.0f) return;
    trem_int_ = intensidade;
    trem_dur_ = trem_t_ = duracao;
}

void Cam3d::recuo(float pitch_graus, float yaw_graus) {
    rec_pitch_ += pitch_graus;
    rec_yaw_   += yaw_graus;
}

void Cam3d::reunir_caixas() {
    n_caixas_ = 0;
    if (!mundo_ || n_mundo_ <= 0) return;

    if (n_mundo_ > cap_caixas_) {
        memoria_free(caixas_);
        caixas_ = (caixa*)memoria_malloc((size_t)n_mundo_ * sizeof(caixa));
        cap_caixas_ = caixas_ ? n_mundo_ : 0;
        if (!caixas_) return;
    }

    for (int i = 0; i < n_mundo_; i++) {
        if (!mundo_[i]) continue;
        if (alvo_ && mundo_[i] == alvo_) continue;
        if (caixa_da_forma(*mundo_[i], caixas_[n_caixas_])) n_caixas_++;
    }
}

float Cam3d::braco_livre(float ox, float oy, float oz, float dx, float dy, float dz, float len) const {
    float melhor = len;

    for (int i = 0; i < n_caixas_; i++) {
        const caixa& b = caixas_[i];
        const vf o = { ox - b.c.x, oy - b.c.y, oz - b.c.z };
        const vf d = { dx, dy, dz };

        float tn = -1e30f, tf = 1e30f;
        bool fora = false;
        for (int k = 0; k < 3; k++) {
            const float e  = (k == 0 ? b.e.x : (k == 1 ? b.e.y : b.e.z)) + margem_cam_;
            const float ol = dot(o, b.u[k]);
            const float dl = dot(d, b.u[k]);
            if (absf(dl) < 1e-6f) {
                if (absf(ol) > e) { fora = true; break; }
                continue;
            }
            float t1 = (-e - ol) / dl, t2 = (e - ol) / dl;
            if (t1 > t2) { const float t = t1; t1 = t2; t2 = t; }
            if (t1 > tn) tn = t1;
            if (t2 < tf) tf = t2;
            if (tn > tf) { fora = true; break; }
        }
        if (fora || tf < 0.0f || tn <= 0.0f) continue;
        if (tn < melhor) melhor = tn;
    }

    if (chao_ativo_ && dy < -1e-4f) {
        const float t = (oy - (chao_y_ + margem_cam_)) / -dy;
        if (t < melhor) melhor = t;
    }
    return maxf(melhor, 0.0f);
}

caixa Cam3d::montar_corpo(float x, float y, float z, float h, float folga) const {
    const float hh = maxf(h - folga, 0.02f) * 0.5f;
    const float r  = maxf(raio_ - folga, 0.005f);

    caixa c;
    c.c = { x, y + folga + hh, z };
    c.e = { r, hh, r };
    if (tipo_col_ == OBB) {
        const float s = sen_g(yaw_), co = cos_g(yaw_);
        c.u[0] = {  co, 0.0f, -s };
        c.u[1] = { 0.0f, 1.0f, 0.0f };
        c.u[2] = {   s, 0.0f, co };
    } else {
        c.u[0] = { 1.0f, 0.0f, 0.0f };
        c.u[1] = { 0.0f, 1.0f, 0.0f };
        c.u[2] = { 0.0f, 0.0f, 1.0f };
    }
    return c;
}

bool Cam3d::colide_em(float x, float y, float z, float h, float folga) const {
    const caixa corpo = montar_corpo(x, y, z, h, folga);
    vf n; float prof;
    for (int i = 0; i < n_caixas_; i++)
        if (col_sat(corpo, caixas_[i], n, prof)) return true;
    return false;
}

void Cam3d::resolver(float h) {
    for (int it = 0; it < 4; it++) {
        bool mexeu = false;

        for (int i = 0; i < n_caixas_; i++) {
            const caixa corpo = montar_corpo(px_, py_, pz_, h, 0.0f);
            vf n; float prof;
            if (!col_sat(corpo, caixas_[i], n, prof)) continue;

            if (degrau_ > 0.0f && no_chao_antes_ && absf(n.y) < 0.5f) {
                const float topo  = topo_da_caixa(caixas_[i]);
                const float sobe  = topo - py_;
                if (sobe > 0.0f && sobe <= degrau_ && !colide_em(px_, topo + 0.01f, pz_, h, 0.03f)) {
                    py_ = topo + 0.01f;
                    suave_y_ -= sobe;
                    no_chao_ = true;
                    if (vy_ < 0.0f) vy_ = 0.0f;
                    mexeu = true;
                    continue;
                }
            }

            const float p = prof + 0.001f;
            px_ += n.x * p;
            py_ += n.y * p;
            pz_ += n.z * p;

            if (n.y > 0.5f)       { no_chao_ = true; if (vy_ < 0.0f) vy_ = 0.0f; }
            else if (n.y < -0.5f) { if (vy_ > 0.0f) vy_ = 0.0f; }
            mexeu = true;
        }

        if (!mexeu) break;
    }
}

void Cam3d::atualizar_corpo(float dt) {
    reunir_caixas();

    if (agachado_) agachado_real_ = true;
    else if (agachado_real_ && !colide_em(px_, py_, pz_, alt_, 0.03f)) agachado_real_ = false;

    const float h = altura_corpo();

    mult_atual_ = agachado_real_ ? mult_agachar_ : (correndo_ ? mult_correr_ : 1.0f);

    const float sy = sen_g(yaw_), cy = cos_g(yaw_);
    const float dx = (in_x_ * cy + in_z_ * sy) * mult_atual_ * dt;
    const float dz = (in_z_ * cy - in_x_ * sy) * mult_atual_ * dt;
    float dy;

    if (grav_ != 0.0f) {
        vy_ -= grav_ * dt;
        if (vy_ < -60.0f) vy_ = -60.0f;
        dy = vy_ * dt;
    } else {
        vy_ = 0.0f;
        dy  = in_y_ * mult_atual_ * dt;
    }
    vel_h_ = raiz(dx * dx + dz * dz) / dt;

    const float desl  = maxf(maxf(absf(dx), absf(dz)), absf(dy));
    const float passo = maxf(raio_ * 0.5f, 0.05f);
    int passos = 1 + (int)(desl / passo);
    if (passos > 32) passos = 32;

    no_chao_antes_ = no_chao_;
    no_chao_ = false;

    for (int s = 0; s < passos; s++) {
        px_ += dx / (float)passos;
        py_ += dy / (float)passos;
        pz_ += dz / (float)passos;

        resolver(h);

        if (chao_ativo_ && py_ < chao_y_) {
            py_ = chao_y_;
            if (vy_ < 0.0f) vy_ = 0.0f;
            no_chao_ = true;
        }
        if (ceu_ativo_ && py_ + h > ceu_y_) {
            py_ = ceu_y_ - h;
            if (vy_ > 0.0f) vy_ = 0.0f;
        }
    }
}

void Cam3d::atualizar_seguindo(float dt) {
    float tx, ty, tz;
    posicao_alvo(tx, ty, tz);

    if (seg_vel_ > 0.0f && seg_ok_) {
        const float k = minf(1.0f, seg_vel_ * dt);
        sx_ += (tx - sx_) * k;
        sy_ += (ty - sy_) * k;
        sz_ += (tz - sz_) * k;
    } else {
        sx_ = tx; sy_ = ty; sz_ = tz;
    }
    if (!seg_ok_) arm_ = dist_;
    seg_ok_ = true;

    const float cp = cos_g(pitch_), sp = sen_g(pitch_);
    const float fx = sen_g(yaw_) * cp, fy = sp, fz = cos_g(yaw_) * cp;

    float livre = dist_;
    if (col_cam_) {
        reunir_caixas();
        livre = braco_livre(sx_, sy_, sz_, -fx, -fy, -fz, dist_);
    }

    if (arm_ > dist_) arm_ = dist_;
    if (livre < arm_) arm_ = livre;
    else              arm_ += (livre - arm_) * minf(1.0f, 5.0f * dt);

    cx_ = sx_ - fx * arm_;
    cy_ = sy_ - fy * arm_;
    cz_ = sz_ - fz * arm_;
}

void Cam3d::efeitos(float dt) {

    if (tipo_ == FPS) {
        const float alvo = agachado_real_ ? olho_agach_ : olho_;
        olho_atual_ += limitar(alvo - olho_atual_, -6.0f * dt, 6.0f * dt);
    }

    if (suave_y_ < 0.0f) { suave_y_ += 4.0f * dt; if (suave_y_ > 0.0f) suave_y_ = 0.0f; }
    else if (suave_y_ > 0.0f) { suave_y_ -= 4.0f * dt; if (suave_y_ < 0.0f) suave_y_ = 0.0f; }

    const bool andando = tipo_ == FPS && !orbita() && bob_ativo_ && no_chao_ && vel_h_ > 0.1f;
    bob_k_ += ((andando ? 1.0f : 0.0f) - bob_k_) * minf(1.0f, 10.0f * dt);
    if (andando) {
        bob_fase_ += bob_freq_ * mult_atual_ * dt;
        if (bob_fase_ > 2.0f * PI_F) bob_fase_ -= 2.0f * PI_F;
    }
    bob_y_ = sen_g(bob_fase_ * (180.0f / PI_F)) * bob_amp_ * bob_k_;

    const float k = minf(1.0f, rec_vel_ * dt);
    rec_pitch_ -= rec_pitch_ * k;
    rec_yaw_   -= rec_yaw_ * k;
    if (absf(rec_pitch_) < 0.001f) rec_pitch_ = 0.0f;
    if (absf(rec_yaw_)   < 0.001f) rec_yaw_   = 0.0f;

    if (trem_t_ > 0.0f) {
        trem_t_ -= dt;
        const float f = trem_t_ > 0.0f ? (trem_t_ / trem_dur_) * trem_int_ : 0.0f;
        trem_yaw_   = aleatorio(rng_) * f;
        trem_pitch_ = aleatorio(rng_) * f;
        trem_roll_  = aleatorio(rng_) * f * 0.5f;
    } else {
        trem_yaw_ = trem_pitch_ = trem_roll_ = 0.0f;
    }
}

void Cam3d::atualizar(float delta) {
    if (!(delta > 0.0f)) return;
    if (delta > 0.1f) delta = 0.1f;

    if (tipo_ == FPS) atualizar_corpo(delta);
    if (tem_alvo() && orbita()) {
        if (tipo_ == FPS) sincronizar_heroi();
        atualizar_seguindo(delta);
    }

    efeitos(delta);
    in_x_ = in_y_ = in_z_ = 0.0f;
}

void Cam3d::vista(float& ex, float& ey, float& ez, float& yaw, float& pitch, float& roll) const {
    if (orbita()) {
        ex = cx_; ey = cy_; ez = cz_;
    } else {
        ex = px_;
        ey = py_ + altura_olho() + suave_y_ + bob_y_;
        ez = pz_;
    }
    yaw   = yaw_ + rec_yaw_ + trem_yaw_;
    pitch = limitar(pitch_ + rec_pitch_ + trem_pitch_, -89.0f, 89.0f);
    roll  = roll_ + trem_roll_;
}

void Cam3d::atualizar_lanterna(float ex, float ey, float ez, float yaw, float pitch) {
    luz3d* l = r_.luz_obter(lant_id_);
    if (!l) return;

    l->ativa = lant_.ligada;
    if (!lant_.ligada) return;

    float ox = ex, oy = ey, oz = ez;
    if (orbita() && tem_alvo() && seg_ok_) { ox = sx_; oy = sy_; oz = sz_; }
    
    const float cp = cos_g(pitch);
    l->spot = true;
    l->x = ox;
    l->y = oy + lant_.desloc_y;
    l->z = oz;
    l->dx = sen_g(yaw) * cp;
    l->dy = sen_g(pitch);
    l->dz = cos_g(yaw) * cp;
    l->cor = lant_.cor;
    l->intensidade  = lant_.intensidade;
    l->alcance      = lant_.alcance;
    l->cone_interno = lant_.cone_interno;
    l->cone_externo = lant_.cone_externo;
}

void Cam3d::desenhar(frame_buffer& fb, Form3d* const* formas, int n) {
    float ex, ey, ez, yw, pt, rl;
    vista(ex, ey, ez, yw, pt, rl);

    atualizar_lanterna(ex, ey, ez, yw, pt);
    atualizar_aura();

    r_.camera(ex, ey, ez);
    r_.camera_angulo(yw, pt, rl);
    r_.ambiente((int)(ambiente_ * 255.0f + 0.5f));
    if (fog_ativa_ && fog_fundo_) fb.limpar(fog_cor_);
    r_.comecar(fb);

    if (formas)
        for (int i = 0; i < n; i++)
            if (formas[i]) r_.desenhar(fb, *formas[i]);
    if (orbita() && alvo_) r_.desenhar(fb, *alvo_);
}

void Cam3d::girar_alvo(float pitch, float yaw, Cam3d* corpo) {
    girar(pitch, yaw, 0.0f);
    if (corpo) corpo->girar(0.0f, yaw, 0.0f);
    if (alvo_) alvo_->angulo_y(yaw_);
}

void Cam3d::sincronizar_alvo(const Cam3d& corpo) {
    const float cx = corpo.px_;
    const float cy = corpo.py_ + corpo.alt_ * 0.5f;
    const float cz = corpo.pz_;

    if (alvo_) {
        alvo_->posicionar(cx, cy, cz);
        alvo_->angulo_y(yaw_);
    }
}

void Cam3d::aura(bool ligada, float desloc_y) { aura_.ligada = ligada; aura_.desloc_y = desloc_y; }
void Cam3d::aura_cor(fb_cores cor)            { aura_.cor = cor; }
void Cam3d::aura_intensidade(float valor)     { aura_.intensidade = valor; }
void Cam3d::aura_alcance(float alcance)       { aura_.alcance = alcance; }
void Cam3d::aura_suavidade(float valor)       { aura_.suavidade = limitar(valor, 0.0f, 1.0f); }

void Cam3d::atualizar_aura() {
    luz3d* l = r_.luz_obter(aura_id_);
    if (!l) return;

    l->ativa = aura_.ligada;
    if (!aura_.ligada) return;

    float ox, oy, oz;
    if (tipo_ == FPS) {
        ox = px_;
        oy = py_ + altura_corpo() * 0.5f;
        oz = pz_;
    } else if (tem_alvo()) {
        posicao_alvo(ox, oy, oz);
    } else {
        ox = px_; oy = py_; oz = pz_;
    }

    l->spot        = false;
    l->x           = ox;
    l->y           = oy + aura_.desloc_y;
    l->z           = oz;
    l->cor         = aura_.cor;
    l->intensidade = aura_.intensidade;
    l->alcance     = aura_.alcance;
    l->suavidade   = aura_.suavidade;
}

void Cam3d::sincronizar_heroi() {
    const float cx = px_, cy = py_ + alt_ * 0.5f, cz = pz_;
    if (alvo_) {
        alvo_->posicionar(cx, cy, cz);
        alvo_->angulo_y(yaw_);
    }
}

void Cam3d::terceira_pessoa(bool ligada) {
    if (tipo_ != FPS || tp_ == ligada) return;
    if (ligada && !tem_alvo()) return;
    tp_ = ligada;
    seg_ok_ = false;
    arm_ = dist_;
    if (tp_) sincronizar_heroi();
}

void Cam3d::alternar() { terceira_pessoa(!tp_); }

void Cam3d::luzes(bool lanterna_ligada, bool aura_ligada) {
    lant_.ligada = lanterna_ligada;
    aura_.ligada = aura_ligada;
}

/* ---- API C ---- */

namespace {
inline Form3d*       F(f3d_handle* h)           { return reinterpret_cast<Form3d*>(h); }
inline Cam3d*        C(cam3d_handle* h)         { return reinterpret_cast<Cam3d*>(h); }
inline const Cam3d*  C(const cam3d_handle* h)   { return reinterpret_cast<const Cam3d*>(h); }
inline fb_cores      cor_de(uint32_t p)         { return fb_cores::desempacotar(p); }
}

extern "C" {

/* ---- Cam3d ---- */
cam3d_handle* cam3d_criar(int tipo, float x, float y, float z, float ax, float ay, float az) {
    Cam3d* c = new Cam3d;
    c->criar(tipo ? Terceira_pessoa : FPS, x, y, z, ax, ay, az);
    return reinterpret_cast<cam3d_handle*>(c);
}
void cam3d_destruir(cam3d_handle* c)        { if (c) { C(c)->destruir(); delete C(c); } }

void cam3d_fov(cam3d_handle* c, float graus)            { if (c) C(c)->render().fov(graus); }
void cam3d_plano_proximo(cam3d_handle* c, float u)      { if (c) C(c)->render().plano_proximo(u); }
int  cam3d_triangulos(const cam3d_handle* c)            { return c ? const_cast<Cam3d*>(C(c))->render().triangulos() : 0; }

void cam3d_chao(cam3d_handle* c, int ativo)             { if (c) C(c)->chao(ativo != 0); }
void cam3d_definir_chao(cam3d_handle* c, float y)       { if (c) C(c)->definir_chao(y); }
void cam3d_maximo_ceu(cam3d_handle* c, float y)         { if (c) C(c)->maximo_ceu(y); }
void cam3d_mover(cam3d_handle* c, float x, float y, float z)      { if (c) C(c)->mover(x, y, z); }
void cam3d_posicionar(cam3d_handle* c, float x, float y, float z) { if (c) C(c)->posicionar(x, y, z); }
void cam3d_girar(cam3d_handle* c, float x, float y, float z)      { if (c) C(c)->girar(x, y, z); }
void cam3d_pular(cam3d_handle* c)                       { if (c) C(c)->pular(); }
void cam3d_agachar(cam3d_handle* c, int ligado)         { if (c) C(c)->agachar(ligado != 0); }
void cam3d_correr(cam3d_handle* c, int ligado)          { if (c) C(c)->correr(ligado != 0); }
void cam3d_multiplicadores(cam3d_handle* c, float correr, float agachar) { if (c) C(c)->multiplicadores(correr, agachar); }
void cam3d_balanco(cam3d_handle* c, int ligado, float amp, float freq)   { if (c) C(c)->balanco(ligado != 0, amp, freq); }
void cam3d_definir_corpo(cam3d_handle* c, float olhos, float agach, float alt, float raio, int tipo) {
    if (c) C(c)->definir_corpo(olhos, agach, alt, raio, tipo ? OBB : AABB);
}
void cam3d_seguir_corpo(cam3d_handle* c, f3d_handle* forma) { if (c) C(c)->seguir_corpo(F(forma)); }
void cam3d_distancia(cam3d_handle* c, float d, float alt)    { if (c) C(c)->distancia(d, alt); }
void cam3d_colisao_camera(cam3d_handle* c, int ligada, float margem) { if (c) C(c)->colisao_camera(ligada != 0, margem); }
void cam3d_suavidade(cam3d_handle* c, float v)          { if (c) C(c)->suavidade(v); }
void cam3d_degrau(cam3d_handle* c, float h)             { if (c) C(c)->degrau(h); }
void cam3d_olhar_para(cam3d_handle* c, float x, float y, float z) { if (c) C(c)->olhar_para(x, y, z); }
void cam3d_fisica(cam3d_handle* c, float g, float pulo) { if (c) C(c)->fisica(g, pulo); }
float cam3d_x(const cam3d_handle* c)                    { return c ? C(c)->x() : 0.0f; }
float cam3d_y(const cam3d_handle* c)                    { return c ? C(c)->y() : 0.0f; }
float cam3d_z(const cam3d_handle* c)                    { return c ? C(c)->z() : 0.0f; }
float cam3d_angulo_x(const cam3d_handle* c)             { return c ? C(c)->angulo_x() : 0.0f; }
float cam3d_angulo_y(const cam3d_handle* c)             { return c ? C(c)->angulo_y() : 0.0f; }
float cam3d_angulo_z(const cam3d_handle* c)             { return c ? C(c)->angulo_z() : 0.0f; }
int   cam3d_no_chao(const cam3d_handle* c)              { return c && C(c)->no_chao(); }
void cam3d_tremer(cam3d_handle* c, float i, float d)    { if (c) C(c)->tremer(i, d); }
void cam3d_recuo(cam3d_handle* c, float p, float y)     { if (c) C(c)->recuo(p, y); }
void cam3d_recuperacao(cam3d_handle* c, float v)        { if (c) C(c)->recuperacao(v); }

int  cam3d_criar_luz(cam3d_handle* c, float x, float y, float z, uint32_t cor, float intensidade, float alcance) {
    if (!c) return -1;
    const fb_cores cc = cor_de(cor);
    return C(c)->criar_luz(x, y, z, &cc, intensidade, alcance);
}
void cam3d_mover_luz(cam3d_handle* c, int id, float x, float y, float z) { if (c) C(c)->mover_luz(id, x, y, z); }
void cam3d_remover_luz(cam3d_handle* c, int id)         { if (c) C(c)->remover_luz(id); }
void cam3d_luz_direcional(cam3d_handle* c, float dx, float dy, float dz) { if (c) C(c)->luz_direcional(dx, dy, dz); }
void cam3d_ambiente(cam3d_handle* c, float v)           { if (c) C(c)->ambiente(v); }
void cam3d_lanterna(cam3d_handle* c, int ligada, float dy) { if (c) C(c)->lanterna(ligada != 0, dy); }
void cam3d_cone_lanterna(cam3d_handle* c, float i, float e) { if (c) C(c)->cone_lanterna(i, e); }
void cam3d_cor_luz(cam3d_handle* c, uint32_t cor)       { if (c) C(c)->cor_luz(cor_de(cor)); }
uint32_t cam3d_get_cor_luz(const cam3d_handle* c)       { return c ? C(c)->cor_luz().empacotar() : 0; }
void cam3d_intensidade_luz(cam3d_handle* c, float v)    { if (c) C(c)->intenciadade_luz(v); }
void cam3d_alcance_luz(cam3d_handle* c, float a)        { if (c) C(c)->alcance_luz(a); }
void cam3d_neblina_usar(cam3d_handle* c, int ativa, uint32_t cor, float ini, float fim, int fundo) {
    if (c) C(c)->neblina_usar(ativa != 0, cor_de(cor), ini, fim, fundo != 0);
}
void cam3d_aura(cam3d_handle* c, int ligada, float dy)  { if (c) C(c)->aura(ligada != 0, dy); }
void cam3d_aura_cor(cam3d_handle* c, uint32_t cor)      { if (c) C(c)->aura_cor(cor_de(cor)); }
void cam3d_aura_intensidade(cam3d_handle* c, float v)   { if (c) C(c)->aura_intensidade(v); }
void cam3d_aura_alcance(cam3d_handle* c, float a)       { if (c) C(c)->aura_alcance(a); }
void cam3d_aura_suavidade(cam3d_handle* c, float v)     { if (c) C(c)->aura_suavidade(v); }
void cam3d_luzes(cam3d_handle* c, int lant, int aura)   { if (c) C(c)->luzes(lant != 0, aura != 0); }

void cam3d_alternar(cam3d_handle* c)                    { if (c) C(c)->alternar(); }
void cam3d_terceira_pessoa(cam3d_handle* c, int ligada) { if (c) C(c)->terceira_pessoa(ligada != 0); }
int  cam3d_em_terceira(const cam3d_handle* c)           { return c && C(c)->em_terceira(); }
void cam3d_girar_alvo(cam3d_handle* c, float pitch, float yaw, cam3d_handle* corpo) { if (c) C(c)->girar_alvo(pitch, yaw, C(corpo)); }
void cam3d_sincronizar_alvo(cam3d_handle* c, const cam3d_handle* corpo) { if (c && corpo) C(c)->sincronizar_alvo(*C(corpo)); }

void cam3d_atualizar(cam3d_handle* c, float delta)      { if (c) C(c)->atualizar(delta); }
void cam3d_colisao_mundo(cam3d_handle* c, f3d_handle* const* formas, int n) {
    if (c) C(c)->colisao(reinterpret_cast<Form3d* const*>(formas), n);
}
void cam3d_desenhar(cam3d_handle* c, fb_handle* fb, f3d_handle* const* formas, int n) {
    if (c && fb) C(c)->desenhar(fb->fb, reinterpret_cast<Form3d* const*>(formas), n);
}

}