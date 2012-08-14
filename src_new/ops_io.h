/*
 * Realmode Emulator
 * - IO Operations Header
 * 
 */
#ifndef _RME_OPS_IO_H_
#define _RME_OPS_IO_H_

#define	outB(state,port,val)	(outb(port,val),0)	// Write 1 byte to an IO Port
#define	outW(state,port,val)	(outw(port,val),0)	// Write 2 bytes to an IO Port
#define	outD(state,port,val)	(outl(port,val),0)	// Write 4 bytes to an IO Port
#define	inB(state,port,dst)	(*(dst)=inb((port)),0)	// Read 1 byte from an IO Port
#define	inW(state,port,dst)	(*(dst)=inw((port)),0)	// Read 2 bytes from an IO Port
#define	inD(state,port,dst)	(*(dst)=inl((port)),0)	// Read 4 bytes from an IO Port

#endif
