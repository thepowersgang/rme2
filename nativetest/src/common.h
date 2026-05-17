#ifndef common_h_
#define common_h_

#include <stdint.h>
#include <stdio.h>

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
extern int HLECall21(struct sRME_State *State, int IntNum);

extern void Input_PushKeysFromChar(char ch);
extern void Video_ScrollUp(int Page, uint8_t Attr, int nLines, int Top, int Left, int Bottom, int Right);
extern void Video_Redraw(void);

extern FILE	*gaFDDs[4];
extern uint8_t	gaMemory[0x110000];
extern int	gKeyBufferPos;
extern struct sKeyBufEnt {
	uint8_t	Scancode;
	uint8_t	ASCII;
} gKeyBuffer[16];

#endif

