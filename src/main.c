// #include "vm_core.h"

#include "vm_cpu.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

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


int main(int argc, char** argv) {
    // atexit(cleanupVm);
    // if (!initVm()) exit(EXIT_FAILURE);
    // startVm();

    if (argc < 2) {
        printf("Missing binary file\n");
        return 1;
    }

    const char* bin_filename = argv[1];

    init_cpu();
    vm.debug = true;
    vm.step = false;

    uint8_t* prg_start = vm.memory + vm.pc;

    FILE* fp = fopen(bin_filename, "rb");

    fread(prg_start, sizeof(vm.memory[0]), MAX_MEMORY - vm.pc, fp);

    fclose(fp);
    fp = NULL;

    while (vm.running) {
        if (vm.debug && vm.step) {
            getchar();
        }

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

        printf("\n");

        if (vm.debug) {
            print_debug();
        }
    }

    printf("\nData Stack:\n");
    for (uint8_t i = 0xFF; i > vm.ds; i--) {
        printf("$%02X: $%02X\n", i, read_byte(COMBINE_TO_WORD(i, 0x01)));
    }

    return 0;
}
