#pragma once

#include <stdint.h>

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

typedef enum {
    TOKEN_DOLLAR, TOKEN_POUND, TOKEN_COMMA, TOKEN_COLON, TOKEN_LT, TOKEN_GT, TOKEN_PERIOD,
    TOKEN_MNEMONIC, TOKEN_REGISTER, TOKEN_IDENTIFIER, TOKEN_NUMBER, TOKEN_DIRECTIVE,
    TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

typedef struct {
    const char* key;
    uint8_t value;
} StrToByteMapping;

extern Scanner scanner;
extern StrToByteMapping mnemonic_map[];
extern const int mnemonic_count;
extern StrToByteMapping register_map[];
extern const int register_count;

uint8_t get_mnemonic_value(const Token* mnemonic);
uint8_t get_register_value(const Token* reg_token);
void init_scanner(const char* src);
Token scan_token();
