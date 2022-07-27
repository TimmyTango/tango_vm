#include "vm_cpu.h"
#include "vm_system.h"

#include <stdbool.h>
#include <stdio.h>

vm_host_t vm_host = {
    .window = NULL,
    .renderer = NULL,
    .screen_width = 0,
    .screen_height = 0,
    .screen_zoom = 0
};

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

    vm.clock_speed = 1000000; // 1Mhz
}

static void print_debug() {
    printf("PC=$%04X X=$%04X Y=$%04X | AS=$%02X DS=$%02X | ", vm.pc, vm.x, vm.y, vm.as, vm.ds);
    for (int i = 0; i < R_COUNT; i++) {
        printf("r%d=$%02X ", i, get_register(i));
    }
    printf(
        "| Z=%d N=%d C=%d\n",
        vm.status & 1,
        (vm.status & 2) == 2,
        (vm.status & 4) == 4
    );
}

static void handle_bad_instruction(uint8_t instruction) {
    printf("$%04X: Unknown opcode $%02X\n", vm.pc, instruction);
    vm.running = false;
}

static bool handle_pop_op(uint8_t mode) {
    if (mode == 4) {
        set_register(next_byte(), pop_byte());
    } else if (mode == 5) {
        write_byte(next_word(), pop_byte());
    } else if (mode == 7) {
        write_byte(read_word(next_word()), pop_byte());
    } else {
        return false;
    }
    return true;
}

static void handle_math_op(uint8_t instruction) {
    uint8_t op = instruction & 0x0F;
    uint8_t mode = instruction >> 4;
    uint8_t value = 0;
    uint8_t reg = 0;

    if (op < 8) {
        reg = next_byte();
    }

    if (op == 8 && mode > 3 && mode < 8) {
        if (!handle_pop_op(mode)) {
            return handle_bad_instruction(instruction);
        }
        return;
    }

    switch (mode) {
        case 0x0:
        case 0x4:
            // add/adc reg, reg
            value = get_register(next_byte());
            break;
        case 0x1:
        case 0x5:
            // add/adc reg, mem
            value = read_byte(next_word());
            break;
        case 0x2:
        case 0x6:
            // add/adc reg, imm
            value = next_byte();
            break;
        case 0x3:
        case 0x7:
            // add/adc reg, indirect mem
            value = read_byte(read_word(next_word()));
            break;
        default:
            handle_bad_instruction(instruction);
    }

    if (op == 3) {
        return add_register(reg, value, mode > 3);
    } else if (op == 4) {
        return sub_register(reg, value, mode > 3);
    } else if (op == 5) {
        return cmp_register(reg, value);
    } else if (op == 7) {
        if (mode < 4) {
            return and_register(reg, value);
        } else if (mode < 8) {
            return or_register(reg, value);
        }
    } else if (op == 8) {
        if (mode < 4) {
            return push_byte(value);
        }
    }

    handle_bad_instruction(instruction);
}

static void handle_mov_op(uint8_t instruction) {
    uint8_t mode = instruction >> 4;
    uint8_t value = 0;
    uint16_t dest = 0;

    if (mode < 0x4) {
        dest = next_byte();
    } else if (mode < 0xc) {
        dest = next_word();
    } else {
        return handle_bad_instruction(instruction);
    }

    uint8_t destMode = mode % 4;

    if (destMode == 0) {
        value = get_register(next_byte());
    } else if (destMode == 1) {
        value = read_byte(next_word());
    } else if (destMode == 2) {
        value = next_byte();
    } else if (destMode == 3) {
        value = read_byte(read_word(next_word()));
    } else {
        return handle_bad_instruction(instruction);
    }

    if (mode < 0x4) {
        set_register(dest, value);
    } else if (mode < 0xc) {
        write_byte(dest, value);
    } else {
        return handle_bad_instruction(instruction);
    }
}

void cpu_cycle() {
    uint8_t instruction = next_byte();
    uint8_t op = instruction & 0x0F;

    switch (op) {
        case 2: // mov 
            handle_mov_op(instruction);
            break;
        case 3: // add/adc
        case 4: // sub/sbb
        case 5: // cmp
        case 7: // and/or
        case 8: // psh/pop
            // add, sub, cmp ops
            handle_math_op(instruction);
            break;
        default:
            switch (instruction) {
                case 0xff: // end
                    vm.running = false;
                    break;
                case 0xfe: // dbg
                    if (!vm.debug) print_debug();
                    break;
                case 0x00: // nop
                    break;
                case 0x10: // jmp
                    vm.pc = next_word();
                    break;
                case 0x20: // inc
                    add_register(next_byte(), 1, false);
                    break;
                case 0x30: // dec
                    sub_register(next_byte(), 1, false);
                    break;
                case 0x40: // clc
                case 0x50: // sec
                    set_flag(FLAG_CARRY, instruction == 0x50);
                    break;
                case 0x60: // not
                    not_register(next_byte());
                    break;
                case 0x70: // jsr
                    push_address(vm.pc + 2);
                    vm.pc = next_word();
                    break;
                case 0x80: // ret
                    vm.pc = pop_address();
                    break;
                case 0x01: // beq
                    {
                        uint16_t addr = next_word();
                        if (get_flag(FLAG_ZERO)) vm.pc = addr;
                    }
                    break;
                case 0x11: // bne
                    {
                        uint16_t addr = next_word();
                        if (!get_flag(FLAG_ZERO)) vm.pc = addr;
                    }
                    break;
                case 0x21: // blt
                    {
                        uint16_t addr = next_word();
                        if (get_flag(FLAG_CARRY)) vm.pc = addr;
                    }
                    break;
                case 0x31: // ble
                    {
                        uint16_t addr = next_word();
                        if (get_flag(FLAG_CARRY) || get_flag(FLAG_ZERO)) vm.pc = addr;
                    }
                    break;
                case 0x41: // bgt
                    {
                        uint16_t addr = next_word();
                        if (!get_flag(FLAG_CARRY) && !get_flag(FLAG_ZERO)) vm.pc = addr;
                    }
                    break;
                case 0x51: // bge
                    {
                        uint16_t addr = next_word();
                        if (get_flag(FLAG_ZERO) || !get_flag(FLAG_CARRY)) vm.pc = addr;
                    }
                    break;
                default:
                    handle_bad_instruction(instruction);
                    break;
            }
    }

    if (vm.debug) {
        printf("\n");
        print_debug();
    }

    // if (vm.debug) {
    //     printf("\nData Stack:\n");
    //     for (uint8_t i = 0xFF; i > vm.ds; i--) {
    //         printf("$%02X: $%02X\n", i, read_byte(COMBINE_TO_WORD(i, 0x01)));
    //     }
    // }
}

uint8_t read_byte(uint16_t addr) {
    vm.cycle++;
    return system_read_byte(addr);
}

uint16_t read_word(uint16_t addr) {
    vm.cycle += 2;
    return system_read_word(addr);
}

void write_byte(uint16_t addr, uint8_t value) {
    vm.cycle++;
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
    vm.cycle++;
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
        case R_X:
            system_write_byte(vm.x, value);
            break;
        case R_Y:
            system_write_byte(vm.y, value);
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
        vm.cycle++;
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
        vm.cycle++;
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
        vm.cycle++;
        uint16_t result = get_register(reg) - value;
        update_status_reg(result);
        return;
    } else if (!is_word_reg(reg)) {
        vm.cycle++;
        uint8_t shift_value = is_high_reg(reg) ? 8 : 0;
        update_status_reg(get_register(reg) - (value << shift_value));
    }
}

void and_register(uint8_t reg, uint8_t value) {
    if (!is_word_reg(reg)) {
        vm.cycle++;
        uint16_t result = get_register(reg) & value;
        set_register(reg, (uint8_t)result);
        update_status_reg(result);
    }
}

void or_register(uint8_t reg, uint8_t value) {
    if (!is_word_reg(reg)) {
        vm.cycle++;
        uint16_t result = get_register(reg) | value;
        set_register(reg, (uint8_t)result);
        update_status_reg(result);
    }
}

void not_register(uint8_t reg) {
    if (!is_word_reg(reg)) {
        vm.cycle++;
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
