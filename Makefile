all: asm emulator_linux

asm: ./assembler/asm.c
	gcc ./assembler/asm.c -o ./asm -Wall -Wextra -Werror -Ofast

emulator_linux: ./emulator/emulate.c
	gcc ./emulator/emulate.c -o ./emulate -lX11 -Wall -Wextra -Werror -Ofast

emulator_windows: ./emulator/emulate.c
	gcc ./emulator/emulate.c -o ./emulate -lgdi32 -luser32 -Wall -Wextra -Werror -Ofast

emulator_windows_cross: ./emulator/emulate.c
	x86_64-w64-mingw32-gcc ./emulator/emulate.c -o ./emulate.exe -lgdi32 -luser32 -Wall -Wextra -Ofast

emulator_other: ./emulator/emulate.c
	gcc ./emulator/emulate.c -o ./emulate -lSDL2 -Wall -Wextra -Werror -Ofast