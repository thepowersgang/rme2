/*
 * Realmode Emulator Test Kernel
 * - By John Hodge
 * This file has been released into the public domain
 * 
 * - Simple Dynamic Memory Allocator
 */
#include "common.h"

// === GLOBAL ===
void	*gHeapPtr;

// === CODE ===
void Heap_Init()
{
	gHeapPtr = &gKernelEnd;
}

void *malloc(size_t bytes)
{
	void	*ret;
	ret = gHeapPtr;
	
	gHeapPtr = (void*)( (intptr_t)gHeapPtr + ((bytes+7)&~7) );
	return ret;
}

void *calloc(size_t nmemb, size_t size)
{
	void	*ret = malloc(nmemb*size);
	memset(ret, 0, nmemb*size);
	return ret;
}

void free(void *ptr)
{
	return;
}
