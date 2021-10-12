CC = gcc
CC_FLAGS = -Wall -Wextra -std=c11
PY = python3
LINK_FLAGS = ""
SRC_DIR = src
OBJ = bin/vm_cpu.o

bin/%.o: src/%.c
	${CC} -c -o $@ $< ${CC_FLAGS}

tangovm: ${SRC_DIR}/main.c ${OBJ}
	${CC} ${CC_FLAGS} ${LINK_FLAGS} $^ -o $@

asm_test: programs/test.bin

programs/test.bin: programs/test.asm
	python3 tools/assembler.py programs/test.asm -o programs/test.bin

test: asm_test tangovm
	./tangovm programs/test.bin

clean:
	rm -r bin/*.o