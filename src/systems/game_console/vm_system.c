#include "../vm_system.h"
#include "../../vm_cpu.h"

#define MAX_RAM 0xF000

uint8_t system_read_byte(uint16_t addr) {
    if (addr < MAX_RAM) {
        return vm.memory[addr];
    }

    return 0xAA;
}

uint16_t system_read_word(uint16_t addr) {
    uint8_t low = system_read_byte(addr);
    uint8_t high = system_read_byte(addr + 1);
    return COMBINE_TO_WORD(low, high);
}

void system_write_byte(uint16_t addr, uint8_t value) {
    if (addr < MAX_RAM) {
        vm.memory[addr] = value;
    }
}
