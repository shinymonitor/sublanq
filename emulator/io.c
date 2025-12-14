#define TYPE int64_t

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <SDL2/SDL.h>

#define SCREEN_HEIGHT 128
#define SCREEN_WIDTH 128

#define PALETTE_R 0
#define PALETTE_G 128
#define PALETTE_B 255

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} FB_Window;
typedef struct {uint8_t r, g, b;} FB_Color;

void FB_render(FB_Window* fb_window, FB_Color frame_buffer[SCREEN_HEIGHT][SCREEN_WIDTH]) {
    SDL_UpdateTexture(fb_window->texture, NULL, frame_buffer, SCREEN_WIDTH * sizeof(FB_Color));
    SDL_RenderClear(fb_window->renderer);
    SDL_RenderCopy(fb_window->renderer, fb_window->texture, NULL, NULL);
    SDL_RenderPresent(fb_window->renderer);
}

void FB_init_window(FB_Window* fb_window) {
    SDL_Init(SDL_INIT_VIDEO);
    fb_window->window = SDL_CreateWindow("SUBLANQ", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, 0);
    fb_window->renderer = SDL_CreateRenderer(fb_window->window, -1, SDL_RENDERER_ACCELERATED);
    fb_window->texture = SDL_CreateTexture(fb_window->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!(fb_window->window && fb_window->renderer && fb_window->texture)) exit(1);
}

void FB_cleanup_window(FB_Window* fb_window) {
    SDL_DestroyTexture(fb_window->texture);
    SDL_DestroyRenderer(fb_window->renderer);
    SDL_DestroyWindow(fb_window->window);
    SDL_Quit();
}

uint8_t FB_input(){
    SDL_Event event;
    if (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return 27;
        if (event.type == SDL_KEYDOWN) {
            SDL_Keycode key = event.key.keysym.sym;
            if (key >= 32 && key <= 126) return (int)key;
            switch (key) {
                case SDLK_UP:    return 128;
                case SDLK_DOWN:  return 129;
                case SDLK_LEFT:  return 130;
                case SDLK_RIGHT: return 131;
                case SDLK_RETURN: return 10;
                case SDLK_ESCAPE: return 27;
                default: return 0;
            }
        }
    }
    return 0;
}

struct termios oldt;
struct termios newt;

size_t x=0;
size_t y=0;
FB_Color fb[SCREEN_HEIGHT][SCREEN_WIDTH]={0};
FB_Window fb_window;
 
void init_io() {
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    FB_init_window(&fb_window);

    srand((unsigned int)time(NULL));
}

void cleanup_io() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    FB_cleanup_window(&fb_window);
}

TYPE input(TYPE port){
    if (port == 0) return (TYPE)getchar();
    else if (port == 1) return (TYPE)FB_input();
    else if (port == 2) return (TYPE)(uint32_t)rand();
    else return 0;
}

void output(TYPE port, TYPE data){
    if (port == 0){if (data >= 0 && data <= 255) {putchar(data); fflush(stdout);}}
    else if (port == 1){
        float t = (float)data/255;
        if(t < 0.5f) {
            float s = t * 2.0f;
            fb[y][x].r = (uint8_t)(PALETTE_R * s);
            fb[y][x].g = (uint8_t)(PALETTE_G * s);
            fb[y][x].b = (uint8_t)(PALETTE_B * s);
        } else {
            float s = (t - 0.5f) * 2.0f;
            fb[y][x].r = (uint8_t)(PALETTE_R + (255 - PALETTE_R) * s);
            fb[y][x].g = (uint8_t)(PALETTE_G + (255 - PALETTE_G) * s);
            fb[y][x].b = (uint8_t)(PALETTE_B + (255 - PALETTE_B) * s);
        }
        x = (x + 1) % SCREEN_WIDTH;
        if (x == 0) y = (y + 1) % SCREEN_HEIGHT;
    }
    else if (port == 2) FB_render(&fb_window, fb);
}