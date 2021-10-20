// #include "vm_core.h"

#include "vm_system.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>


int main(int argc, char** argv) {
    atexit(cleanup_system);

    if (argc < 2) {
        printf("Missing binary file\n");
        return 1;
    }

    init_system();
    vm.debug = false;
    vm.step = false;

    uint8_t* prg_start = vm.memory + vm.pc;

    const char* bin_filename = argv[1];
    FILE* fp = fopen(bin_filename, "rb");
    fread(prg_start, sizeof(vm.memory[0]), MAX_MEMORY - vm.pc, fp);
    fclose(fp);
    fp = NULL;

    start_system_loop();

    return 0;
}
