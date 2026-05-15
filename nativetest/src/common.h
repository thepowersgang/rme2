#ifndef common_h_
#define common_h_

#include <stdint.h>

typedef uint16_t	Uint16;
typedef int16_t	Sint16;
typedef int8_t	Sint8;

#define PACKED	__attribute__((packed))

struct sRME_State;
struct FAR_PTR
{
	uint16_t	Offset;
	uint16_t	Segment;
};

// === IMPORTS ===
extern int LoadDosExe(struct sRME_State *state, const char *file);

#endif

