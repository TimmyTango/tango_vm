CC = gcc
CC_FLAGS = -Wall -Wextra -std=c11
LINK_FLAGS = 
SRC_DIR = src
OBJ = bin/scanner.o

tangoasm: ${SRC_DIR}/asm.c ${OBJ}
	${CC} ${CC_FLAGS} ${LINK_FLAGS} $^ -o $@

bin/%.o: src/%.c
	${CC} -c -o $@ $< ${CC_FLAGS}

test: tangoasm
	./tangoasm ../programs/test2.asm