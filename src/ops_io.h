/*
 * Realmode Emulator
 * - IO Operations Header
 * 
 */
#ifndef _RME_OPS_IO_H_
#define _RME_OPS_IO_H_

#define ENSURE_IN(State) do {\
	if( !State->IoCallbacks.In ) { \
		RME_Int_ErrorPrint(State, "`in` instruction with no IO callback provided"); \
		return RME_ERR_INVAL; \
	} \
} while(0)
static inline int inB(tRME_State* State, uint16_t Port, uint8_t* Val) {
	ENSURE_IN(State);
	return State->IoCallbacks.In(State, Port, 1, Val);
}
static inline int inW(tRME_State* State, uint16_t Port, uint16_t* Val) {
	ENSURE_IN(State);
	return State->IoCallbacks.In(State, Port, 2, Val);
}
static inline int inD(tRME_State* State, uint16_t Port, uint32_t* Val) {
	ENSURE_IN(State);
	return State->IoCallbacks.In(State, Port, 4, Val);
}
#define ENSURE_OUT(State) do {\
	if( !State->IoCallbacks.Out ) { \
		RME_Int_ErrorPrint(State, "`out` instruction with no IO callback provided"); \
		return RME_ERR_INVAL; \
	} \
} while(0)
static inline int outB(tRME_State* State, uint16_t Port, uint8_t Val) {
	ENSURE_OUT(State);
	return State->IoCallbacks.Out(State, Port, 1, Val);
}
static inline int outW(tRME_State* State, uint16_t Port, uint16_t Val) {
	ENSURE_OUT(State);
	return State->IoCallbacks.Out(State, Port, 2, Val);
}
static inline int outD(tRME_State* State, uint16_t Port, uint32_t Val) {
	ENSURE_OUT(State);
	return State->IoCallbacks.Out(State, Port, 4, Val);
}

#endif
