#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 256
#include "picofb.h"

PICOFB_Window io_stdout={0};
 
static inline void init_io() {
    PICOFB_init("SUBLANQ", SCREEN_WIDTH, SCREEN_HEIGHT, &io_stdout);

    srand((unsigned int)time(NULL));
}

static inline void cleanup_io() {
    PICOFB_cleanup(&io_stdout);
}

static inline WORD_UTYPE input(WORD_UTYPE port){
    switch (port){
        case 0: 
            PICOFB_update(&io_stdout);
            return 0;
        break; 
        case 1: 
            if (io_stdout.quit) return (WORD_UTYPE)27; 
            return (WORD_UTYPE)0;
        break;
        case 2: 
            return (WORD_UTYPE)(uint8_t)rand();
        break; 
        default : 
            return 0;
        break;
    }
    return 0;
}

uint8_t io_stdout_mode = 0;
WORD_UTYPE io_stdout_x = 0, io_stdout_y = 0;
uint8_t io_stdout_r = 0, io_stdout_g = 0, io_stdout_b = 0;

static inline void output(WORD_UTYPE port, WORD_UTYPE data){
    switch (port){
        case 0: 
            switch (io_stdout_mode) {
            case 0:
                io_stdout_x = data;
                io_stdout_mode = 1;
            break;
            case 1:
                io_stdout_y = data;
                io_stdout_mode = 2;
            break;
            case 2:
                io_stdout_r = (uint8_t)data;
                io_stdout_mode = 3;
            break;
            case 3:
                io_stdout_g = (uint8_t)data;
                io_stdout_mode = 4;
            break;
            case 4:
                io_stdout_b = (uint8_t)data;
                PICOFB_set_pixel(&io_stdout, io_stdout_x, io_stdout_y, PICOFB_color_argb(0xFF, io_stdout_r, io_stdout_g, io_stdout_b));
                io_stdout_mode = 0;
            break;
            default: break;
            }
        break;
        case 1: break;
        case 2: 
            printf("\033[93;1m%d\033[0m\n", data);
        break;
        default: break;
    }
}