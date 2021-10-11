/*	@(#)fatal.h 1.1 92/07/30 SMI; from System III	*/

#ifndef jmp_buf
# include "setjmp.h"
#endif
extern	int	Fflags;
extern	char	*Ffile;
extern	int	Fvalue;
extern	int	(*Ffunc)();
extern	jmp_buf	Fjmp;
extern	char	*nsedelim;

# define FTLMSG		0100000
# define FTLCLN		 040000
# define FTLFUNC	 020000
# define FTLACT		    077
# define FTLJMP		     02
# define FTLEXIT	     01
# define FTLRET		      0

# define FSAVE(val)	SAVE(Fflags,old_Fflags); Fflags = val;
# define FRSTR()	RSTR(Fflags,old_Fflags);

extern	char	*nse_file_trim();
