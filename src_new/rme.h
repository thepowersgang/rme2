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

#include <stdint.h>

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
	
	RME_ERR_LAST	//!< Last Error
};


/**
 * \brief FLAGS Register Values
 * \{
 */
#define FLAG_CF	0x001	//!< Carry Flag
#define FLAG_PF	0x004	//!< Pairity Flag
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

/**
 * \brief Emulator state structure
 */
typedef struct sRME_State
{
	//! \brief General Purpose Registers
	//! \{
	tGPR	AX, CX, DX, BX, SP, BP, SI, DI;
	
	//! \}

	//! \brief Segment Registers
	//! \{
	uint16_t	SS;	//!< Stack Segment
	uint16_t	DS;	//!< Data Segment
	uint16_t	ES;	//!< Extra Segment
	//! \}

	//! \brief Program Counter
	//! \{
	uint16_t	CS;	//!< Code Segment
	uint16_t	IP;	//!< Instruction Pointer
	//! \}

	uint16_t	Flags;	//!< State Flags

	/**
	 * \brief Emulator's Memory
	 *
	 * The ~1MiB realmode address space is broken into blocks of
	 * ::RME_BLOCK_SIZE bytes that can each point to different areas
	 * of memory.
	 * NOTE: There is no write protection on these blocks
	 * \note A value of NULL in a block indicates that the block is invalid
	 * \note 0x110000 bytes is all that is accessable using the realmode
	 *       segmentation scheme (true max is 0xFFFF0+0xFFFF = 0x10FFEF)
	 */
	uint8_t	*Memory[0x110000/RME_BLOCK_SIZE];	// 1Mib,64KiB in 256 4 KiB blocks

	/**
	 * \brief High-Level Emulation Callback
	 * \param State	Emulation state at the interrupt
	 * \param IntNum	Interrupt number
	 * \return 1 if the call was handled, 0 if it should be emulated
	 * 
	 * Called on all in-emulator INT calls
	 */
	 int	(*HLECallbacks[256])(struct sRME_State *State, int IntNum);

	 int	InstrNum;	//!< Total executed instructions

	// --- Decoder State ---
	/**
	 * \brief Decoder State
	 * \note Should not be touched except by the emulator
	 */
	struct {
		 int	OverrideSegment;	// -1: Unset
		 int	RepeatType;
		 int	bOverrideOperand;	// Operand size override provided
		 int	bOverrideAddress;	// Address size override provided
		 int	bDontChangeIP;	// Don't change IP after the instruction is executed
		 int	IPOffset;
	}	Decoder;
}	tRME_State;


/**
 * \brief Creates a blank RME instance
 */
extern tRME_State	*RME_CreateState(void);

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

/*
 * Definitions specific to the internals of the emulator
 */
#ifdef _RME_C_
# include "rme_internal.h"
#endif

#endif
