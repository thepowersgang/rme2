/*
 * Realmode Emulator
 * - ALU Operations
 * 
 */
#include "rme.h"
#include "rme_internal.h"
#include "opcode_prototypes.h"
#include "ops_alu.h"

// === CODE ===
CREATE_ALU_OPCODE_FCN(ADD, ALU_OPCODE_ADD_CODE)
CREATE_ALU_OPCODE_FCN(OR , ALU_OPCODE_OR_CODE)
CREATE_ALU_OPCODE_FCN(ADC, ALU_OPCODE_ADC_CODE)
CREATE_ALU_OPCODE_FCN(SBB, ALU_OPCODE_SBB_CODE)
CREATE_ALU_OPCODE_FCN(AND, ALU_OPCODE_AND_CODE)
CREATE_ALU_OPCODE_FCN(SUB, ALU_OPCODE_SUB_CODE)
CREATE_ALU_OPCODE_FCN(XOR, ALU_OPCODE_XOR_CODE)
CREATE_ALU_OPCODE_FCN(CMP, ALU_OPCODE_CMP_CODE)

CREATE_ALU_OPCODE_FCN_AI(TEST, ALU_OPCODE_TEST_CODE)
CREATE_ALU_OPCODE_FCN_AIX(TEST, ALU_OPCODE_TEST_CODE)

#define ALU_SELECT_OPERATION() do { switch( op_num ) { \
	case 0: do { ALU_OPCODE_ADD_CODE } while(0);	break; \
	case 1: do { ALU_OPCODE_OR_CODE  } while(0);	break; \
	case 2: do { ALU_OPCODE_ADC_CODE } while(0);	break; \
	case 3: do { ALU_OPCODE_SBB_CODE } while(0);	break; \
	case 4: do { ALU_OPCODE_AND_CODE } while(0);	break; \
	case 5: do { ALU_OPCODE_SUB_CODE } while(0);	break; \
	case 6: do { ALU_OPCODE_XOR_CODE } while(0);	break; \
	case 7: do { ALU_OPCODE_CMP_CODE } while(0);	break; \
	} } while(0)

DEF_OPCODE_FCN(Arith, RI)
{
	 int	ret;
	const int width = 8;
	uint8_t	srcData;
	uint8_t *dest, *src = &srcData;
	 int	op_num;
	
	// Get mmm term from ModRM byte
	READ_INSTR8( srcData );	State->Decoder.IPOffset --;
	op_num = (srcData >> 3) & 7;
	
	// Get operation name
	DEBUG_S("%s", casArithOps[op_num]);
	ret = RME_Int_ParseModRM(State, NULL, &dest, 0);
	if(ret)	return ret;
	
	// Read data, perform operation, set common flags
	READ_INSTR8(srcData);
	ALU_SELECT_OPERATION();
	SET_COMM_FLAGS(State, *dest, width);
	
	return 0;
}

DEF_OPCODE_FCN(Arith, RIX)
{
	 int	ret;
	 int	op_num;
	uint8_t	byte;
	void	*destPtr;
	
	// Read mmm from ModRM
	READ_INSTR8( byte );	State->Decoder.IPOffset --;
	op_num = (byte >> 3) & 7;
	
	DEBUG_S("%s", casArithOps[op_num]);
	ret = RME_Int_ParseModRMX(State, NULL, (void*)&destPtr, 0);
	if(ret)	return ret;
	State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	srcData, *src = &srcData, *dest = destPtr;
		const int	width = 32;
		READ_INSTR32(srcData);
		
		ALU_SELECT_OPERATION();
		SET_COMM_FLAGS(State, *dest, width);
		State->Flags |= (*dest == 0xFFFFFFFF) ? FLAG_OF : 0;
	}
	else
	{
		uint16_t	srcData, *src = &srcData, *dest = destPtr;
		const int	width = 16;
		READ_INSTR16(srcData);
		
		ALU_SELECT_OPERATION();
		SET_COMM_FLAGS(State, *dest, width);
	}
	
	return 0;
}

DEF_OPCODE_FCN(INC, Reg)
{
	State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	*dest = (void*)RegW(State, Param);
		(*dest) ++;
		SET_COMM_FLAGS(State, *dest, 32);
	}
	else
	{
		uint16_t	*dest = RegW(State, Param);
		(*dest) ++;
		SET_COMM_FLAGS(State, *dest, 16);
	}
	
	if(State->Flags & FLAG_ZF)
		State->Flags |= FLAG_OF;
	return 0;
}

DEF_OPCODE_FCN(DEC, Reg)
{
	State->Flags &= ~(FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF);
	if( State->Decoder.bOverrideOperand )
	{
		uint32_t	*dest = (void*)RegW(State, Param);
		(*dest) --;
		SET_COMM_FLAGS(State, *dest, 32);
		State->Flags |= (*dest == 0xFFFFFFFF) ? FLAG_OF : 0;
	}
	else
	{
		uint16_t	*dest = RegW(State, Param);
		(*dest) --;
		SET_COMM_FLAGS(State, *dest, 16);
		State->Flags |= (*dest == 0xFFFF) ? FLAG_OF : 0;
	}
	return 0;
}
