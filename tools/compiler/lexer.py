from dataclasses import dataclass
from enum import Enum, IntEnum
from typing import List

from tools.compiler.constants import KEYWORDS


class TokenType(IntEnum):
    SYMBOL = 1
    NUMBER = 2
    LBRACE = 3
    RBRACE = 4
    SEMICOLON = 5
    LPAREN = 6
    RPAREN = 7
    EQUALS = 8
    PLUS = 9
    MINUS = 10
    INVALID = 11
    KEYWORD = 12
    COMMA = 13

    def __str__(self) -> str:
        return self.name

    def __repr__(self) -> str:
        return self.name


@dataclass
class Token:
    type: TokenType
    value: str

    def __str__(self) -> str:
        return f'"{self.value}" ({self.type})'

    def __repr__(self) -> str:
        return f'"{self.value}" ({self.type})'

class TokenAtom:
    LBRACE = Token(TokenType.LBRACE, '{')
    RBRACE = Token(TokenType.RBRACE, '}')
    SEMICOLON = Token(TokenType.SEMICOLON, ';')
    LPAREN = Token(TokenType.LPAREN, '(')
    RPAREN = Token(TokenType.RPAREN, ')')
    EQUALS = Token(TokenType.EQUALS, '=')
    PLUS = Token(TokenType.PLUS, '+')
    MINUS = Token(TokenType.MINUS, '-')
    COMMA = Token(TokenType.COMMA, ',')

TokenList = List[Token]


class Lexer:
    def __init__(self, source: str) -> None:
        self.source = source
        self.max_i = len(source) - 1
        self.i = -1
        self.tokens: TokenList = []

    def add_token(self, token: Token) -> None:
        self.tokens.append(token)

    def next_char(self) -> str:
        self.i += 1
        if self.i > self.max_i: return ''
        return self.source[self.i]

    def rewind_char(self) -> None:
        self.i -= 1

    def peek_next_char(self) -> str:
        c = self.next_char()
        self.i -= 1
        return c

    def parse_symbol(self) -> None:
        start = self.i
        c = self.source[self.i]
        while c.isalnum() or c == '_':
            c = self.next_char()
        end = self.i
        value = self.source[start:end]
        if value in KEYWORDS:
            token = Token(TokenType.KEYWORD, value)
        else:
            token = Token(TokenType.SYMBOL, value)
        self.tokens.append(token)
        self.rewind_char()

    def parse_number(self) -> None:
        start = self.i
        c = self.next_char()
        while c.isnumeric():
            c = self.next_char()
        end = self.i
        token = Token(TokenType.NUMBER, self.source[start:end])
        self.tokens.append(token)
        self.rewind_char()

    def eat_comment(self) -> None:
        c = self.next_char()
        while c != '' and c != '\n':
            c = self.next_char()
        self.rewind_char()


def lex(source: str) -> TokenList:
    lexer = Lexer(source)
    c = lexer.next_char()

    while c != '':
        if c.isalpha():
            lexer.parse_symbol()
        elif c.isnumeric():
            lexer.parse_number()
        elif c == '{':
            lexer.add_token(TokenAtom.LBRACE)
        elif c == '}':
            lexer.add_token(TokenAtom.RBRACE)
        elif c == '(':
            lexer.add_token(TokenAtom.LPAREN)
        elif c == ')':
            lexer.add_token(TokenAtom.RPAREN)
        elif c == '=':
            lexer.add_token(TokenAtom.EQUALS)
        elif c == ';':
            lexer.add_token(TokenAtom.SEMICOLON)
        elif c == ',':
            lexer.add_token(TokenAtom.COMMA)
        elif c == '+':
            lexer.add_token(TokenAtom.PLUS)
        elif c == '-':
            if lexer.peek_next_char().isnumeric():
                lexer.parse_number()
            else:
                lexer.add_token(TokenAtom.MINUS)
        elif c == '#':
            lexer.eat_comment()
        elif not c.isspace():
            lexer.add_token(Token(TokenType.INVALID, c))

        c = lexer.next_char()

    return lexer.tokens
