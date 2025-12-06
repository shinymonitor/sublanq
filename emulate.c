#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

void set_cbreak(int enable) {
    static struct termios old, new;
    if (enable) {
        tcgetattr(STDIN_FILENO, &old);
        new = old;
        new.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &new);
    }
    else {tcsetattr(STDIN_FILENO, TCSANOW, &old);}
}

int main(int argc, char **argv) {
    if (argc < 2) {fprintf(stderr, "Usage: %s <program.sq>\n", argv[0]); return 1;}

    FILE *f = fopen(argv[1], "r");
    if (!f) {perror("fopen"); return 1;}

    int size = 0;
    int temp;
    while (fscanf(f, "%d", &temp) == 1) size++;
    rewind(f);
    int *mem = malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {if (!fscanf(f, "%d", &mem[i])) return 1;}
    fclose(f);
    
    set_cbreak(1);
    
    int pc = 0;
    while (pc >= 0 && pc < size-2) {
        int a = mem[pc];
        int b = mem[pc + 1];
        int c = mem[pc + 2];
        if (a == -1) {mem[b] = getchar();}
        else if (b == -1) {if (mem[a] >= 0 && mem[a] <= 255) {putchar(mem[a]); fflush(stdout);}}
        else if (c == -1) {break;}
        else {
            mem[a] -= mem[b];
            if (mem[a] <= 0) {
                pc = c;
                continue;
            }
        }
        pc += 3;
    }
    
    set_cbreak(0);
    free(mem);
    return 0;
}
