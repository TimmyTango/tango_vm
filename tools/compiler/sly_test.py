from sly import Lexer, Parser

class CalcLexer(Lexer):
    tokens = {
        NAME,
        NUMBER,
        PLUS, TIMES, MINUS, DIVIDE,
        ASSIGN,
        LPAREN, RPAREN,
        SEMI,
    }
    ignore = ' \t'

    # Tokens
    NAME = r'[a-zA-Z_][a-zA-Z0-9_]*'
    NUMBER = r'\d+'

    # Special symbols
    PLUS = r'\+'
    MINUS = r'-'
    TIMES = r'\*'
    DIVIDE = r'/'
    ASSIGN = r'='
    LPAREN = r'\('
    RPAREN = r'\)'
    SEMI = r';'

    # Ignored pattern
    ignore_newline = r'\n+'

    # Extra action for newlines
    def ignore_newline(self, t):
        self.lineno += t.value.count('\n')

    def error(self, t):
        print("Illegal character '%s'" % t.value[0])
        self.index += 1

class CalcParser(Parser):
    debugfile = 'parser.out.txt'
    tokens = CalcLexer.tokens

    precedence = (
        ('left', PLUS, MINUS),
        ('left', TIMES, DIVIDE),
        ('right', UMINUS),
    )

    def __init__(self):
        self.names = set()

    @_('NAME ASSIGN expr SEMI')
    def statement(self, p):
        self.names[p.NAME] = p.expr
        print(f'{p.NAME} = {p.expr}')
        return ('var-decl', p.NAME, p.expr)

    # @_('expr SEMI')
    # def statement(self, p):
    #     return ('expr-stmt', p.expr)

    @_('expr PLUS expr',
       'expr MINUS expr',
       'expr TIMES expr',
       'expr DIVIDE expr',)
    def expr(self, p):
        return (p[1], p[0], p[2])

    @_('MINUS expr %prec UMINUS')
    def expr(self, p):
        return ('-', p.expr)

    @_('LPAREN expr RPAREN')
    def expr(self, p):
        return ('group-expr', p.expr)

    @_('NUMBER')
    def expr(self, p):
        return ('number-const', int(p[0]))

    @_('NAME')
    def expr(self, p):
        return ('var-ref', p.NAME)

if __name__ == '__main__':
    lexer = CalcLexer()
    parser = CalcParser()
    source = """
    a = 5;
    b = a + a;
    """
    tokens = lexer.tokenize(source)
    # print(list(tokens))
    print(parser.parse(tokens))
    # while True:
    #     try:
    #         text = input('calc > ')
    #     except (EOFError, KeyboardInterrupt):
    #         break
    #     if text:
    #         parser.parse(lexer.tokenize(text))