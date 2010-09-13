
CPPFLAGS := -I . -I ../../src
CFLAGS	:= $(CPPFLAGS) -O3 -g
LDFLAGS	:= -g -lSDL -lSDLmain

OBJ	:= main.o ../../src/rme.o
BIN	:= ../tester

.PHONY: all clean

all:	$(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)
