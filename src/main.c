// #include "vm_core.h"

#include "vm_system.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

static bool read_hex_value(FILE* file, uint32_t* value) {
    if (feof(file)) return false;
    int c = 0;
    int i = 0;
    char buffer[16];
    while (i < 14 && (c = getc(file)) != EOF) {
        if (!isxdigit(c)) {
            ungetc(c, file);
            break;
        }
        buffer[i++] = (char)c;
    }
    buffer[i] = '\0';
    if (strnlen(buffer, 16) == 0) {
        return false;
    }
    return sscanf(buffer, "%X", value);
}


int main(int argc, char** argv) {
    atexit(cleanup_system);

    if (argc < 2) {
        printf("Missing binary file\n");
        return 1;
    }

    init_system();
    vm.debug = false;
    vm.step = false;

    const char* rom_filename = argv[1];
    FILE* fp = fopen(rom_filename, "r");

    while (!feof(fp)) {
        uint32_t value;
        if (read_hex_value(fp, &value)) {
            uint16_t addr = (uint16_t)value;
            int c = getc(fp);
            while (c != '\n' && c != EOF) {
                if (c != ' ' && c != ':') {
                    ungetc(c, fp);
                }
                if (read_hex_value(fp, &value)) {
                    vm.memory[addr++] = value;
                }
                c = getc(fp);
            }
        }
    }

    fclose(fp);
    fp = NULL;

    start_system_loop();

    return 0;
}
