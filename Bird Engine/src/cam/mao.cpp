#include "../frame buffer/fb_3d.hpp"

namespace {

float sen(float g) { return (float)trig::seno(trig::de_graus(g)) / (float)trig::UM; }
float cos_(float g) { return (float)trig::cosseno(trig::de_graus(g)) / (float)trig::UM; }

float envelope(float t, float dur) {
    const float p = t / dur;
    if (p >= 1.0f) return 0.0f;
    return p < 0.2f ? p / 0.2f : (1.0f - p) / 0.8f;
}

}

void Mao::criar(Form3d *forma, Cam3d *camera, lados lado) {
    forma_ = forma;
    cam_ = camera;
    lado_ = lado;
    ox_ = lado == Mao_direita ? 0.4f : -0.4f;
    oy_ = -0.35f;
    oz_ = 0.7f;
    rx_ = ry_ = rz_ = 0;
    tp_ = false;
    fixo_ = false;
    fixa_ = true;
    anim_tipo_ = 0;
    anim_t_ = 0;
    sincronizar(nullptr, nullptr);
}

void Mao::sincronizar(Form3d *corpo, Cam3d *camera) {
    if (corpo) corpo_ = corpo;
    if (camera) cam_ = camera;
    if (!forma_ || !cam_) return;

    const float yaw = cam_->angulo_y();
    const float pitch = cam_->angulo_x();
    const float roll = cam_->angulo_z();
    const bool seguir_corpo = tp_ && corpo_;

    float lx = ox_, ly = oy_, lz = oz_;
    if (anim_tipo_ != 0) {
        const float k = envelope(anim_t_, anim_dur_);
        if (anim_tipo_ == 1) lz -= anim_dist_ * k;
        else                 lz += anim_dist_ * k;
    }

    const bool seguir_pitch = fixa_ && !tp_;
    const float p = seguir_pitch ? pitch : 0.0f;
    const float sp = sen(p), cp = cos_(p);
    const float sy = sen(yaw), cy = cos_(yaw);

    const float y1 = ly * cp + lz * sp;
    const float z1 = lz * cp - ly * sp;

    const float wx = cy * lx + sy * z1;
    const float wy = y1;
    const float wz = -sy * lx + cy * z1;

    float bx, by, bz;
    if (seguir_corpo) {
        bx = corpo_->posicao_atual_x();
        by = corpo_->posicao_atual_y();
        bz = corpo_->posicao_atual_z();
    } else {
        bx = cam_->x();
        by = cam_->y();
        bz = cam_->z();
    }

    fx_ = bx + wx;
    fy_ = by + wy;
    fz_ = bz + wz;

    const bool travado = fixo_ || !seguir_pitch;
    ax_ = (travado ? 0.0f : -pitch) + rx_;
    ay_ = yaw + ry_;
    az_ = (travado ? 0.0f : roll) + rz_;

    forma_->posicionar(fx_, fy_, fz_);
    forma_->angulo_x(ax_);
    forma_->angulo_y(ay_);
    forma_->angulo_z(az_);
}

void Mao::atualizar(float delta) {
    if (anim_tipo_ != 0) {
        anim_t_ += delta;
        if (anim_t_ >= anim_dur_) {
            anim_tipo_ = 0;
            anim_t_ = 0;
        }
    }
    sincronizar(nullptr, nullptr);
}

void Mao::mover(float x, float y, float z) {
    ox_ += x;
    oy_ += y;
    oz_ += z;
}

void Mao::girar(float x, float y, float z) {
    rx_ += x;
    ry_ += y;
    rz_ += z;
}

void Mao::posicionar(float x, float y, float z) {
    ox_ = x;
    oy_ = y;
    oz_ = z;
}

void Mao::terceira_pessoa(bool ativo) { tp_ = ativo; }

void Mao::mudar_visao(TipoCam3d tipo) { tp_ = (tipo == Terceira_pessoa); }

void Mao::recuo(float tras, float tempo_voltar) {
    anim_tipo_ = 1;
    anim_t_ = 0;
    anim_dist_ = tras;
    anim_dur_ = tempo_voltar > 0.001f ? tempo_voltar : 0.001f;
}

void Mao::bater(float frente, float tempo_voltar) {
    anim_tipo_ = 2;
    anim_t_ = 0;
    anim_dist_ = frente;
    anim_dur_ = tempo_voltar > 0.001f ? tempo_voltar : 0.001f;
}

void Mao::nao_mudar_angulo(bool ativo) { fixo_ = ativo; }

void Mao::colisao(Colisao3d tipo) {
    if (forma_) forma_->definir_colisao(tipo);
}

float Mao::angulo_x() const { return ax_; }
float Mao::angulo_y() const { return ay_; }
float Mao::angulo_z() const { return az_; }
float Mao::x() const { return fx_; }
float Mao::y() const { return fy_; }
float Mao::z() const { return fz_; }
void Mao::fixa(bool ativar) { fixa_ = ativar; }