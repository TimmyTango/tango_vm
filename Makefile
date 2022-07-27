CC = gcc
CC_FLAGS = -Wall -Wextra -std=c11
PY = venv/bin/python3
LINK_FLAGS = -lSDL2
SRC_DIR = src
MACHINE = game_console
OBJ = bin/${MACHINE}.o bin/vm_cpu.o 

tangovm: ${SRC_DIR}/main.c ${OBJ}
	${CC} ${CC_FLAGS} ${LINK_FLAGS} $^ -o $@

bin/%.o: src/%.c
	${CC} -c -o $@ $< ${CC_FLAGS}

bin/${MACHINE}.o: src/systems/${MACHINE}/vm_system.c
	${CC} -c -o $@ $< ${CC_FLAGS}

asm_test: programs/test.rom

programs/tiles.rom: assets/tiles.png tools/png_conv.py
	${PY} tools/png_conv.py assets/tiles.png -o programs/tiles.rom

programs/test.rom: programs/test.asm tools/assembler.py programs/tiles.rom
	${PY} tools/assembler.py programs/test.asm -o programs/test.rom -l programs/tiles.rom

test: asm_test tangovm
	./tangovm programs/test.rom

clean:
	rm -r bin/*.o

compile:
	${PY} -m tools.compiler programs/test.tango