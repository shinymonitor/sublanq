#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define WORD_SIZE 16
#define WORD_STYPE int16_t
#define WORD_UTYPE uint16_t
#define WORD_MAX UINT16_MAX

#include "std_io.c"

// #define PRINT_STATE
// #define GET_IPS
// #define MANUAL_STEPPING

#ifdef GET_IPS
    #include <time.h>
    size_t ops = 0;
#endif

static inline void debug(WORD_STYPE* program, WORD_UTYPE program_size, WORD_UTYPE pc){
    if (pc + 3 >= program_size) return;
    WORD_STYPE a, b, c, ma = -1, mb = -1;
    a = program[pc];
    b = program[pc + 1];
    c = program[pc + 2];
    if (a >= 0 && (WORD_UTYPE)a < program_size) ma = program[(WORD_UTYPE)a];
    if (b >= 0 && (WORD_UTYPE)b < program_size) mb = program[(WORD_UTYPE)b];
    printf("\n> %d\n%d, %d, %d\n%d, %d\n", pc, a, b, c, ma, mb);
    for (WORD_UTYPE i = 0; i < program_size; ++i) {
        if (i == pc) printf("\033[31;1m%d\033[0m, ", program[i]);
        else printf("%d, ", program[i]);
    }
    printf("\n");
}

static inline void subleq(WORD_STYPE* program, WORD_UTYPE program_size) {
    // size_t pc = 0;
    WORD_UTYPE pc = 0;
    while (pc + 2 < program_size) {
        #ifdef MANUAL_STEPPING
        getc(stdin);
        #endif
        #ifdef PRINT_STATE
        debug(program, program_size, pc);
        #endif
        #ifdef GET_IPS
        ++ops;
        #endif
        WORD_UTYPE a = program[pc];
        WORD_UTYPE b = program[pc + 1];
        WORD_UTYPE c = program[pc + 2];
        if (a == WORD_MAX) program[b] = input(c);
        else if (b == WORD_MAX) output(c, program[a]);
        else if (c == WORD_MAX) break;
        else {
            program[b] -= program[a];
            if (program[b] <= 0) {
                pc = c;
                continue;
            }
        }
        pc += 3;
    }
}

int main(int argc, char **argv) {

    if (argc < 2) {fprintf(stderr, "Usage: %s <program.sq>\n", argv[0]); return 1;}
    FILE *f = fopen(argv[1], "rb");
    if (!f) {perror("fopen"); return 1;}
    if (fseek(f, 0, SEEK_END) != 0) { perror("fseek"); fclose(f); return 1; }
    long file_size = ftell(f);
    if (file_size < 0) { perror("ftell"); fclose(f); return 1; }
    if (file_size % sizeof(WORD_UTYPE) != 0) { fprintf(stderr, "Invalid binary size\n"); fclose(f); return 1; }
    size_t word_count = (size_t)(file_size / sizeof(WORD_UTYPE));
    if (word_count > WORD_MAX) { fprintf(stderr, "Program too large\n"); fclose(f); return 1; }
    WORD_UTYPE size = (WORD_UTYPE)word_count;
    rewind(f);
    WORD_STYPE *program = malloc(size * sizeof(WORD_STYPE));
    if (!program) { perror("malloc"); fclose(f); return 1; }
    size_t r = fread(program, sizeof(WORD_STYPE), size, f);
    fclose(f);
    if (r != size) { fprintf(stderr, "Failed to read binary program\n"); free(program); return 1; }

    init_io();

    #ifdef GET_IPS
        clock_t start = clock();
    #endif

    subleq(program, size);

    #ifdef GET_IPS
        double clocks = (((double)(clock() - start))/CLOCKS_PER_SEC);
        printf("\n%zu, %f, %f\n", ops, clocks, ((double)ops)/clocks);
    #endif

    cleanup_io();
    free(program);

    return 0;
}
