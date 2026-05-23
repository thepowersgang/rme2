
#include "common.h"
#include "ui_common.h"
#include <SDL/SDL.h>
//#include "rme.h"
#define VIDEO_COLS	80
#define VIDEO_ROWS	25

SDL_Surface	*gScreen;
// Has a redraw been requested already (inhibits the timer)
 int	gbIsRedrawing;

const struct sUiBindings cUiBindings_Sdl = {
    .name = "sdl",
    .init = UiSdl_Init,
    .halted = UiSdl_Halted,
    .poll_events = UiSdl_PollEvents,
};

Uint32 UiSdl_int_RedrawTimerCb(Uint32 interval, void *unused);
void UiSdl_int_HandleEvent(SDL_Event *Event, tRME_State *EmuState);
void UiSdl_int_Redraw(void);

void UiSdl_Init()
{
    SDL_Init(SDL_INIT_TIMER);
    gScreen = SDL_SetVideoMode(VIDEO_COLS*8, VIDEO_ROWS*16, 32, SDL_HWSURFACE);
    SDL_AddTimer(100, UiSdl_int_RedrawTimerCb, NULL);
}

void UiSdl_Halted(const char* msg)
{
    SDL_Event	e;
    SDL_WM_SetCaption("RME - %s, press any key to quit", "RME - Halted");
    while( SDL_WaitEvent(&e) )
    {
        if(e.type == SDL_QUIT || e.type == SDL_KEYDOWN) {
            return;
        }
    }
}

void UiSdl_PollEvents(tRME_State* emu)
{
    // Check for SDL events
    SDL_Event	ev;
    while( SDL_PollEvent(&ev) )
    {
        UiSdl_int_HandleEvent(&ev, emu);
    }
}

void UiSdl_int_HandleEvent(SDL_Event *Event, tRME_State *EmuState)
{
	switch(Event->type)
	{
	case SDL_QUIT:
		RME_DumpRegs(EmuState);
		fprintf(stderr, "Window closed, quitting\n");
		exit(0);
	case SDL_KEYDOWN:
		if( Event->key.keysym.sym == SDLK_BACKSPACE ) {
			RME_DumpRegs(EmuState);
		}
        Input_PushKey(Event->key.keysym.sym, Event->key.keysym.unicode);
		break;
	case SDL_USEREVENT:
		UiSdl_int_Redraw();
		gbIsRedrawing = 0;
		break;
	}
}

/// @brief Timer injecting a user event that requests a redraw of the window
/// @param interval original timer interval
/// @return New timer interval
Uint32 UiSdl_int_RedrawTimerCb(Uint32 interval, void *unused)
{
	if( !gbIsRedrawing )
	{
		gbIsRedrawing = 1;
		SDL_UserEvent	ue = {.type=SDL_USEREVENT,.code=0};
		SDL_Event	e = {.type=SDL_USEREVENT, .user = ue};
		SDL_PushEvent(&e);
	}
	return interval;
}

#include "font.h"

void DrawChar(int X, int Y, uint8_t ch, uint32_t BGC, uint32_t FGC)
{
	Uint8	*font;
	Uint32	*buf;

	SDL_Rect	rc = {0,0,1,1};
	
	font = &VTermFont[ch*FONT_HEIGHT];
	
	rc.w = 1; rc.h = 1;
	rc.x = X*FONT_WIDTH;
	rc.y = Y*FONT_HEIGHT;
	
	for(int y = 0; y < FONT_HEIGHT; y ++, rc.y ++)
	{
		for(int x = 0; x < FONT_WIDTH; x ++, rc.x++)
		{
			if(*font & (1 << (FONT_WIDTH-x-1)))
				SDL_FillRect(gScreen, &rc, FGC);
			else
				SDL_FillRect(gScreen, &rc, BGC);
		}
		rc.x -= FONT_WIDTH;
		buf = (void*)( (intptr_t)buf + gScreen->pitch );
		font ++;
	}
}


void UiSdl_int_Redraw(void)
{
	uint32_t	colours[] = {
		0x000000, 0x0000FF, 0x00FF00, 0xFFFF00, 0xFF0000, 0xFF00FF, 0x884400, 0xCCCCCC,
		0x444444, 0x4444FF, 0x44FF44, 0xFFFF44, 0xFF4444, 0xFF44FF, 0xFF8800, 0xFFFFFF,
	};
	// TODO: Other modes?
	uint8_t	*vidmem = &gaMemory[0xB8000];
	for( int row = 0; row < VIDEO_ROWS; row ++ )
	{
		for( int col = 0; col < VIDEO_COLS; col ++ )
		{
			uint8_t	ch = vidmem[(row*VIDEO_COLS+col)*2+0];
			uint8_t	at = vidmem[(row*VIDEO_COLS+col)*2+1];
			DrawChar(col, row, ch, colours[at>>4], colours[at&15]);
		}
	}
	#if ENABLE_GUI
	if( !gbDisableGUI ) {
		SDL_Flip(gScreen);
	}
	#endif
//	printf("Video redraw complete\n");
}