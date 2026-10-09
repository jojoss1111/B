#include "player.hpp"

static Janela j;

static int32_t altura_origem(const MundoProcedural& mundo) {
    int32_t topo = mundo.superficie(-1, -1);
    for (int dz = -1; dz <= 0; dz++)
        for (int dx = -1; dx <= 0; dx++) {
            const int32_t sup = mundo.superficie(dx, dz);
            if (sup < topo) topo = sup;
        }
    return topo - 3;
}

player_::player_() : mundo(mapa) {
    carregar_assets();
    criar_malhas();
    registrar_tiles();
    configurar_mundo();
    configurar_camera();
    configurar_desenho();

    mundo.atualizar(cam3d.x(), cam3d.y(), cam3d.z(), -1);   // carga inicial completa
}

void player_::carregar_assets() {
    ascii.carregar("res/assets/ascii.png", 8, 8);
    font.criar(&ascii, ASCII, 8, 8);
    spr.carregar("res/assets/tileset.png", 16, 16);
}

void player_::criar_malhas() {
    const fx16_t um = fx16::de_int(1);

    cubo_m.adicionar_cubo(um);
    bloco_m.adicionar_cubo(um);
    torra_m.adicionar_cubo(um);
    folhas_m.adicionar_cubo(um);
    terra_m.adicionar_cubo(um);
    sel_m.adicionar_cubo(um + fx16::UM / 16);   // cubo de seleção, um pouco maior

    cubo_m.carregar_textura(spr, {2, 0});
    terra_m.carregar_textura(spr, {1, 0});
    folhas_m.carregar_textura(spr, {0, 1});
    bloco_m.carregar_textura(spr, {3, 1}, {3, 1}, {0, 0}, {1, 0}, {3, 1}, {3, 1});
    torra_m.carregar_textura(spr, {4, 0}, {4, 0}, {3, 0}, {3, 0}, {4, 0}, {4, 0});
}

void player_::registrar_tiles() {
    mapa.definir_tile('#', bloco_m);   // grama
    mapa.definir_tile('d', terra_m);   // terra
    mapa.definir_tile('p', cubo_m);    // pedra
    mapa.definir_tile('t', torra_m);   // tronco
    mapa.definir_tile('f', folhas_m);  // folhas

    const char solidos[] = {'#', 'd', 'p', 't', 'f'};
    for (char c : solidos) mapa.definir_colisao(c, TipoColisor3d::aabb);
}

void player_::configurar_mundo() {
    mundo.area_livre(0, 0, 4);
    mundo.profundidade(80);
    mundo.raio(4);
}

void player_::configurar_camera() {
    static Cor branco;
    branco = {255, 255, 255, 255};

    cam3d.criar(0, altura_origem(mundo), 0, 0.0);
    cam3d.usar_mapa(mapa);

    cam3d.sem_chao();
    cam3d.balanco(false);
    cam3d.corpo(1.80, 1.80, 0.1);
    cam3d.multiplicadores(1.5, 0.5);
    //cam3d.fisica(20.0, 7.5);

    cam3d.criar_luz(150, 150, 150, &branco, 1.0f, 100.0);
    cam3d.regular_claridade(1.0f);
    cam3d.lanterna(true);
}

void player_::configurar_desenho() {
    const float tile = mapa.tamanho_tile();
    const float alcance = mundo.raio() * 16.0f * tile;

    mapa.distancia_desenho(alcance);
    mapa.distancia_desenho_y((mundo.profundidade() + 64) * tile);
    cam3d.distancia_maxima(alcance);
    cam3d.neblina({0, 100, 250, 255}, alcance * 0.6f, alcance);
}

player_::~player_(){

}

void player_::desenhar_mira(){
    frame_buffer& fb = j.tela();

    if (tem_alvo) {
        const float t = mapa.tamanho_tile();
        const float u = (float)fx16::UM;
        fb_transf tr;
        tr.x = (fx16_t)lroundf((alvo.tx + 0.5f) * t * u);
        tr.y = (fx16_t)lroundf((alvo.ty + 0.5f) * t * u);
        tr.z = (fx16_t)lroundf((alvo.tz + 0.5f) * t * u);
        cam3d.desenhar(fb, sel_m, tr, {255, 0, 0, 110});
    }

    const int cx = fb.largura() / 2, cy = fb.altura() / 2;
    const fb_cores b = {255, 255, 255, 255};
    fb_pixel::linha(fb, cx - 5, cy, cx - 2, cy, b);
    fb_pixel::linha(fb, cx + 2, cy, cx + 5, cy, b);
    fb_pixel::linha(fb, cx, cy - 5, cx, cy - 2, b);
    fb_pixel::linha(fb, cx, cy + 2, cx, cy + 5, b);
}

void player_::rodar(float delta){
    float frente = 0.0f, lados = 0.0f;
    float girar_h = 0.0f, girar_v = 0.0f;
    const float vel = 5.0, vel_cam = 4.0;
    if (j.entrada().segurando(Tecla::w)) { frente += vel; }
    if (j.entrada().segurando(Tecla::s)) { frente -= vel; }
    if (j.entrada().segurando(Tecla::d)) { lados += vel; }
    if (j.entrada().segurando(Tecla::a)) { lados -= vel; }

    if (j.entrada().segurando(Tecla::seta_direita))  { girar_h += vel_cam; }
    if (j.entrada().segurando(Tecla::seta_esquerda)) { girar_h -= vel_cam; }
    if (j.entrada().segurando(Tecla::seta_cima))     { girar_v += vel_cam; }
    if (j.entrada().segurando(Tecla::seta_baixo))    { girar_v -= vel_cam; }
    if (j.entrada().segurando(Tecla::espaco))        { cam3d.pular(); }
    if (j.entrada().segurando(Tecla::shift))         { correr = true;   }
    cam3d.correr(player_::correr);

    if (j.entrada().segurando(Tecla::backspace)) {
        float origem[3], direcao[3];
        cam3d.raio_central(origem, direcao);
        mundo.quebrar_mira(origem, direcao, 5.0f);
    }

    cam3d.mover_suave(frente, lados, 0.0f, delta);
    cam3d.girar_suave(girar_h, girar_v, delta);
    cam3d.atualizar(delta);

    if (cam3d.y() > (float)mundo.profundidade() * mapa.tamanho_tile() + 8.0f) {
        cam3d.posicionar(0.0f, (float)altura_origem(mundo), 0.0f);
    }

    float o[3], d[3];
    cam3d.raio_central(o, d);
    tem_alvo = mundo.mirar(o, d, 5.0f, alvo);
    mundo.atualizar(cam3d.x(), cam3d.y(), cam3d.z());
}

void player_::desenhar(){
    cam3d.ceu(j.tela(), nullptr, {0, 100, 250, 255});
    cam3d.desenhar(j.tela(), mapa);
    desenhar_mira();

}