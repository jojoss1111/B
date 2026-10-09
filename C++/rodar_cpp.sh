#!/bin/bash
set -e

# ============================================================
# rodar_cpp.sh — Build e empacotamento do jogo C++ como AppImage
# ============================================================

APP_NAME="Main"
APP_DIR="${APP_NAME}.AppDir"
APPIMAGE_OUT="${APP_NAME}.AppImage"
DESKTOP_ID="meu-jogo"
BIN_NAME="main"
ASSETS_DIR="res/assets"
ICON_SRC="${ASSETS_DIR}/"
BUNDLE_SDL2=1   # 1 = embute libSDL2 no AppImage, 0 = usa a do sistema

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log()  { echo -e "${GREEN}[OK]${NC} $1"; }
warn() { echo -e "${YELLOW}[AVISO]${NC} $1"; }
erro() { echo -e "${RED}[ERRO]${NC} $1"; exit 1; }

# ============================================================
# 1. Dependências e build
# ============================================================
echo ""
echo "==> Verificando dependências..."

command -v appimagetool &>/dev/null || erro "appimagetool não encontrado. Baixe em: https://github.com/AppImage/AppImageKit/releases"

[ -d "$ASSETS_DIR" ]                || erro "Pasta $ASSETS_DIR/ não encontrada. Rode o script na pasta do projeto."
[ -f "$ASSETS_DIR/ascii.png" ]      || erro "$ASSETS_DIR/ascii.png não encontrado."
[ -f "$ASSETS_DIR/tileset.png" ]    || erro "$ASSETS_DIR/tileset.png não encontrado."

echo "==> Compilando com make..."
rm -f "$BIN_NAME"
make main || erro "Falha ao compilar."

[ -f "$BIN_NAME" ] || erro "Binário '$BIN_NAME' não foi gerado."

log "Build OK."

# ============================================================
# 2. Checagem de compatibilidade de glibc
# ============================================================
echo ""
echo "==> Checando versão de glibc exigida..."

if command -v objdump &>/dev/null; then
    GLIBC_REQ=$(objdump -T "$BIN_NAME" | grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -V | tail -1)
    echo "    Binário exige glibc >= ${GLIBC_REQ:-?}"
    echo "    (Linux Mint 21.x / Ubuntu 22.04 tem glibc 2.35)"
    if [ -n "$GLIBC_REQ" ] && [ "$(printf '%s\n' "2.35" "$GLIBC_REQ" | sort -V | tail -1)" != "2.35" ]; then
        warn "Exige glibc mais nova que a do Mint 21. Compile dentro de um container ubuntu:22.04."
    fi
fi

# ============================================================
# 3. Limpeza
# ============================================================
echo ""
echo "==> Limpando build anterior..."

rm -rf "$APP_DIR"
rm -f "$APPIMAGE_OUT"

log "Limpeza concluída."

# ============================================================
# 4. Estrutura do AppDir
# ============================================================
echo ""
echo "==> Criando estrutura AppDir..."

SHARE_DIR="$APP_DIR/usr/share/$DESKTOP_ID"

mkdir -p "$APP_DIR/usr/bin"
mkdir -p "$APP_DIR/usr/lib"
mkdir -p "$SHARE_DIR/res"

cp "$BIN_NAME" "$APP_DIR/usr/bin/"

# Mantém o caminho res/assets/ exatamente como o código espera
cp -r "$ASSETS_DIR" "$SHARE_DIR/res/"
log "Texturas copiadas: $(ls "$SHARE_DIR/$ASSETS_DIR" | tr '\n' ' ')"

if [ "$BUNDLE_SDL2" = "1" ]; then
    SDL_LIB=$(ldd "$BIN_NAME" | awk '/libSDL2/ {print $3}' | head -1)
    if [ -n "$SDL_LIB" ] && [ -f "$SDL_LIB" ]; then
        cp -L "$SDL_LIB" "$APP_DIR/usr/lib/"
        log "libSDL2 embutida: $(basename "$SDL_LIB")"
    else
        warn "libSDL2 não encontrada via ldd — não embutida."
    fi
fi

log "Arquivos copiados."

# ============================================================
# 5. AppRun
# ============================================================
cat > "$APP_DIR/AppRun" <<'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
cd "$HERE/usr/share/meu-jogo"
exec "$HERE/usr/bin/main" "$@"
EOF

chmod +x "$APP_DIR/AppRun"
log "AppRun criado."

# ============================================================
# 6. .desktop interno
# ============================================================
cat > "$APP_DIR/${DESKTOP_ID}.desktop" <<EOF
[Desktop Entry]
Name=${APP_NAME}
Exec=AppRun %F
Icon=${DESKTOP_ID}
Type=Application
Categories=Game;
Comment=Jogo feito com bird++
EOF

log ".desktop interno criado."

# ============================================================
# 7. Ícone
# ============================================================
cp "$ICON_SRC" "$APP_DIR/${DESKTOP_ID}.png"
log "Ícone copiado de $ICON_SRC."

# ============================================================
# 8. Geração do AppImage
# ============================================================
echo ""
echo "==> Gerando AppImage..."

ARCH=$(uname -m)
ARCH="${ARCH}" appimagetool "$APP_DIR" "$APPIMAGE_OUT"

chmod +x "$APPIMAGE_OUT"
log "AppImage gerado: $APPIMAGE_OUT"

rm -rf "$APP_DIR"
log "Pasta temporária ($APP_DIR) removida."

# ============================================================
# 9. Atalho no menu
# ============================================================
echo ""
read -p "Instalar atalho no menu de aplicativos do sistema? [s/N] " RESPOSTA

if [[ "$RESPOSTA" =~ ^[Ss]$ ]]; then
    APPIMAGE_ABS="$(readlink -f "$APPIMAGE_OUT")"
    ICON_ABS="$(readlink -f "$ICON_SRC")"

    mkdir -p ~/.local/share/applications

    cat > ~/.local/share/applications/${DESKTOP_ID}.desktop <<EOF
[Desktop Entry]
Name=${APP_NAME}
Exec=${APPIMAGE_ABS}
Icon=${ICON_ABS}
Type=Application
Categories=Game;
Comment=Jogo feito com bird++
StartupNotify=true
Terminal=false
EOF

    chmod 644 ~/.local/share/applications/${DESKTOP_ID}.desktop
    update-desktop-database ~/.local/share/applications/ 2>/dev/null || true

    log "Atalho instalado: ~/.local/share/applications/${DESKTOP_ID}.desktop"
fi

echo ""
echo "============================================"
echo -e " ${GREEN}Build concluído com sucesso!${NC}"
echo "============================================"
echo "  AppImage : ./$APPIMAGE_OUT"
echo "  Executar : ./$APPIMAGE_OUT"
echo "============================================"
echo ""