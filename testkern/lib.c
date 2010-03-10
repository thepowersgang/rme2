/*
 * Realmode Emulator Test Kernel
 * - By John Hodge
 * This file has been released into the public domain
 * 
 * - Library Functions
 */
#include "common.h"

// === CODE ===
/**
 * \fn void *memset(void *Dest, int Val, size_t Num)
 * \brief Do a byte granuality set of Dest
 */
void *memset(void *Dest, int Val, size_t Num)
{
	__asm__ __volatile__ (
		"rep stosl;\n\t"
		"movl %3, %%ecx;\n\t"
		"rep stosb"
		:: "D" (Dest), "a" (Val), "c" (Num/4), "r" (Num&3));
	return Dest;
}
/**
 * \fn void *memset(void *Dest, const void *Src, size_t Num)
 * \brief Do a byte granuality copy of Src t Dest
 */
void *memcpy(void *Dest, const void *Src, size_t Num)
{
	__asm__ __volatile__ (
		"rep movsl;\n\t"
		"movl %3, %%ecx;\n\t"
		"rep movsb"
		:: "D" (Dest), "S" (Src), "c" (Num/4), "r" (Num&3));
	return Dest;
}

/**
 * \fn Uint64 __udivdi3(uint64_t Num, uint64_t Den)
 * \brief Divide two 64-bit integers
 */
uint64_t __udivdi3(uint64_t Num, uint64_t Den)
{
	uint64_t	ret = 0;
	
	if(Den == 0)	__asm__ __volatile__ ("int $0x0");	// Call Div by Zero Error
	if(Den == 1)	return Num;	// Speed Hacks
	if(Den == 2)	return Num >> 1;	// Speed Hacks
	if(Den == 4)	return Num >> 2;	// Speed Hacks
	if(Den == 8)	return Num >> 3;	// Speed Hacks
	if(Den == 16)	return Num >> 4;	// Speed Hacks
	if(Den == 32)	return Num >> 5;	// Speed Hacks
	if(Den == 1024)	return Num >> 10;	// Speed Hacks
	if(Den == 2048)	return Num >> 11;	// Speed Hacks
	if(Den == 4096)	return Num >> 12;
	if(Den == 0x2000)	return Num >> 13;	// Speed Hacks
	if(Den == 0x4000)	return Num >> 14;	// Speed Hacks
	if(Den == 0x8000)	return Num >> 15;	// Speed Hacks
	if(Den == 0x10000)	return Num >> 16;	// Speed Hacks
	
	if(Num >> 32 == 0 && Den >> 32 == 0)
		return (uint32_t)Num / (uint32_t)Den;
	
	//Log("__udivdi3: (Num={0x%x:%x}, Den={0x%x:%x})",
	//	Num>>32, Num&0xFFFFFFFF,
	//	Den>>32, Den&0xFFFFFFFF);
	
	while(Num > Den) {
		ret ++;
		Num -= Den;
	}
	return ret;
}

/**
 * \fn Uint64 __umoddi3(uint64_t Num, uint64_t Den)
 * \brief Get the modulus of two 64-bit integers
 */
uint64_t __umoddi3(uint64_t Num, uint64_t Den)
{
	if(Den == 0)	__asm__ __volatile__ ("int $0x0");	// Call Div by Zero Error
	if(Den == 1)	return 0;	// Speed Hacks
	if(Den == 2)	return Num & 1;	// Speed Hacks
	if(Den == 4)	return Num & 3;	// Speed Hacks
	if(Den == 8)	return Num & 7;	// Speed Hacks
	if(Den == 16)	return Num & 15;	// Speed Hacks
	if(Den == 32)	return Num & 31;	// Speed Hacks
	if(Den == 1024)	return Num & 1023;	// Speed Hacks
	if(Den == 2048)	return Num & 2047;	// Speed Hacks
	if(Den == 4096)	return Num & 4095;	// Speed Hacks
	if(Den == 0x2000)	return Num & 0x1FFF;	// Speed Hacks
	if(Den == 0x4000)	return Num & 0x3FFF;	// Speed Hacks
	if(Den == 0x8000)	return Num & 0x7FFF;	// Speed Hacks
	if(Den == 0x10000)	return Num & 0xFFFF;	// Speed Hacks
	
	if(Num >> 32 == 0 && Den >> 32 == 0)
		return (uint32_t)Num % (uint32_t)Den;
	
	while(Num > Den)
		Num -= Den;
	return Num;
}


// === Port IO
void outb(uint16_t Port, uint8_t Data)
{
	__asm__ __volatile__ ("outb %%al, %%dx"::"d"(Port),"a"(Data));
}
void outw(uint16_t Port, uint16_t Data)
{
	__asm__ __volatile__ ("outw %%ax, %%dx"::"d"(Port),"a"(Data));
}
uint8_t inb(uint16_t Port)
{
	uint8_t	ret;
	__asm__ __volatile__ ("inb %%dx, %%al":"=a"(ret):"d"(Port));
	return ret;
}
uint16_t inw(uint16_t Port)
{
	uint16_t	ret;
	__asm__ __volatile__ ("inw %%dx, %%ax":"=a"(ret):"d"(Port));
	return ret;
}


