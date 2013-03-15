
CC = i586-elf-gcc
AS = nasm
LD = i586-elf-ld
RM = rm -f

ASFLAGS = -felf
CPPFLAGS = -I. -ffreestanding
CFLAGS = -Wall -fno-stack-protector -O3
LDFLAGS = -T link.ld

OBJ = start.ao main.o debug.o heap.o lib.o ../src_new/rme_composite.o
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
