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

#include <stdint.h>
#include <stddef.h>

extern void	*calloc(size_t nmemb, size_t size);

#endif
