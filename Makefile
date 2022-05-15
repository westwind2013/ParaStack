CC 				:= mpicc

SRC_PATH := src
INC_PATH := includes
OBJ_PATH := obj

LIBUNWIND_INC_PATH := /usr/include/x86_64-linux-gnu
LIBUNWIND_LIB_PATH := /usr/lib/x86_64-linux-gnu

SOURCES := $(wildcard $(SRC_PATH)/*.c)
OBJECTS := $(SOURCES:$(SRC_PATH)/%.c=$(OBJ_PATH)/%.o) 

CFLAGS := -I $(LIBUNWIND_INC_PATH) -I $(INC_PATH)  
LDFLAGS := -L$(LIBUNWIND_LIB_PATH) -lunwind-x86_64 -lunwind-ptrace -lm

# build all
all: 				stack move

stack: 				$(OBJECTS) 
	$(CC)  -o $@ $^ -O3 $(LDFLAGS) 

$(OBJECTS): $(OBJ_PATH)/%.o : $(SRC_PATH)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

move:
	mv $(objects) $(OBJ_PATH)

# build an executable to do stack trace on a specific process
test: 				unwind move2

move2:
	mv $(objects2) $(OBJ_PATH)

objects2				:= h_process.o h_unwind.o h_basic.o 

unwind: 			$(objects2)
	 $(CC) $(objects2) -O3 -o unwind -lmpich  -lunwind-x86_64 -lunwind-ptrace


.PHONY:				clean
clean: 
	-rm $(OBJ_PATH)/*.o
