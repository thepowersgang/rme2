//
// UI backed by a standard ANSI terminal
//
#include "ui_common.h"
#include "common.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

//#include <unistd.h>
#include <sys/select.h> // select and friends
#include <termios.h>
#include <unistd.h>

void UiTty_Init(void);
void UiTty_Deinit(void);
void UiTty_Halted(const char* msg);
void UiTty_PollEvents(struct sRME_State* State);
void UiTty_WaitEvent(struct sRME_State* State);
void UiTty_int_SetChar(int row, int col, uint8_t ch, uint8_t attr);

const struct sUiBindings cUiBindings_Tty = {
    .name = "tty",
    .init = UiTty_Init,
    .deinit = UiTty_Deinit,
    .halted = UiTty_Halted,
    .poll_events = UiTty_PollEvents,
    .wait_event = UiTty_WaitEvent,
};

struct termios  gUiTty_OrigTermios;
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
    // Draw a line at the bottom of the display area
    printf("\x1b[26H");
    for(int i = 0; i < 80; i ++) {
        printf("-");
    }
    printf("\x1b[H");
    fflush(stdout);
    {
        struct termios  term;
        tcgetattr(0, &gUiTty_OrigTermios);
        tcgetattr(0, &term);
        term.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        term.c_iflag = ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
        tcsetattr(0, TCSANOW, &term);
    }
    atexit(UiTty_Deinit);
    gbUiTty_IsInit = true;
}
void UiTty_Deinit(void)
{
    if( gbUiTty_IsInit ) {
        // HACK: Wait for input before clearing
        // - Note the cursor movement being to stdout while the print is on stderr: That handles redirected stdout debugging
        printf("\x1b[28;1H");
        printf("Press any key to exit\n");
        fflush(stdout);
        fgetc(stdin);

        // Switch back to normal buffer
        // Reset attributes
        printf("\x1b[?1049l\x1b[0m");
        fflush(stdout);
        // Restore the terminal state
        tcsetattr(0, TCSANOW, &gUiTty_OrigTermios);
        gbUiTty_IsInit = false;
    }
}
void UiTty_Halted(const char* msg)
{
    printf("\x1b[27;1H");
    printf("--- HALTED: %s", msg);
    fflush(stdout);
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

    fd_set  fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    struct timeval tv = { 0 };
    if( select(1, &fds, NULL, NULL, &tv) ) {
        int ch = getc(stdin);
        Input_PushKeysFromChar(ch);
    }
}
void UiTty_WaitEvent(struct sRME_State* State)
{
    int ch = getc(stdin);
    Input_PushKeysFromChar(ch);
}


void UiTty_int_SetChar(int row, int col, uint8_t ch, uint8_t attr)
{
    PrintDebugF(NULL, "UiTty_int_SetChar: %i,%i ch=0x%02x '%c' attr=%02x (cur=%i,%i)\n",
        col, row, ch, (0x20 <= ch && ch <= 0x7E) ? ch : '.', attr, giUiTty_OutCursorX,giUiTty_OutCursorY);
    // Move the cursor
    if( row != giUiTty_OutCursorY || col != giUiTty_OutCursorX ) {
        if( row == 0 && col == 0 ) {
            printf("\n");   // HACK: Put a newline, so debug writing to a file is easier to read
            printf("\x1b[H");
        }
        // Special-case backspace
        else if( row == giUiTty_OutCursorY && col == giUiTty_OutCursorX-1 ) {
            printf("\b");
        }
        // Special case 1-3 newlines
        else if( row == giUiTty_OutCursorY+1 && col == 0 ) {
            printf("\n");
        }
        else if( row == giUiTty_OutCursorY+2 && col == 0 ) {
            printf("\n\n");
        }
        else if( row == giUiTty_OutCursorY+3 && col == 0 ) {
            printf("\n\n\n");
        }
        // Fallback: Absolutely position cursor
        else {
            printf("\n");   // HACK: Put a newline, so debug writing to a file is easier to read
            printf("\x1b[%i;%iH", 1+row, 1+col);
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

    case 0xC0:  printf("└");    break;
	case 0xC1:  printf("┴");    break;
	case 0xC2:  printf("┬");    break;
	case 0xC3:  printf("├");    break;
	case 0xC4:  printf("─");    break;
	case 0xC5:  printf("┼");    break;
	case 0xC6:  printf("╞");    break;
	case 0xC7:  printf("╟");    break;
	case 0xC8:  printf("╚");    break;
	case 0xC9:  printf("╔");    break;
	case 0xCA:  printf("╩");    break;
	case 0xCB:  printf("╦");    break;
	case 0xCC:  printf("╠");    break;
	case 0xCD:  printf("═");    break;
	case 0xCE:  printf("╬");    break;
	case 0xCF:  printf("╧");    break;
    
    case 0xDB:  printf("█");    break;
    default:
        PrintDebugF(NULL, "Unknown character 0x%02x\n", ch);
        printf("�");  // unicode U+FFFD
        break;
    }
}