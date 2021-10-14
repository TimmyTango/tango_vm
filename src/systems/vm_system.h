#pragma once

#include <stdint.h>

uint8_t system_read_byte(uint16_t addr);
uint16_t system_read_word(uint16_t addr);
void system_write_byte(uint16_t addr, uint8_t value);