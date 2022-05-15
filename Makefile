CC 				:= mpicc

SRC_PATH := src
INC_PATH := includes
OBJ_PATH := obj
BIN_PATH := bin

LIBUNWIND_INC_PATH := /usr/include/x86_64-linux-gnu
LIBUNWIND_LIB_PATH := /usr/lib/x86_64-linux-gnu

SOURCES := ${wildcard ${SRC_PATH}/*.c}
OBJECTS := ${SOURCES:${SRC_PATH}/%.c=${OBJ_PATH}/%.o} 

CFLAGS := -I ${LIBUNWIND_INC_PATH} -I ${INC_PATH} 
LDFLAGS := -L ${LIBUNWIND_LIB_PATH} -lunwind-x86_64 -lunwind-ptrace -lm

TARGET := stack

all: ${BIN_PATH}/${TARGET} 

${BIN_PATH}/${TARGET}: ${OBJECTS} 
	@$(CC) -o $@ $^ -O3 $(LDFLAGS) 
	@echo "Linked successfully!"

$(OBJECTS): ${OBJ_PATH}/%.o : ${SRC_PATH}/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	@echo ${OBJECTS}
	@rm ${OBJECTS}
	@rm ${BIN_PATH}/${TARGET}
	@echo "Cleanup complete!"

# TODO: Refactor the following makefile rules.

# build an executable to do stack trace on a specific process
test: 				unwind move2

move2:
	mv $(objects2) $(OBJ_PATH)

objects2				:= h_process.o h_unwind.o h_basic.o 

unwind: 			$(objects2)
	 $(CC) $(objects2) -O3 -o unwind -lmpich  -lunwind-x86_64 -lunwind-ptrace
