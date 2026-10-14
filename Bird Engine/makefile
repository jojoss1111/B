CC      ?= c++
STD     ?= -std=c++17
PREFIX  ?= /usr/local

SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null || echo -I/usr/include/SDL2 -D_REENTRANT)
SDL_LIBS   := $(shell pkg-config --libs sdl2 2>/dev/null || echo -lSDL2)

CFLAGS  = $(STD) -Wall -Wextra -Os -g0 -fPIE \
          -ffunction-sections -fdata-sections \
          -fno-unwind-tables -fno-asynchronous-unwind-tables \
          -fno-stack-protector -fno-ident -fmerge-all-constants \
          -D_GNU_SOURCE=1 -D_REENTRANT \
          -I$(PREFIX)/include $(SDL_CFLAGS)

LDFLAGS = -L$(PREFIX)/lib -Wl,--gc-sections -Wl,--as-needed -Wl,-O1 \
          -Wl,--build-id=none -Wl,-z,noseparate-code -s

LDLIBS  = -lbird -lcrock $(SDL_LIBS) -lstdc++ -lm

main: main.cpp
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f main

.PHONY: clean