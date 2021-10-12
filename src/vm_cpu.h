#pragma once

#include <stdint.h>
#include <stdbool.h>

#define MAX_MEMORY 0x1000

#define LO_BYTE(word) ((word) & 0xFF)
#define HI_BYTE(word) ((word) >> 8)
#define COMBINE_TO_WORD(lowb, highb) (((uint16_t)(highb) << 8) + (lowb))

// general purpose regisers r0-r7
enum {
    R_R0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_COUNT,
    R_ST = 0x08,
    R_SL,
    R_SH,
    R_XL,
    R_XH,
    R_YL,
    R_YH,
    R_X = 0xF0,
    R_Y,
    R_SP
};

enum {
    FLAG_ZERO = 1,
    FLAG_NEG = 2,
    FLAG_CARRY = 4,
};

typedef struct {
    uint8_t memory[MAX_MEMORY];
    uint8_t registers[R_COUNT];
    uint8_t status;
    uint16_t pc;
    uint16_t sp;
    uint16_t x;
    uint16_t y;
    bool running;
    bool debug;
    bool step;
} vm_t;

vm_t vm;

void init_cpu();

uint8_t read_byte(uint16_t addr);
uint16_t read_word(uint16_t addr);
void write_byte(uint16_t addr, uint8_t value);
void write_bytes(uint16_t start_addr, uint16_t nbytes, uint8_t* bytes);

uint8_t next_byte();
uint16_t next_word();

void set_register(uint8_t reg, uint8_t value);
uint8_t get_register(uint8_t reg);

void add_register(uint8_t reg, uint8_t value, bool with_carry);
void sub_register(uint8_t reg, uint8_t value, bool with_borrow);
void cmp_register(uint8_t reg, uint8_t value);
void and_register(uint8_t reg, uint8_t value);
void or_register(uint8_t reg, uint8_t value);
void not_register(uint8_t reg);

uint8_t get_flag(uint8_t flag);
void set_flag(uint8_t flag, bool high);

void push_byte(uint8_t value);
uint8_t pop_byte();
