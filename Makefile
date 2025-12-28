default:
	gcc ./emulator/emulate.c -o ./emulate -lSDL2 -lX11 -Wall -Wextra -Werror -O3