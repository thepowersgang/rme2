/*
 * Realmode Emulator Plugin
 * - By John Hodge (thePowersGang)
 *
 * This code is published under the FreeBSD licence
 * (See the file COPYING for details)
 *
 * ---
 * Sample Emulator Configuration Include
 */
#ifndef _RME_CONFIG_H_
#define _RME_CONFIG_H_

#define DEBUG	1	// Enable debug? (2 enables a register dump)
#define ERR_OUTPUT	1	// Enable using printf on an error?

#include <stdio.h>	// printf
#include <stdlib.h>	// calloc
#include <stdint.h>

extern uint8_t	inb(uint16_t port);
extern uint16_t	inw(uint16_t port);
extern uint32_t	inl(uint16_t port);
extern void	outb(uint16_t port, uint8_t val);
extern void	outw(uint16_t port, uint16_t val);
extern void	outl(uint16_t port, uint32_t val);

#endif
