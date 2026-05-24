#ifndef common_h_
#define common_h_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

typedef uint16_t	Uint16;
typedef int16_t	Sint16;
typedef int8_t	Sint8;

#define PACKED	__attribute__((packed))

typedef struct sRME_State tRME_State;
struct sKeyBufEnt
{
	uint8_t Scancode;
	uint8_t	ASCII;
};
struct FAR_PTR
{
	uint16_t	Offset;
	uint16_t	Segment;
};

// === IMPORTS ===
__attribute__((noreturn))
extern void FatalErrorF(struct sRME_State *State, const char* Fmt, ...);
extern void PrintDebugF(struct sRME_State *State, const char* Fmt, ...);

// --- DOS High-Level-Emulation calls ---
extern int LoadDosExe(struct sRME_State *state, const char *file);
extern int HLECall21(struct sRME_State *State, int IntNum);

// --- BIOS High-Level-Emulation calls ---
extern int	HLECall10(tRME_State *State, int IntNum);
extern int	HLECall12(tRME_State *State, int IntNum);
extern int	HLECall13(tRME_State *State, int IntNum);
extern int	HLECall(tRME_State *State, int IntNum);
/// @brief Pause/block emulation until a key is POSSIBLY ready in the buffer, or an error occurs
extern int	Input_WaitForKey(tRME_State* State);
/// @brief Push a key-fire event to the input queue
extern void Input_PushKey(int scancode, int ch);
/// @brief Helper to convert an ASCII character into keyboard input
/// @param ch Character
extern void Input_PushKeysFromChar(char ch);
/// @brief Check if there is a key in the buffer
/// @param out Key information if available
/// @return `true` is there is a key in the buffer. `*out` will be populated with the
extern bool Input_Peek(struct sKeyBufEnt* out);
/// @brief Remove a key from the input buffer (if there)
/// @param out Key information
/// @return `true` if a key was present
extern bool Input_Pop(struct sKeyBufEnt* out);
/// @brief Write text to the screen using the BIOS HLE
extern void Bios_PutString(const char *String, uint8_t attr);

extern FILE	*gaFDDs[4];
extern uint8_t	gaMemory[0x110000];
extern int	gKeyBufferPos;
extern struct sKeyBufEnt gKeyBuffer[16];

#endif

