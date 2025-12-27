#define TYPE int64_t

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define PICOFB_WIDTH 128
#define PICOFB_HEIGHT 128
#include "picofb_sdl.h"

#define PALETTE_R 0
#define PALETTE_G 128
#define PALETTE_B 255

struct termios oldt;
struct termios newt;

size_t x=0;
size_t y=0;
PICOFB_Window fb_window={0};
 
void init_io() {
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    PICOFB_init("SUBLANQ", &fb_window);

    srand((unsigned int)time(NULL));
}

void cleanup_io() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    PICOFB_cleanup(&fb_window);
}

TYPE input(TYPE port){
    switch (port){
        case 0: 
            return (TYPE)getchar();
        break;
        case 1: 
            switch (fb_window.input) {
                case SDLK_UP:    return (TYPE)128;
                case SDLK_DOWN:  return (TYPE)129;
                case SDLK_LEFT:  return (TYPE)130;
                case SDLK_RIGHT: return (TYPE)131;
                case SDLK_RETURN: return (TYPE)10;
                case SDLK_ESCAPE: return (TYPE)27;
                default: if (fb_window.quit) return (TYPE)27; else return (TYPE)0;
            }
        break;
        case 2: 
            return (TYPE)(uint32_t)rand();
        break;
        default : 
            return 0;
        break;
    }
}

void output(TYPE port, TYPE data){
    switch (port){
        case 0: 
            if (data >= 0 && data <= 255) {putchar(data); fflush(stdout);} 
        break;
        case 1:
            float t = (float)data/255;
            if(t < 0.5f) {
                float s = t * 2.0f;
                PICOFB_set_pixel(&fb_window, x, y, (uint8_t)(PALETTE_R * s), (uint8_t)(PALETTE_G * s), (uint8_t)(PALETTE_B * s));
            } else {
                float s = (t - 0.5f) * 2.0f;
                PICOFB_set_pixel(&fb_window, x, y, (uint8_t)(PALETTE_R + (255 - PALETTE_R) * s), (uint8_t)(PALETTE_G + (255 - PALETTE_G) * s), (uint8_t)(PALETTE_B + (255 - PALETTE_B) * s));
            }
            x = (x + 1) % PICOFB_WIDTH;
            if (x == 0) y = (y + 1) % PICOFB_HEIGHT;
        break;
        case 2: 
            PICOFB_update(&fb_window);
        break;
        default:
        break;
    }
}
