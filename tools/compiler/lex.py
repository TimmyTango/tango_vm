import enum
import sys
from typing import Optional

class TokenType(enum.Enum):
    EOF = -1
    NEWLINE = 0
    NUMBER = 1
    IDENT = 2
    STRING = 3
    # Keywords.
    LABEL = 101
    GOTO = 102
    PRINT = 103
    INPUT = 104
    LET = 105
    IF = 106
    THEN = 107
    ENDIF = 108
    WHILE = 109
    REPEAT = 110
    ENDWHILE = 111
    PRINTLN = 112
    # Operators.
    EQ = 201  
    PLUS = 202
    MINUS = 203
    ASTERISK = 204
    SLASH = 205
    EQEQ = 206
    NOTEQ = 207
    LT = 208
    LTEQ = 209
    GT = 210
    GTEQ = 211

class Token:
    text: str
    kind: TokenType

    def __init__(self, text: str, kind: TokenType) -> None:
        self.text = text
        self.kind = kind

    @staticmethod
    def check_if_keyword(token_text: str) -> Optional[TokenType]:
        for kind in TokenType:
            if kind.name == token_text.upper() and kind.value >= 100 and kind.value < 200:
                return kind
        return None

class Lexer:
    source: str
    current_char: str
    current_pos: int

    def __init__(self, input: str):
        self.source = input + '\n'
        self.current_char = ''
        self.current_pos = -1
        self.next_char()

    # Process the next character.
    def next_char(self) -> str:
        self.current_pos += 1
        if self.current_pos >= len(self.source):
            self.current_char = '\0' # EOF
        else:
            self.current_char = self.source[self.current_pos]
        
        return self.current_char

    # Return the lookahead character.
    def peek(self) -> str:
        if self.current_pos + 1 >= len(self.source):
            return '\0'
        return self.source[self.current_pos + 1]

    # Invalid token found, print error message and exit.
    def abort(self, message: str) -> None:
        sys.exit(f'Lexing error. {message}')
		
    # Skip whitespace except newlines, which we will use to indicate the end of a statement.
    def skip_whitespace(self):
        while self.current_char in [' ', '\t', '\r']:
            self.next_char()
		
    # Skip comments in the code.
    def skip_comment(self):
        if self.current_char == '#':
            while self.current_char != '\n':
                self.next_char()

    # Return the next token.
    def get_token(self) -> Token:
        self.skip_whitespace()
        self.skip_comment()
        token: Optional[Token] = None
        # Check the first character of this token to see if we can decide what it is.
        # If it is a multiple character operator (e.g., !=), number, identifier, or keyword then we will process the rest.
        if self.current_char == '+':
            token = Token(self.current_char, TokenType.PLUS)	# Plus token.
        elif self.current_char == '-':
            token = Token(self.current_char, TokenType.MINUS)	# Minus token.
        elif self.current_char == '*':
            token = Token(self.current_char, TokenType.ASTERISK)	# Asterisk token.
        elif self.current_char == '/':
            token = Token(self.current_char, TokenType.SLASH)	# Slash token.
        elif self.current_char == '\n':
            token = Token(self.current_char, TokenType.NEWLINE)	# Newline token.
        elif self.current_char == '\0':
            token = Token(self.current_char, TokenType.EOF)	# EOF token.
        elif self.current_char == '=':
            # check if token is = or ==
            if self.peek() == '=':
                last_char = self.current_char
                self.next_char()
                token = Token(last_char + self.current_char, TokenType.EQEQ)
            else:
                token = Token(self.current_char, TokenType.EQ)
        elif self.current_char == '>':
            # Check whether this is token is > or >=
            if self.peek() == '=':
                last_char = self.current_char
                self.next_char()
                token = Token(last_char + self.current_char, TokenType.GTEQ)
            else:
                token = Token(self.current_char, TokenType.GT)
        elif self.current_char == '<':
                # Check whether this is token is < or <=
                if self.peek() == '=':
                    last_char = self.current_char
                    self.next_char()
                    token = Token(last_char + self.current_char, TokenType.LTEQ)
                else:
                    token = Token(self.current_char, TokenType.LT)
        elif self.current_char == '!':
            if self.peek() == '=':
                last_char = self.current_char
                self.next_char()
                token = Token(last_char + self.current_char, TokenType.NOTEQ)
            else:
                self.abort(f'Expected !=, got !{self.peek()}')
        elif self.current_char == '"':
            self.next_char()
            start_pos = self.current_pos
            illegal_chars = ['\r', '\n', '\t', '\\', '%']
            
            while self.current_char != '"':
                # Don't allow special characters in the string. No escape characters, newlines, tabs, or %.
                # We will be using C's printf on this string.
                if self.current_char in illegal_chars:
                    self.abort(f"Illegal character in string: `{self.current_char}`")
                self.next_char()

            token_text = self.source[start_pos : self.current_pos]
            token = Token(token_text, TokenType.STRING)
        elif self.current_char.isdigit():
            # Leading character is a digit, so this must be a number.
            # Get all consecutive digits and decimal if there is one.
            start_pos = self.current_pos
            while self.peek().isdigit():
                self.next_char()
            if self.peek() == '.': # Decimal!
                self.next_char()

                # Must have at least one digit after decimal.
                if not self.peek().isdigit(): 
                    # Error!
                    self.abort(f"Illegal character in number: `{self.current_char}`")
                while self.peek().isdigit():
                    self.next_char()

            token_text = self.source[start_pos : self.current_pos + 1] # Get the substring.
            token = Token(token_text, TokenType.NUMBER)
        elif self.current_char.isalpha():
            # Leading character is a letter, so this must be an identifier or a keyword.
            # Get all consecutive alpha numeric characters.
            start_pos = self.current_pos
            while self.peek().isalnum():
                self.next_char()

            # Check if the token is in the list of keywords.
            token_text = self.source[start_pos : self.current_pos + 1] # Get the substring.
            keyword = Token.check_if_keyword(token_text)
            if keyword == None: # Identifier
                token = Token(token_text, TokenType.IDENT)
            else:   # Keyword
                token = Token(token_text, keyword)
        else:
            # Unknown token!
            self.abort(f'Unknown token: `{self.current_char}`')
			
        self.next_char()
        return token