
RMEDIR = ../src/

ENABLE_GUI ?=
OBJDIR_SUF := 

CPPFLAGS := -I . -I $(RMEDIR)
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

RME_LIB = $(RMEDIR)rme.a

OBJ := main.o dosexe.o
#OBJ += $(RMEDIR)rme_composite.o
OBJ := $(OBJ:%=.obj$(OBJDIR_SUF)/%)
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

.obj$(OBJDIR_SUF)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -o $@ -c $< $(CFLAGS) -MP -MD -MT $@ -MF .obj/$*.dep

$(RME_LIB): $(wildcard $(RMEDIR)*.c $(RMEDIR)*.h)
	$(MAKE) -C $(RMEDIR)

-include $(DEPFILES)
