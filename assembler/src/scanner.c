#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include "scanner.h"

Scanner scanner;

StrToByteMapping mnemonic_map[] = {
    { .key="nop", .value=0x00 },
    { .key="jmp", .value=0x10 },
    { .key="inc", .value=0x20 },
    { .key="dec", .value=0x30 },
    { .key="clc", .value=0x40 },
    { .key="sec", .value=0x50 },
    { .key="not", .value=0x60 },
    { .key="jsr", .value=0x70 },
    { .key="ret", .value=0x80 },
    { .key="beq", .value=0x01 },
    { .key="bne", .value=0x11 },
    { .key="blt", .value=0x21 },
    { .key="ble", .value=0x31 },
    { .key="bgt", .value=0x41 },
    { .key="bge", .value=0x51 },
    { .key="mov", .value=0x02 },
    { .key="add", .value=0x03 },
    { .key="adc", .value=0x43 },
    { .key="sub", .value=0x04 },
    { .key="sbb", .value=0x44 },
    { .key="cmp", .value=0x05 },
    { .key="and", .value=0x07 },
    { .key="or",  .value=0x47 },
    { .key="psh", .value=0x08 },
    { .key="pop", .value=0x48 },
    { .key="dbg", .value=0xFE },
    { .key="end", .value=0xFF }
};
const int mnemonic_count = sizeof(mnemonic_map)/sizeof(mnemonic_map[0]);

StrToByteMapping register_map[] = {
    { .key="r0", .value=0x00 },
    { .key="r1", .value=0x01 },
    { .key="r2", .value=0x02 },
    { .key="r3", .value=0x03 },
    { .key="r4", .value=0x04 },
    { .key="r5", .value=0x05 },
    { .key="r6", .value=0x06 },
    { .key="r7", .value=0x07 },
    { .key="st", .value=0x08 },
    { .key="as", .value=0x09 },
    { .key="ds", .value=0x0A },
    { .key="xl", .value=0x0B },
    { .key="xh", .value=0x0C },
    { .key="yl", .value=0x0D },
    { .key="yh", .value=0x0E },
    { .key="x",  .value=0xF0 },
    { .key="y",  .value=0xF1 }
};
const int register_count = sizeof(register_map)/sizeof(register_map[0]);

void init_scanner(const char* src) {
    scanner.start = src;
    scanner.current = src;
    scanner.line = 1;
}

uint8_t get_mnemonic_value(const Token* mnemonic_token) {
    for (int i = 0; i < mnemonic_count; i++) {
        if (strncmp(mnemonic_map[i].key, mnemonic_token->start, mnemonic_token->length) == 0) {
            return mnemonic_map[i].value;
        }
    }
    return 0xFF;
}

uint8_t get_register_value(const Token* reg_token) {
    for (int i = 0; i < register_count; i++) {
        if (strncmp(register_map[i].key, reg_token->start, reg_token->length) == 0) {
            return register_map[i].value;
        }
    }
    return 0xFF;
}

static bool is_alpha(char c) {
    return c == '_' || isalpha(c);
}

static bool is_at_end() {
    return *scanner.current == '\0';
}

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static char peek() {
    return *scanner.current;
}

static bool match(char expected) {
    if (is_at_end()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static Token make_token(TokenType type) {
    Token token = {
        .type = type,
        .start = scanner.start,
        .length = (int)(scanner.current - scanner.start),
        .line = scanner.line
    };
    return token;
}

static Token error_token(const char* message) {
    Token token = {
        .type = TOKEN_ERROR,
        .start = message,
        .length = (int)strlen(message),
        .line = scanner.line
    };
    return token;
}

static void skip_whitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case '\n':
                scanner.line++;
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            default:
                return;
        }
    }
}

static TokenType identifier_type() {
    int max_size = scanner.current - scanner.start;
    if (max_size < 0 || max_size > 1024) {
        max_size = 0;
    }

    for (int i = 0; i < register_count; i++) {
        if (strncmp(register_map[i].key, scanner.start, max_size) == 0) {
            return TOKEN_REGISTER;
        }
    }

    for (int i = 0; i < mnemonic_count; i++) {
        if (strncmp(mnemonic_map[i].key, scanner.start, max_size) == 0) {
            return TOKEN_MNEMONIC;
        }
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while (is_alpha(peek()) || isdigit(peek())) advance();
    return make_token(identifier_type());
}

static Token number() {
    while (isdigit(peek())) advance();

    return make_token(TOKEN_NUMBER);
}

static Token directive() {
    while (is_alpha(peek()) || isdigit(peek())) advance();
    return make_token(TOKEN_DIRECTIVE);
}

Token scan_token() {
    skip_whitespace();
    scanner.start = scanner.current;
    
    if (is_at_end()) return make_token(TOKEN_EOF);

    char c = advance();

    if (is_alpha(c)) return identifier();
    if (isdigit(c)) return number();

    switch (c) {
        case '$':
            return make_token(TOKEN_DOLLAR);
        case '#':
            return make_token(TOKEN_POUND);
        case ',':
            return make_token(TOKEN_COMMA);
        case ':':
            return make_token(TOKEN_COLON);
        case '<':
            return make_token(TOKEN_LT);
        case '>':
            return make_token(TOKEN_GT);
        case '.':
            return directive();
    }

    return error_token("Unexpected character.");
}