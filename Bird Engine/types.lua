---@meta

---@alias Cor number

---@alias Fx integer

---@alias Angulo integer

---@alias Tile string|integer

---@alias Tecla integer
---@alias BotaoMouse integer

---@type fun()|nil
iniciar = nil

---@type fun(dt: number)|nil
rodar = nil

---@type fun(tela: FrameBuffer)|nil
desenhar = nil

---@class TeclaConst
---@field seta_cima Tecla
---@field seta_baixo Tecla
---@field seta_esquerda Tecla
---@field seta_direita Tecla
---@field ctrl Tecla
---@field shift Tecla
---@field backspace Tecla
---@field tab Tecla
---@field start Tecla
---@field sair Tecla
---@field espaco Tecla
---@field f1 Tecla
---@field f2 Tecla
---@field f3 Tecla
---@field f4 Tecla
---@field f5 Tecla
---@field f6 Tecla
---@field f7 Tecla
---@field f8 Tecla
---@field f9 Tecla
---@field f10 Tecla
---@field f11 Tecla
---@field f12 Tecla
---@field a Tecla
---@field b Tecla
---@field c Tecla
---@field d Tecla
---@field e Tecla
---@field f Tecla
---@field g Tecla
---@field h Tecla
---@field i Tecla
---@field j Tecla
---@field k Tecla
---@field l Tecla
---@field m Tecla
---@field n Tecla
---@field o Tecla
---@field p Tecla
---@field q Tecla
---@field r Tecla
---@field s Tecla
---@field t Tecla
---@field u Tecla
---@field v Tecla
---@field w Tecla
---@field x Tecla
---@field y Tecla
---@field z Tecla
---@field n0 Tecla
---@field n1 Tecla
---@field n2 Tecla
---@field n3 Tecla
---@field n4 Tecla
---@field n5 Tecla
---@field n6 Tecla
---@field n7 Tecla
---@field n8 Tecla
---@field n9 Tecla
---@type TeclaConst
tecla = nil

---@class BotaoConst
---@field esquerdo BotaoMouse
---@field direito BotaoMouse
---@field meio BotaoMouse
---@type BotaoConst
botao = nil

---@class ModoAudioConst
---@field automatico integer
---@field memoria integer
---@field stream integer
---@type ModoAudioConst
modo_audio = nil

---@class ModoTelaConst
---@field normal integer
---@field letterbox integer
---@field tela_cheia integer
---@type ModoTelaConst
modo_tela = nil

---@class PadraoConst
---@field ascii integer
---@field utf8 integer
---@type PadraoConst
padrao = nil

---@class FrameBuffer
local FrameBuffer = {}

---@return boolean
function FrameBuffer:valido() end
---@return integer
function FrameBuffer:largura() end
---@return integer
function FrameBuffer:altura() end
---@return integer
function FrameBuffer:pitch() end
---@return integer
function FrameBuffer:total_pixels() end
---@return lightuserdata
function FrameBuffer:dados() end
---@param x integer
---@param y integer
---@return boolean
function FrameBuffer:dentro(x, y) end
---@param cor Cor
function FrameBuffer:limpar(cor) end
---@param x integer
---@param y integer
---@param cor Cor
function FrameBuffer:pixel(x, y, cor) end
---@param x Fx
---@param y Fx
---@param cor Cor
function FrameBuffer:pixel_fx(x, y, cor) end
---@param x integer
---@param y integer
---@param cor Cor
function FrameBuffer:misturar(x, y, cor) end
---@param x integer
---@param y integer
---@return Cor
function FrameBuffer:ler(x, y) end
---@param x1 integer
---@param y1 integer
---@param x2 integer
---@param y2 integer
---@param cor Cor
function FrameBuffer:linha(x1, y1, x2, y2, cor) end
---@param x integer
---@param y integer
---@param w integer
---@param h integer
---@param borda number|nil
---@param preenchido boolean|nil
---@param cor Cor
function FrameBuffer:retangulo(x, y, w, h, borda, preenchido, cor) end
---@param x integer
---@param y integer
---@param w integer
---@param h integer
---@param borda number|nil
---@param preenchido boolean|nil
---@param cor Cor
function FrameBuffer:triangulo_retangulo(x, y, w, h, borda, preenchido, cor) end
---@param x integer
---@param y integer
---@param raio integer
---@param borda number|nil
---@param preenchido boolean|nil
---@param cor Cor
function FrameBuffer:circulo(x, y, raio, borda, preenchido, cor) end
---@param origem FrameBuffer
---@param x integer
---@param y integer
function FrameBuffer:blit(origem, x, y) end

---@class FrameBufferRef : FrameBuffer

---@class Atlas
local Atlas = {}

---@param caminho string
---@param tile_largura integer|nil
---@param tile_altura integer|nil
---@return boolean
function Atlas:carregar(caminho, tile_largura, tile_altura) end
---@param dados string
---@param tile_largura integer|nil
---@param tile_altura integer|nil
---@return boolean
function Atlas:carregar_memoria(dados, tile_largura, tile_altura) end
---@return boolean
function Atlas:valido() end
---@return integer
function Atlas:largura() end
---@return integer
function Atlas:altura() end
---@return integer
function Atlas:tile_largura() end
---@return integer
function Atlas:tile_altura() end
---@return integer
function Atlas:colunas() end
---@return integer
function Atlas:linhas() end
---@return integer
function Atlas:passo() end
---@param coluna integer
---@param linha integer
---@return lightuserdata
function Atlas:tile_dados(coluna, linha) end
---@param tela FrameBuffer
---@param x integer
---@param y integer
---@param coluna integer
---@param linha integer
---@param a integer|nil
---@param b integer|nil
function Atlas:desenhar(tela, x, y, coluna, linha, a, b) end
---@param tela FrameBuffer
---@param x integer
---@param y integer
---@param fx1 Fx
---@param fx2 Fx
---@param a integer|nil
---@param b integer|nil
function Atlas:desenhar_fx(tela, x, y, fx1, fx2, a, b) end

---@class FbLib
fb = {}

---@param largura integer
---@param altura integer
---@return FrameBuffer|nil
function fb.novo(largura, altura) end
---@return Atlas
function fb.atlas() end
---@param r integer 0-255
---@param g integer 0-255
---@param b integer 0-255
---@param a integer|nil 0-255, padrao 255
---@return Cor
function fb.cor(r, g, b, a) end
---@param graus number
---@return Angulo
function fb.graus(graus) end
---@param angulo Angulo
---@return Fx
function fb.seno(angulo) end
---@param angulo Angulo
---@return Fx
function fb.cosseno(angulo) end
---@param n integer
---@return Fx
function fb.fx_de_int(n) end
---@param v Fx
---@return integer
function fb.fx_para_int(v) end
---@param a Fx
---@param b Fx
---@return Fx
function fb.fx_mul(a, b) end
---@param a Fx
---@param b Fx
---@return Fx
function fb.fx_div(a, b) end

---@class Audio
local Audio = {}

function Audio:liberar() end
function Audio:tocar() end
function Audio:parar() end
function Audio:voltar() end
---@param volume number|nil padrao 1.0
function Audio:disparar(volume) end
function Audio:parar_tudo() end
---@overload fun(self: Audio): boolean
---@param ligado boolean
function Audio:repetir(ligado) end
---@overload fun(self: Audio): number
---@param v number
function Audio:volume(v) end
---@return boolean
function Audio:tocando() end
---@return boolean
function Audio:valido() end
---@return boolean
function Audio:streaming() end
---@return number
function Audio:duracao() end
---@return number
function Audio:posicao() end

---@class AudioLib
audio = {}

---@param caminho string
---@param loop boolean|nil
---@param volume number|nil  padrao 1.0
---@param modo integer|nil   `modo_audio.*`, padrao automatico
---@return Audio|nil
function audio.novo(caminho, loop, volume, modo) end

---@class Entrada
local Entrada = {}

function Entrada:iniciar() end
function Entrada:atualizar() end
---@return boolean
function Entrada:pediu_fechar() end
---@return string
function Entrada:texto_digitado() end
---@return boolean
function Entrada:capslock() end
---@param t Tecla
---@return boolean
function Entrada:pressionou(t) end
---@param t Tecla
---@return boolean
function Entrada:soltou(t) end
---@param t Tecla
---@return boolean
function Entrada:segurando(t) end
---@return integer
function Entrada:ultima() end
---@param b BotaoMouse
---@return boolean
function Entrada:mouse_pressionou(b) end
---@param b BotaoMouse
---@return boolean
function Entrada:mouse_soltou(b) end
---@param b BotaoMouse
---@return boolean
function Entrada:mouse_segurando(b) end
---@return integer
function Entrada:mouse_x() end
---@return integer
function Entrada:mouse_y() end
---@return integer
function Entrada:mouse_roda() end
---@return integer x
---@return integer y
function Entrada:mouse_janela() end
---@param x integer
---@param y integer
function Entrada:definir_mouse(x, y) end
---@param id integer
---@return boolean
function Entrada:toque_pressionou(id) end
---@param id integer
---@return boolean
function Entrada:toque_soltou(id) end
---@param id integer
---@return boolean
function Entrada:toque_segurando(id) end
---@param id integer
---@return integer
function Entrada:toque_x(id) end
---@param id integer
---@return integer
function Entrada:toque_y(id) end
---@return integer
function Entrada:toques_ativos() end
---@param largura integer
---@param altura integer
function Entrada:definir_area_toque(largura, altura) end

---@class EntradaRef : Entrada

---@class EntradaLib
entrada = {}

---@return Entrada
function entrada.novo() end
---@return string
function entrada.layout_teclado() end

---@class Caixa
local Caixa = {}

---@param x integer
---@param y integer
---@param w integer
---@param h integer
function Caixa:criar_retangulo(x, y, w, h) end
---@param x integer
---@param y integer
---@param raio integer
function Caixa:criar_circulo(x, y, raio) end
function Caixa:destruir() end
---@param x integer
---@param y integer
function Caixa:mover(x, y) end
---@param w integer
---@param h integer
function Caixa:tamanho(w, h) end
---@param raio integer
function Caixa:raio(raio) end
---@return boolean
function Caixa:ativa() end
---@param x integer
---@param y integer
---@return boolean
function Caixa:dentro(x, y) end
---@return boolean
function Caixa:mouse_em_cima() end
---@param b BotaoMouse
---@param entrada Entrada|nil
---@return boolean
function Caixa:pressionou(b, entrada) end
---@param b BotaoMouse
---@param entrada Entrada|nil
---@return boolean
function Caixa:segurou(b, entrada) end
---@param b BotaoMouse
---@param entrada Entrada|nil
---@return boolean
function Caixa:soltou(b, entrada) end

---@class CaixaLib
caixa = {}

---@return Caixa
function caixa.novo() end
---@param entrada Entrada|nil
function caixa.usar_entrada(entrada) end

---@class Obj
local Obj = {}

function Obj:destruir() end
---@return integer
function Obj:x() end
---@return integer
function Obj:y() end
---@return integer
function Obj:largura() end
---@return integer
function Obj:altura() end
---@param x integer
---@param y integer
function Obj:posicionar(x, y) end
---@param dx integer
---@param dy integer
function Obj:mover(dx, dy) end
---@param coluna integer
---@param linha integer
function Obj:tile(coluna, linha) end
---@param cor Cor
function Obj:cor(cor) end
---@param ligado boolean|nil padrao true
function Obj:colorir(ligado) end
---@param alpha integer 0-255
function Obj:alpha(alpha) end
---@param w integer
---@param h integer
function Obj:ampliar(w, h) end
---@param graus number
function Obj:angulo(graus) end
---@param ligado boolean|nil padrao true
function Obj:espelhar_x(ligado) end
---@param ligado boolean|nil padrao true
function Obj:espelhar_y(ligado) end
---@param tela FrameBuffer
function Obj:desenhar(tela) end

---@class ObjLib
obj = {}

---@param atlas Atlas
---@param pw integer
---@param ph integer
---@param x integer|nil
---@param y integer|nil
---@param coluna integer|nil
---@param linha integer|nil
---@return Obj
function obj.novo(atlas, pw, ph, x, y, coluna, linha) end

---@class Animacao
local Animacao = {}

function Animacao:destruir() end
function Animacao:rodar() end
function Animacao:parar() end
function Animacao:voltar() end
---@param tempo integer
function Animacao:atualizar(tempo) end

---@class AnimacaoLib
animacao = {}

---@param obj Obj
---@param quadros integer[][]
---@param fps integer
---@param loop boolean|nil
---@return Animacao
function animacao.novo(obj, quadros, fps, loop) end

---@class AABB
local AABB = {}

---@param obj Obj
function AABB:sincronizar(obj) end
---@param x integer
---@param y integer
function AABB:posicionar(x, y) end
---@return integer
function AABB:x() end
---@return integer
function AABB:y() end
---@return integer
function AABB:largura() end
---@return integer
function AABB:altura() end
---@param outro AABB
---@return boolean
function AABB:colidio(outro) end
---@param outro AABB
---@param dx integer
---@return boolean
function AABB:colidiu_x(outro, dx) end
---@param outro AABB
---@param dy integer
---@return boolean
function AABB:colidiu_y(outro, dy) end

---@class AABBLib
aabb = {}

---@param obj Obj|nil
---@param mascara string|nil
---@param mw integer|nil
---@param mh integer|nil
---@return AABB
function aabb.novo(obj, mascara, mw, mh) end

---@class Mapa
local Mapa = {}

---@param camadas table mapas ASCII multilinha: abaixo, mundo e frente (ou índices 1, 2 e 3)
---@param tile_largura integer largura da célula em pixels
---@param tile_altura integer altura da célula em pixels
---@return boolean
function Mapa:criar_mapa(camadas, tile_largura, tile_altura) end
function Mapa:destruir() end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param x integer
---@param y integer
function Mapa:posicionar(camada, x, y) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param dx integer
---@param dy integer
function Mapa:mover(camada, dx, dy) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@return integer
function Mapa:x(camada) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@return integer
function Mapa:y(camada) end
---@return integer
function Mapa:colunas() end
---@return integer
function Mapa:linhas() end
---@return integer
function Mapa:tile_largura() end
---@return integer
function Mapa:tile_altura() end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param coluna integer
---@param linha integer
---@return Tile
function Mapa:tile(camada, coluna, linha) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param coluna integer
---@param linha integer
---@param tile Tile
function Mapa:definir(camada, coluna, linha, tile) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param antigo Tile
---@param novo Tile
---@param coluna integer|nil se omitida, substitui todas as ocorrências
---@param linha integer|nil
function Mapa:substituir(camada, antigo, novo, coluna, linha) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param tile Tile
---@param atlas Atlas
---@param coluna integer
---@param linha integer
function Mapa:sprites(camada, tile, atlas, coluna, linha) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param tile Tile
---@param largura integer
---@param altura integer
function Mapa:ampliar(camada, tile, largura, altura) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param tile Tile
---@param cor Cor
function Mapa:cor(camada, tile, cor) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param tile Tile
---@param graus number
function Mapa:angulo(camada, tile, graus) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param tile Tile
---@param alpha integer
function Mapa:alpha(camada, tile, alpha) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param tile Tile
---@param mascara string|nil
---@param mw integer|nil
---@param mh integer|nil
function Mapa:colisor(camada, tile, mascara, mw, mh) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param tile Tile
---@param ligado boolean|nil
function Mapa:espelhar_x(camada, tile, ligado) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param tile Tile
---@param ligado boolean|nil
function Mapa:espelhar_y(camada, tile, ligado) end
---@param tile Tile
---@param atlas Atlas
---@param quadros table lista de pares {coluna, linha}
---@param fps integer
---@param repetir boolean|nil tocar em loop (padrão true)
function Mapa:animacao(tile, atlas, quadros, fps, repetir) end
---@param delta_ms integer|nil delta desde a atualização anterior (padrão 17)
function Mapa:atualizar_animacao(delta_ms) end
function Mapa:parar() end
function Mapa:rodar() end
function Mapa:voltar() end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param aabb AABB
---@param dx integer
---@return boolean
function Mapa:colidio_x(camada, aabb, dx) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param aabb AABB
---@param dy integer
---@return boolean
function Mapa:colidio_y(camada, aabb, dy) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param tile Tile
---@param aabb AABB
---@param dx integer
---@return boolean
function Mapa:colidio_tile_x(camada, tile, aabb, dx) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param tile Tile
---@param aabb AABB
---@param dy integer
---@return boolean
function Mapa:colidio_tile_y(camada, tile, aabb, dy) end
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
---@param tela FrameBuffer
function Mapa:desenhar(camada, tela) end

---@class MapaLib
mapa = {}
---@return Mapa
function mapa.novo() end

---@class Camera
local Camera = {}

function Camera:destruir() end
---@return boolean
function Camera:valida() end
---@return integer
function Camera:x() end
---@return integer
function Camera:y() end
---@return integer
function Camera:largura() end
---@return integer
function Camera:altura() end
---@return integer
function Camera:largura_mundo() end
---@return integer
function Camera:altura_mundo() end
---@overload fun(self: Camera): number
---@param graus number
function Camera:angulo(graus) end
---@param delta number
function Camera:girar(delta) end
---@param zoom number|nil quando omitido, apenas consulta o valor atual
---@return number zoom atual aplicado
function Camera:zoom(zoom) end
---@param zoom number
function Camera:zoom_definir(zoom) end
---@param fator number
function Camera:zoom_relativo(fator) end
---@param zoom number
---@param x integer
---@param y integer coordenadas locais do viewport
function Camera:zoom_em(zoom, x, y) end
---@param zoom number
---@param suavidade integer|nil entre 0 e 255; padrão 180
function Camera:zoom_suave(zoom, suavidade) end
---@param minimo number
---@param maximo number
function Camera:limites_zoom(minimo, maximo) end
---@param x integer
---@param y integer
function Camera:posicionar(x, y) end
---@param dx integer
---@param dy integer
function Camera:mover(dx, dy) end
---@param x integer
---@param y integer
function Camera:centralizar(x, y) end
---@param x integer
---@param y integer
---@param suavidade integer
---@param dt number|nil
function Camera:mover_suave(x, y, suavidade, dt) end
---@param mapa Mapa
---@param tile_w integer|nil
---@param tile_h integer|nil
function Camera:centralizar_mundo(mapa, tile_w, tile_h) end
---@param alvo Obj
---@param ox integer|nil
---@param oy integer|nil
---@param suavidade integer|nil
function Camera:ancorar(alvo, ox, oy, suavidade) end
function Camera:soltar() end
---@param dt number|nil
function Camera:atualizar(dt) end
---@param mapa Mapa
---@param tile_w integer|nil
---@param tile_h integer|nil
function Camera:travar_mapa(mapa, tile_w, tile_h) end
---@param x1 integer
---@param y1 integer
---@param x2 integer
---@param y2 integer
function Camera:travar_limites(x1, y1, x2, y2) end
function Camera:destravar() end
---@param x integer
---@param y integer
---@return integer tx
---@return integer ty
function Camera:mundo_para_tela(x, y) end
---@param x integer
---@param y integer
---@return integer mx
---@return integer my
function Camera:tela_para_mundo(x, y) end
--- Inicia o quadro, atualiza a transformação e limpa o viewport antes dos desenhos.
---@param cor Cor|nil
function Camera:iniciar(cor) end
---@param x integer
---@param y integer
---@param w integer
---@param h integer
---@return boolean
function Camera:visivel(x, y, w, h) end
---@param item Obj
function Camera:desenhar_obj(item) end
---@param mapa Mapa
---@param camada 1|2|3|'abaixo'|'mundo'|'frente'
function Camera:desenhar_mapa(mapa, camada) end
---@param tela FrameBuffer
---@param x integer|nil
---@param y integer|nil
function Camera:apresentar(tela, x, y) end
---@return lightuserdata
function Camera:viewport_dados() end
---@return integer
function Camera:viewport_largura() end
---@return integer
function Camera:viewport_altura() end

---@class CameraLib
camera = {}

---@param w integer
---@param h integer
---@param x integer|nil
---@param y integer|nil
---@return Camera|nil
function camera.criar(w, h, x, y) end
---@deprecated use camera.criar
---@param w integer
---@param h integer
---@param x integer|nil
---@param y integer|nil
---@return Camera|nil
function camera.novo(w, h, x, y) end

---@class Fonte
local Fonte = {}

---@param atlas Atlas
---@param padrao integer|nil
---@param a integer|nil
---@param b integer|nil
function Fonte:criar(atlas, padrao, a, b) end
---@param atlas Atlas
---@param pw integer
---@param ph integer
---@param letras string|(string|integer)[][]
function Fonte:criar_proprio(atlas, pw, ph, letras) end
---@param tela FrameBuffer
---@param frase string
---@param x integer
---@param y integer
---@param cor Cor|nil
---@param escala integer|nil
function Fonte:escrever(tela, frase, x, y, cor, escala) end
---@param tela FrameBuffer
---@param frase string
---@param x integer
---@param y integer
---@param cor Cor|nil
---@param escala integer|nil
---@param intervalo number|nil
function Fonte:escrever_lento(tela, frase, x, y, cor, escala, intervalo) end
---@param frase string
---@param escala integer|nil
---@return integer
function Fonte:largura_texto(frase, escala) end
---@param frase string
---@param escala integer|nil
---@return integer
function Fonte:altura_texto(frase, escala) end

---@class FonteLib
fonte = {}
---@return Fonte
function fonte.novo() end

---@class Digitar
local Digitar = {}

---@param entrada Entrada
---@param max_digitos integer|nil
---@return integer
function Digitar:inteiro(entrada, max_digitos) end
---@param entrada Entrada
---@param max_digitos integer|nil
---@return number
function Digitar:decimal(entrada, max_digitos) end
---@param entrada Entrada
---@param max_letras integer|nil
---@return string
function Digitar:texto(entrada, max_letras) end
---@return string
function Digitar:conteudo() end
---@param tela FrameBuffer
---@param fonte Fonte
---@param x integer
---@param y integer
---@param cor Cor|nil
---@param escala integer|nil
---@param cursor boolean|nil
function Digitar:desenhar(tela, fonte, x, y, cor, escala, cursor) end
function Digitar:limpar() end
function Digitar:deletar() end
function Digitar:travar() end
function Digitar:destravar() end
---@return boolean
function Digitar:finalizado() end
---@return boolean
function Digitar:travado() end

---@class DigitarLib
digitar = {}
---@return Digitar
function digitar.novo() end

---@class JanelaLib
janela = {}

---@param largura integer
---@param altura integer
---@param fps integer|nil
---@param cor Cor|nil
---@param ampliar integer|nil
---@param modo integer|nil
---@param nome string|nil
---@param icone Atlas|nil
---@return integer
function janela.iniciar(largura, altura, fps, cor, ampliar, modo, nome, icone) end
function janela.fechar() end
function janela.mudar_modo() end
---@param modo integer
function janela.definir_modo(modo) end
---@return integer
function janela.modo_atual() end
---@param escala integer
function janela.ampliar(escala) end
---@return integer
function janela.ampliar_atual() end
---@param ativa boolean|nil
function janela.tela_cheia(ativa) end
---@param titulo string
function janela.titulo(titulo) end
---@return EntradaRef
function janela.entrada() end
---@return integer
function janela.largura() end
---@return integer
function janela.altura() end
---@return number
function janela.fps_atual() end

---@class Tempo
local Tempo = {}

---@return boolean
function Tempo:passou() end
---@return boolean
function Tempo:nao_passou() end
function Tempo:parar() end
function Tempo:retomar() end
function Tempo:reiniciar() end
---@param segundos number
function Tempo:definir(segundos) end
---@param segundos number
function Tempo:adicionar(segundos) end
---@return number
function Tempo:restante() end
---@return number
function Tempo:decorrido() end
---@return number
function Tempo:progresso() end
---@return boolean
function Tempo:parado() end

---@class TempoLib
tempo = {}

---@param segundos number
---@return Tempo
function tempo.criar(segundos) end
---@param segundos number
---@return Tempo
function tempo.criar_repetido(segundos) end
---@return number
function tempo.agora() end
function tempo.parar() end
function tempo.retomar() end
---@return boolean
function tempo.parado() end
---@param escala number
function tempo.escala(escala) end
---@return number
function tempo.escala_atual() end

---@class ParticulasConfig
---@field n number|number[]|nil quantidade emitida (número ou {mínimo, máximo})
---@field vida number|number[]|nil vida em segundos
---@field vel number|number[]|nil velocidade inicial em pixels por segundo
---@field angulo number|nil direção central em graus (0 direita, 90 cima)
---@field abertura number|nil meio ângulo de dispersão em graus
---@field dispersao number|nil raio da área de nascimento
---@field grav number|nil gravidade (positivo faz cair)
---@field vento number|nil aceleração horizontal
---@field arrasto number|nil perda de velocidade por segundo
---@field chao number|nil coordenada Y do chão
---@field quique number|nil energia mantida no quique (padrão 0.5)
---@field atrito number|nil velocidade horizontal mantida ao tocar o chão (padrão 0.8)
---@field tam_ini number|number[]|nil raio inicial em pixels
---@field tam_fim number|number[]|nil raio final em pixels
---@field paleta integer[]|nil índices de 0 a 15 da paleta PICO-8
---@field cores Cor[]|nil cores ARGB, têm prioridade sobre paleta
---@field forma 'circulo'|'pixel'|'quadrado'|'linha'|nil forma de desenho
---@field rastro number|nil comprimento da linha em segundos de velocidade
---@field desvanecer number|nil fração final da vida em que desaparece (0 a 1)

---@class Particulas
local Particulas = {}

---@param spec string|ParticulasConfig preset ou configuração
---@param x number
---@param y number
---@param sobrepor ParticulasConfig|nil valores que sobrescrevem a configuração
---@return integer quantidade efetivamente emitida
function Particulas:emitir(spec, x, y, sobrepor) end
---@param delta number segundos desde o último quadro
function Particulas:atualizar(delta) end
---@param tela FrameBuffer
---@param camera Camera|nil câmera 2D usada para converter mundo para tela
---@param ox integer|nil deslocamento horizontal adicional
---@param oy integer|nil deslocamento vertical adicional
function Particulas:desenhar(tela, camera, ox, oy) end
function Particulas:limpar() end
---@return integer
function Particulas:quantidade() end

---@class ParticulasLib
particulas = {}
---@param limite integer|nil limite de partículas vivas (padrão 512)
---@return Particulas|nil
function particulas.novo(limite) end

---@class ZBuffer
local ZBuffer = {}

function ZBuffer:destruir() end
function ZBuffer:limpar() end
---@return integer
function ZBuffer:largura() end
---@return integer
function ZBuffer:altura() end
---@param x integer
---@param y integer
---@return number
function ZBuffer:ler(x, y) end

---@class ZBufferLib
zbuffer = {}

---@param largura integer
---@param altura integer
---@return ZBuffer|nil
function zbuffer.criar(largura, altura) end

---@class Obj3D
local Obj3D = {}

function Obj3D:destruir() end
---@return integer forma `obj3d.CUBO`, `obj3d.ESFERA`, `obj3d.BILLBOARD`...
function Obj3D:forma() end
---@return number x
---@return number y
---@return number z
function Obj3D:posicao() end
---@return number rx
---@return number ry
---@return number rz
function Obj3D:rotacao() end
---@return number sx
---@return number sy
---@return number sz
function Obj3D:escala() end

---@param x number
---@param y number
---@param z number
function Obj3D:posicionar(x, y, z) end
---@param dx number
---@param dy number
---@param dz number
function Obj3D:mover(dx, dy, dz) end
---@param rx number graus
---@param ry number graus
---@param rz number graus
function Obj3D:rotacionar(rx, ry, rz) end
---@param rx number graus
---@param ry number graus
---@param rz number graus
function Obj3D:girar(rx, ry, rz) end
---@param graus number
function Obj3D:rotacionar_x(graus) end
---@param graus number
function Obj3D:rotacionar_y(graus) end
---@param graus number
function Obj3D:rotacionar_z(graus) end
--- Com um argumento, escala uniforme.
---@param sx number
---@param sy number|nil
---@param sz number|nil
function Obj3D:escalar(sx, sy, sz) end
---@param s number
function Obj3D:escalar_uniforme(s) end
--- Multiplica a escala atual.
---@param sx number
---@param sy number
---@param sz number
function Obj3D:ampliar(sx, sy, sz) end

---@param atlas Atlas|nil
function Obj3D:atlas(atlas) end
---@param coluna integer
---@param linha integer
function Obj3D:tile(coluna, linha) end
---@param coluna integer
---@param linha integer
function Obj3D:tile_cima(coluna, linha) end
---@param coluna integer
---@param linha integer
function Obj3D:tile_lado(coluna, linha) end
---@param coluna integer
---@param linha integer
function Obj3D:tile_baixo(coluna, linha) end
--- Define o tile da face frontal do cubo (lado -Z).
---@param coluna integer
---@param linha integer
function Obj3D:tile_frente(coluna, linha) end
--- Define o tile da face traseira do cubo (lado +Z).
---@param coluna integer
---@param linha integer
function Obj3D:tile_tras(coluna, linha) end
--- Define o tile da face esquerda do cubo (lado -X).
---@param coluna integer
---@param linha integer
function Obj3D:tile_esquerda(coluna, linha) end
--- Define o tile da face direita do cubo (lado +X).
---@param coluna integer
---@param linha integer
function Obj3D:tile_direita(coluna, linha) end
---@param cor Cor
function Obj3D:cor(cor) end
---@param modo integer `obj3d.MODO_*`
function Obj3D:modo(modo) end
---@param cull integer `obj3d.CULL_*`
function Obj3D:cull(cull) end
---@param ligado boolean|nil padrao true
function Obj3D:visivel(ligado) end

---@param rep_largura number repetições por unidade do mundo na horizontal
---@param rep_altura number repetições por unidade do mundo na vertical
---@param repetir boolean|nil padrao true; false estica a imagem
function Obj3D:repetir(rep_largura, rep_altura, repetir) end
---@param rep_largura number
---@param rep_altura number
---@param repetir boolean|nil padrao true; false estica a imagem
function Obj3D:repetir_cima(rep_largura, rep_altura, repetir) end
---@param rep_largura number
---@param rep_altura number
---@param repetir boolean|nil padrao true; false estica a imagem
function Obj3D:repetir_lado(rep_largura, rep_altura, repetir) end
---@param rep_largura number
---@param rep_altura number
---@param repetir boolean|nil padrao true; false estica a imagem
function Obj3D:repetir_baixo(rep_largura, rep_altura, repetir) end
---@param i integer a partir de 1
---@param rep_largura number
---@param rep_altura number
---@param repetir boolean|nil padrao true; false estica a imagem
function Obj3D:parte_repetir(i, rep_largura, rep_altura, repetir) end


--- Faz o objeto sempre encarar a câmera.
--- `true` equivale a `obj3d.BB_CILINDRICO` e `false` a `obj3d.BB_NENHUM`.
--- A rotação em z vira o giro da imagem sobre o próprio plano.
---@param tipo boolean|integer `obj3d.BB_NENHUM`, `obj3d.BB_ESFERICO` ou `obj3d.BB_CILINDRICO`
function Obj3D:billboard(tipo) end
---@return integer tipo `obj3d.BB_*`
function Obj3D:billboard_tipo() end

---@param tipo boolean|integer true/false ou `obj3d.COL_*`
function Obj3D:colisao(tipo) end
--- Raio do colisor cilíndrico do billboard. 0 usa metade da largura.
---@param raio number
function Obj3D:colisao_raio(raio) end
--- Caixa de colisão no mundo: centro e meias-medidas.
---@return number cx
---@return number cy
---@return number cz
---@return number ex
---@return number ey
---@return number ez
function Obj3D:caixa() end
function Obj3D:recalcular_limites() end
function Obj3D:normalizar() end


---@return integer
function Obj3D:partes() end
---@param i integer a partir de 1
---@return string
function Obj3D:parte_nome(i) end
---@param nome string
---@return integer|nil indice a partir de 1
function Obj3D:parte_buscar(nome) end
---@param i integer a partir de 1
---@param ligado boolean|nil padrao true
function Obj3D:parte_visivel(i, ligado) end
---@param i integer a partir de 1
---@param coluna integer
---@param linha integer
function Obj3D:parte_tile(i, coluna, linha) end


---@return integer
function Obj3D:poligono_n() end
---@param i integer a partir de 1
---@return number|nil x
---@return number|nil z
function Obj3D:poligono_ler(i) end
--- Aceita `{x1, z1, x2, z2, ...}` ou `{{x1, z1}, {x2, z2}, ...}`.
---@param pontos number[]|number[][]
---@return boolean
function Obj3D:poligono_pontos(pontos) end
---@param i integer a partir de 1
---@param x number
---@param z number
---@return boolean
function Obj3D:poligono_ponto(i, x, z) end
---@param graus number
---@param cx number|nil padrao 0
---@param cz number|nil padrao 0
---@return boolean
function Obj3D:poligono_girar_pontos(graus, cx, cz) end
---@param px number
---@param pz number
---@return boolean
function Obj3D:poligono_pivo(px, pz) end
---@param altura number
function Obj3D:poligono_altura(altura) end

---@param tela FrameBuffer
---@param zb ZBuffer
---@param cam Camera3D
function Obj3D:desenhar(tela, zb, cam) end

---@class Obj3DLib
---@field CUBO integer
---@field ESFERA integer
---@field CONE integer
---@field CILINDRO integer
---@field MODELO integer
---@field POLIGONO integer
---@field BILLBOARD integer
---@field BB_NENHUM integer
---@field BB_ESFERICO integer
---@field BB_CILINDRICO integer
---@field COL_NENHUMA integer
---@field COL_CAIXA integer
---@field MODO_PLANO integer
---@field MODO_GOURAUD integer
---@field MODO_TEXTURA integer
---@field CULL_NENHUM integer
---@field CULL_TRAS integer
---@field CULL_FRENTE integer
obj3d = {}

---@param atlas Atlas|nil
---@param x number
---@param y number
---@param z number
---@param largura number
---@param altura number
---@param profundidade number
---@param rep_largura number|nil repeticoes por unidade do mundo na horizontal (padrao 1)
---@param rep_altura number|nil repeticoes por unidade do mundo na vertical (padrao 1)
---@param repetir boolean|nil padrao true; false estica a imagem
---@return Obj3D|nil
function obj3d.cubo(atlas, x, y, z, largura, altura, profundidade, rep_largura, rep_altura, repetir) end
---@param atlas Atlas|nil
---@param x number
---@param y number
---@param z number
---@param raio number
---@param fatias integer|nil padrao 16
---@param aneis integer|nil padrao 12
---@param rep_largura number|nil padrao 1
---@param rep_altura number|nil padrao 1
---@param repetir boolean|nil padrao true; false estica a imagem
---@return Obj3D|nil
function obj3d.esfera(atlas, x, y, z, raio, fatias, aneis, rep_largura, rep_altura, repetir) end
---@param atlas Atlas|nil
---@param x number
---@param y number
---@param z number
---@param raio number
---@param altura number
---@param fatias integer|nil padrao 16
---@param rep_largura number|nil padrao 1
---@param rep_altura number|nil padrao 1
---@param repetir boolean|nil padrao true; false estica a imagem
---@return Obj3D|nil
function obj3d.cone(atlas, x, y, z, raio, altura, fatias, rep_largura, rep_altura, repetir) end
---@param atlas Atlas|nil
---@param x number
---@param y number
---@param z number
---@param raio number
---@param altura number
---@param fatias integer|nil padrao 16
---@param rep_largura number|nil padrao 1
---@param rep_altura number|nil padrao 1
---@param repetir boolean|nil padrao true; false estica a imagem
---@return Obj3D|nil
function obj3d.cilindro(atlas, x, y, z, raio, altura, fatias, rep_largura, rep_altura, repetir) end
--- Prisma extrudado a partir de pontos no plano xz.
--- Aceita `{x1, z1, x2, z2, ...}` ou `{{x1, z1}, {x2, z2}, ...}`.
---@param atlas Atlas|nil
---@param x number
---@param y number
---@param z number
---@param pontos number[]|number[][] pelo menos 3 pontos
---@param altura number|nil padrao 1
---@param rep_largura number|nil padrao 1
---@param rep_altura number|nil padrao 1
---@param repetir boolean|nil padrao true; false estica a imagem
---@return Obj3D|nil
function obj3d.poligono(atlas, x, y, z, pontos, altura, rep_largura, rep_altura, repetir) end
--- Quad que encara a câmera (padrão `obj3d.BB_CILINDRICO`, sem colisão).
--- Use `repetir = false` para esticar a imagem inteira no sprite.
---@param atlas Atlas|nil
---@param x number
---@param y number
---@param z number
---@param largura number
---@param altura number
---@param rep_largura number|nil padrao 1
---@param rep_altura number|nil padrao 1
---@param repetir boolean|nil padrao true; false estica a imagem
---@return Obj3D|nil
function obj3d.billboard(atlas, x, y, z, largura, altura, rep_largura, rep_altura, repetir) end

---@param atlas Atlas|nil
---@param caminho string .obj ou .fbx binario
---@param x number
---@param y number
---@param z number
---@param escala number|nil padrao 1
---@return Obj3D|nil
function obj3d.carregar(atlas, caminho, x, y, z, escala) end
---@param atlas Atlas|nil
---@param dados string
---@param x number
---@param y number
---@param z number
---@param escala number|nil padrao 1
---@return Obj3D|nil
function obj3d.carregar_memoria(atlas, dados, x, y, z, escala) end

---@class Camera3D
local Camera3D = {}

function Camera3D:destruir() end

---@param fov number graus
---@param perto number
---@param longe number
function Camera3D:lente(fov, perto, longe) end
---@param graus number
function Camera3D:fov(graus) end
---@param aspecto number
function Camera3D:aspecto(aspecto) end

---@param ativo boolean|nil padrao true
function Camera3D:chao(ativo) end
---@param y number
function Camera3D:definir_chao(y) end
---@param ativo boolean|nil padrao true
function Camera3D:ceu(ativo) end
---@param y number
function Camera3D:maximo_ceu(y) end

---@param altura_olhos number
---@param olhos_agachado number
---@param altura_colisao number
---@param raio number
---@param tipo integer|nil `cam3d.COL_AABB` (padrao) ou `cam3d.COL_OBB`
function Camera3D:definir_corpo(altura_olhos, olhos_agachado, altura_colisao, raio, tipo) end
---@param gravidade number
---@param forca_pulo number
function Camera3D:fisica(gravidade, forca_pulo) end
---@param altura_maxima number
function Camera3D:degrau(altura_maxima) end
---@param correr number
---@param agachar number
function Camera3D:multiplicadores(correr, agachar) end
---@param ligado boolean|nil padrao true
---@param amplitude number|nil padrao 0.05
---@param frequencia number|nil padrao 10
function Camera3D:balanco(ligado, amplitude, frequencia) end
---@param velocidade number
function Camera3D:recuperacao(velocidade) end

---@param x number
---@param y number
---@param z number
function Camera3D:mover(x, y, z) end
---@param x number
---@param y number
---@param z number posicao dos olhos
function Camera3D:posicionar(x, y, z) end
---@param pitch number
---@param yaw number
---@param roll number
function Camera3D:girar(pitch, yaw, roll) end
---@param pitch number
---@param yaw number
---@param roll number
function Camera3D:angulo(pitch, yaw, roll) end
---@param x number
---@param y number
---@param z number
function Camera3D:olhar_para(x, y, z) end
function Camera3D:pular() end
---@param ligado boolean|nil padrao true
function Camera3D:agachar(ligado) end
---@param ligado boolean|nil padrao true
function Camera3D:correr(ligado) end
---@param intensidade number
---@param duracao number segundos
function Camera3D:tremer(intensidade, duracao) end
---@param pitch number
---@param yaw number
function Camera3D:recuo(pitch, yaw) end

---@param objetos Obj3D[]|nil
function Camera3D:mundo(objetos) end
---@param dt number segundos
function Camera3D:atualizar(dt) end

---@return number
function Camera3D:x() end
---@return number
function Camera3D:y() end
---@return number
function Camera3D:z() end
---@return number
function Camera3D:pes_y() end
---@return number x
---@return number y
---@return number z
function Camera3D:posicao() end
---@return number pitch
---@return number yaw
---@return number roll
function Camera3D:angulos() end
---@return number
function Camera3D:altura_corpo() end
---@return number
function Camera3D:velocidade() end
---@return boolean
function Camera3D:no_chao() end
---@return boolean
function Camera3D:agachado() end
---@return number x
---@return number y
---@return number z
function Camera3D:frente() end
---@return number ex
---@return number ey
---@return number ez
---@return number yaw
---@return number pitch
---@return number roll
function Camera3D:vista() end
---@param valor number
function Camera3D:ambiente(valor) end
---@param ligado boolean|nil padrao true
function Camera3D:sombras(ligado) end
---@param ligada boolean|nil padrao true
---@param desloc_y number|nil padrao 0
function Camera3D:aura(ligada, desloc_y) end
---@param ligada boolean|nil padrao true
---@param desloc_y number|nil padrao 0
function Camera3D:lanterna(ligada, desloc_y) end
---@param interno number
---@param externo number
function Camera3D:cone_lanterna(interno, externo) end

---@param x number
---@param y number
---@param z number
---@param cor Cor|nil padrao branco (só RGB, alfa ignorado)
---@param intensidade number|nil padrao 1
---@param alcance number|nil padrao 10
---@return integer|nil id nil se não houver vaga
function Camera3D:luz_criar(x, y, z, cor, intensidade, alcance) end
---@param id integer
function Camera3D:luz_remover(id) end
---@param id integer
---@param x number
---@param y number
---@param z number
function Camera3D:luz_mover(id, x, y, z) end
---@param id integer
---@param ativa boolean|nil padrao true
function Camera3D:luz_ativa(id, ativa) end
---@param id integer
---@param cor Cor
function Camera3D:luz_cor(id, cor) end
---@param id integer
---@param valor number
function Camera3D:luz_intensidade(id, valor) end
---@param id integer
---@param valor number
function Camera3D:luz_alcance(id, valor) end
---@param id integer
---@param ligado boolean|nil padrao true
function Camera3D:luz_sombra(id, ligado) end
---@param id integer
function Camera3D:luz_ponto(id) end
---@param id integer
---@param dx number
---@param dy number
---@param dz number
---@param interno number|nil padrao 20 (graus)
---@param externo number|nil padrao 35 (graus)
function Camera3D:luz_spot(id, dx, dy, dz, interno, externo) end

---@param tela FrameBuffer
---@param zb ZBuffer
---@param objetos Obj3D[]|nil
function Camera3D:iluminar(tela, zb, objetos) end

---@param ativo boolean|nil padrao true
---@param inicio number|nil distancia onde a neblina comeca (padrao 0)
---@param fim number|nil distancia onde a neblina chega no maximo
---@param no_ceu boolean|nil pinta o fundo com a cor da neblina (padrao true)
---@param cor Cor|nil padrao preto (alfa ignorado)
---@param intensidade number|nil 0 a 1, quanto a neblina cobre no maximo (padrao 0.6)
function Camera3D:neblina(ativo, inicio, fim, no_ceu, cor, intensidade) end

---@param alcance number
---@param objetos Obj3D[]|nil
---@return integer|nil indice a partir de 1
---@return number|nil distancia
function Camera3D:raio(alcance, objetos) end
---@param tela FrameBuffer
---@param zb ZBuffer
---@param objetos Obj3D[]|nil
function Camera3D:desenhar(tela, zb, objetos) end

---@class Cam3DLib
---@field COL_AABB integer
---@field COL_OBB integer
---@field LUZ_AURA integer id fixo da aura
---@field LUZ_LANTERNA integer id fixo da lanterna
---@field LUZ_PONTO integer
---@field LUZ_SPOT integer
cam3d = {}

---@param x number
---@param y number
---@param z number
---@param angulo_x number|nil padrao 0
---@param angulo_y number|nil padrao 0
---@param angulo_z number|nil padrao 0
---@return Camera3D|nil
function cam3d.criar(x, y, z, angulo_x, angulo_y, angulo_z) end


---@alias IdAtuador integer id devolvido por `Mapa3D:atuador`, `porta_*`, `elevador` ou `andador` (comeca em 0)
---@alias IdInterativo integer id devolvido por `Mapa3D:interativo` ou `interativo_em` (comeca em 0)

--- Referencia a um objeto que pertence a um `Mapa3D`.
--- Tem todos os metodos de `Obj3D`, mas nao e destruida pelo coletor de lixo:
--- quem destroi o objeto e o mapa (`Mapa3D:remover`, `limpar` ou `destruir`).
--- `destruir()` nao funciona em referencias.
--- Depois de remover o objeto ou limpar/destruir o mapa, nao guarde a referencia.
---@class Obj3DRef : Obj3D

---@class Mapa3D
local Mapa3D = {}

function Mapa3D:destruir() end
--- Destroi todos os objetos, atuadores e interativos do mapa.
function Mapa3D:limpar() end
--- Atlas usado pelos objetos criados pelo mapa daqui para frente. `nil` remove.
---@param atlas Atlas|nil
function Mapa3D:atlas(atlas) end

--- Define um nome para um tile do atlas (maximo de 32 nomes).
---@param nome string
---@param coluna integer
---@param linha integer
---@return boolean
function Mapa3D:tile(nome, coluna, linha) end
---@param nome string
---@return integer|nil coluna
---@return integer|nil linha
function Mapa3D:tile_buscar(nome) end

--- O mapa passa a ser dono do objeto. A variavel original fica inutilizavel; use a referencia devolvida.
---@param obj Obj3D
---@return Obj3DRef|nil
function Mapa3D:adicionar(obj) end
--- Destroi o objeto e invalida a referencia passada.
---@param obj Obj3DRef|Obj3D
---@return boolean
function Mapa3D:remover(obj) end
---@return Obj3DRef[]
function Mapa3D:objetos() end
---@return integer
function Mapa3D:total() end

--- O tile pode ser um nome definido com `Mapa3D:tile` ou o par `coluna, linha`.
--- Sem tile, o objeto mantem o padrao.
---@param x number
---@param y number
---@param z number
---@param largura number
---@param altura number
---@param profundidade number
---@param tile string|integer|nil
---@param linha integer|nil usado quando `tile` e a coluna
---@return Obj3DRef|nil
function Mapa3D:cubo(x, y, z, largura, altura, profundidade, tile, linha) end
---@param x number
---@param y number
---@param z number
---@param raio number
---@param altura number
---@param fatias integer|nil padrao 16
---@param tile string|integer|nil
---@param linha integer|nil
---@return Obj3DRef|nil
function Mapa3D:cilindro(x, y, z, raio, altura, fatias, tile, linha) end
---@param x number
---@param y number
---@param z number
---@param raio number
---@param altura number
---@param fatias integer|nil padrao 16
---@param tile string|integer|nil
---@param linha integer|nil
---@return Obj3DRef|nil
function Mapa3D:cone(x, y, z, raio, altura, fatias, tile, linha) end
---@param x number
---@param y number
---@param z number
---@param raio number
---@param fatias integer|nil padrao 16
---@param aneis integer|nil padrao 12
---@param tile string|integer|nil
---@param linha integer|nil
---@return Obj3DRef|nil
function Mapa3D:esfera(x, y, z, raio, fatias, aneis, tile, linha) end
--- Aceita `{x1, z1, x2, z2, ...}` ou `{{x1, z1}, {x2, z2}, ...}`.
---@param x number
---@param y number
---@param z number
---@param pontos number[]|number[][] pelo menos 3 pontos
---@param altura number
---@param tile string|integer|nil
---@param linha integer|nil
---@return Obj3DRef|nil
function Mapa3D:poligono(x, y, z, pontos, altura, tile, linha) end
---@param x number
---@param y number
---@param z number
---@param largura number
---@param altura number
---@param tile string|integer|nil
---@param linha integer|nil
---@return Obj3DRef|nil
function Mapa3D:billboard(x, y, z, largura, altura, tile, linha) end

--- Chao quadrado centrado na origem, dividido em `divisao` x `divisao` blocos.
---@param tamanho number
---@param divisao integer|nil padrao 1
---@param tile string|integer|nil
---@param linha integer|nil
function Mapa3D:chao(tamanho, divisao, tile, linha) end
--- Quatro paredes ao redor de um chao de `tamanho`.
---@param tamanho number
---@param altura number
---@param tile string|integer|nil
---@param linha integer|nil
function Mapa3D:paredes(tamanho, altura, tile, linha) end
---@param x number
---@param z number
---@param largura number
---@param degraus integer
---@param altura_degrau number
---@param profundidade number
---@param tile string|integer|nil
---@param linha integer|nil
function Mapa3D:escada(x, z, largura, degraus, altura_degrau, profundidade, tile, linha) end
--- Gira o objeto continuamente, em graus por segundo. Use 0, 0, 0 para parar.
---@param obj Obj3DRef|Obj3D
---@param rx number|nil
---@param ry number|nil
---@param rz number|nil
function Mapa3D:girar(obj, rx, ry, rz) end

--- Atuadores: movem ou giram um objeto entre a posicao inicial (t = 0) e a final (t = 1).

--- Cria um atuador sem deslocamento nem giro; configure com `atuador_*`.
---@param obj Obj3DRef|Obj3D
---@return IdAtuador|nil
function Mapa3D:atuador(obj) end
--- Deslocamento total entre fechado e aberto.
---@param id IdAtuador
---@param dx number
---@param dy number
---@param dz number
function Mapa3D:atuador_deslocar(id, dx, dy, dz) end
--- Giro em torno de um pivo no plano xz (dobradica). A velocidade passa a valer em graus por segundo.
---@param id IdAtuador
---@param pivo_x number
---@param pivo_z number
---@param graus number
function Mapa3D:atuador_giro(id, pivo_x, pivo_z, graus) end
--- Unidades por segundo no deslizamento, graus por segundo no giro (padrao 2).
---@param id IdAtuador
---@param velocidade number
function Mapa3D:atuador_velocidade(id, velocidade) end
---@param id IdAtuador
---@param modo integer `mapa3d.MOV_ALTERNAR` ou `mapa3d.MOV_VAIVEM`
function Mapa3D:atuador_modo(id, modo) end
---@param id IdAtuador
---@param curva integer `mapa3d.CURVA_LINEAR` ou `mapa3d.CURVA_SUAVE` (padrao)
function Mapa3D:atuador_curva(id, curva) end
--- Pausa em cada ponta no modo vaivem.
---@param id IdAtuador
---@param segundos number
function Mapa3D:atuador_pausa(id, segundos) end
--- Fecha automaticamente depois que permanecer aberto pelo tempo informado; 0 desativa.
---@param id IdAtuador
---@param segundos number
function Mapa3D:atuador_auto_fechar(id, segundos) end
--- Se ligado, o jogador em cima do objeto e carregado junto.
---@param id IdAtuador
---@param ligado boolean|nil padrao true
function Mapa3D:atuador_carregar(id, ligado) end
--- Liga ou desliga o movimento do modo vaivem.
---@param id IdAtuador
---@param ligado boolean|nil padrao true
function Mapa3D:atuador_ativo(id, ligado) end
--- Anexa outro objeto que acompanha o deslocamento (nao a rotacao). Maximo de 8.
---@param id IdAtuador
---@param obj Obj3DRef|Obj3D
---@return boolean
function Mapa3D:atuador_anexar(id, obj) end
---@param id IdAtuador
---@param acao integer `mapa3d.ACAO_*`
---@param valor number|nil usado por `ACAO_IR` (0 a 1)
function Mapa3D:atuador_acao(id, acao, valor) end
---@param id IdAtuador
---@return number t de 0 (fechado) a 1 (aberto)
function Mapa3D:atuador_t(id) end
---@param id IdAtuador
---@return boolean
function Mapa3D:atuador_aberto(id) end
---@param id IdAtuador
---@return boolean
function Mapa3D:atuador_movendo(id) end
---@param id IdAtuador
---@return boolean
function Mapa3D:atuador_travado(id) end

--- Portas, elevador e andador: atuadores ja configurados.

---@param obj Obj3DRef|Obj3D
---@param dx number
---@param dy number
---@param dz number
---@param velocidade number|nil padrao 2
---@return IdAtuador|nil
function Mapa3D:porta_deslizar(obj, dx, dy, dz, velocidade) end
---@param obj Obj3DRef|Obj3D
---@param altura number
---@param velocidade number|nil padrao 2
---@return IdAtuador|nil
function Mapa3D:porta_subir(obj, altura, velocidade) end
---@param obj Obj3DRef|Obj3D
---@param altura number
---@param velocidade number|nil padrao 2
---@return IdAtuador|nil
function Mapa3D:porta_descer(obj, altura, velocidade) end
--- Porta com dobradica: gira em torno do pivo (x, z) no mundo.
---@param obj Obj3DRef|Obj3D
---@param pivo_x number
---@param pivo_z number
---@param graus number
---@param velocidade number|nil graus por segundo, padrao 120
---@return IdAtuador|nil
function Mapa3D:porta_girar(obj, pivo_x, pivo_z, graus, velocidade) end
--- Plataforma que sobe/desce e carrega o jogador.
---@param obj Obj3DRef|Obj3D
---@param altura number negativo desce
---@param velocidade number|nil padrao 2
---@return IdAtuador|nil
function Mapa3D:elevador(obj, altura, velocidade) end
--- Plataforma que vai e volta sozinha e carrega o jogador.
---@param obj Obj3DRef|Obj3D
---@param dx number
---@param dy number
---@param dz number
---@param velocidade number|nil padrao 2
---@param pausa number|nil segundos em cada ponta
---@param ativo boolean|nil padrao true
---@return IdAtuador|nil
function Mapa3D:andador(obj, dx, dy, dz, velocidade, pausa, ativo) end

--- Interativos: botoes, alavancas e areas que disparam acoes nos atuadores ligados.

--- Interativo ancorado a um objeto (a distancia e medida ate a caixa dele).
---@param obj Obj3DRef|Obj3D
---@param alcance number|nil padrao 2
---@return IdInterativo|nil
function Mapa3D:interativo(obj, alcance) end
--- Interativo num ponto fixo do mundo.
---@param x number
---@param y number
---@param z number
---@param alcance number|nil padrao 2
---@return IdInterativo|nil
function Mapa3D:interativo_em(x, y, z, alcance) end
---@param id IdInterativo
---@param entrada integer `mapa3d.ENTRADA_APERTAR` (padrao), `ENTRADA_SEGURAR` ou `ENTRADA_PROXIMIDADE`
function Mapa3D:interativo_entrada(id, entrada) end
--- Acao disparada nos atuadores ligados (padrao `ACAO_ALTERNAR`).
--- Em `SEGURAR` e `PROXIMIDADE`, `ABRIR`/`FECHAR`/`TRAVAR`/`DESTRAVAR` sao desfeitas ao soltar ou sair.
---@param id IdInterativo
---@param acao integer `mapa3d.ACAO_*`
---@param valor number|nil usado por `ACAO_IR` (0 a 1)
function Mapa3D:interativo_acao(id, acao, valor) end
--- Liga o interativo a um atuador (maximo de 8 por interativo).
---@param id IdInterativo
---@param atuador IdAtuador
---@return boolean
function Mapa3D:interativo_ligar(id, atuador) end
---@param id IdInterativo
---@param alcance number
function Mapa3D:interativo_alcance(id, alcance) end
--- Exige que o jogador esteja olhando na direcao do interativo (angulo maximo, em graus). 0 desliga.
---@param id IdInterativo
---@param graus number
function Mapa3D:interativo_olhar(id, graus) end
---@param id IdInterativo
---@param segundos number
function Mapa3D:interativo_recarga(id, segundos) end
---@param id IdInterativo
---@param ligado boolean|nil padrao true
function Mapa3D:interativo_uma_vez(id, ligado) end
---@param id IdInterativo
---@param ligado boolean|nil padrao true
function Mapa3D:interativo_ativar(id, ligado) end
--- True no quadro em que disparou.
---@param id IdInterativo
---@return boolean
function Mapa3D:interativo_disparou(id) end
---@param id IdInterativo
---@return integer
function Mapa3D:interativo_disparos(id) end
--- True enquanto o jogador esta dentro do alcance (so para `ENTRADA_PROXIMIDADE`).
---@param id IdInterativo
---@return boolean
function Mapa3D:interativo_dentro(id) end
--- Interativo que o jogador esta mirando agora (util para mostrar "Aperte E").
---@return IdInterativo|nil
function Mapa3D:foco() end

--- Atualiza interativos, giros e atuadores e passa o mundo para a colisao da camera.
---@param dt number segundos
---@param cam Camera3D|nil
---@param apertou boolean|nil botao de interacao apertado neste quadro
---@param segurando boolean|nil botao de interacao segurado
function Mapa3D:atualizar(dt, cam, apertou, segurando) end
--- Desenha todos os objetos do mapa, com luzes e neblina da camera.
---@param cam Camera3D
---@param tela FrameBuffer
---@param zb ZBuffer
function Mapa3D:desenhar(cam, tela, zb) end

---@class MaoFPS
local MaoFPS = {}
mao = {}
---@return MaoFPS|nil
function mao.novo() end
function MaoFPS:destruir() end
function MaoFPS:limpar() end
--- Adiciona um Obj3D como parte do viewmodel. x/y/z sao offsets locais da camera; grupo 0 fica sempre visivel.
---@param obj Obj3D
---@param x number|nil
---@param y number|nil
---@param z number|nil distancia a frente da camera
---@param rx number|nil
---@param ry number|nil
---@param rz number|nil
---@param grupo integer|nil grupo da arma (padrao 1)
---@return integer|nil id comeca em 0
function MaoFPS:adicionar3d(obj, x, y, z, rx, ry, rz, grupo) end
--- Adiciona um PNG/atlas como sprite 2D. x/y/w/h sao fracoes do tamanho da tela.
---@param atlas Atlas
---@param coluna integer
---@param linha integer
---@param x number|nil
---@param y number|nil
---@param w number|nil
---@param h number|nil
---@param grupo integer|nil grupo da arma (padrao 1)
---@return integer|nil id comeca em 0
function MaoFPS:adicionar2d(atlas, coluna, linha, x, y, w, h, grupo) end
---@param grupo integer id do grupo de arma selecionado
function MaoFPS:selecionar(grupo) end
---@param dt number segundos
---@param cam Camera3D
function MaoFPS:atualizar(dt, cam) end
--- Chame depois de Camera3D:desenhar.
---@param cam Camera3D
---@param tela FrameBuffer
---@param zb ZBuffer
function MaoFPS:desenhar(cam, tela, zb) end
--- Tipo `mao.CURTA` ou `mao.LONGA`; alcance e distancia do golpe/disparo.
---@param grupo integer id do grupo
---@param tipo integer `mao.CURTA` ou `mao.LONGA`
---@param dano number
---@param alcance number
---@param intervalo number segundos entre usos
---@param pente integer|nil 0 para munição ilimitada/arma curta
---@param reserva integer|nil
---@param recuo number|nil graus de recuo da camera
function MaoFPS:arma(grupo, tipo, dano, alcance, intervalo, pente, reserva, recuo) end
--- Hitscan; retorna disparou, indice do alvo (1-based)|nil, dano, distancia, pente, reserva.
---@param cam Camera3D
---@param objetos Obj3D[]|nil
---@return boolean disparou
---@return integer|nil alvo
---@return number dano
---@return number distancia
---@return integer pente
---@return integer reserva
function MaoFPS:usar(cam, objetos) end
---@param grupo integer
---@return integer munição transferida para o pente
function MaoFPS:recarregar(grupo) end
---@param grupo integer
---@return integer pente
---@return integer reserva
function MaoFPS:municao(grupo) end

---@class Mapa3DLib
---@field MOV_ALTERNAR integer
---@field MOV_VAIVEM integer
---@field CURVA_LINEAR integer
---@field CURVA_SUAVE integer
---@field ENTRADA_APERTAR integer
---@field ENTRADA_SEGURAR integer
---@field ENTRADA_PROXIMIDADE integer
---@field ACAO_ALTERNAR integer
---@field ACAO_ABRIR integer
---@field ACAO_FECHAR integer
---@field ACAO_IR integer
---@field ACAO_TRAVAR integer
---@field ACAO_DESTRAVAR integer
mapa3d = {}

---@return Mapa3D|nil
function mapa3d.criar() end
