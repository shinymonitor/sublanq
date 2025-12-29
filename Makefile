linux:
	gcc ./emulator/emulate.c -o ./emulate -lX11 -Wall -Wextra -Werror -O3

windows:
	gcc ./emulator/emulate.c -o ./emulate -Wall -Wextra -Werror -O3

other:
	gcc ./emulator/emulate.c -o ./emulate -lSDL2 -Wall -Wextra -Werror -O3