#include "mundo_procedural.hpp"
#include <math.h>
#include <string.h>
#include <algorithm>

namespace {

int32_t dist(int32_t a, int32_t b) { return a > b ? a - b : b - a; }

uint32_t mistura(int32_t x, int32_t z, uint32_t seed) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)z * 668265263u + seed * 2246822519u + 1013904223u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return h;
}

float aleat(int32_t x, int32_t z, uint32_t seed) {
    return (float)(mistura(x, z, seed) & 0xFFFF) / 65535.0f;
}

float suave(float t) { return t * t * (3.0f - 2.0f * t); }

float lim01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

float ruido(float x, float z, uint32_t seed) {
    const float fx = floorf(x), fz = floorf(z);
    const int32_t x0 = (int32_t)fx, z0 = (int32_t)fz;
    const float tx = suave(x - fx), tz = suave(z - fz);
    const float a = aleat(x0,     z0,     seed);
    const float b = aleat(x0 + 1, z0,     seed);
    const float c = aleat(x0,     z0 + 1, seed);
    const float d = aleat(x0 + 1, z0 + 1, seed);
    const float l1 = a + (b - a) * tx;
    const float l2 = c + (d - c) * tx;
    return l1 + (l2 - l1) * tz;
}

float fbm(float x, float z, uint32_t seed, int oitavas) {
    float soma = 0.0f, amp = 1.0f, norm = 0.0f;
    for (int i = 0; i < oitavas; i++) {
        soma += ruido(x, z, seed + (uint32_t)i * 977u) * amp;
        norm += amp;
        amp *= 0.5f;
        x = x * 2.0f + 31.7f;
        z = z * 2.0f + 17.3f;
    }
    return soma / norm;
}

float serra(float x, float z, uint32_t seed, int oitavas) {
    float soma = 0.0f, amp = 1.0f, norm = 0.0f;
    for (int i = 0; i < oitavas; i++) {
        float n = 1.0f - fabsf(2.0f * ruido(x, z, seed + (uint32_t)i * 613u) - 1.0f);
        n *= n;
        soma += n * amp;
        norm += amp;
        amp *= 0.5f;
        x = x * 2.03f + 11.1f;
        z = z * 2.03f + 7.7f;
    }
    return soma / norm;
}

}

MundoProcedural::MundoProcedural(Mapa3d& mapa, int raio_xz)
    : mapa_(mapa), raio_xz_(raio_xz < 1 ? 1 : raio_xz) {
    mapa_.definir_pedir_chunk(&MundoProcedural::pedir, this);
}

MundoProcedural::~MundoProcedural() {
    mapa_.definir_pedir_chunk(nullptr, nullptr);
    for (auto& par : blocos_) {
        const Bloco& b = par.second;
        mapa_.remover_chunk(b.cx * TAM, b.cy * TAM, b.cz * TAM);
    }
}

void MundoProcedural::limpar_colunas() {
    colunas_.clear();
    completo_ = false;
}

void MundoProcedural::raio(int raio_xz) {
    raio_xz_ = raio_xz < 1 ? 1 : raio_xz;
    completo_ = false;
}

void MundoProcedural::semente(uint32_t valor) {
    seed_ = valor;
    limpar_colunas();
}

void MundoProcedural::relevo(float amplitude, float escala) {
    amplitude_ = amplitude < 0.0f ? 0.0f : amplitude;
    escala_    = escala < 1.0f ? 1.0f : escala;
    limpar_colunas();
}

void MundoProcedural::montanhas(float altura) {
    montanha_ = altura < 0.0f ? 0.0f : altura;
    limpar_colunas();
}

void MundoProcedural::altitudes(int altura_pedra, int altura_arvore) {
    alt_pedra_  = altura_pedra < 6 ? 6 : altura_pedra;
    alt_arvore_ = altura_arvore;
    limpar_colunas();
}

void MundoProcedural::arvores(float densidade) {
    densidade_ = lim01(densidade);
    limpar_colunas();
}

void MundoProcedural::area_livre(int32_t tx, int32_t tz, int raio) {
    livre_tx_ = tx;
    livre_tz_ = tz;
    livre_raio_ = raio;
    limpar_colunas();
}

uint64_t MundoProcedural::chave(int32_t cx, int32_t cy, int32_t cz) {
    return ((uint64_t)((uint32_t)cx & 0x1FFFFF) << 42) |
           ((uint64_t)((uint32_t)cy & 0x1FFFFF) << 21) |
            (uint64_t)((uint32_t)cz & 0x1FFFFF);
}

uint64_t MundoProcedural::chave_coluna(int32_t cx, int32_t cz) {
    return ((uint64_t)(uint32_t)cx << 32) | (uint64_t)(uint32_t)cz;
}

int32_t MundoProcedural::superficie(int32_t tx, int32_t tz) const {
    const float x = (float)tx, z = (float)tz;

    float colinas = fbm(x / escala_, z / escala_, seed_, 4);
    colinas = lim01((colinas - 0.5f) * 2.0f + 0.5f);
    float h = colinas * amplitude_;

    float zona = fbm(x / (escala_ * 4.0f), z / (escala_ * 4.0f), seed_ + 31u, 3);
    zona = suave(lim01((zona - 0.44f) / 0.14f));

    if (livre_raio_ >= 0) {
        const float dx = (float)(tx - livre_tx_), dz = (float)(tz - livre_tz_);
        const float d = sqrtf(dx * dx + dz * dz);
        zona *= lim01((d - 20.0f) / 30.0f);
    }

    if (zona > 0.0f) {
        float r = serra(x / (escala_ * 2.2f), z / (escala_ * 2.2f), seed_ + 53u, 4);
        r = lim01(r * 1.7f);
        h += zona * (r * montanha_ + colinas * amplitude_ * 0.4f);
    }

    return -(int32_t)floorf(h + 0.5f);
}

bool MundoProcedural::protegido(int32_t tx, int32_t tz) const {
    return livre_raio_ >= 0 && dist(tx, livre_tx_) <= livre_raio_ && dist(tz, livre_tz_) <= livre_raio_;
}

bool MundoProcedural::pedra_topo(int32_t tx, int32_t tz, int32_t s, int32_t a, int32_t b, int32_t c, int32_t d) const {
    int32_t inc = dist(s, a);
    if (dist(s, b) > inc) inc = dist(s, b);
    if (dist(s, c) > inc) inc = dist(s, c);
    if (dist(s, d) > inc) inc = dist(s, d);
    if (inc >= 3) return true;

    const int32_t alt = -s;
    const int32_t ini = alt_pedra_ - 5;
    if (alt <= ini) return false;
    if (alt >= alt_pedra_) return true;
    return aleat(tx, tz, seed_ + 77u) < (float)(alt - ini) / 5.0f;
}

int MundoProcedural::altura_arvore(int32_t tx, int32_t tz) const {
    if (protegido(tx, tz)) return 0;
    const uint32_t h = mistura(tx, tz, seed_ + 101u);
    if ((float)(h & 0xFFFF) / 65535.0f >= densidade_) return 0;

    const int32_t s = superficie(tx, tz);
    if (-s > alt_arvore_) return 0;
    if (pedra_topo(tx, tz, s, superficie(tx - 1, tz), superficie(tx + 1, tz),
                   superficie(tx, tz - 1), superficie(tx, tz + 1))) return 0;
    return 4 + (int)((h >> 16) % 3u);
}

char MundoProcedural::decidir(int32_t ty, int32_t s, bool pedra, const Arvore* v, int n, int32_t fundo) {
    if (ty >= fundo) return '.';
    if (ty >= s) {
        const int32_t prof = ty - s;
        if (pedra) return 'p';
        if (prof == 0) return '#';
        if (prof < CAMADAS_TERRA) return 'd';
        return 'p';
    }

    char c = '.';
    for (int i = 0; i < n; i++) {
        const Arvore& a = v[i];
        const int32_t rel = ty - a.top;
        if (a.di == 0 && a.dj == 0 && rel >= 0) return 't';

        const int ax = a.di < 0 ? -a.di : a.di;
        const int az = a.dj < 0 ? -a.dj : a.dj;
        bool folha = false;
        if (rel == 0 || rel == 1) folha = !(ax == 2 && az == 2);
        else if (rel == -1)       folha = ax <= 1 && az <= 1;
        else if (rel == -2)       folha = ax + az <= 1;
        if (folha) c = 'f';
    }
    return c;
}

char MundoProcedural::tile_em(int32_t tx, int32_t ty, int32_t tz) const {
    Arvore v[25];
    int n = 0;
    for (int dj = -2; dj <= 2; dj++)
        for (int di = -2; di <= 2; di++) {
            const int h = altura_arvore(tx + di, tz + dj);
            if (!h) continue;
            const int32_t base = superficie(tx + di, tz + dj);
            v[n].di = di; v[n].dj = dj;
            v[n].base = base; v[n].top = base - h;
            n++;
        }
    const int32_t s = superficie(tx, tz);
    const bool pedra = pedra_topo(tx, tz, s, superficie(tx - 1, tz), superficie(tx + 1, tz),
                                  superficie(tx, tz - 1), superficie(tx, tz + 1));
    return decidir(ty, s, pedra, v, n, fundo_);
}

const MundoProcedural::Coluna& MundoProcedural::coluna(int32_t cx, int32_t cz) {
    const uint64_t k = chave_coluna(cx, cz);
    auto it = colunas_.find(k);
    if (it != colunas_.end()) return it->second;

    Coluna& col = colunas_[k];
    const int32_t ox = cx * TAM, oz = cz * TAM;
    col.topo = 0;
    col.fundo = 0;
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++) {
            const int32_t tx = ox - M + i, tz = oz - M + j;
            const int32_t s = superficie(tx, tz);
            col.sup[j * N + i] = s;
            if ((j == 0 && i == 0) || s < col.topo) col.topo = s;
            if ((j == 0 && i == 0) || s > col.fundo) col.fundo = s;
        }
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++)
            col.arv[j * N + i] = (uint8_t)altura_arvore(ox - M + i, oz - M + j);
    return col;
}

bool MundoProcedural::gerar_chunk(int32_t cx, int32_t cy, int32_t cz) {
    const uint64_t k = chave(cx, cy, cz);
    if (blocos_.find(k) != blocos_.end()) return true;

    const Coluna& col = coluna(cx, cz);
    const int32_t ox = cx * TAM, oy = cy * TAM, oz = cz * TAM;

    std::unique_ptr<char[]> dados(new char[TAM * TAM * TAM]);

    if (oy + TAM - 1 < col.topo - ARV_MAX) {
        memset(dados.get(), '.', TAM * TAM * TAM);
    } else if (oy >= col.fundo + CAMADAS_TERRA && oy + TAM <= fundo_) {
        memset(dados.get(), 'p', TAM * TAM * TAM);
    } else {
        for (int z = 0; z < TAM; z++)
            for (int x = 0; x < TAM; x++) {
                const int cj = z + M, ci = x + M;
                Arvore lista[25];
                int n = 0;
                for (int dj = -M; dj <= M; dj++)
                    for (int di = -M; di <= M; di++) {
                        const int h = col.arv[(cj + dj) * N + (ci + di)];
                        if (!h) continue;
                        const int32_t base = col.sup[(cj + dj) * N + (ci + di)];
                        lista[n].di = di; lista[n].dj = dj;
                        lista[n].base = base; lista[n].top = base - h;
                        n++;
                    }

                const int32_t s = col.sup[cj * N + ci];
                const bool pedra = pedra_topo(ox + x, oz + z, s,
                                              col.sup[cj * N + ci - 1], col.sup[cj * N + ci + 1],
                                              col.sup[(cj - 1) * N + ci], col.sup[(cj + 1) * N + ci]);
                for (int y = 0; y < TAM; y++)
                    dados[(y * TAM + z) * TAM + x] = decidir(oy + y, s, pedra, lista, n, fundo_);
            }
    }

    auto ed = edicoes_.find(k);
    if (ed != edicoes_.end())
        for (const Edicao& e : ed->second) dados[e.idx] = e.c;

    if (!mapa_.add_chunk(dados.get(), TAM, TAM, TAM, TAM, ox, oy, oz)) return false;

    Bloco b;
    b.cx = cx; b.cy = cy; b.cz = cz;
    b.dados = std::move(dados);
    blocos_.emplace(k, std::move(b));
    return true;
}

bool MundoProcedural::mirar(const float* origem, const float* direcao, float alcance, Alvo& alvo) const {
    const float tam = mapa_.tamanho_tile();
    float d[3] = {direcao[0], direcao[1], direcao[2]};
    const float len = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    if (len < 1e-6f) return false;
    for (int k = 0; k < 3; k++) d[k] /= len;

    const float p[3] = {origem[0] / tam, origem[1] / tam, origem[2] / tam};
    int32_t t[3], passo[3];
    float tmax[3], tdelta[3];
    for (int k = 0; k < 3; k++) {
        t[k] = (int32_t)floorf(p[k]);
        if (d[k] > 0.0f) {
            passo[k] = 1;
            tdelta[k] = 1.0f / d[k];
            tmax[k] = ((float)(t[k] + 1) - p[k]) / d[k];
        } else if (d[k] < 0.0f) {
            passo[k] = -1;
            tdelta[k] = -1.0f / d[k];
            tmax[k] = (p[k] - (float)t[k]) / -d[k];
        } else {
            passo[k] = 0;
            tdelta[k] = tmax[k] = 1e30f;
        }
    }

    const float limite = alcance / tam;
    int32_t ant[3] = {t[0], t[1], t[2]};
    for (int i = 0; i < 512; i++) {
        const char c = mapa_.obter_char(t[0], t[1], t[2]);
        if (c != '.' && c != ' ' && c != 0) {
            alvo.tx = t[0];   alvo.ty = t[1];   alvo.tz = t[2];
            alvo.ax = ant[0]; alvo.ay = ant[1]; alvo.az = ant[2];
            alvo.tile = c;
            return true;
        }
        ant[0] = t[0]; ant[1] = t[1]; ant[2] = t[2];

        const int k = tmax[0] < tmax[1] ? (tmax[0] < tmax[2] ? 0 : 2) : (tmax[1] < tmax[2] ? 1 : 2);
        if (tmax[k] > limite) break;
        t[k] += passo[k];
        tmax[k] += tdelta[k];
    }
    return false;
}

bool MundoProcedural::modificar(int32_t tx, int32_t ty, int32_t tz, char c) {
    if (!mapa_.definir_char(tx, ty, tz, c)) return false;

    const uint64_t k = chave(tx >> 4, ty >> 4, tz >> 4);
    const uint16_t idx = (uint16_t)((((ty & 15) * TAM) + (tz & 15)) * TAM + (tx & 15));
    std::vector<Edicao>& v = edicoes_[k];
    for (Edicao& e : v) {
        if (e.idx == idx) { e.c = c; return true; }
    }
    Edicao e;
    e.idx = idx;
    e.c = c;
    v.push_back(e);
    return true;
}

bool MundoProcedural::quebrar(int32_t tx, int32_t ty, int32_t tz) {
    if (protegido(tx, tz)) return false;
    const char c = mapa_.obter_char(tx, ty, tz);
    if (c == '.' || c == ' ' || c == 0) return false;
    return modificar(tx, ty, tz, '.');
}

bool MundoProcedural::quebrar_mira(const float* origem, const float* direcao, float alcance) {
    Alvo a;
    if (!mirar(origem, direcao, alcance, a)) return false;
    return quebrar(a.tx, a.ty, a.tz);
}

bool MundoProcedural::descarregar_chunk(int32_t cx, int32_t cy, int32_t cz) {
    auto it = blocos_.find(chave(cx, cy, cz));
    if (it == blocos_.end()) return false;
    mapa_.remover_chunk(cx * TAM, cy * TAM, cz * TAM);
    blocos_.erase(it);
    completo_ = false;
    return true;
}

bool MundoProcedural::pedir(Mapa3d&, int32_t tx, int32_t ty, int32_t tz, void* usr) {
    MundoProcedural* m = static_cast<MundoProcedural*>(usr);
    return m->gerar_chunk(tx >> 4, ty >> 4, tz >> 4);
}

void MundoProcedural::atualizar(float x, float y, float z, int max_novos) {
    const int32_t pcx = mapa_.mundo_para_tile(x) >> 4;
    const int32_t pcy = mapa_.mundo_para_tile(y) >> 4;
    const int32_t pcz = mapa_.mundo_para_tile(z) >> 4;
    const int32_t cy_fundo = (fundo_ - 1) >> 4;

    if (!iniciado_) {
        centro_cx_ = pcx;
        centro_cz_ = pcz;
        iniciado_ = true;
    }

    const int32_t velho_cx = centro_cx_, velho_cz = centro_cz_;
    const int gatilho = std::max(1, raio_xz_ - borda_);
    while (pcx - centro_cx_ >= gatilho) centro_cx_ += passo_;
    while (centro_cx_ - pcx >= gatilho) centro_cx_ -= passo_;
    while (pcz - centro_cz_ >= gatilho) centro_cz_ += passo_;
    while (centro_cz_ - pcz >= gatilho) centro_cz_ -= passo_;

    if (completo_ && centro_cx_ == velho_cx && centro_cz_ == velho_cz &&
        pcx == ult_pcx_ && pcy == ult_pcy_ && pcz == ult_pcz_) return;
    ult_pcx_ = pcx; ult_pcy_ = pcy; ult_pcz_ = pcz;

    fila_.clear();
    for (int dz = -raio_xz_; dz <= raio_xz_; dz++)
        for (int dx = -raio_xz_; dx <= raio_xz_; dx++) {
            const int32_t cx = centro_cx_ + dx, cz = centro_cz_ + dz;
            const Coluna& col = coluna(cx, cz);
            const int32_t cy_topo = (col.topo - ARV_MAX) >> 4;

            const int32_t ddx = cx - pcx, ddz = cz - pcz;
            for (int32_t cy = cy_topo; cy <= cy_fundo; cy++) {
                if (blocos_.find(chave(cx, cy, cz)) != blocos_.end()) continue;
                Pendente p;
                p.cx = cx; p.cy = cy; p.cz = cz;
                p.prioridade = (ddx * ddx + ddz * ddz) * 64 + dist(cy, pcy);
                fila_.push_back(p);
            }
        }

    std::sort(fila_.begin(), fila_.end(),
              [](const Pendente& a, const Pendente& b) { return a.prioridade < b.prioridade; });

    int gerados = 0;
    bool falta = false;
    for (const Pendente& p : fila_) {
        if (max_novos >= 0 && gerados >= max_novos) { falta = true; break; }
        if (gerar_chunk(p.cx, p.cy, p.cz)) gerados++;
        else falta = true;
    }
    completo_ = !falta;

    for (auto it = blocos_.begin(); it != blocos_.end();) {
        const Bloco& b = it->second;
        if (dist(b.cx, centro_cx_) > raio_xz_ + 1 || dist(b.cz, centro_cz_) > raio_xz_ + 1) {
            mapa_.remover_chunk(b.cx * TAM, b.cy * TAM, b.cz * TAM);
            it = blocos_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = colunas_.begin(); it != colunas_.end();) {
        const int32_t cx = (int32_t)(uint32_t)(it->first >> 32);
        const int32_t cz = (int32_t)(uint32_t)(it->first & 0xFFFFFFFFu);
        if (dist(cx, centro_cx_) > raio_xz_ + 1 || dist(cz, centro_cz_) > raio_xz_ + 1) it = colunas_.erase(it);
        else ++it;
    }
}