#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define TYPE int64_t

#include "io.c"

int main(int argc, char **argv) {
    if (argc < 2) {fprintf(stderr, "Usage: %s <program.sq>\n", argv[0]); return 1;}
    FILE *f = fopen(argv[1], "r");
    if (!f) {perror("fopen"); return 1;}
    TYPE size = 0;
    TYPE temp;
    while (fscanf(f, "%zu", &temp) == 1) size++;
    rewind(f);
    TYPE *mem = malloc(size * sizeof(TYPE));
    for (TYPE i = 0; i < size; i++) {if (!fscanf(f, "%zu", &mem[i])) return 1;}
    fclose(f);

    init_io();

    TYPE pc = 0;
    while (pc >= 0 && pc < size-2) {
        TYPE a = mem[pc];
        TYPE b = mem[pc + 1];
        TYPE c = mem[pc + 2];
        if (a == -1) {mem[b] = input(c);}
        else if (b == -1) {output(c, mem[a]);}
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

    cleanup_io();
    free(mem);
    return 0;
}
