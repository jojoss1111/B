#include "obj.hpp"

void Obj::criar(fb_atlas *sprite, uint16_t pw, uint16_t ph, int32_t x, int32_t y, uint16_t coluna, uint16_t linha) {
    sprite_ = sprite;
    pw_ = pw; ph_ = ph;
    x_ = x; y_ = y;
    col_ = coluna; lin_ = linha;
    ang_ = 0.0f;
    alpha_ = 255;
    flip_x_ = flip_y_ = false;
    tint_ = {255, 255, 255, 255};
}

void Obj::destruir() {
    sprite_ = nullptr;
    x_ = y_ = 0;
    pw_ = ph_ = col_ = lin_ = 0;
    ang_ = 0.0f;
}

void Obj::posicionar(int32_t x, int32_t y) { x_ = x; y_ = y; }
void Obj::mover(int32_t x, int32_t y)      { x_ += x; y_ += y; }

void Obj::tile(uint16_t coluna, uint16_t linha) { col_ = coluna; lin_ = linha; }
void Obj::cor(const fb_cores &c)                { tint_ = c; }
void Obj::alpha(uint8_t a)                      { alpha_ = a; }
void Obj::ampliar(uint16_t pw, uint16_t ph)     { pw_ = pw; ph_ = ph; }
void Obj::angulo(float a)                       { ang_ = a; }
void Obj::espelhar_x(bool e)                    { flip_x_ = e; }
void Obj::espelhar_y(bool e)                    { flip_y_ = e; }
void Obj::colorir(bool s) { subst_ = s; }

void Obj::desenhar(frame_buffer &fb) const {
    if (!sprite_ || !sprite_->valido() || !fb.valido()) return;
    if (pw_ == 0 || ph_ == 0 || alpha_ == 0 || tint_.a == 0) return;
    if (col_ >= sprite_->colunas() || lin_ >= sprite_->linhas()) return;

    const int fw = sprite_->tile_largura(), fh = sprite_->tile_altura();
    const int passo = sprite_->passo();
    const uint32_t *tile = sprite_->tile_dados(col_, lin_);

    const int w = pw_, h = ph_;
    const int cx2 = 2 * x_ + w, cy2 = 2 * y_ + h;
    const uint16_t ang = fb_trig::de_graus(ang_);
    const int32_t c = fb_trig::cosseno(ang), s = fb_trig::seno(ang);

    int x0 = x_, y0 = y_, x1 = x_ + w, y1 = y_ + h;
    if (ang) {
        const int r = (w + h) / 2 + 1;
        const int cx = cx2 / 2, cy = cy2 / 2;
        x0 = cx - r; x1 = cx + r;
        y0 = cy - r; y1 = cy + r;
    }
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > fb.largura()) x1 = fb.largura();
    if (y1 > fb.altura())  y1 = fb.altura();

    for (int y = y0; y < y1; y++) {
        const int64_t uy = 2 * y + 1 - cy2;
        for (int x = x0; x < x1; x++) {
            const int64_t ux = 2 * x + 1 - cx2;

            int rx = (int)ux, ry = (int)uy;
            if (ang) {
                rx = (int)((ux * c + uy * s) >> fb_trig::BITS);
                ry = (int)((uy * c - ux * s) >> fb_trig::BITS);
            }
            if (rx < -w || rx >= w || ry < -h || ry >= h) continue;

            int tx = (int)(((int64_t)(rx + w) * fw) / (2 * w));
            int ty = (int)(((int64_t)(ry + h) * fh) / (2 * h));
            if (flip_x_) tx = fw - 1 - tx;
            if (flip_y_) ty = fh - 1 - ty;

            fb_cores t = fb_cores::desempacotar(tile[(size_t)ty * passo + tx]);
            if (t.a == 0) continue;

            if (subst_) {
                t.r = tint_.r;
                t.g = tint_.g;
                t.b = tint_.b;
            } else {
                t.r = (uint8_t)(t.r * tint_.r / 255);
                t.g = (uint8_t)(t.g * tint_.g / 255);
                t.b = (uint8_t)(t.b * tint_.b / 255);
            }
            t.a = (uint8_t)((t.a * alpha_ / 255) * tint_.a / 255);
            fb.misturar(x, y, t);
        }
    }
}

void Animacao::criar(Obj *obj, const int16_t (*quadros)[2], uint8_t total, uint8_t fps, bool loop) {
    obj_ = obj;
    arr_ = quadros;
    total_ = total;
    ms_quadro_ = fps ? 1000u / fps : 1000u;
    if (ms_quadro_ == 0) ms_quadro_ = 1;
    loop_ = loop;
    idx_ = 0;
    acum_ = 0;
    tocando_ = false;
    aplicar();
}

void Animacao::destruir() {
    obj_ = nullptr;
    arr_ = nullptr;
    total_ = idx_ = 0;
    acum_ = 0;
    tocando_ = false;
}

void Animacao::rodar()  { tocando_ = true; }
void Animacao::parar()  { tocando_ = false; }

void Animacao::voltar() {
    idx_ = 0;
    acum_ = 0;
    aplicar();
}

void Animacao::aplicar() {
    if (!obj_ || !arr_ || total_ == 0) return;
    obj_->tile((uint16_t)arr_[idx_][0], (uint16_t)arr_[idx_][1]);
}

void Animacao::atualizar(uint32_t dt_ms) {
    if (!tocando_ || !obj_ || !arr_ || total_ == 0) return;

    acum_ += dt_ms;
    while (acum_ >= ms_quadro_) {
        acum_ -= ms_quadro_;
        if (++idx_ >= total_) {
            if (loop_) {
                idx_ = 0;
            } else {
                idx_ = total_ - 1;
                tocando_ = false;
                acum_ = 0;
                break;
            }
        }
    }
    aplicar();
}

void AABB_2d::criar(Obj *obj, const char *mascara, uint16_t mascara_w, uint16_t mascara_h) {
    mascara_ = mascara;
    mascara_w_ = mascara_w;
    mascara_h_ = mascara_h;
    w_ = mascara_w;
    h_ = mascara_h;
    if (obj) sincronizar_colisao(obj);
}

void AABB_2d::sincronizar_colisao(Obj *obj) {
    if (!obj) return;
    x_ = obj->x();
    y_ = obj->y();
}

bool AABB_2d::solido_local(int32_t lx, int32_t ly) const {
    if (lx < 0 || ly < 0 || lx >= w_ || ly >= h_) return false;
    if (!mascara_) return true;
    return mascara_[(size_t)ly * mascara_w_ + lx] == '#';
}

bool AABB_2d::testar(const AABB_2d &a, int32_t ax, int32_t ay, const AABB_2d &b, int32_t bx, int32_t by) {
    const int32_t x0 = ax > bx ? ax : bx;
    const int32_t y0 = ay > by ? ay : by;
    const int32_t ea = ax + a.w_, eb = bx + b.w_;
    const int32_t fa = ay + a.h_, fb = by + b.h_;
    const int32_t x1 = ea < eb ? ea : eb;
    const int32_t y1 = fa < fb ? fa : fb;

    if (x0 >= x1 || y0 >= y1) return false;
    if (!a.mascara_ && !b.mascara_) return true;

    for (int32_t y = y0; y < y1; y++)
        for (int32_t x = x0; x < x1; x++)
            if (a.solido_local(x - ax, y - ay) && b.solido_local(x - bx, y - by)) return true;
    return false;
}

bool AABB_2d::colidio(const AABB_2d *a, const AABB_2d *b) {
    if (!a || !b) return false;
    return testar(*a, a->x_, a->y_, *b, b->x_, b->y_);
}

bool AABB_2d::colidiu_x(const AABB_2d *outro, int32_t dx) const {
    if (!outro) return false;
    return testar(*this, x_ + dx, y_, *outro, outro->x_, outro->y_);
}

bool AABB_2d::colidiu_y(const AABB_2d *outro, int32_t dy) const {
    if (!outro) return false;
    return testar(*this, x_, y_ + dy, *outro, outro->x_, outro->y_);
}

/* ---- API C ---- */

namespace {
inline Obj*        O(obj_handle* h)          { return reinterpret_cast<Obj*>(h); }
inline const Obj*  O(const obj_handle* h)    { return reinterpret_cast<const Obj*>(h); }
inline Animacao*   A(anim_handle* h)         { return reinterpret_cast<Animacao*>(h); }
inline AABB_2d*       B(aabb_2d_handle* h)         { return reinterpret_cast<AABB_2d*>(h); }
inline const AABB_2d* B(const aabb_2d_handle* h)   { return reinterpret_cast<const AABB_2d*>(h); }
}

extern "C" {

/* ---- Obj ---- */
obj_handle* obj_criar(fb_atlas_handle* sprite, uint16_t pw, uint16_t ph, int32_t x, int32_t y, uint16_t coluna, uint16_t linha) {
    Obj* o = new Obj;
    o->criar(sprite ? &sprite->at : nullptr, pw, ph, x, y, coluna, linha);
    return reinterpret_cast<obj_handle*>(o);
}
void obj_destruir(obj_handle* o)                { if (o) { O(o)->destruir(); delete O(o); } }
int32_t  obj_x(const obj_handle* o)             { return o ? O(o)->x() : 0; }
int32_t  obj_y(const obj_handle* o)             { return o ? O(o)->y() : 0; }
uint16_t obj_largura(const obj_handle* o)       { return o ? O(o)->largura() : 0; }
uint16_t obj_altura(const obj_handle* o)        { return o ? O(o)->altura() : 0; }
void obj_posicionar(obj_handle* o, int32_t x, int32_t y)       { if (o) O(o)->posicionar(x, y); }
void obj_mover(obj_handle* o, int32_t x, int32_t y)            { if (o) O(o)->mover(x, y); }
void obj_tile(obj_handle* o, uint16_t coluna, uint16_t linha)  { if (o) O(o)->tile(coluna, linha); }
void obj_cor(obj_handle* o, uint32_t cor)       { if (o) O(o)->cor(fb_cores::desempacotar(cor)); }
void obj_alpha(obj_handle* o, uint8_t a)        { if (o) O(o)->alpha(a); }
void obj_ampliar(obj_handle* o, uint16_t pw, uint16_t ph)      { if (o) O(o)->ampliar(pw, ph); }
void obj_angulo(obj_handle* o, float graus)     { if (o) O(o)->angulo(graus); }
void obj_espelhar_x(obj_handle* o, int e)       { if (o) O(o)->espelhar_x(e != 0); }
void obj_espelhar_y(obj_handle* o, int e)       { if (o) O(o)->espelhar_y(e != 0); }
void obj_desenhar(const obj_handle* o, fb_handle* fb) { if (o && fb) O(o)->desenhar(fb->fb); }

/* ---- Animacao ---- */
anim_handle* anim_criar(obj_handle* o, const int16_t* quadros, uint8_t total, uint8_t fps, int loop) {
    Animacao* a = new Animacao;
    a->criar(O(o), reinterpret_cast<const int16_t (*)[2]>(quadros), total, fps, loop != 0);
    return reinterpret_cast<anim_handle*>(a);
}
void anim_destruir(anim_handle* a)              { if (a) { A(a)->destruir(); delete A(a); } }
void anim_rodar(anim_handle* a)                 { if (a) A(a)->rodar(); }
void anim_parar(anim_handle* a)                 { if (a) A(a)->parar(); }
void anim_voltar(anim_handle* a)                { if (a) A(a)->voltar(); }
void anim_atualizar(anim_handle* a, uint32_t dt_ms) { if (a) A(a)->atualizar(dt_ms); }

/* ---- AABB_2d ---- */
aabb_2d_handle* aabb_2d_criar(obj_handle* o, const char* mascara, uint16_t mascara_w, uint16_t mascara_h) {
    AABB_2d* b = new AABB_2d;
    b->criar(O(o), mascara, mascara_w, mascara_h);
    return reinterpret_cast<aabb_2d_handle*>(b);
}
void aabb_2d_destruir(aabb_2d_handle* a)              { delete B(a); }
void aabb_2d_sincronizar(aabb_2d_handle* a, obj_handle* o) { if (a) B(a)->sincronizar_colisao(O(o)); }
void aabb_2d_posicionar(aabb_2d_handle* a, int32_t x, int32_t y) { if (a) B(a)->posicionar(x, y); }
int32_t  aabb_2d_x(const aabb_2d_handle* a)           { return a ? B(a)->x() : 0; }
int32_t  aabb_2d_y(const aabb_2d_handle* a)           { return a ? B(a)->y() : 0; }
uint16_t aabb_2d_largura(const aabb_2d_handle* a)     { return a ? B(a)->largura() : 0; }
uint16_t aabb_2d_altura(const aabb_2d_handle* a)      { return a ? B(a)->altura() : 0; }
int aabb_2d_colidio(const aabb_2d_handle* a, const aabb_2d_handle* b)   { return AABB_2d::colidio(B(a), B(b)); }
int aabb_2d_colidiu_x(const aabb_2d_handle* a, const aabb_2d_handle* outro, int32_t dx) { return a && B(a)->colidiu_x(B(outro), dx); }
int aabb_2d_colidiu_y(const aabb_2d_handle* a, const aabb_2d_handle* outro, int32_t dy) { return a && B(a)->colidiu_y(B(outro), dy); }

}