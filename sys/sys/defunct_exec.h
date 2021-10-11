/*	@(#)defunct_exec.h	1.1	7/30/92	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef _sys_defunct_exec_h
#define _sys_defunct_exec_h

/* format of the exec header, known by kernel and by user programs */

/* "struct exec" is the header at the beginning of every object file. */
struct exec
{
#ifdef sun
	unsigned char		a_toolversion;	/* version # of toolset which
						 * created the object file
						 */
#define TV_SUN2_SUN3	0	/* old sun2/3	a.out format		*/
#define TV_SUN4    (1+0x83)	/*  sun4 a.out format (+dev.ver#+0x80)  */
	unsigned char		a_machtype;	/* machine type		      */
#define M_OLDSUN2	0	/* old sun2 executable; 68010 code	*/
#define M_68010		1	/* runs on either 68010 or 68020	*/
#define M_68020		2	/* runs on 68020 only			*/
#define M_SUNRISE	3	/* runs on Sunrise only			*/
	unsigned short int	a_magic;	/* magic #		      */
#else /* not sun */
	unsigned long int	a_magic;	/* magic #		      */
#endif sun
#define	OMAGIC	0407		/* old impure format */
#define	NMAGIC	0410		/* read-only text */
#define	ZMAGIC	0413		/* demand load format */

	unsigned long int	a_text;		/* length of text  segment    */
	unsigned long int	a_data;		/* length of data  segment    */
	unsigned long int	a_bss;		/* length of bss   segment    */
	unsigned long int	a_syms;		/* length of syms  segment    */
	unsigned long int	a_entry;	/* entry point addr in Text   */
	unsigned long int	a_trsize;	/* length of text rel. segment*/
	unsigned long int	a_drsize;	/* length of data rel. segment*/

#ifdef sun4
	unsigned long int	a_string;	/* length of string segment   */
	unsigned long int	a_sdata;	/* length of sdata segment    */
	unsigned long int	a_sdrsize;	/* length of sdata rel.segment*/

		/* following are unassigned and must be 0 */
	unsigned long int	a_spare5;	/* length of ????  segment    */
	unsigned long int	a_spare4;	/* length of ????  segment    */
	unsigned long int	a_spare3;	/* length of ????  segment    */
	unsigned long int	a_spare2;	/* length of ????  segment    */
	unsigned long int	a_spare1;	/* length of ????  segment    */
#endif
};

/*----------------------------------------------------------------------------
 * The remaining declarations are explicit ones for the variants of "exec",
 * present for use in cross-compilation situations.
 *----------------------------------------------------------------------------
 */

struct exec_vax	/* for VAX */
{
	unsigned long	a_magic;	/* magic number */
	unsigned long	a_text;		/* size of text segment */
	unsigned long	a_data;		/* size of initialized data */
	unsigned long	a_bss;		/* size of uninitialized data */
	unsigned long	a_syms;		/* size of symbol table */
	unsigned long	a_entry;	/* entry point */
	unsigned long	a_trsize;	/* size of text relocation */
	unsigned long	a_drsize;	/* size of data relocation */
};


struct exec_sun2_sun3	/* for Sun-1, Sun-2, and Sun-3 */
{
	unsigned char	a_toolversion;	/* version # of toolset which
					 * created the object file
					 */
	unsigned char	a_machtype;	/* machine type */
	unsigned short	a_magic;	/* magic number */
	unsigned long	a_text;		/* size of text segment */
	unsigned long	a_data;		/* size of initialized data */
	unsigned long	a_bss;		/* size of uninitialized data */
	unsigned long	a_syms;		/* size of symbol table */
	unsigned long	a_entry;	/* entry point */
	unsigned long	a_trsize;	/* size of text relocation */
	unsigned long	a_drsize;	/* size of data relocation */
};


struct exec_sun4	/* for Sun-4 */
{
	unsigned char		a_toolversion;	/* version # of toolset which
						 * created the object file    */
	unsigned char		a_machtype;	/* machine type		      */
	unsigned short int	a_magic;	/* magic #		      */

	unsigned long int	a_text;		/* length of text  segment    */
	unsigned long int	a_data;		/* length of data  segment    */
	unsigned long int	a_bss;		/* length of bss   segment    */
	unsigned long int	a_syms;		/* length of syms  segment    */
	unsigned long int	a_entry;	/* entry point addr in Text   */
	unsigned long int	a_trsize;	/* length of text rel. segment*/
	unsigned long int	a_drsize;	/* length of data rel. segment*/

	/* The above fields are compatible with Sun2/Sun3 "exec" struct. */
	/* The below fields are added ones, for Sun4.			 */

	unsigned long int	a_string;	/* length of string segment   */
	unsigned long int	a_sdata;	/* length of sdata segment    */
	unsigned long int	a_sdrsize;	/* length of sdata rel.segment*/

		/* following are unassigned and must be 0 */
	unsigned long int	a_spare5;	/* length of ????  segment    */
	unsigned long int	a_spare4;	/* length of ????  segment    */
	unsigned long int	a_spare3;	/* length of ????  segment    */
	unsigned long int	a_spare2;	/* length of ????  segment    */
	unsigned long int	a_spare1;	/* length of ????  segment    */
};

/*** #define OLD_EXEC_SIZE (sizeof(struct exec_sun2_sun3)) ***/

#endif /*!_sys_defunct_exec_h*/
