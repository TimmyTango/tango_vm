#include "../../vm_system.h"

#define MAX_RAM 0x1000
#define TILESET_SIZE 0x0800
#define TILESET_START 0xF000
#define TILESET_END 0xF800
#define SCREEN_SIZE 0x0240  // 576 bytes, 32x18 tiles
#define SCREEN1_START 0xF800 // F800
#define SCREEN1_END 0xFA40
#define CONTROLLER1 0xFCB0
#define CONTROLLER2 0xFCB1
#define SPRITE1 0xFCB2
#define SPRITE1_X 0xFCB3
#define SPRITE1_Y 0xFCB4



palette_color_t palette[16] = {
    {0, 0, 0},        // COLOR_BLACK
    {255, 255, 255},  // COLOR_WHITE
    {127, 127, 127},  // COLOR_GRAY
    {29, 41, 119},    // COLOR_DARK_BLUE
    {29, 97, 236},    // COLOR_LIGHT_BLUE
    {14, 89, 12},     // COLOR_DARK_GREEN
    {29, 171, 24},    // COLOR_LIGHT_GREEN
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {255, 0, 255},     // COLOR_KEY
};

bool rebuild_tileset = true;

enum {
    COLOR_BLACK,
    COLOR_WHITE,
    COLOR_GRAY,
    COLOR_DARK_BLUE,
    COLOR_LIGHT_BLUE,
    COLOR_DARK_GREEN,
    COLOR_LIGHT_GREEN,
    // COLOR_DARK_RED,
    // COLOR_LIGHT_RED,
    // COLOR_DARK_YELLOW,
    // COLOR_LIGHT_YELLOW,
    // COLOR_DARK_BLUE,
    // COLOR_LIGHT_BLUE,
    COLOR_KEY = 15
};

uint8_t system_read_byte(uint16_t addr) {
    return vm.memory[addr];
}

uint16_t system_read_word(uint16_t addr) {
    uint8_t low = system_read_byte(addr);
    uint8_t high = system_read_byte(addr + 1);
    return COMBINE_TO_WORD(low, high);
}

void system_write_byte(uint16_t addr, uint8_t value) {
    if (addr < MAX_RAM) {
        vm.memory[addr] = value;
    } else if (addr >= TILESET_START && addr < TILESET_END ) {
        vm.memory[addr] = value;
        rebuild_tileset = true;
    } else if (addr >= SCREEN1_START && addr < SCREEN1_END) {
        vm.memory[addr] = value;
    } else {
        printf("addr %04X = %02X\n", addr, value);
        vm.memory[addr] = value;
    }
}

void init_system() {
    init_cpu();

    vm_host.screen_width = 256;   // 32 tiles
    vm_host.screen_height = 144;  // 18 tiles
    vm_host.screen_zoom = 4;

    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        exit(1);
	}

    vm_host.window = SDL_CreateWindow(
        "TangoVM",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        vm_host.screen_width * vm_host.screen_zoom,
        vm_host.screen_height * vm_host.screen_zoom,
        SDL_WINDOW_SHOWN
    );
    if( vm_host.window == NULL ) {
        printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
        exit(1);
    }

    vm_host.renderer = SDL_CreateRenderer(vm_host.window, -1, SDL_RENDERER_ACCELERATED);
    if (vm_host.renderer == NULL) {
        printf("Could not create renderer. SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_RenderSetLogicalSize(vm_host.renderer, vm_host.screen_width, vm_host.screen_height);
}

void cleanup_system() {
    puts("Cleaning up VM");

    if (vm_host.renderer) {
        SDL_DestroyRenderer(vm_host.renderer);
        vm_host.renderer = NULL;
    }

    if (vm_host.window) {
        SDL_DestroyWindow(vm_host.window);
        vm_host.window = NULL;
    }

    SDL_Quit();
}

void handle_controller_event(SDL_Event* event) {
    int bit = -1;
    switch (event->key.keysym.sym) {
        case SDLK_UP:
            bit = 0;
            break;
        case SDLK_DOWN:
            bit = 1;
            break;
        case SDLK_LEFT:
            bit = 2;
            break;
        case SDLK_RIGHT:
            bit = 3;
            break;
        case SDLK_z:
            bit = 4;
            break;
        case SDLK_x:
            bit = 5;
            break;
        case SDLK_LSHIFT:
            bit = 6;
            break;
        case SDLK_RETURN:
            bit = 7;
            break;
        default:
            return;
    }

    if (event->type == SDL_KEYDOWN) {
        SET_BIT(vm.memory[CONTROLLER1], bit);
    } else if (event->type == SDL_KEYUP) {
        CLEAR_BIT(vm.memory[CONTROLLER1], bit);
    }
}

void start_system_loop() {
    vm.running = true;
    vm.cycle = 0;

    if (vm.clock_speed > 100000 && !vm.step) {
        vm.debug = false;
    }

    uint32_t last_tick = SDL_GetTicks();
    float perf_counter_freq = (float)SDL_GetPerformanceFrequency();
    double cycles_left = 0;

    const uint8_t tileset_width = 64;
    const uint8_t tileset_height = 64;

    SDL_Texture* tileset_texture = SDL_CreateTexture(
        vm_host.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        tileset_width,
        tileset_height
    );

    SDL_SetTextureBlendMode(tileset_texture, SDL_BLENDMODE_BLEND);

    SDL_Event e;
    while (vm.running) {
        uint64_t start_frame = SDL_GetPerformanceCounter();

        while (SDL_PollEvent(&e)) {
            handle_controller_event(&e);
            switch (e.type) {
                case SDL_QUIT:
                    vm.running = false;
                    break;
                case SDL_KEYDOWN:
                    switch (e.key.keysym.sym) {
                        case SDLK_RETURN:
                            {
                                if (vm.step) {
                                    vm.cycle = 0;
                                    cpu_cycle();
                                }
                            }
                            break;
                    }
                    break;
            }
        }

        uint32_t current_tick = SDL_GetTicks();
        double delta = (current_tick - last_tick) / 1000.0;
        last_tick = current_tick;

        if (!vm.step) {
            if (cycles_left < 1) {
                cycles_left += delta * vm.clock_speed;
            } else {
                cycles_left = delta * vm.clock_speed;
            }

            while (cycles_left >= 1.0) {
                vm.cycle = 0;
                cpu_cycle();

                if (vm.debug) {
                    printf("Cycles: %d\n\n", (vm.cycle));
                }
                cycles_left -= vm.cycle;
            }
        }

        if (rebuild_tileset) {
            rebuild_tileset = false;
            uint8_t* lockedPixels = NULL;
            int pitch = 0;
            
            SDL_LockTexture(tileset_texture, NULL, (void**)&lockedPixels, &pitch);

            for (int offset = 0; offset < TILESET_SIZE; offset++) {
                uint8_t col2 = (vm.memory[TILESET_START + offset]) & 0x0F;
                uint8_t col1 = (vm.memory[TILESET_START + offset]) >> 4;

                lockedPixels[(offset * 8) + 0] = palette[col1].b;
                lockedPixels[(offset * 8) + 1] = palette[col1].g;
                lockedPixels[(offset * 8) + 2] = palette[col1].r;
                lockedPixels[(offset * 8) + 3] = (col1 == COLOR_KEY ? SDL_ALPHA_TRANSPARENT : SDL_ALPHA_OPAQUE);
                lockedPixels[(offset * 8) + 4] = palette[col2].b;
                lockedPixels[(offset * 8) + 5] = palette[col2].g;
                lockedPixels[(offset * 8) + 6] = palette[col2].r;
                lockedPixels[(offset * 8) + 7] = (col2 == COLOR_KEY ? SDL_ALPHA_TRANSPARENT : SDL_ALPHA_OPAQUE);
            }

            SDL_UnlockTexture(tileset_texture);
        }

        SDL_SetRenderDrawColor(vm_host.renderer, 0, 0, 0, 255);
        SDL_RenderClear(vm_host.renderer);

        for (uint16_t i = 0; i < SCREEN_SIZE; i++) {
            uint8_t tile = vm.memory[i + SCREEN1_START];
            SDL_Rect srcrect = { .x=(tile % 8) * 8, .y=(tile / 8) * 8, .w=8, .h=8 };
            SDL_Rect dstrect = { .x=(i % 32) * 8, .y=(i / 32) * 8, .w=8, .h=8 };
            SDL_RenderCopy(vm_host.renderer, tileset_texture, &srcrect, &dstrect);
        }

        uint8_t tile = vm.memory[SPRITE1];
        uint8_t x = vm.memory[SPRITE1_X];
        uint8_t y = vm.memory[SPRITE1_Y];
        SDL_Rect srcrect = { .x=(tile % 8) * 8, .y=(tile / 8) * 8, .w=8, .h=8 };
        SDL_Rect dstrect = { .x=x, .y=y, .w=8, .h=8 };
        SDL_RenderCopy(vm_host.renderer, tileset_texture, &srcrect, &dstrect);

        SDL_RenderPresent(vm_host.renderer);

        uint64_t end_frame = SDL_GetPerformanceCounter();
        float elapsed_ms = (end_frame - start_frame) / perf_counter_freq * 1000.0f;
        float delay = fmaxf(floorf(16.666f - elapsed_ms), 0);
        SDL_Delay(delay);
    }

    SDL_DestroyTexture(tileset_texture);
}
