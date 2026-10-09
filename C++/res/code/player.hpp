#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <birdpp.hpp>
#include "mundo_procedural.hpp"

class player_{
public:
    player_();
    ~player_();

    void rodar(float delta);
    void desenhar();

private:
    Atlas spr, ascii;
    Fonte font;
    fb_malha cubo_m;
    fb_malha bloco_m, torra_m, folhas_m, terra_m;
    Objeto3d cubo;
    Camera3d cam3d;
    Mapa3d mapa;
    MundoProcedural mundo;
    bool tem_alvo = false;
    MundoProcedural::Alvo alvo;
    void desenhar_mira();
    fb_malha sel_m;
    bool correr = false;
    void carregar_assets();
    void criar_malhas();
    void registrar_tiles();
    void configurar_mundo();
    void configurar_camera();
    void configurar_desenho();
};

#endif