#ifndef MUNDO_PROCEDURAL_HPP
#define MUNDO_PROCEDURAL_HPP

#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <birdpp.hpp>

class MundoProcedural {
public:
    static const int TAM = 16;

    struct Alvo {
        int32_t tx, ty, tz;
        int32_t ax, ay, az;
        char    tile;
    };

    explicit MundoProcedural(Mapa3d& mapa, int raio_xz = 10);
    ~MundoProcedural();

    MundoProcedural(const MundoProcedural&) = delete;
    MundoProcedural& operator=(const MundoProcedural&) = delete;

    void raio(int raio_xz);
    int  raio() const { return raio_xz_; }
    void semente(uint32_t valor);
    void relevo(float amplitude, float escala);
    void montanhas(float altura);
    void altitudes(int altura_pedra, int altura_arvore);
    void arvores(float densidade);
    void area_livre(int32_t tx, int32_t tz, int raio);
    void profundidade(int tiles) { fundo_ = tiles; }
    int  profundidade() const { return fundo_; }
    bool protegido(int32_t tx, int32_t tz) const;

    void atualizar(float x, float y, float z, int max_novos = 4);
    bool gerar_chunk(int32_t cx, int32_t cy, int32_t cz);
    bool descarregar_chunk(int32_t cx, int32_t cy, int32_t cz);
    int  num_chunks() const { return (int)blocos_.size(); }

    bool mirar(const float* origem, const float* direcao, float alcance, Alvo& alvo) const;
    bool modificar(int32_t tx, int32_t ty, int32_t tz, char c);
    bool quebrar(int32_t tx, int32_t ty, int32_t tz);
    bool quebrar_mira(const float* origem, const float* direcao, float alcance = 5.0f);

    int32_t superficie(int32_t tx, int32_t tz) const;
    char    tile_em(int32_t tx, int32_t ty, int32_t tz) const;
    void streaming(int borda, int passo) {
        borda_ = borda < 0 ? 0 : borda;
        passo_ = passo < 1 ? 1 : passo;
    }
private:
    static const int M = 2;
    static const int N = TAM + 2 * M;
    static const int ARV_MAX = 9;
    static const int CAMADAS_TERRA = 4;

    struct Bloco {
        int32_t cx, cy, cz;
        std::unique_ptr<char[]> dados;
    };

    struct Edicao {
        uint16_t idx;
        char     c;
    };

    struct Arvore {
        int     di, dj;
        int32_t top, base;
    };

    struct Coluna {
        int32_t sup[N * N];
        uint8_t arv[N * N];
        int32_t topo, fundo;
    };

    struct Pendente {
        int32_t cx, cy, cz;
        int32_t prioridade;
    };

    static bool pedir(Mapa3d& mapa, int32_t tx, int32_t ty, int32_t tz, void* usr);
    static uint64_t chave(int32_t cx, int32_t cy, int32_t cz);
    static uint64_t chave_coluna(int32_t cx, int32_t cz);
    static char decidir(int32_t ty, int32_t s, bool pedra, const Arvore* v, int n, int32_t fundo);

    int  altura_arvore(int32_t tx, int32_t tz) const;
    bool pedra_topo(int32_t tx, int32_t tz, int32_t s, int32_t a, int32_t b, int32_t c, int32_t d) const;
    const Coluna& coluna(int32_t cx, int32_t cz);
    void limpar_colunas();

    Mapa3d& mapa_;
    int raio_xz_;
    uint32_t seed_ = 1337;
    float amplitude_ = 10.0f;
    float escala_ = 24.0f;
    float montanha_ = 40.0f;
    float densidade_ = 0.01f;
    int32_t alt_pedra_ = 24;
    int32_t alt_arvore_ = 18;
    int32_t livre_tx_ = 0, livre_tz_ = 0;
    int livre_raio_ = -1;
    int32_t fundo_ = 48;
    std::unordered_map<uint64_t, std::vector<Edicao>> edicoes_;
    std::unordered_map<uint64_t, Bloco> blocos_;
    std::unordered_map<uint64_t, Coluna> colunas_;
    std::vector<Pendente> fila_;
    int32_t centro_cx_ = 0, centro_cz_ = 0;
    bool    iniciado_  = false;
    int     borda_     = 2;
    int     passo_     = 3;
    int32_t ult_pcx_ = 0, ult_pcy_ = 0, ult_pcz_ = 0;
    bool    completo_ = false;
};

#endif