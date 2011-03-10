
CPPFLAGS := -I . -I ../../src_new
CFLAGS	:= $(CPPFLAGS) -O3 -g
LDFLAGS	:= -g -lSDL -lSDLmain

RME_LIB = ../../src_new/rme.a

OBJ	:= main.o
#OBJ += ../../src_new/rme_composite.o
OBJ += $(RME_LIB)
BIN	:= ../tester

DEPFILES := $(OBJ:%.o=%.dep)
DEPFILES := $(filter %.dep,$(DEPFILES))

.PHONY: all clean

all:	$(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(DEPFILES)

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS) -MD -MT $@ -MF $*.dep

.PHONY: $(RME_LIB)
$(RME_LIB):
	$(MAKE) -C ../../src_new

-include $(DEPFILES)
