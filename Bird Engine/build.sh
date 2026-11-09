set -euo pipefail

CXX=clang++
LTO=0

ARGS=()
for arg in "$@"; do
    case "$arg" in
        --gcc)      CXX=g++ ;;
        --clang)    CXX=clang++ ;;
        --cc=*)     CXX="${arg#--cc=}" ;;
        --lto)      LTO=1 ;;
        *)          ARGS+=("$arg") ;;
    esac
done
set -- "${ARGS[@]+"${ARGS[@]}"}"

case "$CXX" in
    *clang*) FAMILIA=clang; CC="${CXX/clang++/clang}"; AR_LTO=llvm-ar ;;
    *)       FAMILIA=gcc;   CC="${CXX/g++/gcc}";       AR_LTO=gcc-ar ;;
esac

SRC_DIR=src
BUILD_DIR=build
LOCAL_INC="$BUILD_DIR/include"
OBJ_DIR="$BUILD_DIR/obj"
LIBNAME=libbird.a
HDRDIR=bird

PREFIX="${PREFIX:-/usr/local}"
INCLUDEDIR="$PREFIX/include"
LIBOUTDIR="$PREFIX/lib"

STD="${STD:--std=c++17}"
SDL_CFLAGS="${SDL_CFLAGS-$(pkg-config --cflags sdl2 2>/dev/null || true)}"
SDL_LIBS="${SDL_LIBS-$(pkg-config --libs sdl2 2>/dev/null || echo -lSDL2)}"
EXTRA_LIBS="${EXTRA_LIBS--lcrock}"
LUA_CFLAGS="${LUA_CFLAGS-$(pkg-config --cflags luajit 2>/dev/null || true)}"
LUA_LIBS="${LUA_LIBS-$(pkg-config --libs luajit 2>/dev/null || echo -lluajit-5.1)}"

COMUM=(-Os -g0 -ffunction-sections -fdata-sections
       -fno-unwind-tables -fno-asynchronous-unwind-tables
       -fno-stack-protector -fPIC)

if (( LTO )); then COMUM+=(-flto); fi

CXXFLAGS=("$STD" -Wall -Wextra "${COMUM[@]}"
          -fno-exceptions -fno-rtti -fno-threadsafe-statics)
CFLAGS=(-std=c11 -D_POSIX_C_SOURCE=199309L -Wall -Wextra "${COMUM[@]}")

AR=ar
DEFS=(-DFB_BIBLIOTECA)
if (( LTO )); then AR="$AR_LTO"; fi

# Se ainda sobrou algum .cpp na biblioteca, o link do app/so em C precisa do runtime C++.
tem_cpp() {
    [[ -n "$(find "$SRC_DIR" -name '*.cpp' -print -quit 2>/dev/null)" ]]
}

checar_ferramentas() {
    if ! command -v "$CC" >/dev/null 2>&1; then
        echo "erro: compilador C '$CC' nao encontrado (tente --gcc ou --cc=<binario-c++>)" >&2
        exit 1
    fi
    if [[ -d "$SRC_DIR" ]] && tem_cpp && ! command -v "$CXX" >/dev/null 2>&1; then
        echo "erro: ha fontes .cpp em $SRC_DIR/ mas '$CXX' nao foi encontrado (tente --gcc ou --cc=<binario>)" >&2
        exit 1
    fi
    if [[ -z "$LUA_CFLAGS" ]] && ! pkg-config --exists luajit 2>/dev/null; then
        echo "aviso: luajit nao encontrado via pkg-config (instale libluajit-5.1-dev ou defina LUA_CFLAGS/LUA_LIBS)" >&2
    fi
    if (( LTO )) && ! command -v "$AR_LTO" >/dev/null 2>&1; then
        echo "erro: --lto precisa de '$AR_LTO'" >&2
        exit 1
    fi
}

compilar() {
    rm -rf "$OBJ_DIR"
    local src obj base n=0

    while IFS= read -r -d '' src; do
        obj="$OBJ_DIR/${src#"$SRC_DIR"/}.o"
        mkdir -p "$(dirname "$obj")"
        echo "$CXX $src"
        "$CXX" "${CXXFLAGS[@]}" "${DEFS[@]+"${DEFS[@]}"}" $SDL_CFLAGS $LUA_CFLAGS -c "$src" -o "$obj"
        n=$((n + 1))
    done < <(find "$SRC_DIR" -name '*.cpp' -print0)

    while IFS= read -r -d '' src; do
        base="$(basename "$src")"
        if grep -rqsF --include='*.cpp' --include='*.hpp' --include='*.c' --include='*.h' -- "$base\"" "$SRC_DIR"; then
            continue
        fi
        obj="$OBJ_DIR/${src#"$SRC_DIR"/}.o"
        mkdir -p "$(dirname "$obj")"
        local -a extra=()
        # LuaJIT (x64/arm64) propaga erros via unwinder: as funcoes C que
        # chamam luaL_error/luaL_check* precisam de .eh_frame
        case "$src" in
            */lua/*.c|*/lua_*.c) extra=(-funwind-tables -fasynchronous-unwind-tables) ;;
        esac
        echo "$CC $src"
        "$CC" "${CFLAGS[@]}" "${extra[@]+"${extra[@]}"}" "${DEFS[@]+"${DEFS[@]}"}" $SDL_CFLAGS $LUA_CFLAGS -c "$src" -o "$obj"
        n=$((n + 1))
    done < <(find "$SRC_DIR" -name '*.c' -print0)

    if (( n == 0 )); then
        echo "erro: nenhum .c ou .cpp encontrado em $SRC_DIR/" >&2
        exit 1
    fi
}

empacotar_lib() {
    local objs=() obj lib="$BUILD_DIR/$LIBNAME"
    while IFS= read -r -d '' obj; do
        objs+=("$obj")
    done < <(find "$BUILD_DIR/obj" -name '*.o' -print0 | sort -z)

    rm -f "$lib"
    echo "$AR rcsD $lib (${#objs[@]} objetos)"
    "$AR" rcsD "$lib" "${objs[@]}"

    if (( ! LTO )) && command -v strip >/dev/null 2>&1; then
        strip --strip-unneeded -R .comment "$lib" 2>/dev/null || true
        "$AR" s "$lib"
    fi

    echo "$lib: $(du -h "$lib" | cut -f1)"
}

gerar_bird_h() {
    echo '#ifndef BIRD_H'
    echo '#define BIRD_H'
    echo ''
    echo '#include "bird/renderizador/render.h"'
    echo '#include "bird/lua_bindings.h"'
    echo ''
    echo '#endif'
}

copiar_headers() {
    local inc="$1" dest="$1/$HDRDIR" h
    install -d "$inc"
    rm -rf "$dest"
    while IFS= read -r -d '' h; do
        install -D -m 644 "$SRC_DIR/$h" "$dest/$h"
    done < <(find "$SRC_DIR" \( -name '*.hpp' -o -name '*.h' \) -printf '%P\0')

    rm -f "$inc/bird.hpp"
    gerar_bird_h > "$inc/bird.h"
    chmod 644 "$inc/bird.h"
}

lib_atual() {
    [[ -f "$BUILD_DIR/$LIBNAME" && -f "$BUILD_DIR/.cfg" ]] || return 1
    [[ "$(cat "$BUILD_DIR/.cfg")" == "$CXX lto=$LTO" ]] || return 1
    [[ -f "$LOCAL_INC/bird.h" ]] || return 1
    [[ -z "$(find "$SRC_DIR" -newer "$BUILD_DIR/$LIBNAME" -print -quit)" ]]
}

cmd_build() {
    compilar
    empacotar_lib
    copiar_headers "$LOCAL_INC"
    echo "$CXX lto=$LTO" > "$BUILD_DIR/.cfg"
    if [[ -n "${SUDO_USER:-}" ]]; then chown -R "$SUDO_USER": "$BUILD_DIR"; fi
}

cmd_install() {
    if lib_atual; then
        echo "$BUILD_DIR/$LIBNAME ja esta atualizada, so instalando."
    else
        cmd_build
    fi

    install -d "$LIBOUTDIR"
    copiar_headers "$INCLUDEDIR"
    install -m 644 "$BUILD_DIR/$LIBNAME" "$LIBOUTDIR/$LIBNAME"

    echo ""
    echo "Instalado em $PREFIX (use #include <bird.h>). Para compilar o main.c:"
    echo "  ./build.sh app main.c programa"
}

cmd_app() {
    local main="${1:-main.c}" out="${2:-programa}" inc libdir

    if [[ ! -f "$main" ]]; then
        echo "erro: $main nao existe" >&2
        exit 1
    fi

    if [[ -d "$SRC_DIR" ]]; then
        lib_atual || cmd_build
        inc="$LOCAL_INC"
        libdir="$BUILD_DIR"
    elif [[ -f "$LIBOUTDIR/$LIBNAME" ]]; then
        inc="$INCLUDEDIR"
        libdir="$LIBOUTDIR"
    else
        echo "erro: nao achei $SRC_DIR/ nem $LIBOUTDIR/$LIBNAME. Rode: sudo ./build.sh install" >&2
        exit 1
    fi

    local link=(-Wl,--gc-sections -Wl,--as-needed -Wl,-O1
                -Wl,--build-id=none -Wl,-z,noseparate-code -s)
    if (( LTO )) && [[ "$FAMILIA" == clang ]] && command -v ld.lld >/dev/null 2>&1; then
        link+=(-fuse-ld=lld)
    fi

    # main.c e C, mas a lib pode ter objetos C++ (precisam de libstdc++)
    local cxxrt=()
    if [[ -d "$SRC_DIR" ]]; then
        if tem_cpp; then cxxrt=(-lstdc++); fi
    else
        cxxrt=(-lstdc++)
    fi

    echo "$CC ${CFLAGS[*]} -I$inc $SDL_CFLAGS $LUA_CFLAGS $main -o $out -L$libdir -lbird $EXTRA_LIBS $LUA_LIBS $SDL_LIBS ${cxxrt[*]-} -lm -ldl ${link[*]}"
    "$CC" "${CFLAGS[@]}" -funwind-tables -fasynchronous-unwind-tables -I"$inc" $SDL_CFLAGS $LUA_CFLAGS "$main" -o "$out" \
        -L"$libdir" -lbird $EXTRA_LIBS $LUA_LIBS $SDL_LIBS "${cxxrt[@]+"${cxxrt[@]}"}" -lm -ldl "${link[@]}"
    echo "$out: $(du -h "$out" | cut -f1)"
}

cmd_clean() {
    rm -rf "$BUILD_DIR"
    rm -rf "$INCLUDEDIR/$HDRDIR"
    rm -f "$INCLUDEDIR/bird.h" "$INCLUDEDIR/bird.hpp" "$LIBOUTDIR/$LIBNAME"
    echo "bird removida do sistema."
}

cmd_so() {
    local out="${1:-$BUILD_DIR/libbird.so}" objs=() obj

    OBJ_DIR="$BUILD_DIR/obj_so"
    DEFS=(-DFB_BIBLIOTECA)
    mkdir -p "$BUILD_DIR"
    compilar

    while IFS= read -r -d '' obj; do
        objs+=("$obj")
    done < <(find "$OBJ_DIR" -name '*.o' -print0 | sort -z)

    local link=(-shared -Wl,--no-undefined -Wl,--gc-sections -Wl,--as-needed -Wl,-O1
                -Wl,--build-id=none -Wl,-soname,"$(basename "$out")"
                '-Wl,-rpath,$ORIGIN' -s)
    if (( LTO )) && [[ "$FAMILIA" == clang ]] && command -v ld.lld >/dev/null 2>&1; then
        link+=(-fuse-ld=lld)
    fi

    # linka com o driver C++ so se ainda houver .cpp; senao, com o driver C
    local ld="$CC" cxxrt=()
    if tem_cpp; then ld="$CXX"; cxxrt=(-lstdc++); fi

    echo "$ld -shared -> $out (${#objs[@]} objetos)"
    "$ld" "${COMUM[@]}" -o "$out" "${objs[@]}" \
        $EXTRA_LIBS $LUA_LIBS $SDL_LIBS "${cxxrt[@]+"${cxxrt[@]}"}" -lm -ldl "${link[@]}"

    echo "$out: $(du -h "$out" | cut -f1)"
    if [[ -n "${SUDO_USER:-}" ]]; then chown -R "$SUDO_USER": "$BUILD_DIR"; fi
}

case "${1:-}" in
    build)    checar_ferramentas; cmd_build ;;
    install)  checar_ferramentas; cmd_install ;;
    app)      shift; checar_ferramentas; cmd_app "$@" ;;
    clean)    cmd_clean ;;
    so)       shift; checar_ferramentas; cmd_so "$@" ;;
    *)
        echo "Uso: $0 {build|install|so [saida.so]|app [main.c] [saida]|clean} [--gcc|--clang|--cc=<binario>|--lto]" >&2
        exit 1
        ;;
esac