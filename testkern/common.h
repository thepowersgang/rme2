/*
 * Realmode Emulator Test Kernel
 * - By John Hodge
 * This file has been released into the public domain
 * 
 * - Core Header
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stddef.h>
#include <stdint.h>

// === Symbols ===
extern void	gKernelEnd;

// === Functions ===
extern int	putchar(int ch);
extern int	puts(const char *str);
extern int	printf(const char *fmt, ...);

extern void	Heap_Init();
extern void	*malloc(size_t bytes);
extern void	*calloc(size_t nmemb, size_t size);
extern void	free(void *ptr);

extern void *memset(void *dst, int val, size_t num);
extern void *memcpy(void *Dest, const void *Src, size_t Num);

extern void	outb(uint16_t Port, uint8_t Data);
extern void	outw(uint16_t Port, uint16_t Data);
extern uint8_t	inb(uint16_t Port);
extern uint16_t	inw(uint16_t Port);

#endif
