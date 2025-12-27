#pragma once

#ifndef PICOFB_WIDTH
#define PICOFB_WIDTH 640
#endif
#ifndef PICOFB_HEIGHT
#define PICOFB_HEIGHT 360
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <SDL2/SDL.h>

typedef struct {
    uint32_t frame_buffer[PICOFB_HEIGHT][PICOFB_WIDTH];
    SDL_KeyCode input;
    bool quit;

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Event event;
} PICOFB_Window;

static inline bool PICOFB_init(const char* window_title, PICOFB_Window* picofb_window) {
    SDL_Init(SDL_INIT_VIDEO);
    picofb_window->window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, PICOFB_WIDTH, PICOFB_HEIGHT, 0);
    picofb_window->renderer = SDL_CreateRenderer(picofb_window->window, -1, SDL_RENDERER_ACCELERATED);
    picofb_window->texture = SDL_CreateTexture(picofb_window->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, PICOFB_WIDTH, PICOFB_HEIGHT);
    if (!(picofb_window->window && picofb_window->renderer && picofb_window->texture)) return false;
    return true;
}

static inline uint32_t PICOFB_pixel(uint8_t r, uint8_t g, uint8_t b){return (0xFFu << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;}
static inline void PICOFB_set_pixel(PICOFB_Window* w, int x, int y, uint8_t r, uint8_t g, uint8_t b){
    if (x < 0 || x >= PICOFB_WIDTH || y < 0 || y >= PICOFB_HEIGHT) return;
    w->frame_buffer[y][x] = PICOFB_pixel(r,g,b);
}

static inline void PICOFB_update(PICOFB_Window* picofb_window) {
    SDL_UpdateTexture(picofb_window->texture, NULL, picofb_window->frame_buffer, PICOFB_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(picofb_window->renderer);
    SDL_RenderCopy(picofb_window->renderer, picofb_window->texture, NULL, NULL);
    SDL_RenderPresent(picofb_window->renderer);
    while (SDL_PollEvent(&picofb_window->event)){
        switch(picofb_window->event.type){
            case SDL_KEYDOWN: picofb_window->input=(picofb_window->event.key.keysym.sym); break;
            case SDL_QUIT: picofb_window->quit=true; break;
            default: picofb_window->input=SDLK_UNKNOWN; break;
        }
    }
}

static inline void PICOFB_cleanup(PICOFB_Window* picofb_window) {
    SDL_DestroyTexture(picofb_window->texture);
    SDL_DestroyRenderer(picofb_window->renderer);
    SDL_DestroyWindow(picofb_window->window);
    SDL_Quit();
}

static inline void PICOFB_save_ppm(PICOFB_Window* picofb_window, const char *path){
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6 %d %d 255\n", PICOFB_WIDTH, PICOFB_HEIGHT);
    size_t count = 0;
    for (int y = 0; y < PICOFB_HEIGHT; y++){
        for (int x = 0; x < PICOFB_WIDTH; x++){
            uint32_t px = picofb_window->frame_buffer[y][x];
            if (px) count++;
            fwrite(&px, 1, 3, f);
        }
    }
    fclose(f);
}
