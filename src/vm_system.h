#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>

#include "vm_cpu.h"

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} palette_color_t;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;

    int screen_width;
    int screen_height;
    int screen_zoom;
} vm_host_t;

extern vm_host_t vm_host;

void init_system();
void start_system_loop();
void cleanup_system();

uint8_t system_read_byte(uint16_t addr);
uint16_t system_read_word(uint16_t addr);
void system_write_byte(uint16_t addr, uint8_t value);
