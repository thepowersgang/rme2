
RME_OBJ := rme.o prefixes.o ops_alu.o ops_stack.o ops_io.o ops_mov.o
RME_OBJ += ops_string.o ops_jump.o ops_call.o ops_flags.o ops_misc.o
RME_BIN = rme.a

RME_OBJ := $(addprefix obj/,$(RME_OBJ))
RME_DEPS := $(RME_OBJ:%.o=%.dep)
RME_DEPS := $(filter %.dep,$(RME_DEPS))

CFLAGS = -Wall -O2 -Wextra -Wno-unused-parameter
CFLAGS += -Werror=implicit-function-declaration
CPPFLAGS = -I .

all: $(RME_BIN)

clean:
	$(RM) $(RME_BIN) $(RME_OBJ) $(RME_DEPS)

$(RME_BIN): $(RME_OBJ)
	$(AR) cur $(RME_BIN) $(RME_OBJ)

obj/%.o: %.c Makefile
	@mkdir -p obj/
	$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS) -MD -MT $@ -MF obj/$*.dep

-include $(RME_DEPS)
