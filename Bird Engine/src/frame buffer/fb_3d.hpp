#ifndef FB_3D_HPP
#define FB_3D_HPP

#include "frame_buffer.hpp"

struct neblina{
    bool     ativa  = false;
    fb_cores cor    = {0, 0, 0, 255};
    float    inicio = 5.0f;
    float    fim    = 30.0f;
};

namespace trig{
    constexpr int      BITS_INDICE    = 10;
    constexpr int      BITS_FRACAO    = 16 - BITS_INDICE;
    constexpr int      TAM            = 1 << BITS_INDICE;
    constexpr int      MASCARA_FRACAO = (1 << BITS_FRACAO) - 1;

    constexpr int      BITS = 15;
    constexpr int32_t  UM   = 1 << BITS;

    constexpr uint16_t QUARTO_VOLTA = 16384;
    constexpr uint16_t MEIA_VOLTA   = 32768;

    constexpr double seno_taylor(double x) {
        double x2 = x * x, termo = x, soma = x;
        for (int i = 1; i <= 11; i++) {
            termo *= -x2 / ((2.0 * i) * (2.0 * i + 1.0));
            soma += termo;
        }
        return soma;
    }

    struct tabela {
        int32_t v[TAM];
        constexpr tabela() : v() {
            constexpr double PI = 3.14159265358979323846;
            for (int i = 0; i < TAM; i++) {
                double ang = 2.0 * PI * i / TAM;
                if (ang > PI) ang -= 2.0 * PI;
                double r = seno_taylor(ang) * UM;
                v[i] = (int32_t)(r >= 0 ? r + 0.5 : r - 0.5);
            }
        }
    };

    inline constexpr tabela TAB{};

    constexpr int32_t seno(uint16_t a) {
        const int i = a >> BITS_FRACAO, f = a & MASCARA_FRACAO;
        const int32_t v0 = TAB.v[i], v1 = TAB.v[(i + 1) & (TAM - 1)];
        return v0 + (((v1 - v0) * f) >> BITS_FRACAO);
    }

    constexpr int32_t  cosseno(uint16_t a) { return seno((uint16_t)(a + QUARTO_VOLTA)); }
    constexpr uint16_t graus(int g)        { return (uint16_t)(((int64_t)g * 65536) / 360); }

    constexpr uint16_t de_graus(float g) {
        const float v = g * (65536.0f / 360.0f);
        return (uint16_t)(int32_t)(v >= 0.0f ? v + 0.5f : v - 0.5f);
    }
}

typedef int32_t q16_t;

namespace q16 {
    constexpr int   BITS = 16;
    constexpr q16_t UM   = 1 << BITS;

    constexpr q16_t de_float(float v)    { return (q16_t)(v >= 0.0f ? v * UM + 0.5f : v * UM - 0.5f); }
    constexpr float para_float(q16_t v)  { return (float)v / (float)UM; }
    constexpr q16_t mul(q16_t a, q16_t b){ return (q16_t)(((int64_t)a * b) >> BITS); }
}

struct vertice {
    q16_t x, y, z;
};

struct face {
    uint16_t a, b, c;
    int16_t  nx, ny, nz;
};

namespace lut3d {

    struct tabela_recip {
        uint32_t v[257];
        constexpr tabela_recip() : v() {
            for (int i = 0; i <= 256; i++) {
                const double m = 1.0 + i / 256.0;
                v[i] = (uint32_t)(2147483648.0 / m + 0.5);
            }
        }
    };

    inline constexpr tabela_recip RECIP{};

    inline int msb32(uint32_t x) {
#if defined(__GNUC__) || defined(__clang__)
        return 31 - __builtin_clz(x);
#else
        int r = 0;
        while (x >>= 1) r++;
        return r;
#endif
    }

    inline uint32_t recip24(uint32_t z) {
        const int      e  = msb32(z);
        const uint32_t m  = z << (31 - e);
        const uint32_t i  = (m >> 23) & 255u;
        const uint32_t fr = (m >> 7) & 0xFFFFu;
        const uint32_t a = RECIP.v[i], b = RECIP.v[i + 1];
        const uint32_t r = a - (uint32_t)(((uint64_t)(a - b) * fr) >> 16);
        return e >= 9 ? r >> (e - 9) : r << (9 - e);
    }

    inline int msb64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
        return 63 - __builtin_clzll(x);
#else
        int r = 0;
        while (x >>= 1) r++;
        return r;
#endif
    }

    constexpr double raiz_const(double x) {
        double y = x > 1.0 ? x * 0.5 : 1.0;
        for (int i = 0; i < 40; i++) y = 0.5 * (y + x / y);
        return y;
    }

    struct tabela_rsqrt {
        uint32_t v[193];
        constexpr tabela_rsqrt() : v() {
            for (int i = 0; i <= 192; i++) {
                const double m = 1.0 + i / 64.0;
                v[i] = (uint32_t)(65536.0 / raiz_const(m) + 0.5);
            }
        }
    };

    inline constexpr tabela_rsqrt RSQRT{};

    inline void rsqrt64(uint64_t x, uint32_t& r, int& e2) {
        const int e = msb64(x) & ~1;
        const uint64_t off = (x << (62 - e)) - ((uint64_t)1 << 62);
        const uint32_t i  = (uint32_t)(off >> 56);
        const uint32_t fr = (uint32_t)((off >> 40) & 0xFFFFu);
        const uint32_t a = RSQRT.v[i], b = RSQRT.v[i + 1];
        r  = a - (uint32_t)(((uint64_t)(a - b) * fr) >> 16);
        e2 = e >> 1;
    }

    inline uint64_t isqrt64(uint64_t n) {
        uint64_t r = 0, bit = (uint64_t)1 << 62;
        while (bit > n) bit >>= 2;
        while (bit) {
            if (n >= r + bit) { n -= r + bit; r = (r >> 1) + bit; }
            else r >>= 1;
            bit >>= 2;
        }
        return r;
    }
}

struct mat3 {
    int32_t m[3][3];
};

class z_buffer {
public:
    z_buffer() = default;
    ~z_buffer();

    z_buffer(const z_buffer&) = delete;
    z_buffer& operator=(const z_buffer&) = delete;

    bool criar(int largura, int altura);
    void liberar();
    void limpar();

    bool      valido()  const { return d_ != nullptr; }
    int       largura() const { return w_; }
    int       altura()  const { return h_; }
    uint32_t* dados()         { return d_; }

private:
    uint32_t* d_ = nullptr;
    int       w_ = 0;
    int       h_ = 0;
};

class malha {
public:
    malha() = default;
    ~malha();

    malha(const malha&) = delete;
    malha& operator=(const malha&) = delete;
    malha(malha&& o) noexcept;
    malha& operator=(malha&& o) noexcept;

    bool reservar(int max_vertices, int max_faces);
    void liberar();

    int  add_vertice(q16_t x, q16_t y, q16_t z);
    bool add_face(int a, int b, int c);
    void add_quad(int a, int b, int c, int d);

    void orientar_para_fora();

    void calcular_normais();

    bool           valida()        const { return vs_ != nullptr; }
    int            num_vertices()  const { return nv_; }
    int            num_faces()     const { return nf_; }
    const vertice* vertices()      const { return vs_; }
    const face*    faces()         const { return fs_; }

private:
    void roubar(malha& o) noexcept;

    vertice* vs_    = nullptr;
    face*    fs_    = nullptr;
    int      nv_    = 0;
    int      nf_    = 0;
    int      cap_v_ = 0;
    int      cap_f_ = 0;
};

enum Colisao3d{
    AABB,
    OBB,
};

struct Vert3d {
    float x, y, z;
};

class Form3d {
public:

    void cubo     (float x, float y, float z, float w, float h, float d);
    void esfera   (float x, float y, float z, float raio, int aneis = 8, int setores = 12);
    void prisma   (float x, float y, float z, float w, float h, float d);
    void piramide (float x, float y, float z, float w, float h, float d);
    void cilindro (float x, float y, float z, float raio, float h, int segmentos = 12);

    void angulo_x(float x);
    void angulo_y(float y);
    void angulo_z(float z);

    void usar_cor(const fb_cores* cor);

    void mover_x(float x);
    void mover_y(float y);
    void mover_z(float z);
    void mover(float x, float y, float z);
    void girar(float x, float y, float z);

    void posicionar(float x, float y, float z);

    float posicao_atual_x() const { return px_; }
    float posicao_atual_y() const { return py_; }
    float posicao_atual_z() const { return pz_; }
    void definir_colisao(Colisao3d colisao);
    bool obj(float x, float y, float z, const char* texto,
    float tamanho = 0.0f, bool inverter_z = true, bool orientar = false);
    bool obj_arquivo(float x, float y, float z, const char* caminho,
    float tamanho = 0.0f, bool inverter_z = true, bool orientar = false);
    bool carregar_vertices(float x, float y, float z,
                       const Vert3d* verts, int n_verts,
                       const uint16_t* idx = nullptr, int n_idx = 0,
                       bool orientar = false);
    template <int N>
    bool carregar_vertices(float x, float y, float z, const Vert3d (&verts)[N], bool orientar = false) {
        return carregar_vertices(x, y, z, verts, N, nullptr, 0, orientar);
    }

    template <int N, int M>
    bool carregar_vertices(float x, float y, float z, const Vert3d (&verts)[N],
                        const uint16_t (&idx)[M], bool orientar = false) {
        return carregar_vertices(x, y, z, verts, N, idx, M, orientar);
    }
    Colisao3d colisao() const { return col_; }

    const malha&    dados()    const { return m_; }
    fb_cores        cor()      const { return cor_; }
    uint16_t        rot_x()    const { return ax_; }
    uint16_t        rot_y()    const { return ay_; }
    uint16_t        rot_z()    const { return az_; }

private:
    malha    m_;
    float    px_ = 0, py_ = 0, pz_ = 0;
    uint16_t ax_ = 0, ay_ = 0, az_ = 0;
    fb_cores cor_ = {255, 255, 255, 255};
    Colisao3d col_ = OBB;
};

constexpr int LUZES_MAX = 8;

struct luz3d {
    bool     ativa        = true;
    bool     spot         = false;
    float    x = 0.0f, y = 0.0f, z = 0.0f;
    float    dx = 0.0f, dy = 0.0f, dz = 1.0f;
    fb_cores cor          = {255, 255, 255, 255};
    float    intensidade  = 1.0f;
    float    alcance      = 10.0f;
    float    cone_interno = 15.0f;
    float    cone_externo = 25.0f;
    float    suavidade    = 0.5f;
};

class render3d {
public:
    render3d();
    ~render3d();

    render3d(const render3d&) = delete;
    render3d& operator=(const render3d&) = delete;

    void fov(float graus);
    void plano_proximo(float unidades);
    void camera(float x, float y, float z);
    void camera_angulo(float yaw, float pitch, float roll = 0.0f);
    void luz(float x, float y, float z);
    void ambiente(int v);
    void neblina_usar(const neblina& n);

    int     luz_nova(const luz3d& l);
    luz3d*  luz_obter(int id);
    void    luz_remover(int id);
    void    luzes_limpar();

    void comecar(frame_buffer& fb);
    void desenhar(frame_buffer& fb, const Form3d& f);

    int  triangulos() const { return tris_; }

private:
    struct vproj    { int32_t vx, vy, vz; int32_t sx, sy; uint32_t d; };
    struct ponto    { int32_t x, y; uint32_t d; };

    struct luz_cam {
        int64_t x, y, z;
        int64_t alc, r2, inv;
        int     sh;
        int32_t c[3];
        bool    spot;
        int32_t sx, sy, sz, cos_ext;
        int64_t inv_cone;
        int32_t wrap;
    };

    struct face_ctx {
        int     r, g, b;
        int     br, bg, bb;
        int     k;
        int32_t nx, ny, nz;
        int     nluz;
        uint8_t luz[LUZES_MAX];
    };

    ponto projetar(int32_t vx, int32_t vy, int32_t vz) const;
    int   sombrear(const mat3& modelo, const face& fc) const;
    void  preparar_luzes();
    void  preparar_face(face_ctx& f, const mat3& c, const face& fc, int32_t ax, int32_t ay, int32_t az) const;
    void  iluminar(const face_ctx& f, int x, int y, uint32_t dv, int& r, int& g, int& b) const;
    void  rasterizar(frame_buffer& fb, const ponto& a, const ponto& b, const ponto& c,
                     int64_t area, const face_ctx& fx);

    z_buffer z_;
    vproj*   vp_     = nullptr;
    int      cap_vp_ = 0;

    float    fov_    = 70.0f;
    q16_t    near_q_ = q16::de_float(0.1f);
    float    cam_x_  = 0.0f, cam_y_ = 0.0f, cam_z_ = 0.0f;
    uint16_t yaw_    = 0, pitch_ = 0, roll_ = 0;

    int32_t  luz_[3] = {0, 0, 0};
    int      amb_    = 70;

    luz3d    luzes_[LUZES_MAX];
    bool     usada_[LUZES_MAX] = {};
    luz_cam  lz_[LUZES_MAX];
    int      n_lz_ = 0;
    mat3     mv_ = {};
    int64_t  ifocal_ = 0;

    neblina  neb_;
    uint8_t  fog_lut_[512] = {};

    int      w_ = 0, h_ = 0;
    int32_t  cx4_ = 0, cy4_ = 0;
    int64_t  focal4_ = 0;
    int      tris_ = 0;
};

namespace col3d {
    struct vf { float x, y, z; };

    struct caixa {
        vf c;
        vf u[3];
        vf e;
    };
}

enum TipoCam3d{
    FPS,
    Terceira_pessoa,
};

class Cam3d{
public:
    Cam3d() = default;
    ~Cam3d();

    Cam3d(const Cam3d&) = delete;
    Cam3d& operator=(const Cam3d&) = delete;

    void criar(TipoCam3d tipo, float x, float y, float z, float angulo_x, float angulo_y, float angulo_z);
    void destruir();

    render3d& render() { return r_; }

    void chao(bool ativo)       { chao_ativo_ = ativo; }
    void definir_chao(float y)  { chao_y_ = y; chao_ativo_ = true; }
    void maximo_ceu(float y)    { ceu_y_ = y; ceu_ativo_ = true; }

    void mover(float x, float y, float z);
    void posicionar(float x, float y, float z);
    void girar(float x, float y, float z);
    void pular();
    void agachar(bool ligado) { agachado_ = ligado; }
    void correr(bool ligado)  { correndo_ = ligado; }
    void multiplicadores(float correr, float agachar) { mult_correr_ = correr; mult_agachar_ = agachar; }
    void balanco(bool ligado, float amplitude = 0.05f, float frequencia = 8.0f);
    void definir_corpo(float altura_olhos, float agachados_olhos, float altura_colisao, float raio, Colisao3d tipo_colisao);
    void seguir_corpo(Form3d *forma);
    void distancia(float d, float altura_alvo = 0.0f);

    void colisao_camera(bool ligada, float margem = 0.25f);

    void suavidade(float velocidade) { seg_vel_ = velocidade < 0.0f ? 0.0f : velocidade; }
    void degrau(float altura_maxima);
    void olhar_para(float x, float y, float z);
    void fisica(float gravidade, float forca_pulo);

    float x() const;
    float y() const;
    float z() const;
    float angulo_x() const { return pitch_; }
    float angulo_y() const { return yaw_; }
    float angulo_z() const { return roll_; }
    bool  no_chao() const  { return no_chao_; }
    void tremer(float intensidade, float duracao);
    void recuo(float pitch_graus, float yaw_graus = 0.0f);
    void recuperacao(float velocidade) { rec_vel_ = velocidade; }

    int  criar_luz(float x, float y, float z, const fb_cores *cor, float intensidade, float alcance);
    void mover_luz(int id, float x, float y, float z);
    void remover_luz(int id);
    void luz_direcional(float dx, float dy, float dz);
    void ambiente(float valor) { ambiente_ = valor; }
    void lanterna(bool ligada, float desloc_y = 0.0f);
    void cone_lanterna(float interno, float externo);
    void cor_luz(fb_cores cor);
    fb_cores cor_luz() const { return lant_.cor; }
    void intenciadade_luz(float valor);
    void alcance_luz(float alcance);
    void neblina_usar(bool ativa, fb_cores cor = {0, 0, 0, 255}, float inicio = 5.0f, float fim = 30.0f, bool pintar_fundo = true);
    void atualizar(float delta);

    void colisao(Form3d* const* formas, int n);

    void desenhar(frame_buffer& fb, Form3d* const* formas, int n);
    void girar_alvo(float pitch, float yaw, Cam3d* corpo = nullptr);
    void sincronizar_alvo(const Cam3d& corpo);
    void aura(bool ligada, float desloc_y = 0.0f);
    void aura_cor(fb_cores cor);
    void aura_intensidade(float valor);
    void aura_alcance(float alcance);
    void aura_suavidade(float valor);
    void alternar();
    void terceira_pessoa(bool ligada);
    bool em_terceira() const { return orbita(); }
    void luzes(bool lanterna_ligada, bool aura_ligada);

private:
    struct lanterna_cfg {
        bool     ligada       = false;
        float    desloc_y     = 0.0f;
        fb_cores cor          = {255, 255, 255, 255};
        float    intensidade  = 1.0f;
        float    alcance      = 25.0f;
        float    cone_interno = 18.0f;
        float    cone_externo = 28.0f;
    };
    struct aura_cfg {
        bool     ligada      = false;
        float    desloc_y    = 0.0f;
        fb_cores cor         = {255, 255, 255, 255};
        float    intensidade = 1.0f;
        float    alcance     = 12.0f;
        float    suavidade   = 0.5f;
    };
    bool orbita() const { return tp_ || tipo_ == Terceira_pessoa; }
    void sincronizar_heroi();
    void atualizar_aura();
    float altura_olho()  const;
    float altura_corpo() const;
    void  reunir_caixas();
    col3d::caixa montar_corpo(float x, float y, float z, float h, float folga) const;
    bool  colide_em(float x, float y, float z, float h, float folga) const;
    void  resolver(float h);
    void  atualizar_corpo(float dt);
    void  atualizar_seguindo(float dt);
    bool  tem_alvo() const { return alvo_ != nullptr; }
    void  posicao_alvo(float& x, float& y, float& z) const;
    float braco_livre(float ox, float oy, float oz, float dx, float dy, float dz, float len) const;
    void  atualizar_lanterna(float ex, float ey, float ez, float yaw, float pitch);
    void  efeitos(float dt);
    void  vista(float& ex, float& ey, float& ez, float& yaw, float& pitch, float& roll) const;

    render3d r_;
    TipoCam3d tipo_ = FPS;

    float px_ = 0, py_ = 0, pz_ = 0;
    float yaw_ = 0, pitch_ = 0, roll_ = 0;
    float vy_ = 0, vel_h_ = 0, mult_atual_ = 1.0f;
    bool  no_chao_ = false, no_chao_antes_ = false;
    float in_x_ = 0, in_y_ = 0, in_z_ = 0;

    float olho_ = 1.7f, olho_agach_ = 1.0f, alt_ = 1.8f, raio_ = 0.3f;
    float olho_atual_ = 1.7f;
    Colisao3d tipo_col_ = AABB;
    bool  agachado_ = false, agachado_real_ = false, correndo_ = false;
    float mult_correr_ = 1.6f, mult_agachar_ = 0.5f;
    float grav_ = 0, pulo_ = 0, degrau_ = 0;

    bool  chao_ativo_ = false, ceu_ativo_ = false;
    float chao_y_ = 0, ceu_y_ = 0;

    bool  bob_ativo_ = false;
    float bob_amp_ = 0.05f, bob_freq_ = 8.0f, bob_fase_ = 0, bob_k_ = 0, bob_y_ = 0;
    float suave_y_ = 0;
    float rec_pitch_ = 0, rec_yaw_ = 0, rec_vel_ = 8.0f;
    float trem_int_ = 0, trem_dur_ = 0, trem_t_ = 0, trem_yaw_ = 0, trem_pitch_ = 0, trem_roll_ = 0;
    uint32_t rng_ = 2463534242u;

    Form3d* alvo_     = nullptr;
    float   dist_ = 6.0f, alvo_alt_ = 0.0f;
    float   arm_ = 6.0f;
    bool    col_cam_ = true;
    float   margem_cam_ = 0.25f;
    float   seg_vel_ = 0.0f;
    bool    seg_ok_ = false;
    float   sx_ = 0, sy_ = 0, sz_ = 0;

    Form3d* const*  mundo_ = nullptr;
    int             n_mundo_ = 0;
    col3d::caixa*   caixas_ = nullptr;
    int             n_caixas_ = 0, cap_caixas_ = 0;

    lanterna_cfg lant_;
    int      lant_id_    = -1;
    float    ambiente_   = 70.0f / 255.0f;
    bool     fog_ativa_  = false;
    bool     fog_fundo_  = true;
    fb_cores fog_cor_    = {0, 0, 0, 255};
    aura_cfg aura_;
    int      aura_id_ = -1;
    bool  tp_ = false;
    float cx_ = 0, cy_ = 0, cz_ = 0;
};

enum lados{
    Mao_direita,
    Mao_esquerda,
};

class Mao{
public:
    void criar(Form3d *forma, Cam3d *camera, lados lado);
    float angulo_x() const;
    float angulo_y() const;
    float angulo_z() const;
    float x() const;
    float y() const;
    float z() const;
    void sincronizar(Form3d *corpo, Cam3d *camera);
    void atualizar(float delta);
    void mover(float x, float y, float z);
    void girar(float x, float y, float z);
    void terceira_pessoa(bool ativo);
    void mudar_visao(TipoCam3d tipo);
    void recuo(float tras = 0.5f, float tempo_voltar = 0.5f);
    void bater(float frente = 0.5f, float tempo_voltar = 0.5f);
    void nao_mudar_angulo(bool ativo = true);
    void posicionar(float x, float y, float z);
    void colisao(Colisao3d tipo);
    void fixa(bool ativar);

private:
    Form3d* forma_ = nullptr;
    Form3d* corpo_ = nullptr;
    Cam3d*  cam_   = nullptr;
    lados   lado_  = Mao_direita;

    float ox_ = 0.4f, oy_ = -0.35f, oz_ = 0.7f;
    float rx_ = 0, ry_ = 0, rz_ = 0;
    float fx_ = 0, fy_ = 0, fz_ = 0;
    float ax_ = 0, ay_ = 0, az_ = 0;

    bool tp_   = false;
    bool fixo_ = false;

    int   anim_tipo_ = 0;
    float anim_t_    = 0;
    float anim_dur_  = 0.5f;
    float anim_dist_ = 0.5f;
    bool fixa_ = true;
};

extern "C" {

typedef struct f3d_handle  f3d_handle;   /* Form3d */
typedef struct cam3d_handle cam3d_handle; /* Cam3d  */
typedef struct z3d_handle      z3d_handle;       /* z_buffer */
typedef struct render3d_handle render3d_handle;  /* render3d */

/* ---- Form3d ---- */
FB_API f3d_handle* f3d_criar(void);
FB_API void  f3d_destruir(f3d_handle* f);
FB_API void  f3d_cubo(f3d_handle* f, float x, float y, float z, float w, float h, float d);
FB_API void  f3d_esfera(f3d_handle* f, float x, float y, float z, float raio, int aneis, int setores);
FB_API void  f3d_prisma(f3d_handle* f, float x, float y, float z, float w, float h, float d);
FB_API void  f3d_piramide(f3d_handle* f, float x, float y, float z, float w, float h, float d);
FB_API void  f3d_cilindro(f3d_handle* f, float x, float y, float z, float raio, float h, int segmentos);
FB_API void  f3d_angulo_x(f3d_handle* f, float x);
FB_API void  f3d_angulo_y(f3d_handle* f, float y);
FB_API void  f3d_angulo_z(f3d_handle* f, float z);
FB_API void  f3d_usar_cor(f3d_handle* f, uint32_t cor);          /* ARGB */
FB_API uint32_t f3d_cor(const f3d_handle* f);
FB_API void  f3d_mover_x(f3d_handle* f, float x);
FB_API void  f3d_mover_y(f3d_handle* f, float y);
FB_API void  f3d_mover_z(f3d_handle* f, float z);
FB_API void  f3d_mover(f3d_handle* f, float x, float y, float z);
FB_API void  f3d_girar(f3d_handle* f, float x, float y, float z);
FB_API void  f3d_posicionar(f3d_handle* f, float x, float y, float z);
FB_API float f3d_x(const f3d_handle* f);
FB_API float f3d_y(const f3d_handle* f);
FB_API float f3d_z(const f3d_handle* f);
FB_API uint16_t f3d_rot_x(const f3d_handle* f);
FB_API uint16_t f3d_rot_y(const f3d_handle* f);
FB_API uint16_t f3d_rot_z(const f3d_handle* f);
FB_API void  f3d_definir_colisao(f3d_handle* f, int tipo);       /* 0=AABB 1=OBB */
FB_API int   f3d_colisao(const f3d_handle* f);
FB_API int   f3d_num_vertices(const f3d_handle* f);
FB_API int   f3d_num_faces(const f3d_handle* f);
FB_API int   f3d_obj(f3d_handle* f, float x, float y, float z, const char* texto, float tamanho, int inverter_z, int orientar);
FB_API int   f3d_obj_arquivo(f3d_handle* f, float x, float y, float z, const char* caminho, float tamanho, int inverter_z, int orientar);
/* verts = n_verts * 3 floats (x,y,z); idx pode ser NULL */
FB_API int   f3d_carregar_vertices(f3d_handle* f, float x, float y, float z, const float* verts, int n_verts, const uint16_t* idx, int n_idx, int orientar);

/* ---- Cam3d ---- */
FB_API cam3d_handle* cam3d_criar(int tipo, float x, float y, float z, float ax, float ay, float az); /* 0=FPS 1=Terceira */
FB_API void  cam3d_destruir(cam3d_handle* c);

FB_API void  cam3d_fov(cam3d_handle* c, float graus);
FB_API void  cam3d_plano_proximo(cam3d_handle* c, float unidades);
FB_API int   cam3d_triangulos(const cam3d_handle* c);

FB_API void  cam3d_chao(cam3d_handle* c, int ativo);
FB_API void  cam3d_definir_chao(cam3d_handle* c, float y);
FB_API void  cam3d_maximo_ceu(cam3d_handle* c, float y);
FB_API void  cam3d_mover(cam3d_handle* c, float x, float y, float z);
FB_API void  cam3d_posicionar(cam3d_handle* c, float x, float y, float z);
FB_API void  cam3d_girar(cam3d_handle* c, float x, float y, float z);
FB_API void  cam3d_pular(cam3d_handle* c);
FB_API void  cam3d_agachar(cam3d_handle* c, int ligado);
FB_API void  cam3d_correr(cam3d_handle* c, int ligado);
FB_API void  cam3d_multiplicadores(cam3d_handle* c, float correr, float agachar);
FB_API void  cam3d_balanco(cam3d_handle* c, int ligado, float amplitude, float frequencia);
FB_API void  cam3d_definir_corpo(cam3d_handle* c, float altura_olhos, float agachados_olhos, float altura_colisao, float raio, int tipo_colisao);
FB_API void  cam3d_seguir_corpo(cam3d_handle* c, f3d_handle* forma);
FB_API void  cam3d_distancia(cam3d_handle* c, float d, float altura_alvo);
FB_API void  cam3d_colisao_camera(cam3d_handle* c, int ligada, float margem);
FB_API void  cam3d_suavidade(cam3d_handle* c, float velocidade);
FB_API void  cam3d_degrau(cam3d_handle* c, float altura_maxima);
FB_API void  cam3d_olhar_para(cam3d_handle* c, float x, float y, float z);
FB_API void  cam3d_fisica(cam3d_handle* c, float gravidade, float forca_pulo);
FB_API float cam3d_x(const cam3d_handle* c);
FB_API float cam3d_y(const cam3d_handle* c);
FB_API float cam3d_z(const cam3d_handle* c);
FB_API float cam3d_angulo_x(const cam3d_handle* c);
FB_API float cam3d_angulo_y(const cam3d_handle* c);
FB_API float cam3d_angulo_z(const cam3d_handle* c);
FB_API int   cam3d_no_chao(const cam3d_handle* c);
FB_API void  cam3d_tremer(cam3d_handle* c, float intensidade, float duracao);
FB_API void  cam3d_recuo(cam3d_handle* c, float pitch_graus, float yaw_graus);
FB_API void  cam3d_recuperacao(cam3d_handle* c, float velocidade);

FB_API int   cam3d_criar_luz(cam3d_handle* c, float x, float y, float z, uint32_t cor, float intensidade, float alcance);
FB_API void  cam3d_mover_luz(cam3d_handle* c, int id, float x, float y, float z);
FB_API void  cam3d_remover_luz(cam3d_handle* c, int id);
FB_API void  cam3d_luz_direcional(cam3d_handle* c, float dx, float dy, float dz);
FB_API void  cam3d_ambiente(cam3d_handle* c, float valor);
FB_API void  cam3d_lanterna(cam3d_handle* c, int ligada, float desloc_y);
FB_API void  cam3d_cone_lanterna(cam3d_handle* c, float interno, float externo);
FB_API void  cam3d_cor_luz(cam3d_handle* c, uint32_t cor);
FB_API uint32_t cam3d_get_cor_luz(const cam3d_handle* c);
FB_API void  cam3d_intensidade_luz(cam3d_handle* c, float valor);
FB_API void  cam3d_alcance_luz(cam3d_handle* c, float alcance);
FB_API void  cam3d_neblina_usar(cam3d_handle* c, int ativa, uint32_t cor, float inicio, float fim, int pintar_fundo);
FB_API void  cam3d_aura(cam3d_handle* c, int ligada, float desloc_y);
FB_API void  cam3d_aura_cor(cam3d_handle* c, uint32_t cor);
FB_API void  cam3d_aura_intensidade(cam3d_handle* c, float valor);
FB_API void  cam3d_aura_alcance(cam3d_handle* c, float alcance);
FB_API void  cam3d_aura_suavidade(cam3d_handle* c, float valor);
FB_API void  cam3d_luzes(cam3d_handle* c, int lanterna_ligada, int aura_ligada);

FB_API void  cam3d_alternar(cam3d_handle* c);
FB_API void  cam3d_terceira_pessoa(cam3d_handle* c, int ligada);
FB_API int   cam3d_em_terceira(const cam3d_handle* c);
FB_API void  cam3d_girar_alvo(cam3d_handle* c, float pitch, float yaw, cam3d_handle* corpo);
FB_API void  cam3d_sincronizar_alvo(cam3d_handle* c, const cam3d_handle* corpo);

FB_API void  cam3d_atualizar(cam3d_handle* c, float delta);
FB_API void  cam3d_colisao_mundo(cam3d_handle* c, f3d_handle* const* formas, int n);
FB_API void  cam3d_desenhar(cam3d_handle* c, fb_handle* fb, f3d_handle* const* formas, int n);

/* ---- mat3d: matriz 3x3 como int32_t[9], linha a linha, escala 1<<15 ---- */
FB_API void mat3d_rot_x(uint16_t angulo, int32_t* out9);
FB_API void mat3d_rot_y(uint16_t angulo, int32_t* out9);
FB_API void mat3d_rot_z(uint16_t angulo, int32_t* out9);
FB_API void mat3d_mul(const int32_t* a9, const int32_t* b9, int32_t* out9);
FB_API void mat3d_modelo(const f3d_handle* forma, int32_t* out9);

/* ---- z_buffer ---- */
FB_API z3d_handle* z3d_criar(int largura, int altura);   /* NULL em falha */
FB_API void      z3d_destruir(z3d_handle* z);
FB_API void      z3d_limpar(z3d_handle* z);
FB_API int       z3d_valido(const z3d_handle* z);
FB_API int       z3d_largura(const z3d_handle* z);
FB_API int       z3d_altura(const z3d_handle* z);
FB_API uint32_t* z3d_dados(z3d_handle* z);

/* ---- render3d ---- */
FB_API render3d_handle* render3d_criar(void);
FB_API void  render3d_destruir(render3d_handle* r);
FB_API void  render3d_fov(render3d_handle* r, float graus);
FB_API void  render3d_plano_proximo(render3d_handle* r, float unidades);
FB_API void  render3d_camera(render3d_handle* r, float x, float y, float z);
FB_API void  render3d_camera_angulo(render3d_handle* r, float yaw, float pitch, float roll);
FB_API void  render3d_luz(render3d_handle* r, float x, float y, float z);   /* luz direcional */
FB_API void  render3d_ambiente(render3d_handle* r, int v);                  /* 0..255 */
FB_API void  render3d_neblina_usar(render3d_handle* r, int ativa, uint32_t cor, float inicio, float fim);

/* luzes pontuais/spot: retorna id (0..7) ou -1 se cheio */
FB_API int   render3d_luz_nova(render3d_handle* r, int ativa, int spot,
                               float x, float y, float z, float dx, float dy, float dz,
                               uint32_t cor, float intensidade, float alcance,
                               float cone_interno, float cone_externo, float suavidade);
FB_API int   render3d_luz_atualizar(render3d_handle* r, int id, int ativa, int spot,
                                    float x, float y, float z, float dx, float dy, float dz,
                                    uint32_t cor, float intensidade, float alcance,
                                    float cone_interno, float cone_externo, float suavidade);  /* 0 se id inválido */
FB_API void  render3d_luz_remover(render3d_handle* r, int id);
FB_API void  render3d_luzes_limpar(render3d_handle* r);

FB_API void  render3d_comecar(render3d_handle* r, fb_handle* fb);
FB_API void  render3d_desenhar(render3d_handle* r, fb_handle* fb, const f3d_handle* forma);
FB_API int   render3d_triangulos(const render3d_handle* r);

}

#endif
