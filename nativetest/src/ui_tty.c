//
// UI backed by a standard ANSI terminal
//
#include "ui_common.h"
#include "common.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

void UiTty_Init(void);
void UiTty_Deinit(void);
void UiTty_Halted(const char* msg);
void UiTty_PollEvents(struct sRME_State* State);
void UiTty_int_SetChar(int row, int col, uint8_t ch, uint8_t attr);

const struct sUiBindings cUiBindings_Tty = {
    .name = "tty",
    .init = UiTty_Init,
    .deinit = UiTty_Deinit,
    .halted = UiTty_Halted,
    .poll_events = UiTty_PollEvents,
};

bool gbUiTty_IsInit;
// Previous state of VGA memory
uint8_t gUiTty_PrevState[80*25*2];
// Current output location (optimizes moving about)
uint8_t giUiTty_OutCursorX;
uint8_t giUiTty_OutCursorY;
// Last set VGA attribute byte
uint8_t giUiTty_LastAttr;

void UiTty_Init(void)
{
    // Switch to alternate buffer
    printf("\x1b[?1049h");
    // Clear
    printf("\x1b[0m\x1b[2J");
    for(int i = 0; i < 25; i++) {
        printf("\n");
    }
    printf("\x1b[H");
    fflush(stdout);
    atexit(UiTty_Deinit);
    gbUiTty_IsInit = true;
}
void UiTty_Deinit(void)
{
    if( gbUiTty_IsInit ) {
        fgetc(stdin);
        // Switch back to normal buffer
        // Reset attributes
        printf("\x1b[?1049l\x1b[0m");
        fflush(stdout);
        gbUiTty_IsInit = false;
    }
}
void UiTty_Halted(const char* msg)
{
}
void UiTty_PollEvents(struct sRME_State* State)
{
    // Look for differences in the VGA buffer
    // Since this (usually) runs every instruction, there should only be one or two differences... unless a string op was run
    // For single differences, a linear scan works.
    const uint8_t* cur = &gaMemory[0xB8000];
    uint8_t* prev = gUiTty_PrevState;
    for(int r = 0; r < 25; r++) {
        for(int c = 0; c < 80; c++) {
            if( cur[0] != prev[0] || cur[1] != prev[1] ) {
                UiTty_int_SetChar(r, c, cur[0], cur[1]);
                prev[0] = cur[0];
                prev[1] = cur[1];
            }
            cur += 2;
            prev += 2;
        }
    }
    fflush(stdout);
}


void UiTty_int_SetChar(int row, int col, uint8_t ch, uint8_t attr)
{
    // Move the cursor
    if( row != giUiTty_OutCursorY && col != giUiTty_OutCursorX ) {
        if( row == 0 && col == 0 ) {
            printf("\x1b[H");
        }
        else if( row == giUiTty_OutCursorY+1 && col == 0) {
            printf("\n");
        }
        else {
            printf("\x1b[%i;%if", row, col);
        }
    }
    giUiTty_OutCursorY = row;
    giUiTty_OutCursorX = col+1;
    if( attr != giUiTty_LastAttr ) {
        // VGA attribute bits:
        uint8_t fg = attr & 15;
        uint8_t bg = attr >> 4;
        if( fg != (giUiTty_LastAttr & 15) && bg != (giUiTty_LastAttr >> 4) ) {
            // Both
            printf("\x1b[%i;%im", 30 + (fg & 7), 40 + (bg & 7));
        }
        else if( fg != (giUiTty_LastAttr & 15) ) {
            printf("\x1b[%im", 30 + (fg & 7));
        }
        else {
            // BG only
            printf("\x1b[%im", 40 + (bg & 7));
        }
        giUiTty_LastAttr = attr;
    }
    switch(ch)
    {
    case 0x00:  printf(" ");    break;
    case 0x20 ... 0x7E: printf("%c", ch);   break;
    // https://en.wikipedia.org/wiki/Code_page_437
    case 0xB5:  printf("┤");    break;
    case 0xB6:  printf("╡");    break;
    case 0xB7:  printf("╢");    break;
    default:
        printf("�");  // unicode U+FFFD
        break;
    }
}