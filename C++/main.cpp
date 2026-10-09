#include <birdpp.hpp>
#include "res/code/player.hpp"

static Janela j;
static player_ p1;

static void iniciar() {
}

static void rodar(float delta) {
    p1.rodar(delta);
}

static void desenhar() {
    p1.desenhar();
}

int main() {
    const uint8_t fps = 60;
    return iniciar_motor(256, 244, fps, iniciar, rodar, desenhar,
    {0, 0, 0, 255}, "Meu jogo", 2, ModoTela::normal);
}