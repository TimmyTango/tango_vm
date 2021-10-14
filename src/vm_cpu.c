#include "vm_cpu.h"
#include "systems/vm_system.h"

#include <stdbool.h>
#include <stdio.h>

uint8_t get_flag(uint8_t flag) {
    return (vm.status & flag) == flag;
}

void set_flag(uint8_t flag, bool high) {
    if (high) {
        vm.status |= flag;
    } else {
        vm.status &= 0xFF - flag;
    }
}

static void update_status_reg(uint16_t result) {
    // status register (bits: 7-0 = xxxxxCNZ)
    set_flag(FLAG_ZERO, (result & 0xFF) == 0);
    set_flag(FLAG_NEG, (result & 0x80) == 0x80);
    set_flag(FLAG_CARRY, result > 0xFF);
}

static bool is_high_reg(uint8_t reg) {
    if (reg == R_XH || reg == R_YH) {
        return true;
    }
    return false;
}

static bool is_word_reg(uint8_t reg) {
    if (reg == R_X || reg == R_Y) {
        return true;
    }
    return false;
}

void init_cpu() {
    vm.pc = 0x0200;
    vm.as = 0xFF;
    vm.ds = 0xFF;
    vm.running = true;
}

uint8_t read_byte(uint16_t addr) {
    return system_read_byte(addr);
}

uint16_t read_word(uint16_t addr) {
    return system_read_word(addr);
}

void write_byte(uint16_t addr, uint8_t value) {
    system_write_byte(addr, value);
}

void write_bytes(uint16_t start_addr, uint16_t nbytes, uint8_t* bytes) {
    for (int i = 0; i < nbytes; i++) {
        write_byte(start_addr + i, bytes[i]);
    }
}

uint8_t next_byte() {
    if (vm.debug) {
        printf("$%02X ", vm.memory[vm.pc]);
    }
    return vm.memory[vm.pc++];
}

uint16_t next_word() {
    uint8_t low = next_byte();
    uint8_t high = next_byte();
    uint16_t word = (high << 8) + low;
    return word;
}


void set_register(uint8_t reg, uint8_t value) {
    if (reg < R_COUNT) {
        vm.registers[reg] = value;
    }

    switch (reg) {
        case R_ST:
            vm.status = value;
            break;
        case R_AS:
            vm.as = value;
            break;
        case R_DS:
            vm.ds = value;
            break;
        case R_XL:
            vm.x = (vm.x & 0xFF00) + value;
            break;
        case R_XH:
            vm.x = (vm.x & 0x00FF) + (value << 8);
            break;
        case R_YL:
            vm.y = (vm.y & 0xFF00) + value;
            break;
        case R_YH:
            vm.y = (vm.y & 0x00FF) + (value << 8);
            break;
        default:
            return;
    }
}

uint8_t get_register(uint8_t reg) {
    if (reg < R_COUNT) {
        return vm.registers[reg];
    }

    switch (reg) {
        case R_ST:
            return vm.status;
        case R_AS:
            return vm.as;
        case R_DS:
            return vm.ds;
        case R_XL:
            return LO_BYTE(vm.x);
        case R_XH:
            return HI_BYTE(vm.x);
        case R_X:
            return read_byte(vm.x);
        case R_YL:
            return LO_BYTE(vm.y);
        case R_YH:
            return HI_BYTE(vm.y);
        case R_Y:
            return read_byte(vm.y);
        default:
            return 0x00;
    }
}

void add_register(uint8_t reg, uint8_t value, bool with_carry) {
    if (!is_word_reg(reg)) {
        uint16_t result = get_register(reg) + value;
    
        if (with_carry) {
            result += get_flag(FLAG_CARRY);
            set_flag(FLAG_CARRY, 0);
        }

        set_register(reg, (uint8_t)result);
        update_status_reg(result);
        return;
    }
}

void sub_register(uint8_t reg, uint8_t value, bool with_borrow) {
    if (!is_word_reg(reg)) {
        uint16_t result = get_register(reg) - value;

        if (with_borrow) {
            result -= get_flag(FLAG_CARRY);
            set_flag(FLAG_CARRY, 0);
        }

        set_register(reg, (uint8_t)result);
        update_status_reg(result);
    }
}

void cmp_register(uint8_t reg, uint8_t value) {
    if (reg < R_COUNT || reg == R_ST || reg == R_AS || reg == R_DS) {
        uint16_t result = get_register(reg) - value;
        update_status_reg(result);
        return;
    } else if (!is_word_reg(reg)) {
        uint8_t shift_value = is_high_reg(reg) ? 8 : 0;
        update_status_reg(get_register(reg) - (value << shift_value));
    }
}

void and_register(uint8_t reg, uint8_t value) {
    if (!is_word_reg(reg)) {
        uint16_t result = get_register(reg) & value;
        set_register(reg, (uint8_t)result);
        update_status_reg(result);
    }
}

void or_register(uint8_t reg, uint8_t value) {
    if (!is_word_reg(reg)) {
        uint16_t result = get_register(reg) | value;
        set_register(reg, (uint8_t)result);
        update_status_reg(result);
    }
}

void not_register(uint8_t reg) {
    if (!is_word_reg(reg)) {
        uint8_t result = ~get_register(reg);
        set_register(reg, result);
        update_status_reg(result);
    }
}

void push_byte(uint8_t value) {
    write_byte(COMBINE_TO_WORD(vm.ds--, 0x01), value);
}

uint8_t pop_byte() {
    return read_byte(COMBINE_TO_WORD(++vm.ds, 0x01));
}

void push_address(uint16_t addr) {
    write_byte(COMBINE_TO_WORD(vm.as--, 0x00), LO_BYTE(addr));
    write_byte(COMBINE_TO_WORD(vm.as--, 0x00), HI_BYTE(addr));
}

uint16_t pop_address() {
    uint8_t high = read_byte(COMBINE_TO_WORD(++vm.as, 0x00));
    uint8_t low = read_byte(COMBINE_TO_WORD(++vm.as, 0x00));
    return COMBINE_TO_WORD(low, high);
}
