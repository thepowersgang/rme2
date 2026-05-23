//
//
#pragma once
#include <stdint.h>
#include <stdbool.h>

struct sRME_State;
extern void VGA_Resync(struct sRME_State* State);
extern int VGA_IoPort_In(struct sRME_State* State, uint16_t Port, int Size, void* Dst);
extern int VGA_IoPort_Out(struct sRME_State* State, uint16_t Port, int Size, uint32_t Val);
