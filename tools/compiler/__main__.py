import argparse

from tools.compiler.lexer import lex
from tools.compiler.parser import parse, print_node

parser = argparse.ArgumentParser(description='Compiler for Tango language')
parser.add_argument('source_file', metavar='source_file', type=str, help='Tango source file to compile')
parser.add_argument('-v', '--verbose', dest='verbose', action='store_true')
parser.add_argument('-o', '--out', dest='out_file', metavar='output_file', default='compiled.asm')
args = parser.parse_args()

with open(args.source_file, 'r') as source_file:
    source = source_file.read()

if len(source) == 0:
    exit(1)

tokens = lex(source)
ast = parse(tokens)
print_node(ast)
