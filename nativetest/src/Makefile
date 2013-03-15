
RMEDIR = ../../src/

CPPFLAGS := -I . -I $(RMEDIR)
CFLAGS	:= $(CPPFLAGS) -O3 -g
LDFLAGS	:= -g -lSDL -lSDLmain

RME_LIB = $(RMEDIR)rme.a

OBJ := main.o dosexe.o
#OBJ += $(RMEDIR)rme_composite.o
OBJ += $(RME_LIB)
BIN := ../tester

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
	$(MAKE) -C $(RMEDIR) CPPFLAGS=$(CPPFLAGS)

-include $(DEPFILES)
