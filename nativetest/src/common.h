#ifndef common_h_
#define common_h_

#include <stdint.h>

typedef uint16_t	Uint16;
typedef int16_t	Sint16;
typedef int8_t	Sint8;

#define PACKED	__attribute__((packed))

typedef struct {
	uint16_t	Offset;
	uint16_t	Segment;
} PACKED	t_farptr;


#endif

