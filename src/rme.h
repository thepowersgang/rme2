/*
 * Realmode Emulator Plugin
 * - By John Hodge (thePowersGang)
 *
 * This code is published under the FreeBSD licence
 * (See the file COPYING for details)
 *
 * ---
 * Core Emulator Include
 */
#ifndef _RME_H_
#define _RME_H_

#include <rme_config.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifndef NULL
# define NULL ((void*)0)
#endif

/**
 * \file rme.h
 * \brief Realmode Emulator Header
 * \author John Hodge (thePowersGang)
 *
 * \section using Using RME
 *
 */


/**
 * \brief Enable the use of size overrides
 * \note Disabling this will speed up emulation, but may cause undefined
 *       behavior with some BIOSes.
 * 
 * If set to -1, size overrides will cause a \#UD
 */
#define USE_SIZE_OVERRIDES	1

/**
 * \brief Use the magic breakpoint (XCHG (e)bx, (e)bx)
 */
#define USE_MAGIC_BREAK	1

/**
 * \brief Size of a memory block
 * \note Feel free to edit this value, just make sure it stays a power
 *       of two.
 */
#define RME_BLOCK_SIZE	0x1000

/**
 * \brief Magic return Instruction Pointer
 */
#define RME_MAGIC_IP	0xFFFF
/**
 * \brief Magic return Code Segment
 */
#define RME_MAGIC_CS	0xFFFF

#define RME_HLE_CS	0xB800

/**
 * \brief Error codes returned by ::RME_Call and ::RME_CallInt
 */
enum eRME_Errors
{
	RME_ERR_OK,	//!< Exited successfully
	RME_ERR_CONTINUE,	//!< Internal non-error, continue decoding
	
	RME_ERR_INVAL,	//!< Bad paramater passed to emulator
	RME_ERR_BADMEM,	//!< Emulator accessed invalid memory
	RME_ERR_UNDEFOPCODE,	//!< Undefined opcode
	RME_ERR_DIVERR,	//!< Divide error
	RME_ERR_BUG,	//!< Bug in the emulator
	RME_ERR_BREAKPOINT,	//!< Breakpoint hit
	
	RME_ERR_HALT,	//!< CPU Halted
	RME_ERR_FCNRET,	//!< Magic CS/IP Reached, function return
	
	RME_ERR_LAST	//!< Last Error
};


/**
 * \brief FLAGS Register Values
 * \{
 */
#define FLAG_CF	0x001	//!< Carry Flag
#define FLAG_PF	0x004	//!< Parity Flag
#define FLAG_AF	0x010	//!< Adjust Flag
#define FLAG_ZF	0x040	//!< Zero Flag
#define FLAG_SF	0x080	//!< Sign Flag
#define FLAG_TF	0x100	//!< Trap Flag (for single stepping)
#define FLAG_IF	0x200	//!< Interrupt Flag
#define FLAG_DF	0x400	//!< Direction Flag
#define FLAG_OF	0x800	//!< Overflow Flag
/**
 * \}
 */

typedef union uGPR
{
	#if USE_SIZE_OVERRIDES == 1
	uint32_t	D;
	#endif
	uint16_t	W;
	struct {
		uint8_t	L;
		uint8_t	H;
	}	B;
}	tGPR;

enum eRME_CPUType
{
	RME_CPU_8086,
	RME_CPU_80286,
	RME_CPU_386
};

struct sRME_State;

typedef struct sRME_Callbacks
{
	/**
	 * \brief Print debug/logging text
	 */
	void (*PrintDebug)(struct sRME_State* State, const char* fmt, va_list args);
	/**
	 * \brief Print an error message
	 */
	void (*PrintError)(struct sRME_State* State, const char* fmt, va_list args);
	/**
	 * \brief High-Level Emulation Callbacks, one per interrupt
	 * \param State	Emulation state at the interrupt
	 * \param IntNum	Interrupt number
	 * \return Error code (see `eRME_Errors`)
	 * 
	 * Called when execution reaches 0xB800:00xx (i.e. executing the VGA text buffer)
	 */
	 int	(*HLECallbacks[256])(struct sRME_State *State, int IntNum);
	
	/**
	 * \brief Handle a memory read of unmapped (NULL-backed) memory (e.g. a device)
	 */
	 int (*Read)(struct sRME_State* State, uint32_t Addr, size_t Size, void* Dst);
	/**
	 * \brief Handle a memory read of unmapped (NULL-backed) memory (e.g. a device)
	 */
	 int (*Write)(struct sRME_State* State, uint32_t Addr, size_t Size, const void* Src);

	/**
	 * \brief Handle an `IN[BWL]` opcode
	 * \param State Emulation state
	 * \param Addr	IO bus address
	 * \param Size	Size of the operation, must be 1, 2, or 4
	 * \param Dst	Destination buffer, will be have a size size and alignment of `Size`
	 */
	int	(*In)(struct sRME_State* State, uint16_t Addr, size_t Size, void* Dst);
	/**
	 * \brief Handle an `OUT[BWL]` opcode
	 * \param State Emulation state
	 * \param Addr	IO bus address
	 * \param Size	Size of the operation, must be 1, 2, or 4
	 * \param Value	Value to write to the bus
	 */
	int	(*Out)(struct sRME_State* State, uint16_t Addr, size_t Size, uint32_t Value);
} tRME_Callbacks;

/**
 * \brief Emulator state structure
 */
typedef struct sRME_State
{
	/// @brief Arbitrary context pointer for use by callbacks
	void*	Context;
	/// @brief Callbacks out of the emulator
	const tRME_Callbacks*	Callbacks;

	//! \brief General Purpose Registers
	//! \{
	union {
		struct {
			tGPR	AX, CX, DX, BX, SP, BP, SI, DI;
		};
		tGPR	GPRs[8];
	};
	
	//! \}

	//! \brief Segment Registers
	//! \{
	uint16_t	SS;	//!< Stack Segment
	uint16_t	DS;	//!< Data Segment
	uint16_t	ES;	//!< Extra Segment
	uint16_t	FS;	//!< Extra Segment 2
	uint16_t	GS;	//!< Extra Segment 3
	//! \}

	//! \brief Program Counter
	//! \{
	uint16_t	CS;	//!< Code Segment
	uint16_t	IP;	//!< Instruction Pointer
	//! \}

	uint16_t	Flags;	//!< State Flags

	/// Emulated CPU variant
	enum eRME_CPUType	CPUType;
	/// Emulator debug level (0: off, 1: print instructions, 2: dump register state)
	 int	DebugLevel;

	/**
	 * \brief Emulator's Memory
	 *
	 * The ~1MiB realmode address space is broken into blocks of
	 * ::RME_BLOCK_SIZE bytes that can each point to different areas
	 * of memory.
	 * NOTE: There is no write protection on these blocks
	 * \note A value of NULL in a block indicates that the block is invalid
	 * \note 0x110000 bytes is all that is accessible using the realmode
	 *       segmentation scheme (true max is 0xFFFF0+0xFFFF = 0x10FFEF)
	 */
	void	*Memory[0x110000/RME_BLOCK_SIZE];	// 1Mib+64KiB in 4 KiB blocks
	/**
	 * Flags indicating that a memory block has been touched (not necessarily written)
	 */
	uint8_t	MemoryTouched[0x110000/RME_BLOCK_SIZE];

	unsigned	InstrNum;	//!< Total executed instructions

	// --- Decoder State ---
	/// Was the last instruction `00 00`, used to spot execution of zeroed memory
	 int	bWasLastOperationNull;
	/**
	 * \brief Decoder state for a single instruction
	 * \note Should not be touched except by the emulator
	 */
	struct {
		uint8_t	OverrideSegment;	// 0: Unset, otherwise it's the SREG_* value plus 1
		uint8_t	RepeatType;			// 0 if unset, otherwise it's the repeat prefix byte/opcode
		bool	bOverrideOperand;	// Operand size override provided
		bool	bOverrideAddress;	// Address size override provided
		bool	bDontChangeIP;	// Don't change IP after the instruction is executed
		uint8_t	IPOffset;	// Length of the current instruction sequence

		uint8_t	DebugStringLen;
		char	DebugString[64];	// Debug text
	}	Decoder;

}	tRME_State;


/**
 * \brief Creates a blank RME instance
 */
extern tRME_State *RME_CreateState(tRME_Callbacks* Callbacks, void* Context);
/**
 * \brief Initialise a pre-allocated RME instance
 */
extern void RME_InitState(tRME_State *State, tRME_Callbacks* Callbacks, void* Context);

/**
 * \brief Run one instruction
 * \param State	State returned from ::RME_CreateState
 * \retval 0	No error
 * \retval RME_ERR_FCNRET	RME_MAGIC_CS:RME_MAGIC_IP was reached
 */
extern int	RME_RunOne(tRME_State *State);

/**
 * \brief Calls an interrupt
 * \param State	State returned from ::RME_CreateState
 * \param Num	Interrupt number
 */
extern int	RME_CallInt(tRME_State *State, int Num);

/**
 * \brief Executes the emulator until RME_MAGIC_CS:RME_MAGIC_IP is reached
 * \param State	State returned from ::RME_CreateState
 */
extern int	RME_Call(tRME_State *State);

/**
 * \brief Prints contents of the state's registers to debug
 * \param State	State returned from ::RME_CreateState
 */
extern void RME_DumpRegs(tRME_State *State);

struct sRME_MemRef {
	/// @brief Pointer to the first (and maybe only) memory range used
	void* range_1;
	/// @brief Number of bytes in the first range (behind `range_1`), any subsequent requested bytes are behind `range_2`
	size_t len_1;
	/// @brief Pointer to the optional second memory range
	void* range_2;
};
/**
 * \brief Convert a segment-offset pointer into a pointer into emulated memory
 * \param State Emulator state (from ::RME_CreateState)
 * \param Seg	Segment selector/number
 * \param Ofs	Offset in segment
 * \param Len	Length of the requested range (must be less than [RME_BLOCK_SIZE])
 * \return Zero on success, non-zero on error  (see eRME_Errors)
 */
extern int RME_GetPtr(tRME_State *State, uint16_t Seg, uint32_t Ofs, uint16_t Len, struct sRME_MemRef* Out);

/*
 * Definitions specific to the internals of the emulator
 */
#ifdef _RME_C_
# include "rme_internal.h"
#endif

#endif
