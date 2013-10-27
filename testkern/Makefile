
CC = i586-elf-gcc
AS = nasm
LD = i586-elf-ld
RM = rm -f

ASFLAGS = -felf
CPPFLAGS = -I. -ffreestanding
CFLAGS = -Wall -fno-stack-protector -O3 -std=gnu99
LDFLAGS = -T link.ld

OBJ = start.ao main.o debug.o heap.o lib.o rme_composite.o
BIN = ../rmetest.bin

OBJ := $(OBJ:%=obj/%)

.PHONY: all clean

all:	$(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

$(BIN):	$(OBJ)
	$(LD) $(LDFLAGS) -o $(BIN) $(OBJ)

obj/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS) -MMD -MP

obj/rme_composite.o: ../src/rme_composite.c
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS) -MMD -MP

obj/%.ao: %.asm
	mkdir -p $(dir $@)
	$(AS) -o $@ $< $(ASFLAGS)

-include $(OBJ:%=%.d)
