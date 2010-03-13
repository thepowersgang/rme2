
CC = gcc
AS = nasm
LD = ld
RM = rm -f

ASFLAGS = -felf
CPPFLAGS = -I.
CFLAGS = -Wall -Werror -fno-stack-protector -O3
LDFLAGS = -T link.ld

OBJ = start.ao main.o debug.o heap.o lib.o ../src/rme.o
BIN = ../rmetest.bin

.PHONY: all clean

all:	$(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

$(BIN):	$(OBJ)
	$(LD) $(LDFLAGS) -o $(BIN) $(OBJ)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

%.ao: %.asm
	$(AS) -o $@ $< $(ASFLAGS)
