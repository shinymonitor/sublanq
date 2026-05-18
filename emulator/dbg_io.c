#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

struct termios oldt;
struct termios newt;
unsigned char ch = 0;
 
static inline void init_io() {
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

static inline void cleanup_io() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

static inline WORD_UTYPE input(WORD_UTYPE port){
    switch (port){
        case 0: return 0;
        break; 
        case 1: 
            ssize_t n = read(STDIN_FILENO, &ch, 1);
            if (n <= 0) return 0;
            return (WORD_UTYPE)ch;
        break;
        default : 
            return 0;
        break;
    }
    return 0;
}

static inline void output(WORD_UTYPE port, WORD_UTYPE data){
    switch (port){
        case 0: break;
        case 1: break;
        case 2: 
            if (data >= 0 && data <= 255) {putchar(data); fflush(stdout);} 
        break;
        case 3: 
            printf("\033[93;1m%d\033[0m\n", (WORD_STYPE)data);
        break;
        default: break;
    }
}