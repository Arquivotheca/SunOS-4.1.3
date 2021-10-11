	.data
	.asciz	"@(#)scb.s 1.1 92/07/30"
	.even
	.text

|	Copyright (c) 1986 by Sun Microsystems, Inc.

#define	ZERO	.long	0		/* zero - don't change */
#define	ERRV	.long	errorvec	/* generic for unused vectors */

/*
 * This is the prototype vector table
 * which gets copied to location _scb by startup.
 * Zero entries are not copied, the monitor's vector
 * is preserved when zero.
 */
	.globl	_protoscb
_protoscb:
	ERRV			| 0x0	initial SSP (not used)
	ERRV			| 0x1	initial PC  (not used)
	.long	buserr		| 0x2	Bus Error
	.long	addrerr		| 0x3	Address Error
	.long	illinst		| 0x4	Illegal Instruction
	.long	zerodiv		| 0x5 	Zero divide
	.long	chkinst		| 0x6	CHK, CHK2 instruction
	.long	trapv		| 0x7	TRAPV, cpTRAPcc, TRAPcc instruction
	.long	privvio		| 0x8	Privilege violation
	.long	trace		| 0x9	Trace trap
	.long	emu1010		| 0xA	Line 1010 emulator trap
	.long	emu1111		| 0xB	Line 1111 emulator trap
	ERRV			| 0xC	Reserved
	.long	coprocerr	| 0xD	Coprocessor protocol error
	.long	fmterr		| 0xE	Stack format error
	.long	spurious	| 0xF	Unitialized vector
	ERRV			| 0x10	Reserved
	ERRV			| 0x11	Reserved
	ERRV			| 0x12	Reserved
	ERRV			| 0x13	Reserved
	ERRV			| 0x14	Reserved
	ERRV			| 0x15	Reserved
	ERRV			| 0x16	Reserved
	ERRV			| 0x17	Reserved
	.long	spurious	| 0x18	Spurious interrupt
	.long	softint		| 0x19	Level 1 software interrupt
	.long	level2		| 0x1A	Level 2 interrupt
	.long	level3		| 0x1B	Level 3 interrupt
	.long	level4		| 0x1C	Level 4 interrupt
	.long	level5		| 0x1D	Level 5 interrupt
	.long	zslevel6	| 0x1E	Level 6 interrupt   (UARTs)
	ZERO			| 0x1F	Level 7 interrupt   (patched later)
	.long	syscall		| 0x20	Trap instruction 0  (system call)
	.long	badtrap		| 0x21	Trap instruction 1  (monitor breakpoint)
	.long	flush		| 0x22	Trap instruction 2  (cache flush)
	.long	badtrap		| 0x23	Trap instruction 3
	.long	badtrap		| 0x24	Trap instruction 4
	.long	badtrap		| 0x25	Trap instruction 5
	.long	badtrap		| 0x26	Trap instruction 6
	.long	badtrap		| 0x27	Trap instruction 7
	.long	badtrap		| 0x28	Trap instruction 8
	.long	badtrap		| 0x29	Trap instruction 9
	.long	badtrap		| 0x2A	Trap instruction 10 (kernel debug bpt)
	.long	badtrap		| 0x2B	Trap instruction 11
	.long	badtrap		| 0x2C	Trap instruction 12
	.long	badtrap		| 0x2D	Trap instruction 13
	.long	badtrap		| 0x2E	Trap instruction 14
	.long	brkpt		| 0x2F	Trap instruction 15 (debug breakpoint)
	.long	floaterr	| 0x30	Float branch or set on unordered cond
	.long	floaterr	| 0x31	Float inexact result
	.long	floaterr	| 0x32	Float divide by zero
	.long	floaterr	| 0x33	Float underflow
	.long	floaterr	| 0x34	Float operand error
	.long	floaterr	| 0x35	Float overflow
	.long	floaterr	| 0x36	Float NAN
	ERRV			| 0x37	Reserved
	ERRV			| 0x38	Reserved
	ERRV			| 0x39	Reserved
	ERRV			| 0x3A	Reserved
	ERRV			| 0x3B	Reserved
	ERRV			| 0x3C	Reserved
	ERRV			| 0x3D	Reserved
	ERRV			| 0x3E	Reserved
	ERRV			| 0x3F	Reserved
|
| The next 0xC0 (192) entries are for use with vectored interrupts.
| Vectors 0xC8-0xFF (200-255) are reserved for customer use.
|
	ERRV; ERRV; ERRV; ERRV	| 0x40 - 0x43  sc0  | sc?
	ERRV; ERRV; ERRV; ERRV	| 0x44 - 0x47  xdc0 | xdc1 | xdc2 | xdc3
	ERRV; ERRV; ERRV; ERRV	| 0x48 - 0x4B  xyc0 | xyc1 | xyc?
	ERRV; ERRV; ERRV; ERRV	| 0x4C - 0x4F  future disk controllers
	ERRV; ERRV; ERRV; ERRV	| 0x50 - 0x53  future disk controllers
	ERRV; ERRV; ERRV; ERRV	| 0x54 - 0x57  future disk controllers
	ERRV; ERRV; ERRV; ERRV	| 0x58 - 0x5B  future disk controllers
	ERRV; ERRV; ERRV; ERRV	| 0x5C - 0x5F  future disk controllers
	ERRV; ERRV; ERRV; ERRV	| 0x60 - 0x63  tm0  | tm1  | tm?
	ERRV; ERRV; ERRV; ERRV	| 0x64 - 0x67  xtc0 | xtc1 | xtc?
	ERRV; ERRV; ERRV; ERRV	| 0x68 - 0x6B  future tape controllers
	ERRV; ERRV; ERRV; ERRV	| 0x6C - 0x6F  future tape controllers
	ERRV; ERRV; ERRV; ERRV	| 0x70 - 0x73  ec?
	ERRV; ERRV; ERRV; ERRV	| 0x74 - 0x77  ie0 | ie1 | ie?
	ERRV; ERRV; ERRV; ERRV	| 0x78 - 0x7B  future ethernet devices
	ERRV; ERRV; ERRV; ERRV	| 0x7C - 0x7F  future ethernet devices
	ERRV; ERRV; ERRV; ERRV	| 0x80 - 0x83  vpc0 | vpc1 | vpc?
	ERRV; ERRV; ERRV; ERRV	| 0x84 - 0x87  vp?
	ERRV; ERRV; ERRV; ERRV	| 0x88 - 0x8B  mti0 | mti1 | mti2 | mti3
	ERRV; ERRV; ERRV; ERRV	| 0x8C - 0x8F  SunLink SCP (Systech DCP-8804)
	ERRV; ERRV; ERRV; ERRV	| 0x90 - 0x93  Sun-3 zs0 (8 even vectors)
	ERRV; ERRV; ERRV; ERRV	| 0x94 - 0x97  Sun-3 zs1 (8 odd vectors)
	ERRV; ERRV; ERRV; ERRV	| 0x98 - 0x9B  Sun-3 zs0 (8 even vectors)
	ERRV; ERRV; ERRV; ERRV	| 0x9C - 0x9F  Sun-3 zs1 (8 odd vectors)
	ERRV; ERRV; ERRV; ERRV	| 0xA0 - 0xA3  future serial
	ERRV; ERRV; ERRV; ERRV	| 0xA4 - 0xA7  pc0  | pc1  | pc2  | pc3
	ERRV; ERRV; ERRV; ERRV	| 0xA8 - 0xAB  cg2 | future frame buffers
	ERRV; ERRV; ERRV; ERRV	| 0xAC - 0xAF  gp1 | future graphics processors
	ERRV; ERRV; ERRV; ERRV	| 0xB0 - 0xB3  sky0 | ?
	ERRV; ERRV; ERRV; ERRV	| 0xB4 - 0xB7  Sun-3 SunLink / channel attach
	ERRV; ERRV; ERRV; ERRV	| 0xB8 - 0xBB  (token bus) tbi0 | tbi1 | ?
	ERRV; ERRV; ERRV; ERRV	| 0xBC - 0xBF  Reserved for Sun
	ERRV; ERRV; ERRV; ERRV	| 0xC0 - 0xC3  Reserved for Sun
	ERRV; ERRV; ERRV; ERRV	| 0xC4 - 0xC7  Reserved for Sun
	ERRV; ERRV; ERRV; ERRV	| 0xC8 - 0xCB  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xCC - 0xCF  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xD0 - 0xD3  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xD4 - 0xD7  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xD8 - 0xDB  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xDC - 0xDF  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xE0 - 0xE3  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xE4 - 0xE7  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xE8 - 0xEB  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xEC - 0xEF  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xF0 - 0xF3  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xF4 - 0xF7  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xF8 - 0xFB  Reserved for User
	ERRV; ERRV; ERRV; ERRV	| 0xFC - 0xFF  Reserved for User
