#include <bird.hpp>

// Globais: a malha é liberada quando o Form3d é destruído.
Form3d f_cubo1;
Form3d f_cilindro1;
Form3d f_esfera1;
Form3d f_esfera2;
Form3d f_esfera3;

Form3d* formas_le_big_penis[] = {
    &f_cubo1, &f_cilindro1, &f_esfera1, &f_esfera2,
    &f_esfera3
};
const int N_FORMAS_LE_BIG_PENIS = 5;

Cam3d cam;

void le_big_penis() {
    // cubo1 (cubo)
    f_cubo1.cubo(0.0f, 0.0f, 0.0f, 40.0f, 0.5f, 40.0f);
    fb_cores cor_cubo1 = {89, 175, 18, 255};
    f_cubo1.usar_cor(&cor_cubo1);

    // cilindro1 (cilindro)
    f_cilindro1.cilindro(-0.25f, 3.0f, -1.5f, 1.01f, 5.0f, 20);
    fb_cores cor_cilindro1 = {88, 50, 50, 255};
    f_cilindro1.usar_cor(&cor_cilindro1);

    // esfera1 (esfera)
    f_esfera1.esfera(-0.25f, 0.5f, -0.5f, 1.11f, 32, 30);
    fb_cores cor_esfera1 = {88, 50, 50, 255};
    f_esfera1.usar_cor(&cor_esfera1);

    // esfera2 (esfera)
    f_esfera2.esfera(-0.25f, 0.5f, -2.5f, 1.11f, 32, 30);
    fb_cores cor_esfera2 = {88, 50, 50, 255};
    f_esfera2.usar_cor(&cor_esfera2);

    // esfera3 (esfera)
    f_esfera3.esfera(-0.25f, 5.5f, -1.5f, 1.11f, 8, 12);
    fb_cores cor_esfera3 = {242, 141, 223, 255};
    f_esfera3.usar_cor(&cor_esfera3);
}

void iniciar() {
    cam.criar(FPS, 0.0f, 1.7f, -9.0f, 0.0f, 0.0f, 0.0f);
    cam.ambiente(0.27f);
    cam.luz_direcional(-0.35f, 0.75f, -0.55f);
    cam.definir_chao(0.0f);
    le_big_penis();
    cam.colisao(formas_le_big_penis, N_FORMAS_LE_BIG_PENIS);
}

void rodar(float delta) {
    Entrada& in = entrada();

    if (in.pressionou(Tecla::sair)) { fechar_janela(); return; }

    if (in.pressionou(Tecla::c)) cam.alternar();

    const float vel = 7.0f;
    float lado   = (in.segurando(Tecla::d) ? 1.0f : 0.0f) - (in.segurando(Tecla::a) ? 1.0f : 0.0f);
    float frente = (in.segurando(Tecla::w) ? 1.0f : 0.0f) - (in.segurando(Tecla::s) ? 1.0f : 0.0f);
    if (lado != 0.0f && frente != 0.0f) { lado *= 0.7071f; frente *= 0.7071f; }
    cam.mover(lado * vel, 0, frente * vel);

    cam.correr(in.segurando(Tecla::shift));
    cam.agachar(in.segurando(Tecla::ctrl));
    if (in.pressionou(Tecla::espaco)) cam.pular();


    static int mx_ant = 0, my_ant = 0;
    if (in.mouse_pressionou(MouseBotao::direito)) { mx_ant = in.mouse_x(); my_ant = in.mouse_y(); }
    if (in.mouse_segurando(MouseBotao::direito)) {
        const float sens = 0.25f;
        cam.girar(-(in.mouse_y() - my_ant) * sens, (in.mouse_x() - mx_ant) * sens, 0);
        mx_ant = in.mouse_x();
        my_ant = in.mouse_y();
    }

    const float giro = 90.0f * delta;
    if (in.segurando(Tecla::seta_esquerda)) cam.girar(0, -giro, 0);
    if (in.segurando(Tecla::seta_direita))  cam.girar(0,  giro, 0);
    if (in.segurando(Tecla::seta_cima))     cam.girar( giro, 0, 0);
    if (in.segurando(Tecla::seta_baixo))    cam.girar(-giro, 0, 0);

    cam.atualizar(delta);
}

void desenhar(janela *j) {
    cam.desenhar(*j, formas_le_big_penis, N_FORMAS_LE_BIG_PENIS);
}


int main() {
    return iniciar_janela(256, 244, 60, {20, 30, 120, 255}, 2, ModoTela::letterbox, "bird");
}
