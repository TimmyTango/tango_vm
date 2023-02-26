from dataclasses import dataclass
import sys
from typing import List, Set, Union
from emit import Emitter
from lex import *


"""
Variable scoping

Have a scopes list where head is global scope
Every time we encounter an if/while statement, push a new scope on to the list
All new variables are added to tail scope
Check variables in current scope, then check parent scope
Pop scope when exiting if/while block
"""

@dataclass
class Scope:
    code_index: int
    symbols: Set[str]
    labels_declared: Set[str]
    labels_gotoed: Set[str]

class ScopeProperty(enum.Enum):
    SYMBOLS = "symbols"
    LABELS_DECLARED = "labels_declared"
    LABELS_GOTOED = "labels_gotoed"


# Parser object keeps track of current token and checks if the code matches the grammar.
class Parser:
    lexer: Lexer
    emitter: Emitter
    current_token: Token
    peek_token: Token
    scopes: List[Scope]
    scope_index: int

    def __init__(self, lexer: Lexer, emitter: Emitter):
        self.lexer = lexer
        self.emitter = emitter
        self.scopes = []
        self.scope_index = -1

        self.current_token = None
        self.peek_token = None
        self.next_token()
        self.next_token()

    def push_scope(self):
        self.scopes.append(Scope(len(self.emitter.code), set(), set(), set()))
        self.scope_index += 1

    def pop_scope(self):
        if len(self.scopes) > 1:
            scope = self.scopes.pop()
            self.scope_index -= 1
            self.emit_symobls(scope)
        else:
            self.abort("Tried to pop global scope!")

    def check_all_scopes_for_identifier(self, identifier: str, property: ScopeProperty) -> bool:
        for scope in reversed(self.scopes):
            if identifier in getattr(scope, property.value):
                return True
        return False
    
    def emit_symobls(self, scope: Scope):
        for symbol in scope.symbols:
            self.emitter.emit_line_at(f"TinyVar {symbol};", scope.code_index)

    @property
    def current_scope(self) -> Scope:
        return self.scopes[self.scope_index]

    # Return true if the current token matches.
    def check_token(self, kind: TokenType) -> bool:
        return kind == self.current_token.kind
    
    # Returns true if current token matches any of the provided types
    def check_multiple_tokens(self, kinds: List[TokenType]) -> bool:
        return self.current_token.kind in kinds

    # Return true if the next token matches.
    def check_peek(self, kind: TokenType) -> bool:
        return kind == self.peek_token.kind

    # Try to match current token. If not, error. Advances the current token.
    def match(self, kind: TokenType):
        if not self.check_token(kind):
            self.abort(f"Expected {kind.name}, got {self.current_token.text}({self.current_token.kind.name})")
        self.next_token()

    # Advances the current token.
    def next_token(self):
        self.current_token = self.peek_token
        self.peek_token = self.lexer.get_token()

    def abort(self, message):
        sys.exit("Error. " + message)

    def program(self):
        self.emitter.header_line("#include <stdio.h>")
        self.emitter.header_line('#include "src/tinyvars.c"')
        self.emitter.header_line("int main() {")
        self.push_scope() # global scope

        while self.check_token(TokenType.NEWLINE):
            self.next_token()

        while not self.check_token(TokenType.EOF):
            self.statement()

        self.emitter.emit_line("return 0;")
        self.emitter.emit_line("}")
        self.emit_symobls(self.scopes[0])

        for label in self.current_scope.labels_gotoed:
            if label not in self.current_scope.labels_declared:
                self.abort(f"Attempting to GOTO undeclared label: {label}")

    def statement(self):
        if self.check_multiple_tokens([TokenType.PRINT, TokenType.PRINTLN]):
            nl = r'\n' if self.check_token(TokenType.PRINTLN) else ''
            self.next_token()

            if self.check_token(TokenType.STRING):
                self.emitter.emit_line(rf'printf("{self.current_token.text}{nl}");')
                self.next_token()
            else:
                self.emitter.emit(rf'printf("%.2f{nl}", (float)(')
                self.expression()
                self.emitter.emit_line(r'));')
        elif self.check_token(TokenType.IF):
            self.next_token()
            self.emitter.emit("if(")
            self.comparison()

            self.match(TokenType.THEN)
            self.newline()
            self.emitter.emit_line(") {")
            self.push_scope()

            while not self.check_token(TokenType.ENDIF):
                self.statement()

            self.match(TokenType.ENDIF)
            self.emitter.emit_line("}")
            self.pop_scope()
        elif self.check_token(TokenType.WHILE):
            self.next_token()
            self.emitter.emit("while(")
            self.comparison()

            self.match(TokenType.REPEAT)
            self.newline()
            self.emitter.emit_line("){")

            while not self.check_token(TokenType.ENDWHILE):
                self.statement()

            self.match(TokenType.ENDWHILE)
            self.emitter.emit_line("}")
        elif self.check_token(TokenType.LABEL):
            self.next_token()

            if self.current_token.text in self.current_scope.labels_declared:
                self.abort(f'Label already exists: {self.current_token.text}')
            self.current_scope.labels_declared.add(self.current_token.text)

            self.emitter.emit_line(f"{self.current_token.text}:")
            self.match(TokenType.IDENT)
        elif self.check_token(TokenType.GOTO):
            self.next_token()
            self.current_scope.labels_gotoed.add(self.current_token.text)
            self.emitter.emit_line(rf"goto {self.current_token.text};")
            self.match(TokenType.IDENT)
        elif self.check_token(TokenType.LET):
            self.next_token()

            if self.current_token.text not in self.current_scope.symbols:
                self.current_scope.symbols.add(self.current_token.text)
            else:
                self.abort(f"Variable {self.current_token.text} already declared!")

            self.emitter.emit_line(rf'{self.current_token.text}.type = TINY_VAR_FLOAT;')
            self.emitter.emit(rf'{self.current_token.text}.f64 = ')
            self.match(TokenType.IDENT)
            self.match(TokenType.EQ)
            self.expression()
            self.emitter.emit_line(";")
        elif self.check_token(TokenType.INPUT):
            self.next_token()

            if not self.check_all_scopes_for_identifier(self.current_token.text, ScopeProperty.SYMBOLS):
                self.abort(f'Referencing variable before declaration: {self.current_token.text}')

            self.emitter.emit_line(rf'if (0 == scanf("%f", &{self.current_token.text})) {{')
            self.emitter.emit_line(rf'{self.current_token.text} = 0;')
            self.emitter.emit(r'scanf("%')
            self.emitter.emit_line(r'*s");')
            self.emitter.emit_line(r'}')
            self.match(TokenType.IDENT)
        elif self.check_token(TokenType.IDENT):
            if not self.check_all_scopes_for_identifier(self.current_token.text, ScopeProperty.SYMBOLS):
                self.abort(f'Referencing variable before declaration: {self.current_token.text}')
            
            self.emitter.emit(rf'{self.current_token.text} = ')
            self.next_token()
            self.match(TokenType.EQ)
            self.expression()
            self.emitter.emit_line(";")

        self.newline()

    def newline(self):
        self.match(TokenType.NEWLINE)
        while self.check_token(TokenType.NEWLINE):
            self.next_token()

    def comparison(self):
        self.expression()
        if self.is_comparison_operator():
            self.emitter.emit(self.current_token.text);
            self.next_token()
            self.expression()
        else:
            self.abort(f"Expected comparison operator at: {self.current_token.text}")
    
        while self.is_comparison_operator():
            self.emitter.emit(self.current_token.text);
            self.next_token()
            self.expression()

    def is_comparison_operator(self):
        return self.check_multiple_tokens([
            TokenType.GT,
            TokenType.GTEQ,
            TokenType.LT,
            TokenType.LTEQ,
            TokenType.EQEQ,
            TokenType.NOTEQ,
        ])

    def expression(self):
        self.term()
        while self.check_multiple_tokens([TokenType.PLUS, TokenType.MINUS]):
            self.emitter.emit(self.current_token.text);
            self.next_token()
            self.term()
    
    def term(self):
        self.unary()
        while self.check_multiple_tokens([TokenType.ASTERISK, TokenType.SLASH]):
            self.emitter.emit(self.current_token.text);
            self.next_token()
            self.unary()

    def unary(self):
        if self.check_multiple_tokens([TokenType.PLUS, TokenType.MINUS]):
            self.emitter.emit(self.current_token.text);
            self.next_token()
        self.primary()

    def primary(self):
        if self.check_token(TokenType.NUMBER):
            self.emitter.emit(self.current_token.text);
            self.next_token()
        elif self.check_token(TokenType.IDENT):
            if not self.check_all_scopes_for_identifier(self.current_token.text, ScopeProperty.SYMBOLS):
                self.abort(f'Referencing variable before declaration: {self.current_token.text}')
            self.emitter.emit(f"{self.current_token.text}.f64");
            self.next_token()
        else:
            self.abort(f"Unexpected token at {self.current_token.text}")
