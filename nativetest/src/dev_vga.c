//
//
#include "dev_vga.h"
#include "rme.h"
#include <string.h>

enum eRegs_Graphics {
    /// @brief One bit for each of the 4 planes. Used by write modes 0/3. Provides overrides for the entire byte of that plane
    REG_Graphics_SetReset = 0,
    /// @brief Enable set/reset when in write mode 0
    REG_Graphics_EnSetReset,
    /// @brief Reference colour for each plane, used in read mode 1
    REG_Graphics_ColourCompare,
    /// @brief Logical operation for write modes 0/2 (bits 3/4) and rotate count for writes modes 0/3
    ///
    /// - 0:2 Rotate count
    /// - 3:4: Logical operation
    ///   - 0: unmodified
    ///   - 1: AND with latch
    ///   - 2: OR with latch
    ///   - 3: XOR with latch
    REG_Graphics_DataRotate,
    /// @brief (Read mode 0) Selects the source plane for a read operation.
    REG_Graphics_ReadMapSelect,
    /// @brief Graphics mode register
    ///
    /// - 0:1 - Write mode
    /// - 3 - Read mode (0: Read from plane, 1: compare to colour compareregister)
    /// - 4 - Host odd/even read addressing enable
    /// - 5: Shift register interleave mode
    /// - 6: 256-colour shift mode
    REG_Graphics_GraphicsMode,
    /// @brief Misc Graphics Register
    ///
    /// - 0: Disable the character generator (switch from text to graphics mode)
    ///
    /// - 1: Chain Odd/Even Enable
    ///
    /// - 2:3: Memory map select (0: A-C, 1: A-B, 2: B-B8, 3: B8-C)
    REG_Graphics_MiscGraphics,
    /// @brief Enables comparison of a plane with `ColourCompare`
    REG_Graphics_ColourDontCare,
    /// @brief Bit mask used by write modes 0,2,3
    REG_Graphics_BitMask,
};
enum eRegs_Sequencer {
    REG_Sequencer_Reset,
    REG_Sequencer_ClockingMode,
    /// @brief One bit per plane, enables writing of data to the plane
    REG_Sequencer_MapMask,
    REG_Sequencer_CharacterMapSelect,
    /// @brief 
    REG_Sequencer_MemoryMode,
};

#define PLANE_SIZE  0x10000 // 64KiB
typedef struct sVgaState {
    // 0x3C[EF]: Graphics Registers
    uint8_t regs_graphics[9];
    uint8_t regs_sequencer[5];

    uint8_t index_graphics;

    bool memory_touched[0x20000 / RME_BLOCK_SIZE];
    uint8_t latch[4];
    // Backing planes
    uint8_t planes[4][PLANE_SIZE];
} tVgaState;

static inline tVgaState* get_state(struct sRME_State* State) {
    static tVgaState    g_state;
    return &g_state;
}

/// @brief Update internal flags of changed device memory
/// @param State 
void VGA_Resync(struct sRME_State* State)
{
    for(int a = 0xA0000; a < 0xC0000; a += RME_BLOCK_SIZE)
    {
        if( State->MemoryTouched[a] ) {
            get_state(State)->memory_touched[(a - 0xA0000) / RME_BLOCK_SIZE] = true;
        }
    }
}
uint16_t translate_addr(tVgaState* vga, uint32_t addr)
{
    uint8_t map_select = (vga->regs_graphics[REG_Graphics_MiscGraphics] >> 2) & 3;
    return addr & 0xFFFF;
}
void VGA_WriteByte(struct sRME_State* State, uint32_t addr, uint8_t v)
{
    tVgaState* vga = get_state(State);
    uint8_t map_select = (vga->regs_graphics[REG_Graphics_MiscGraphics] >> 2) & 3;
    uint8_t write_mode = (vga->regs_graphics[REG_Graphics_GraphicsMode] >> 0) & 3;
    uint8_t rotate_count = (vga->regs_graphics[REG_Graphics_DataRotate] >> 0) & 7;
    uint8_t logical_op = (vga->regs_graphics[REG_Graphics_DataRotate] >> 3) & 3;
    uint8_t bit_mask = (vga->regs_graphics[REG_Graphics_BitMask]);
    
    uint16_t ofs = translate_addr(vga, addr);
    switch(write_mode)
    {
    case 0:
        // Rotate
        v = rotate_8(v, rotate_count);
        for(int i = 0; i < 4; i++)
        {
            // Select for each plane using EnableSetReset and SetReset
            uint8_t lv = 
                (vga->regs_graphics[REG_Graphics_EnSetReset] & (1 << i)) == 0 ? v
                : (vga->regs_graphics[REG_Graphics_SetReset] & (1 << i)) == 0 ? 0x00
                : 0xFF;
            // Logical operation
            switch(logical_op)
            {
            case 0: break;
            case 1: lv &= vga->latch[i]; break;
            case 2: lv |= vga->latch[i]; break;
            case 3: lv ^= vga->latch[i]; break;
            }
            // Bit mask
            lv = (lv & bit_mask) | (vga->latch[i] & ~bit_mask);
            // Write to enabled planes (see sequencer MemoryPlaneWriteEnable)
            if( (vga->regs_sequencer[REG_Sequencer_MapMask] & (1 << i)) != 0 )
            {
                vga->planes[i][ofs] = v;
            }
            break;
        }
        break;
    }
}
/// @brief Synchronize the emulator memory state with VGA backing memory
/// @param State Emulator state pointer
void VGA_FullResync(struct sRME_State* State)
{
    tVgaState* vga = get_state(State);

    uint8_t map_select = (vga->regs_graphics[REG_Graphics_MiscGraphics] >> 2) & 3;

    // Get memory map, iterate over memory range, then fill backing memory
    // - Fill depends on the write mode
    size_t s=0, e=0;
    switch(map_select)
    {
    case 0: s = 0xA0000; e = 0xC0000; break;
    case 1: s = 0xA0000; e = 0xB0000; break;
    case 2: s = 0xB0000; e = 0xB8000; break;
    case 3: s = 0xB8000; e = 0xC0000; break;
    }

    for(size_t a = s; a < e; a += RME_BLOCK_SIZE)
    {
        if( vga->memory_touched[(a - 0xA0000) / RME_BLOCK_SIZE] ) {
            for(size_t o = 0; o < RME_BLOCK_SIZE; o ++) {
                VGA_WriteByte(State, a + o, ((const uint8_t*)State->Memory[a / RME_BLOCK_SIZE])[o]);
            }
        }
    }
    
    // Clear the "memory touched" flags
    memset(vga->memory_touched, 0, sizeof(vga->memory_touched));
}

int VGA_SetReg_Graphics(struct sRME_State* State, uint8_t reg, uint8_t val)
{
    if(reg > 8) {
        return -1;
    }
    VGA_FullResync(State);
    get_state(State)->regs_graphics[reg] = val;
}

int VGA_IoPort_In(struct sRME_State* State, uint16_t Port, int Size, void* Dst)
{
    return -1;
}
int VGA_IoPort_Out(struct sRME_State* State, uint16_t Port, int Size, uint32_t Val)
{
    switch(Port)
    {
    case 0x3CE:
        if( Size == 2 ) {
            return VGA_SetReg_Graphics(State, Val & 0xFF, Val >> 8);
        }
        if( Size != 1 ) {
            return -1;
        }
        get_state(State)->index_graphics = Val;
        return 0;
    case 0x3CF:
        if( Size != 1 ) {
            return -1;
        }
        return VGA_SetReg_Graphics(State, get_state(State)->index_graphics, Val);
    default:
        return -1;
    }
}