from lex import *
from emit import *
from parse import *
import sys
import subprocess

def main():
    print("Teeny Tiny Compiler")

    if len(sys.argv) != 2:
        sys.exit("Error: Compiler needs source file as argument.")
    with open(sys.argv[1], 'r') as input_file:
        input = input_file.read()

    lexer = Lexer(input)
    emitter = Emitter("out.c")
    parser = Parser(lexer, emitter)

    parser.program()
    emitter.write_file()

    binary_name = sys.argv[1].split(".")[0]

    subprocess.run(["clang-format", "-i", "out.c"])
    result = subprocess.run(["gcc", "out.c", "-o", binary_name, "-Os"])
    if result.returncode != 0:
        print("Could not compile code")
        sys.exit(1)


if __name__ == '__main__':
    main()