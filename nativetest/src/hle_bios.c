#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "rme.h"

#define VIDEO_COLS	80
#define VIDEO_ROWS	25

void	PutChar(uint8_t ch, uint8_t attr);
void	Bios_PutString(const char *String, uint8_t attr);

 int	giCursorX, giCursorY;
uint8_t	gCurAttributes = 0x0F;

/**
 * \brief Screen output
 */
void PutChar(uint8_t ch, uint8_t attrib)
{
	switch( ch )
	{
	case '\n':
		giCursorY ++;
		// TODO: Scroll
		if(giCursorY == 25)
			giCursorY = 0;
		return ;
	case '\r':
		giCursorX = 0;
		return ;
	case 8:
		if( giCursorX > 0 )
			giCursorX --;
		return ;
	}

	gaMemory[0xB8000 + (giCursorY*VIDEO_COLS+giCursorX)*2 + 0] = ch;
	gaMemory[0xB8000 + (giCursorY*VIDEO_COLS+giCursorX)*2 + 1] = attrib;	// TODO: Better attrib
	
	giCursorX ++;
	if(giCursorX == VIDEO_COLS) {
		giCursorX = 0;
		giCursorY ++;
		if(giCursorY == VIDEO_ROWS) {
			Video_ScrollUp(0, 1, attrib, 0, 0, VIDEO_ROWS, VIDEO_COLS);
			giCursorY --;
		}
	}
}

void Video_ScrollUp(int Page, uint8_t Attr, int nLines, int Top, int Left, int Bottom, int Right)
{
	// TODO
}

void Bios_PutString(const char *String, uint8_t attr)
{
	while(*String) {
		PutChar(*String++, attr);
	}
}

/**
 */
int GetDiskParams(int Disk, int *NCyl, int *NHead, int *SPT)
{
	switch(Disk)
	{
	case 0:
		*NCyl = 80;
		*NHead = 2;
		*SPT = 18;
		return 4;	// Type = 4 (3.5" 1.44MB)
	default:
		return 0;
	}
}

int ReadDiskLBA(tRME_State* State, int Disk, int LBAAddr, int Count, void *Data)
{
	 int	ret;
	//PrintDebugF(State, "ReadDiskLBA: (Disk=%i, LBAAddr=0x%x, Count=%i, Data=%p)\n",
	//	Disk, LBAAddr, Count, Data);
	switch(Disk)
	{
	case 0:
		if( fseek(gaFDDs[0], LBAAddr * 512, SEEK_SET) ) {
			PrintDebugF(State, "fseek(gaFDDs[0], 0x%x*512, SEEK_SET)\n", LBAAddr);
			perror("FDD fseek failed");
			memset(Data, 0, 512*Count);
			return 0;
		}
		ret = fread(Data, 512, Count, gaFDDs[0]);
		if( ret != Count ) {
			perror("ReadDiskLBA  fread");
			PrintDebugF(State," %i/%i sectors read\n", ret, Count);
		}
		return ret;
	default:
		return 0;
	}
}

int ReadDiskCHS(tRME_State* State, int Disk, int Cylinder, int Head, int Sector, int Count, void *DataPtr)
{
	 int	lbaAddr;
	 int	nCyl, nHead, spt;
	
	//printf("ReadDiskCHS: (Disk=%i, Cylinder=%i, Head=%i, Sector=%i, Count %i, Data=%p)\n",
	//	Disk, Cylinder, Head, Sector, Count, DataPtr);
	
	if( GetDiskParams(Disk, &nCyl, &nHead, &spt) == 0 ) {
		PrintDebugF(State, " GetDiskParams(Disk=0x%02x) return 0\n", Disk);
		return -1;
	}
	
	//printf(" nCyl=%i, nHead=%i, spt=%i\n", nCyl, nHead, spt);
	
	if( Cylinder >= nCyl ) {
		PrintDebugF(State, " Cylinder(%i) >= nCyl(%i)\n", Cylinder, nCyl);
		return -0x01;
	}
	if( Head >= nHead ) {
		PrintDebugF(State, " Head(%i) >= nHead(%i)\n", Head, nHead);
		return -0x01;
	}
	if( Sector > spt ) {
		PrintDebugF(State, " Sector(%i) >= spt(%i)\n", Sector, spt);
		return -0x01;
	}
	// Multi-track reads allowed (because they are easy)
	
	lbaAddr = Cylinder * nHead * spt + Head * spt + Sector - 1;
	//printf(" lbaAddr = %x\n", lbaAddr);
	
	return ReadDiskLBA(State, Disk, lbaAddr, Count, DataPtr);
}

/**
 * \brief Do a HLE call (VGA BIOS)
 */
int HLECall10(tRME_State *State, int IntNum)
{
	 int	ret;
	switch(State->AX.B.H)
	{
	// VIDEO - SET VIDEO MODE
	case 0x00:
		if( State->AX.B.L == 3 ) {
			State->AX.B.L = 0x30;
			return 0;
		}
		FatalErrorF(State, "HLE Call INT 0x10/AH=0x00: VIDEO - SET VIDEO MODE AL=0x%x\n", State->AX.B.L);
	// VIDEO - SET TEXT-MODE CURSOR SHAPE
	case 0x01:
		PrintDebugF(State, "INT10/AH=01 - TODO: Set cursor shape CH=%02x, CL=%02x", State->CX.B.H, State->CX.B.L);
		break;
	// VIDEO - SET CURSOR POSITION
	case 0x02:
		giCursorX = State->DX.B.L;
		giCursorY = State->DX.B.H;
		break;
	// VIDEO - SET ACTIVE DISPLAY PAGE
	case 0x05: {
		// giCurPage = State->AX.B.L;
		break; }
	// VIDEO - SCROLL UP WINDOW
	case 0x06: {
		 int	lines = State->AX.B.L;
		uint8_t	attr = (State->BX.B.H << 8);
		int tx = State->CX.B.L;
		int ty = State->CX.B.H;
		int bx = State->DX.B.L;
		int by = State->DX.B.H;
		
		if( lines == 0 ) {
			// AL=0: Clear screen
			for( int i = 0; i < VIDEO_ROWS*VIDEO_COLS; i ++ )
			{
				gaMemory[0xB8000+i*2+0] = 0;
				gaMemory[0xB8000+i*2+1] = attr;
			}
		}
		else {
			Video_ScrollUp(0, lines, attr, ty, tx, by, bx);
		}
		
		break; }
	// VIDEO - READ CHARACTER AND ATTRIBUTE AT CURSOR POSITION
	case 0x08:
		State->AX.B.L = gaMemory[0xB8000+(giCursorY*80+giCursorX)*2+0];
		State->AX.B.H = gaMemory[0xB8000+(giCursorY*80+giCursorX)*2+1];
		break;
	// VIDEO - WRITE CHARACTER AND ATTRIBUTE AT CURSOR POSITION
	case 0x09: {
		uint8_t	ch = State->AX.B.L;
		 int	page = State->BX.B.H;
		uint8_t	attr = State->BX.B.L;
		 int	count = State->CX.W;
		for( int i = 0; i < count; i ++ )
		{
			gaMemory[0xB8000+(giCursorY*80+giCursorX)*2+0] = ch;
			gaMemory[0xB8000+(giCursorY*80+giCursorX)*2+1] = attr;
			giCursorX ++;
			if( giCursorX == VIDEO_ROWS ) {
				giCursorX --;
				break;
			}
		}
		break; }
	// VIDEO - TELETYPE OUTPUT
	case 0x0E:
		// TODO: Better Colours
		PutChar(State->AX.B.L, gCurAttributes);
		//Video_Redraw();
		break;
	// VIDEO - GET CURRENT VIDEO MODE
	case 0x0F:
		State->AX.B.H = 80;	// Cols
		State->AX.B.L = 0x03;	// Mode Number
		State->BX.B.L = 0;	// Page
		break;
	// Extensions: 0x10XX
	case 0x10:
		switch(State->AX.W)
		{
		//  VIDEO - SET BORDER (OVERSCAN) COLOR (PCjr,Tandy,EGA,VGA)
		case 0x1001:
			break;
		// VIDEO - TOGGLE INTENSITY/BLINKING BIT (Jr, PS, TANDY 1000, EGA, VGA)
		case 0x1003:
			// TODO: Attributes
			gCurAttributes ^= 0x80;
			break;
		default:
			PrintDebugF(State, "HLE Call INT 0x10 AX=0x%04x Unk\n", State->AX.W);
			RME_DumpRegs(State);
			exit(1);
		}
		break;
	// VIDEO - GET BLANKING ATTRIBUTE
	case 0x12:
		State->BX.B.H = 0;
		break;
	
	default:
		PrintDebugF(State, "HLE Call INT 0x10 AX=%04x Unk\n", State->AX.W);
		RME_DumpRegs(State);
		exit(1);
	}
	return 0;	// Silently ignore VGA BIOS calls
}


/**
 * \brief Do a HLE call (Get memory size)
 */
int HLECall12(tRME_State *State, int IntNum)
{
	State->AX.W = 0xA0000/1024 - 1;
	return 0;
}

/*
 * \brief HLE Calls "INT 0x13" Disk Subsystem
 */
int HLECall13(tRME_State *State, int IntNum)
{
	 int	ret;
	switch(State->AX.B.H)
	{
	// DISK - RESET DISK SYSTEM
	case 0x00:
		PrintDebugF(State, "HLE 0x13:0x00 - Reset disk subsystem\n");
		State->Flags &= ~FLAG_CF;
		State->AX.B.H = 0;
		// Does anything need to be done here?
		break;
	
	case 0x02: {	// Read Sector(s) into memory
		//RME_DumpRegs(State);
		// AL - Number of sectors to read
		// CH - Cylinder Number Low Bits
		// CL - Sector Number (bits 0-5), Cylinder Number High (bits 6,7)
		// DH - Head Number
		// DL - Disk Number
		// ES:BX - Destination Buffer
		
		// Zero count?
		if( (State->AX.B.L & 0x3F) == 0 ) {
			PrintDebugF(State, " 0x13:0x02 Zero sectors\n");
			State->Flags |= 1;
			break;
		}
		
		 int	disk = State->DX.B.L;
		 int	cyl = State->CX.B.H | ((State->CX.B.L & 0xC0)<<2);
		 int	head = State->DX.B.H;
		 int	sect = State->CX.B.L & 0x3F;
		 int	count = State->AX.B.L;
		ret = ReadDiskCHS( State, disk, cyl, head, sect, count, &gaMemory[ State->ES*16 + State->BX.W ] );
		// Error check
		if( ret < 0 ) {
			PrintDebugF(State, " 0x13:0x02 ReadDiskCHS Ret -0x%x\n", -ret);
			State->AX.B.H = -ret;
			State->Flags |= 1;
			break;
		}
		if( ret != State->AX.B.L ) {
			PrintDebugF(State, " 0x13:0x02 Incomplete read: %i/%i\n", ret, State->AX.B.L);
			State->AX.B.L = ret;
			State->Flags |= FLAG_CF;
			break;
		}
		PrintDebugF(State, "HLE 0x13:0x02 - Read sectors (D%02x,%i,%i,%i)+%i to %x:%x\n",
			disk, cyl, head, sect, count, State->ES, State->BX.W 
			);
		State->AX.B.H = 0;
		State->Flags &= ~1;
		break; }
	
	case 0x08: {	// Get Drive Parameters
		PrintDebugF(State, "HLE 0x13:0x08 - Get Drive Parameters (D%02x)\n", State->DX.B.L);
		State->Flags &= ~(FLAG_CF);
		 int	cyl, heads, sec;
		 int	type;
		type = GetDiskParams(State->DX.B.L, &cyl, &heads, &sec);
		if( type == 0 ) {
			State->Flags |= 1;
			break;
		}
		
		// Maximum values are wanted
		cyl --;
		heads --;
		// Sector numbers are 1 based, so `sec` doesn't need to be changed
		
		State->AX.W = 0x0000;	// AX - Zero for success
		State->BX.B.L = type;	// BL - Disk Type (1.44M Floppy)
		State->CX.B.L = sec | ((cyl>>8)<<6);	// CL - Max Sector Number
		State->CX.B.H = cyl&0xFF;	// CH - Cylinder Count (Bits 0-7)
		State->DX.B.L = 1;	// DL - Number of drives
		State->DX.B.H = heads;	// DH - Maximum Head Number
		// Disk Parameter block
		State->ES = 0xF100;	State->DI.W = 0x0000;
		break; }

	case 0x15:	// Get Disk Type
		PrintDebugF(State, "HLE 0x13:0x15 - Get Disk Type 0x%02x\n", State->DX.B.L);
		{
			 int	cyl, heads, sec;
			State->Flags &= ~FLAG_CF;
			switch(GetDiskParams(State->DX.B.L, &cyl, &heads, &sec))
			{
			case 4:
				State->AX.B.H = 2;
				State->CX.W = sec * heads * cyl;
				State->DX.W = 0;
				break;
			default:
				PrintDebugF(State, " - Disk type unknown\n");
				State->AX.B.H = 0;
				State->Flags |= FLAG_CF;
				break;
			}
		}
		break;		

	// Extended Read
	case 0x42:
		{
			uint32_t	laddr;
			struct {
				uint8_t	Size;
				uint8_t	Rsvd;	// Zero
				uint16_t	Count;
				struct FAR_PTR	Buffer;
				uint64_t	LBAStart;
				uint64_t	BufferLong;
			}	PACKED	*packet = (void*)&gaMemory[State->DS*16+State->SI.W];
			
			laddr = packet->Buffer.Segment*16 + packet->Buffer.Offset;
			
			
			//PrintDebugF(State, " packet = %p{Size:%i,Count=%i,Buffer=%04x:%04x,"
			//	"LBAStart=0x%"PRIx64",BufferLong=0x%"PRIx64"}\n",
			//	packet,
			//	packet->Size, packet->Count,
			//	packet->Buffer.Segment, packet->Buffer.Offset,
			//	packet->LBAStart, packet->BufferLong);
			
			if(laddr + packet->Count*512 > 0x110000) {
				State->Flags |= 1;
				State->AX.B.H = 0xBB;
				PrintDebugF(State, "Read past end of memory! (0x%x)\n",
					laddr + packet->Count*512);
				break;
			}
			ReadDiskLBA(State, State->DX.B.L,
				packet->LBAStart, packet->Count,
				&gaMemory[laddr]);
			State->Flags &= ~1;
			State->AX.B.H = 0x00;
			
		}
		break;
	
	default:
		PrintDebugF(State, "HLE Call INT 0x13 AH=0x%02x unknown\n", State->AX.B.H);
		RME_DumpRegs(State);
		exit(1);
	}
	return 0;
}


/**
 * \brief Do a HLE call
 */
int HLECall(tRME_State *State, int IntNum)
{
	 int	ret;
	switch( IntNum )
	{
	case 0x11:	// BIOS - GET EQUIPMENT LIST
		State->AX.W = 0
			| 1	// FDDs present
			| (1 << 4)	// Video mode: 40x25 colour
			| (1 << 6)	// 1 FDD installed
			;
		break;
	// --- Keyboard Input ---
	case 0x16:
		switch(State->AX.B.H)
		{
		case 0x00:	// KEYBOARD - GET KEYSTROKE
		case 0x10:	// KEYBOARD - GET ENHANCED KEYSTROKE
			while( gKeyBufferPos == 0 )
			{
				#if ENABLE_GUI
				if( !gbDisableGUI ) {
					SDL_Event	e;
					SDL_WaitEvent(&e);
					HandleEvent(&e, State);
				} else
				#endif
				{
					int ch = getchar();
					assert(ch > 0);
					Input_PushKeysFromChar(ch);
				}
			}
			State->AX.B.H = gKeyBuffer[0].Scancode;
			State->AX.B.L = gKeyBuffer[0].ASCII;
			gKeyBufferPos --;
			memmove(gKeyBuffer, gKeyBuffer+1, gKeyBufferPos*sizeof(gKeyBuffer[0]));
			break;
		case 0x01:	// KEYBOARD - CHECK FOR KEYSTROKE
		case 0x11:	// KEYBOARD - CHECK FOR ENHANCED KEYSTROKE
			// TODO: Keyboard queue
			if( gKeyBufferPos > 0 )
			{
				// Ignore scancodes > 83? (Non 83/84 keycodes)
				State->AX.B.H = gKeyBuffer[0].Scancode;
				State->AX.B.L = gKeyBuffer[0].ASCII;
				State->Flags &= ~FLAG_ZF;
			}
			else
			{
				State->Flags |= FLAG_ZF;
			}
			break;
		// KEYBOARD - GET SHIFT FLAGS
		case 0x02:
			// 0: Right Shift
			// 1: Left Shift
			// 2: Ctrl key
			// 3: Alt key
			// 4: Scroll Lock
			// 5: Num Lock
			// 6: Caps Lock
			// 7: Insert Lock
			State->AX.B.L = 0;
			break;
		default:
			PrintDebugF(State, "HLE Call INT 0x16: AH=0x%02x unk\n", State->AX.B.H);
			RME_DumpRegs(State);
			exit(1);
		}
		break;
	
	// --- Diskless Boot Hook (Boot error) ---
	case 0x18:
	// --- System Bootstrap Loader (called by MSDOS to reboot) ---
	case 0x19:
		Bios_PutString("\r\n[BIOS] Boot Error. Press any key to terminate emulator", 0x04);
		#if ENABLE_GUI
		{
			SDL_Event	e;
			while( SDL_WaitEvent(&e) )
			{
				if(e.type == SDL_QUIT)
					exit(0);
				else if(e.type == SDL_KEYDOWN)
					exit(0);
			}
		}
		#else
		exit(0);
		#endif
		break;
	
	case 0x1A:
		switch(State->AX.B.H)
		{
		case 0x00: {
			uint32_t ticks = 0;
			State->AX.B.L = 0;
			State->CX.W = ticks >> 16;
			State->DX.W = ticks & 0xFFFF;
			break;
			}
		default:
			PrintDebugF(State, "HLE Call INT 0x%02x AH=%02x Unknown\n", IntNum, State->AX.B.H);
			RME_DumpRegs(State);
			return RME_ERR_BUG;
		}
		break;

	default:
		PrintDebugF(State, "HLE Call INT 0x%02x Unknown\n", IntNum);
		RME_DumpRegs(State);
		return RME_ERR_BUG;
	}
	return 0;	// Emulate
}