
RME_DIR = ../src/

ENABLE_GUI ?=
OBJDIR_SUF := 

CPPFLAGS := -I . -I $(RME_DIR)
CFLAGS	:= $(CPPFLAGS) -O3 -g -std=gnu99
LDFLAGS	:= -g
ifneq ($(ENABLE_GUI),)
CFLAGS += -D ENABLE_GUI=1
#CFLAGS += $(shell pkg-config sdl1.2 --cflags)
#LDFLAGS += $(shell pkg-config sdl1.2 --libs)
LDFLAGS += -lSDL
OBJDIR_SUF = -gui
#LDFLAGS += /usr/lib/x86_64-linux-gnu/libSDL2-2.0.so.0
endif

RME_LIB = $(RME_DIR)rme.a

OBJ := main.o hle_dos.o hle_bios.o
OBJ += ui_tty.o
#OBJ += $(RME_DIR)rme_composite.o
OBJ := $(OBJ:%=.obj$(OBJDIR_SUF)/%)
BIN := bin/tester

DEP_FILES := $(OBJ:%.o=%.dep)
DEP_FILES := $(filter %.dep,$(DEP_FILES))

.PHONY: all clean

all:	$(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(DEP_FILES)

$(BIN): $(OBJ) $(RME_LIB)
	@mkdir -p $(dir $@)
	$(CC) -o $(BIN) $(OBJ) $(RME_LIB) $(LDFLAGS)

.obj$(OBJDIR_SUF)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -o $@ -c $< $(CFLAGS) -MP -MD -MT $@ -MF .obj/$*.dep

$(RME_LIB): $(wildcard $(RME_DIR)*.c $(RME_DIR)*.h)
	$(MAKE) -C $(RME_DIR)

-include $(DEP_FILES)
