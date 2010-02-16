/*
 * Realmode Emulator Test Kernel
 * - By John Hodge
 * This file has been released into the public domain
 * 
 * - Debug IO
 */
#include <stdarg.h>
#include "common.h"

#define DEBUG_TO_SERIAL	1
#define SERIAL_PORT	0x3F8

// === CODE ===
void Debug_Init()
{
	#if DEBUG_TO_SERIAL
	outb(SERIAL_PORT + 1, 0x00);    // Disable all interrupts
	outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outb(SERIAL_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outb(SERIAL_PORT + 1, 0x00);    //                  (hi byte)
	outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
	outb(SERIAL_PORT + 2, 0xC7);    // Enable FIFO with 14-byte threshold and clear it
	outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
	#endif
}

int putchar(int ch)
{
	__asm__ __volatile__ ("outb %%al, $0xE9"::"a"(ch));
	
	#if DEBUG_TO_SERIAL
	while( (inb(SERIAL_PORT + 5) & 0x20) == 0 );
	outb(SERIAL_PORT, ch);
	#endif
	
	return ch;
}

const char cHEXCHARS[] = "0123456789ABCDEF";

/**
 * \brief Convert a integer into a string
 */
int itoa(char *buf, uint64_t val, int base, int minLength, char pad)
{
	char	tmp[65];
	 int	i = 0, j;
	
	do
	{
		tmp[i++] = cHEXCHARS[val%base];
		val /= base;
	} while(val);	// Makes sure at least one character is printed
	
	// Pad
	while(i < minLength)	tmp[i++] = pad;
	// Reverse
	j = 0;
	while(i--)	buf[j++] = tmp[i];
	buf[j] = '\0';
	return j;
}

/**
 * \brief Print a string to the debug stream (and append a newline)
 */
int puts(const char *str)
{
	 int	num = 0;
	for(num=0;*str;str++,num++)
	{
		putchar(*str);
	}
	putchar('\n');
	return num;
}

/**
 * \brief Print formatted
 */
int printf(const char *fmt, ...)
{
	va_list	args;
	char	pad;
	 int	minLen;
	char	*str;
	char	buf[65];
	uint64_t	val;
	 int	num = 0;
	
	va_start(args, fmt);
	
	for( ; *fmt; fmt++)
	{
		if(*fmt != '%')
		{
			putchar( *fmt );
			num ++;
			continue;
		}
		
		// Padding
		fmt ++;
		if(*fmt == '0') {
			pad = '0';
			fmt ++;
		} else
			pad = ' ';
		
		// Minimum Length
		minLen = 0;
		if('1' <= *fmt && *fmt <= '9')
		{
			for( ; '0' <= *fmt && *fmt <= '9'; fmt++)
				minLen = minLen*10 + *fmt-'0';
		}
		
		// TODO: 64-bit
		
		// Get value
		val = va_arg(args, uint32_t);
		
		// Format Code
		switch(*fmt)
		{		
		case 'o':
			itoa(buf, val, 8, minLen, pad);
			str = buf;
			goto puts;
		
		case 'i':
			itoa(buf, val, 10, minLen, pad);
			str = buf;
			goto puts;
		
		case 'x':
			itoa(buf, val, 16, minLen, pad);
			str = buf;
			goto puts;
		
		case 'p':
			putchar('*');	num ++;
			putchar('0');	num ++;
			putchar('x');	num ++;
			itoa(buf, val, 16, 8, '0');
			str = buf;
			goto puts;
		
		case 's':
			str = (char*)(intptr_t)val;
			if(!str)	str = "(null)";
		puts:
			while(*str)
			{
				putchar( *str++ );
				num ++;
			}
			break;
		}
	}
	
	va_end(args);
	return num;
}
