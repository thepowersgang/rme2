
OBJ := rme.o prefixes.o ops_alu.o ops_stack.o ops_io.o ops_mov.o
OBJ += ops_string.o ops_jump.o ops_call.o ops_flags.o ops_misc.o
BIN = rme.a

CFLAGS = -Wall
CPPFLAGS = -I .

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

$(BIN): $(OBJ)
	$(AR) cur $(BIN) $(OBJ)
