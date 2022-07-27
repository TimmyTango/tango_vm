from typing import List, Optional
from tools.compiler.constants import VAR_TYPES
from tools.compiler.lexer import (
    TokenAtom, Token, TokenList, TokenType
)


def parse(tokens: TokenList):
    parser = Parser(tokens)
    nodes = parser.parse()
    parent = ProgramNode(nodes)
    return parent


def is_var_type(token: Token) -> bool:
    return token.type == TokenType.KEYWORD and token.value in VAR_TYPES


def is_return_val_type(token: Token) -> bool:
    return token.type == TokenType.KEYWORD and token.value in VAR_TYPES + ['void']


class Node:
    def __init__(self, node_type: str, children: List['Node'] = []) -> None:
        self.type = node_type
        self.children: List[Node] = children

    def __str__(self) -> str:
        return f'<{self.type}>'

class ProgramNode(Node):
    def __init__(self, children: List['Node'] = []) -> None:
        super().__init__('program', children)

class UnknownNode(Node):
    def __init__(self, value: str, children: List['Node'] = []) -> None:
        super().__init__('unknown', children)
        self.value = value

    def __str__(self) -> str:
        return f'<{self.type} {self.value}>'

class GlobalsNode(Node):
    def __init__(self, children: List['Node'] = []) -> None:
        super().__init__('globals', children)

class ProcDeclNode(Node):
    def __init__(self, proc_name: str, return_type: str, children: List['Node'] = []) -> None:
        super().__init__('proc', children)
        self.name = proc_name
        self.return_type = return_type

    def __str__(self) -> str:
        return f'<{self.type} {self.name} -> {self.return_type}>'

class ProcParamsNode(Node):
    def __init__(self, children: List['Node'] = []) -> None:
        super().__init__('proc-params', children)

class MainNode(Node):
    def __init__(self, children: List['Node'] = []) -> None:
        super().__init__('main', children)

class VarDeclNode(Node):
    def __init__(self, var_name: str, data_type: str, children: List['Node'] = []) -> None:
        super().__init__('var-decl', children)
        self.name = var_name
        self.data_type = data_type

    def __str__(self) -> str:
        return f'<{self.type} {self.name}: {self.data_type}>'

class ConstDeclNode(VarDeclNode):
    def __init__(self, var_name: str, data_type: str, children: List['Node'] = []) -> None:
        super().__init__(var_name, data_type, children)
        self.type = 'const-decl'

class NumberLiteralNode(Node):
    def __init__(self, value: int, children: List['Node'] = []) -> None:
        super().__init__('number', children)
        self.value = value

    def __str__(self) -> str:
        return f'<{self.type}> {self.value}'

class ExpressionNode(Node):
    def __init__(self, children: List['Node'] = []) -> None:
        super().__init__('expression', children)


class Parser:
    def __init__(self, tokens: TokenList) -> None:
        self.tokens = tokens
        self.max_i = len(tokens) - 1
        self.i = -1
        self.nodes = []

        self.parser_map = {
            'globals': self.parse_globals,
            'const': self.parse_const,
            'var': self.parse_var,
            'byte': self.parse_var_declaration,
            'proc': self.parse_proc,
            'void': None,
            'main': self.parse_main,
            'repeat': self.parse_proc,
            'return': None,
        }

    def next_token(self) -> Optional[Token]:
        self.i += 1
        if self.i > self.max_i: return None
        return self.tokens[self.i]

    def rewind_token(self) -> None:
        self.i -= 1

    def peek_next_token(self) -> str:
        t = self.next_token()
        self.i -= 1
        return t

    def parse(self, single_expr=False):
        if (self.i < 0):
            self.i += 1
        token = self.tokens[self.i]
        nodes = []
        while token is not None:
            if token.type == TokenType.KEYWORD:
                nodes.append(self.parse_keyword())
            else:
                nodes.append(self.parse_expression())
            if single_expr:
                return nodes[0]
            token = self.next_token()
        return nodes

    def parse_expression(self) -> Node:
        token = self.tokens[self.i]
        expression: List[Node] = []
        while token is not None and token is not TokenAtom.SEMICOLON:
            if token.type == TokenType.NUMBER:
                expression.append(NumberLiteralNode(int(token.value)))
            else:
                expression.append(UnknownNode(token.value))
            token = self.next_token()
        if len(expression) == 1:
            return expression[0]
        return ExpressionNode(expression)

    def parse_keyword(self) -> Node:
        token = self.tokens[self.i]
        parser_func = self.parser_map.get(token.value)
        if parser_func is None:
            return
        node = parser_func()
        return node

    def parse_globals(self):
        token = self.next_token()
        assert token is TokenAtom.LBRACE, 'Expected "{" after "globals" keyword.'
        global_defs = []
        token = self.next_token()
        while token is not None and token is not TokenAtom.RBRACE:
            global_defs.append(self.parse(True))
            token = self.next_token()
        return GlobalsNode(global_defs)

    def parse_proc(self):
        token = self.next_token()
        assert is_return_val_type(token), 'Expected var type as proc return type.'
        return_type = token.value
        token = self.next_token()
        assert token.type == TokenType.SYMBOL, 'Expected proc name.'
        proc_name = token.value
        token = self.next_token()
        param_node = self.parse_proc_decl_params()
        while token is not None and token is not TokenAtom.RBRACE:
            token = self.next_token()
        return ProcDeclNode(proc_name, return_type, [param_node])

    def parse_proc_decl_params(self):
        token = self.tokens[self.i]
        assert token is TokenAtom.LPAREN, 'Expected "(" after proc name.'
        param_nodes = []
        token = self.next_token()
        while token is not TokenAtom.RPAREN:
            assert token is not None, 'Expected matching ")" before EOF for proc.'
            assert token is not TokenAtom.RBRACE,  'Expected matching ")" before "}" for proc.'
            var_type, var_name, expression = self.parse_var_declaration()
            assert len(expression) == 0, 'Unexpected expression after proc param declaration.'
            param_nodes.append(VarDeclNode(var_name, var_type, []))
            token = self.tokens[self.i]
            assert token in [TokenAtom.RPAREN, TokenAtom.COMMA], 'Expected ")" or "," after proc param declaration.'
        return ProcParamsNode(param_nodes)

    def parse_main(self):
        token = self.next_token()
        while token is not None and token is not TokenAtom.RBRACE:
            token = self.next_token()
        return MainNode()

    def parse_const(self):
        token = self.next_token()
        assert is_var_type(token), 'Expected variable declaration after "const" keyword.'
        var_type, var_name, expression = self.parse_var_declaration()
        return ConstDeclNode(var_name, var_type, expression)

    def parse_var(self):
        token = self.next_token()
        assert is_var_type(token), 'Expected variable declaration after "var" keyword.'
        var_type, var_name, expression = self.parse_var_declaration()
        return VarDeclNode(var_name, var_type, expression)

    def parse_var_declaration(self) -> tuple:
        var_type = self.tokens[self.i].value
        token = self.next_token()
        assert token.type == TokenType.SYMBOL, 'Expected variable name after var type declaration.'
        var_name = token.value
        token = self.next_token()
        if token is TokenAtom.EQUALS:
            assert token is TokenAtom.EQUALS, 'Expected "=" after variable name.'
            token = self.next_token()
            expression = self.parse_expression()
            return (var_type, var_name, [expression])
        return (var_type, var_name, [])


def print_node(node: Node, tab_level: int = 0) -> None:
    print(f'{"  " * tab_level}{node}')
    if len(node.children) > 0:
        for child_node in node.children:
            print_node(child_node, tab_level + 1)
