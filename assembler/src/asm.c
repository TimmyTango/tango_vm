#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "stretchy_buffer.h"
#include "scanner.h"

static char* read_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(file_size + 1);
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Could not read file \"%s\".\n", filename);
        exit(1);
    }
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

typedef struct {
    const Token* tokens;
    const Token* current;
    uint8_t* output;
    int line;
    int pc;
    bool error;
} Assembler;

Assembler assembler;

static void init_assembler(const Token* tokens) {
    assembler.tokens = tokens;
    assembler.current = tokens;
    assembler.output = NULL;
    assembler.line = assembler.current->line;
    assembler.pc = 0;
    assembler.error = false;
}

static const Token* advance() {
    assembler.current++;
    return &assembler.current[-1];
}

static const Token* peek() {
    return assembler.current;
}

static const Token* peek_next() {
    const Token* next = &assembler.current[1];
    if (next <= &sb_last(assembler.tokens)) {
        return next;
    }
    return NULL;
}

static bool is_same_line(const Token* token) {
    if (token) {
        return token->line == assembler.line;
    }
    return false;
}

static bool is_at_end() {
    return assembler.current->type == TOKEN_EOF || assembler.current == &sb_last(assembler.tokens);
}

static uint8_t* handle_mnemonic() {
    const Token* mnemonic = assembler.current;
    const Token* operand1 = NULL;
    int operand1_size = 0;
    const Token* operand2 = NULL;
    int operand2_size = 0;
    bool found_comma = false;

    while (!is_at_end() && is_same_line(peek_next())) {
        advance();
        if (!operand1) {
            operand1 = assembler.current;
            operand1_size++;
        } else if (assembler.current->type == TOKEN_COMMA) {
            if (!operand2) {
                found_comma = true;
            } else {
                fprintf(stderr, "Unexpected operand on line %d\n", assembler.line);
                assembler.error = true;
            }
        } else if (found_comma) {
            operand2 = assembler.current;
            operand2_size++;
        } else {
            operand2_size++;
        }
    }

    sb_push(assembler.output, get_mnemonic_value(mnemonic));
    assembler.pc++;
    for (int i = 0; i < operand1_size; i++) {
        sb_push(assembler.output, 1);
        assembler.pc++;
    }
    for (int i = 0; i < operand2_size; i++) {
        sb_push(assembler.output, 2);
        assembler.pc++;
    }

    return assembler.output;
}


int main(int argc, char** argv) {
    if (argc < 2) {
        puts("Missing source file");
        return 1;
    }

    char* buffer = read_file(argv[1]);

    init_scanner(buffer);
    int line = -1;
    Token* sb_tokens = NULL;
    for (;;) {
        sb_push(sb_tokens, scan_token());
        if (sb_last(sb_tokens).type == TOKEN_EOF) break;
    }

    init_assembler(sb_tokens);

    for (int i = 0; i < 10; i++) {
        if (assembler.error) break;
        if (is_at_end()) break;

        switch (assembler.current->type) {
            case TOKEN_DIRECTIVE:
                while (!is_at_end() && is_same_line(peek_next())) {
                    advance();
                }
                break;
            case TOKEN_IDENTIFIER:
                while (!is_at_end() && is_same_line(peek_next())) {
                    advance();
                }
                break;
            case TOKEN_MNEMONIC:
                assembler.output = handle_mnemonic();
                break;
            default:
                fprintf(stderr, "Invalid start of expression on line %d\n", assembler.current->line);
                assembler.error = true;
        }

        advance();
    }

    sb_free(sb_tokens);
    free(buffer);

    if (assembler.output) {
        for (int i = 0; i < sb_count(assembler.output); i++) {
            printf("%02X ", assembler.output[i]);
        }
        sb_free(assembler.output);
    }

    puts("");
    return 0;
}
