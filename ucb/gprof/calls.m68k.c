#ifndef lint
static	char sccsid[] = "@(#)calls.m68k.c 1.1 92/07/30 SMI"; /* from UCB 1.1 02/16/83 */
#endif

#include	"gprof.h"

#define TORIGIN N_TXTADDR( xbuf )

struct instruct {
	unsigned short	opcode:10,
			modefield:3,
			regfield:3;
};

    /*
     *	a namelist entry to be the child of indirect calls
     */
nltype	indirectchild = {
	"(*)" ,				/* the name */
	(unsigned long) 0 ,		/* the pc entry point */
	(unsigned long) 0 ,		/* aligned entry point */
	(double) 0.0 ,			/* ticks in this routine */
	(double) 0.0 ,			/* cumulative ticks in children */
	(long) 0 ,			/* how many times called */
	(long) 0 ,			/* how many calls to self */
	(double) 1.0 ,			/* propagation fraction */
	(double) 0.0 ,			/* self propagation time */
	(double) 0.0 ,			/* child propagation time */
	(bool) 0 ,			/* print flag */
	(int) 0 ,			/* index in the graph list */
	(int) 0 , 			/* graph call chain top-sort order */
	(int) 0 ,			/* internal number of cycle on */
	(struct nl *) &indirectchild ,	/* pointer to head of cycle */
	(struct nl *) 0 ,		/* pointer to next member of cycle */
	(arctype *) 0 ,			/* list of caller arcs */
	(arctype *) 0 			/* list of callee arcs */
    };

operandenum
operandmode( modep )
    struct instruct	*modep;
{
    long	usesreg = modep -> regfield;
    
    switch ( modep -> modefield ) {
	case 0:
	    return datareg;
	case 1:
	    return addrreg;
	case 2:
	    return defer;
	case 3:
	    return postinc;
	case 4:
	    return predec;
	case 5:
	    return displ;
	case 6:
	    return indexed;
	case 7:
	    switch ( usesreg ) {
		case 0:
		    return shortabsolute;
		case 1:
		    return longabsolute;
		case 2:
		    return pcrel;
		case 3:
		    return pcindexed;
		case 4:
		    return immediate;
	    }
    }
    return ((operandenum)-1);
}

char *
operandname( mode )
    operandenum	mode;
{
    
    switch ( mode ) {
	case datareg:
	    return "data register";
	case addrreg:
	    return "address register";
	case defer:
	    return "register indirect";
	case postinc:
	    return "postincrement";
	case predec:
	    return "predecrement";
	case displ:
	    return "register+displacement";
	case indexed:
	    return "indexed";
	case shortabsolute:
	    return "short absolute";
	case longabsolute:
	    return "long absolute";
	case pcrel:
	    return "pc relative";
	case pcindexed:
	    return "pc indexed";
	case immediate:
	    return "immediate";
    }
    return "unknown";
}

long
operandlength( modep )
    struct instruct	*modep;
{
    
    if (*(char *)modep == BSR) {
	if (*((char *)modep+1) == 0)
	    return 0;
	return 2;
    }
    switch ( operandmode( modep ) ) {
	case datareg:
	case addrreg:
	case defer:
	case postinc:
	case predec:
	    return 0;
	case displ:
	case indexed:
	case shortabsolute:
	case pcrel:
	case pcindexed:
	    return 2;
	case longabsolute:
	    return 4;
	case immediate:
	    return 4;
    }
    /* NOTREACHED */
}

unsigned long
reladdr( modep )
    struct instruct	*modep;
{
    operandenum	mode = operandmode( modep );
    short	*sp;
    long	*lp;

    sp = (short *) modep;
    sp += (sizeof modep)/(sizeof *sp);	/* skip over the instruction */
    switch ( mode ) {
	default:
	    fprintf( stderr , "[reladdr] not relative address\n" );
	    return (unsigned long) modep;
	case pcrel:
	    return (unsigned long)sp + *sp + TORIGIN + 2 ;
    }
}

findcalls( parentp , p_lowpc , p_highpc )
    nltype		*parentp;
    unsigned long	p_lowpc;
    unsigned long	p_highpc;
{
    unsigned char	*instructp;
    long		length;
    nltype		*childp;
    operandenum		mode;
    operandenum		firstmode;
    unsigned long	destpc;

    if ( textspace == 0 ) {
	return;
    }
    if ( p_lowpc < s_lowpc ) {
	p_lowpc = s_lowpc;
    }
    if ( p_highpc > s_highpc ) {
	p_highpc = s_highpc;
    }
#   ifdef DEBUG
	if ( debug & CALLSDEBUG ) {
	    printf( "[findcalls] %s: 0x%x to 0x%x\n" ,
		    parentp -> name , p_lowpc , p_highpc );
	}
#   endif DEBUG
    for (   instructp = textspace + p_lowpc -  TORIGIN;
	    instructp < textspace + p_highpc - TORIGIN;
	    instructp += length ) {
	length = 2;
	if ( ((struct instruct *)instructp)->opcode == JSR ) {
		/*
		 *	may be a call, better check it out.
		 *	skip the count of the number of arguments.
		 */
#	    ifdef DEBUG
		if ( debug & CALLSDEBUG ) {
		    printf( "[findcalls]\t0x%x:jsr" , instructp - textspace );
		}
#	    endif DEBUG
	    mode = operandmode( (struct instruct *) instructp  );
#	    ifdef DEBUG
		if ( debug & CALLSDEBUG ) {
		    printf( "\toperand is %s\n" , operandname( mode ) );
		}
#	    endif DEBUG
	    switch ( mode ) {
		case defer:
		case displ:
		case indexed:
		case pcindexed:
			/*
			 *	indirect call: call through pointer
			 *	either	*d(r)	as a parameter or local
			 *		(r)	as a return value
			 *		*f	as a global pointer
			 *	[are there others that we miss?,
			 *	 e.g. arrays of pointers to functions???]
			 */
		    addarc( parentp , &indirectchild , (long) 0 );
		    length += operandlength( (struct instruct *) instructp );
		    continue;
		case shortabsolute:
		    destpc = *(short *)(instructp +2);
		    goto callinstruction;
		case longabsolute:
		    destpc = *(long *)(instructp +2);
		    goto callinstruction;
		case pcrel:
			/*
			 *	regular pc relative addressing
			 *	check that this is the address of 
			 *	a function.
			 */
		    destpc = reladdr( (struct instruct *) instructp )
				- (unsigned long) textspace;
		callinstruction:
		    if ( destpc >= s_lowpc && destpc <= s_highpc ) {
			childp = nllookup( destpc );
#			ifdef DEBUG
			    if ( debug & CALLSDEBUG ) {
				printf( "[findcalls]\tdestpc 0x%x" , destpc );
				printf( " childp->name %s" , childp -> name );
				printf( " childp->value 0x%x\n" ,
					childp -> value );
			    }
#			endif DEBUG
			if ( childp -> value == destpc ) {
				/*
				 *	a hit
				 */
			    addarc( parentp , childp , (long) 0 );
			    length += operandlength( (struct instruct *)
					    instructp );
			    continue;
			}
			goto botched;
		    }
			/*
			 *	else:
			 *	it looked like a calls,
			 *	but it wasn't to anywhere.
			 */
		    goto botched;
		default:
		botched:
			/*
			 *	something funny going on.
			 */
#		    ifdef DEBUG
			if ( debug & CALLSDEBUG ) {
			    printf( "[findcalls]\tbut it's a botch\n" );
			}
#		    endif DEBUG
		    length = 2;
		    continue;
	    }
	} else if ( *instructp == BSR ) {
	    /*
	     * bsr's have fewer choices, but are more easily confused
	     * with other random bit patterns.
	     */
	    destpc = *(instructp+1);
	    if ( destpc == 0 )
		destpc = *(short *)(instructp + 2);
	    destpc += (unsigned long)instructp + TORIGIN + 2 
		- (unsigned long)textspace;
#	    ifdef DEBUG
		if ( debug & CALLSDEBUG ) {
		    printf( "[findcalls]\t0x%x:bsr\n" , instructp - textspace );
		}
#	    endif DEBUG
	    goto callinstruction;
	}
    }
}
