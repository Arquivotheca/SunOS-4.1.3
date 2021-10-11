/*	@(#)sky.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

| as include file for interfacing the sky ffp board
|		21 June, 1983	rt

| shape of the ffp register data structure:
| struct sky {
|	short	sky_command;
|	short	sky_status;
|	long	sky_data;
|	long	sky_ucode;
| };

	COMMAND	=	0
	STATUS	=	2
	OPERAND	=	4
	UCODE	=	8

|
| some commands:
|
| control commands:
	S_INIT	=	0x1000
	S_SAVE	=	0x1040
	S_REST	=	0x1041
	S_NOP	=	0x1063
| state-free conversions: b <- f(a)
	S_ITOS	=	0x1024
	S_ITOD	=	0x1044
	S_STOD	=	0x1042
	S_DTOS	=	0x1043
	S_DTOI	=	0x1045
	S_STOI	=	0x1027
| state-free single-precision arithmetic: b <- f(a1,a2)
	S_SADD3	=	0x1001
	S_SSUB3	=	0x1007
	S_SMUL3	=	0x100B
	S_SDIV3	=	0x1013
| state-dependent single-precision arithmetic: b <- f(r0,a1)
	S_SADD2	=	0x1003	| S_SADD3+2
	S_SSUB2	=	0x1009	|   &c.
	S_SMUL2	=	0x100D
	S_SDIV2	=	0x1015
| state-free double-precision arithmetic: b <- f(a1,a2)
	S_DADD3	=	0x1002	| S_SADD3+1
	S_DSUB3	=	0x1008	|   &c.
	S_DMUL3	=	0x100C
	S_DDIV3	=	0x1014
| state-dependent double-precision arithmetic: b <- f(r0,a1)
	S_DADD2	=	0x1004	| S_DADD3+2 or S_SADD2+1 or S_SADD3+3
	S_DSUB2	=	0x100A	|		  &c.
	S_DMUL2	=	0x100E
	S_DDIV2	=	0x1016
| state-free comparisons: b <- a1 vs. a2
	S_SCMP3	=	0x105D
	S_DCMP3	=	0x105E
| state-dependent comparisons: b <- r0 vs. a1
	S_SCMP2	=	0x105F	| S_SCMP3+2
	S_DCMP2	=	0x1060	| S_DCMP3+2
| state-free elementary functions:
	S_SSQRT =       0x102f
	S_SEXP  =       0x102c
	S_SLOG  =       0x102d
	S_SSIN  =       0x1029
	S_SCOS  =       0x1028
	S_STAN  =       0x102a
	S_SATAN =       0x102b
	S_SPOW	=       0x102e
| random data movement
	S_LDS	=	0x1031
	S_LDD	=	0x1034
