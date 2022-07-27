import argparse
from enum import Enum
import re
from typing import List, Optional

class TokenType(Enum):
    OP_CODE = 1
    REGISTER = 2
    ADDRESS = 3
    INDIRECT = 4
    IMMEDIATE = 5
    COMMENT = 6
    LABEL = 7
    LABEL_DEF = 8
    DIRECTIVE = 9
    EQU_DEF = 10

class Token:
    def __init__(self, token_type: TokenType, value):
        self.type = token_type
        self.value = value

    def __repr__(self):
        if self.type in [TokenType.LABEL, TokenType.LABEL_DEF, TokenType.DIRECTIVE, TokenType.EQU_DEF]:
            return f'{str(self.type)[10:]}("{self.value}")'
        return f'{str(self.type)[10:]}(${self.value:02X})'

instruction_map = {
    'nop': 0x00,
    'jmp': 0x10,
    'inc': 0x20,
    'dec': 0x30,
    'clc': 0x40,
    'sec': 0x50,
    'not': 0x60,
    'jsr': 0x70,
    'ret': 0x80,
    'beq': 0x01,
    'bne': 0x11,
    'blt': 0x21,
    'ble': 0x31,
    'bgt': 0x41,
    'bge': 0x51,
    'mov': 0x02,
    'add': 0x03,
    'adc': 0x43,
    'sub': 0x04,
    'sbb': 0x44,
    'cmp': 0x05,
    'and': 0x07,
    'or': 0x47,
    'psh': 0x08,
    'pop': 0x48,
    'dbg': 0xFE,
    'end': 0xFF
}

class Directive(Enum):
    DECL_BYTE = '.byte'
    EQU = '.equ'
    ORG = '.org'

directive_map = [e.value for e in Directive]

special_registers = {
    'st': 0x08,
    'as': 0x09,
    'ds': 0x0A,
    'xl': 0x0B,
    'xh': 0x0C,
    'yl': 0x0D,
    'yh': 0x0E,
    'x': 0xF0,
    'y': 0xF1,
}

def remove_trailing_comma(word: str) -> str:
    if word.endswith(','): return word[:-1]
    return word

def is_indirect_label(word: str) -> bool:
    return word.startswith('[') and word.endswith(']')

def is_half_label(word: str) -> bool:
    return word.startswith('<') or word.startswith('>')

def process_number(word: str, first_token: Optional[Token]) -> Optional[Token]:
    token_type = TokenType.ADDRESS
    hexidecimal = False
    value = word

    value = remove_trailing_comma(value)

    if value[0] == '#':
        token_type = TokenType.IMMEDIATE
        value = value[1:]
    elif value[0] == '[':
        if not value.endswith(']'): return None
        token_type = TokenType.INDIRECT
        value = value[1:-1]
    elif first_token and first_token.type == TokenType.DIRECTIVE:
        if first_token.value == Directive.DECL_BYTE.value:
            token_type = TokenType.IMMEDIATE

    if not value: return None

    if value[0] == '$':
        hexidecimal = True
        value = value[1:]
    
    if not value: return None

    base = 16 if hexidecimal else 10
    int_value = int(value, base)
    if token_type == TokenType.IMMEDIATE:
        if int_value > 0xFF:
            print('Invalid immediate value')
    else:
        if int_value > 0xFFFF:
            print('Invalid address value')

    return Token(token_type, int_value)

def process_word(word: str, first_token: Optional[Token]) -> Optional[Token]:
    if word[0] == ';':
        return Token(TokenType.COMMENT, 0)

    word_lower = word.lower()

    if word_lower in instruction_map:
        return Token(TokenType.OP_CODE, instruction_map[word_lower])

    if word_lower in directive_map:
        return Token(TokenType.DIRECTIVE, word_lower)
    
    match = re.match(r'r([0-7]),?$', word_lower)
    if match:
        return Token(TokenType.REGISTER, int(match.group(1), 10))
    
    match = re.match(r'(st|xl|xh|yl|yh|as|ds|x|y),?$', word_lower)
    if match:
        reg = match.group(1)
        return Token(TokenType.REGISTER, special_registers[reg])
    
    match = re.match(r'([a-zA-Z_][\w]+):$', word)
    if match:
        return Token(TokenType.LABEL_DEF, match.group(1))

    match = re.match(r'[(<|>)]?\[?[a-zA-Z_][\w]+\]?,?$', word)
    if match:
        if first_token and first_token.type == TokenType.DIRECTIVE and first_token.value == Directive.EQU.value:
            return Token(TokenType.EQU_DEF, word)
        else:
            return Token(TokenType.LABEL, remove_trailing_comma(word))

    if word[0] in ['#', '[', '$'] or re.match(r'[0-9a-fA-F]', word[0]):
        return process_number(word, first_token)

    return None

def process_line(line: str) -> List[Token]:
    output: List[Token] = []
    words = list(filter(lambda w: len(w) > 0, line.split(' ')))
    first_token = None
    for word in words:
        token = process_word(word, first_token)
        
        if first_token is None:
            first_token = token

        if not token:
            print('syntax error?', word)
            return output
        elif token.type == TokenType.COMMENT:
            return output
        output.append(token)
    return output

def split_word_into_bytes(word: int):
    low = word & 0xFF
    high = word >> 8 & 0xFF
    return [low, high]

def main():
    parser = argparse.ArgumentParser(description='Assembler for a made-up instruction set')
    parser.add_argument('source_file', metavar='source_file', type=str, help='ASM source file to assemble')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true')
    parser.add_argument('-o', '--out', dest='out_file', metavar='output_file', default='default.rom')
    parser.add_argument('-l', '--link', dest='linked_roms', metavar='rom_file', type=str, nargs='+',
                        help='Additional rom files to link')
    args = parser.parse_args()

    pc = 0
    equivalents = {}
    labels = {}
    output: List[List[Token]] = []
    with open(args.source_file, 'r') as source:
        for line_no, line in enumerate(source):
            line = line.strip('\n')
            tokens = process_line(line)

            processed_tokens = []
            equ_def = None
            next_token_sets_pc = False
            initial_pc = pc

            for token in tokens:
                if equ_def is not None:
                    equivalents[equ_def] = token
                    equ_def = None
                elif next_token_sets_pc:
                    if token.type not in [TokenType.ADDRESS, TokenType.IMMEDIATE]:
                        print(f'{line_no}: Invalid value for .org')
                        return
                    if token.value < pc:
                        print(f'{line_no}: Can only use .org to advance PC')
                        return
                    pc = token.value
                    next_token_sets_pc = False
                    continue
                elif token.type == TokenType.DIRECTIVE:
                    if token.value == Directive.ORG.value:
                        next_token_sets_pc = True
                        continue
                elif token.type == TokenType.LABEL_DEF:
                    if token.value in labels and labels[token.value] != -1:
                            print(f'{line_no}: label "{token.value}" already defined!')
                            return
                    else:
                        labels[token.value.strip('[]')] = pc
                elif token.type == TokenType.EQU_DEF:
                    equ_def = token.value
                elif token.type == TokenType.LABEL:
                    if token.value in equivalents:
                        equ_token = equivalents[token.value]
                        token.type = equ_token.type
                        token.value = equ_token.value

                        if token.type in [TokenType.ADDRESS, TokenType.INDIRECT]:
                            pc += 2
                        elif token.type in [TokenType.IMMEDIATE, TokenType.REGISTER, TokenType.OP_CODE]:
                            pc += 1
                    else:
                        label: str = token.value
                        is_indirect = is_indirect_label(label)
                        label = label.strip('[]')
                        if label in labels and labels[label] != -1:
                            token.type = TokenType.INDIRECT if is_indirect else TokenType.ADDRESS
                            token.value = labels[label]
                        else:
                            labels[token.value] = -1
                        
                        if label.startswith('<') or label.startswith('>'):
                            pc += 1
                        else:
                            pc += 2
                elif token.type not in [TokenType.DIRECTIVE, TokenType.ADDRESS, TokenType.INDIRECT]:
                    pc += 1
                elif token.type in [TokenType.ADDRESS, TokenType.INDIRECT]:
                    pc += 2

                if equ_def is not None and token.type != TokenType.EQU_DEF:
                    print(f'{line_no}: equ "{equ_def}" not defined!')
                    return

                processed_tokens.append(token)

            if processed_tokens:
                # print(f'tokens: {processed_tokens}')
                if processed_tokens[0].type == TokenType.OP_CODE:
                    op = processed_tokens[0].value & 0xF
                    if op in [2, 3, 4, 5, 7, 8]:
                        if (len(processed_tokens) != 3 and op < 8) or (len(processed_tokens) != 2 and op == 8):
                            print(f'{line_no}: Unknown mode based on operands')

                        if op == 8:
                            op_type1 = TokenType.REGISTER
                            op_value1 = None
                            op_type2 = processed_tokens[1].type
                            op_value2 = processed_tokens[1].value
                        else:
                            op_type1 = processed_tokens[1].type
                            op_value1 = processed_tokens[1].value
                            op_type2 = processed_tokens[2].type
                            op_value2 = processed_tokens[2].value

                        if op_type1 == TokenType.REGISTER:
                            pass
                        elif op_type1 == TokenType.ADDRESS:
                            processed_tokens[0].value += 0x40
                        elif op_type1 == TokenType.INDIRECT:
                            processed_tokens[0].value += 0x80
                        elif op_type1 == TokenType.LABEL:
                            if is_indirect_label(op_value1):
                                processed_tokens[0].value += 0x80
                            elif is_half_label(op_value1):
                                pass
                            else:
                                processed_tokens[0].value += 0x40

                        else:
                            print(f'{line_no}: Unknown mode based on operands')
                            return

                        if op_type2 == TokenType.REGISTER:
                            pass
                        elif op_type2 == TokenType.ADDRESS:
                            processed_tokens[0].value += 0x10
                        elif op_type2 == TokenType.IMMEDIATE:
                            processed_tokens[0].value += 0x20
                        elif op_type2 == TokenType.INDIRECT:
                            processed_tokens[0].value += 0x30
                        elif op_type2 == TokenType.LABEL:
                            if is_indirect_label(op_value2):
                                processed_tokens[0].value += 0x30
                            elif is_half_label(op_value2):
                                processed_tokens[0].value += 0x20
                            else:
                                processed_tokens[0].value += 0x10
                        else:
                            print(f'{line_no}: Unknown mode based on operands')
                            return
                if processed_tokens[0].type == TokenType.DIRECTIVE and processed_tokens[0].value == Directive.EQU.value:
                    continue
                output.append((initial_pc, processed_tokens))

    file_output = []
    
    for line_no, (pc, line) in enumerate(output):
        if args.verbose:
            print(line)
        mem_map = f'${pc:04X}:'
        for token in line:
            if token.type in [TokenType.DIRECTIVE, TokenType.LABEL_DEF]:
                continue
           
            if token.type == TokenType.LABEL:
                label: str = token.value

                is_indirect = False
                take_low_byte = False
                take_high_byte = False
                if label.startswith('[') and label.endswith(']'):
                    is_indirect = True
                elif label.startswith('<'):
                    take_low_byte = True
                elif label.startswith('>'):
                    take_high_byte = True


                label = label.strip('[]<>')
                if label not in labels or labels[label] == -1:
                    print(f'{line_no}: label "{label}" not found!')
                    return
                else:
                    token.value = labels[label]
                    low, high = split_word_into_bytes(token.value)

                    if is_indirect:
                        token.type = TokenType.INDIRECT
                    elif take_low_byte:
                        token.type = TokenType.IMMEDIATE
                        token.value = low
                    elif take_high_byte:
                        token.type = TokenType.IMMEDIATE
                        token.value = high
                    else:
                        token.type = TokenType.ADDRESS

            if token.type in [TokenType.ADDRESS, TokenType.INDIRECT]:
                low, high = split_word_into_bytes(token.value)
                mem_map += f' ${low:02X} ${high:02X}'
            else:
                mem_map += f' ${token.value:02X}'

        if len(mem_map) > 6:
            file_output.append(f'{mem_map.replace("$", "")}\n')
            if args.verbose:
                print(mem_map)

    
    with open(args.out_file, 'w') as out_file:
        for data in file_output:
            out_file.write(data)

        for rom in args.linked_roms:
            if args.verbose:
                print(f'Linking {rom}')
            with open(rom, 'r') as linked_rom_file:
                out_file.write(linked_rom_file.read())

if __name__ == '__main__':
    main()
    