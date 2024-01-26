
RMEDIR = ../src/

ENABLE_GUI :=

CPPFLAGS := -I . -I $(RMEDIR)
CFLAGS	:= $(CPPFLAGS) -O3 -g -std=gnu99
LDFLAGS	:= -g
ifneq ($(ENABLE_GUI),)
CPPFLAGS += -D ENABLE_GUI=1
LDFLAGS += -lSDL -lSDLmain
endif

RME_LIB = $(RMEDIR)rme.a

OBJ := main.o dosexe.o
#OBJ += $(RMEDIR)rme_composite.o
OBJ := $(OBJ:%=.obj/%)
BIN := bin/tester

DEPFILES := $(OBJ:%.o=%.dep)
DEPFILES := $(filter %.dep,$(DEPFILES))

.PHONY: all clean

all:	$(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(DEPFILES)

$(BIN): $(OBJ) $(RME_LIB)
	@mkdir -p $(dir $@)
	$(CC) -o $(BIN) $(OBJ) $(RME_LIB) $(LDFLAGS)

.obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -o $@ -c $< $(CFLAGS) -MP -MD -MT $@ -MF .obj/$*.dep

$(RME_LIB): $(wildcard $(RMEDIR)*.c $(RMEDIR)*.h)
	$(MAKE) -C $(RMEDIR)

-include $(DEPFILES)
