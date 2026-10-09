all: clean main

main:
	clang++ -std=c++17 -Wall -Wextra -Os -g0 -ffunction-sections \
	-fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables \
	-fno-stack-protector -fPIE -fno-exceptions -fno-rtti -fno-threadsafe-statics \
	-I/usr/local/include -I/usr/include/SDL2 -D_GNU_SOURCE=1 -D_REENTRANT \
	res/code/mundo_procedural.cpp res/code/player.cpp main.cpp -o main -L/usr/local/lib -lbirdpp -lcrock -lSDL2 -lm -Wl,--gc-sections -Wl,--as-needed -Wl,-O1 -Wl,--build-id=none \
	-Wl,-z,noseparate-code -s

clean:
	rm -- ./main