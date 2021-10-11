#ifndef lint
static	char sccsid[] = "@(#)calls.sparc.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include	"gprof.h"

    /*
     *	a namelist entry to be the child of indirect calls
     */
nltype	indirectchild = {
	"(*)",				/* the name */
	(unsigned long) 0,		/* the pc entry point */
	(unsigned long) 0,		/* aligned entry point */
	(double) 0.0,			/* ticks in this routine */
	(double) 0.0,			/* cumulative ticks in children */
	(long) 0,			/* how many times called */
	(long) 0,			/* how many calls to self */
	(double) 1.0,			/* propagation fraction */
	(double) 0.0,			/* self propagation time */
	(double) 0.0,			/* child propagation time */
	(bool) 0,			/* print flag */
	(int) 0,			/* index in the graph list */
	(int) 0, 			/* graph call chain top-sort order */
	(int) 0,			/* internal number of cycle on */
	(struct nl *) &indirectchild,	/* pointer to head of cycle */
	(struct nl *) 0,		/* pointer to next member of cycle */
	(arctype *) 0,			/* list of caller arcs */
	(arctype *) 0 			/* list of callee arcs */
    };

findcalls(parentp, p_lowpc, p_highpc)
    nltype		*parentp;
    unsigned long	p_lowpc;
    unsigned long	p_highpc;
{
    unsigned long 	instructp;
    long		length;
    nltype		*childp;
    unsigned long	destpc;

    if (textspace == 0) {
	return;
    }
    if (p_lowpc < s_lowpc) {
	p_lowpc = s_lowpc;
    }
    if (p_highpc > s_highpc) {
	p_highpc = s_highpc;
    }
#   ifdef DEBUG
	if (debug & CALLSDEBUG) {
	    printf("[findcalls] %s: 0x%x to 0x%x\n",
		    parentp -> name, p_lowpc, p_highpc);
	}
#   endif DEBUG
    length = 4;
    for (instructp = (unsigned long) textspace + p_lowpc -  TORIGIN;
	    instructp < (unsigned long) textspace + p_highpc - TORIGIN;
	    instructp += length) {

	switch (OP(instructp)) {
	    case CALL:
		/*
		 *	May be a call, better check it out.
		 */
#		ifdef DEBUG
		    if (debug & CALLSDEBUG) {
			printf("[findcalls]\t0x%x:call\n", PC_VAL(instructp));
		    }
#		endif DEBUG
	        destpc = (DISP30(instructp) << 2) + PC_VAL(instructp);
		break;

	    case FMT3_0x10:
		if (OP3(instructp) != JMPL) {
		    continue;
		}
	    /*
	     */
#		ifdef DEBUG
		    if (debug & CALLSDEBUG) {
			printf("[findcalls]\t0x%x:jmpl", PC_VAL(instructp));
		    }
#		endif DEBUG
		if (RD(instructp) == R_G0) {
#		    ifdef DEBUG
			if (debug & CALLSDEBUG) {
			    switch (RS1(instructp)) {
				case R_O7:
				    printf("\tprobably a RETL\n");
				    break;
				case R_I7:
				    printf("\tprobably a RET\n");
				    break;
				default:
				    printf(", but not a call: linked to g0\n");
			    }
			}
#		    endif DEBUG
		    continue;
		}
#		ifdef DEBUG
		    if (debug & CALLSDEBUG) {
			printf("\toperands are DST = R%d,\tSRC = R%d", 
						RD(instructp), RS1(instructp));
		    }
#		endif DEBUG
		if (IMMED(instructp)) {
#		    ifdef DEBUG
			if (debug & CALLSDEBUG) {
			    if (SIMM13(instructp) < 0) {
				printf(" - 0x%x\n", -(SIMM13(instructp)));
			    } else {
				printf(" + 0x%x\n", SIMM13(instructp));
			    }
			}
#		    endif DEBUG
		    switch (RS1(instructp)) {
			case R_G0:
			    /*
			     *	absolute address, signed immediate 13
			     */
			    destpc = SIMM13(instructp);
			    break;
			default:
			    /*
			     *	indirect call
			     */
			    addarc(parentp, &indirectchild, (long) 0);
			    continue;
		    }
		} else {
			/*
			 *	two register sources, all cases are indirect
			 */
#		    ifdef DEBUG
			if (debug & CALLSDEBUG) {
			    printf(" + R%d\n", RS2(instructp));
			}
#		    endif DEBUG
		    addarc(parentp, &indirectchild, (long) 0);
		    continue;
		}
		break;
	    default:
		continue;
	}

callinstruction:
		/*
		 *	Check that the destination is the address of 
		 *	a function; this allows us to differentiate
		 *	real calls from someone trying to get the PC,
		 *	e.g. position independent switches.
		 */
	if (destpc >= s_lowpc && destpc <= s_highpc) {
	    childp = nllookup(destpc);
#	    ifdef DEBUG
		if (debug & CALLSDEBUG) {
		    printf("[findcalls]\tdestpc 0x%x", destpc);
		    printf(" childp->name %s", childp -> name);
		    printf(" childp->value 0x%x\n", childp -> value);
		}
#	    endif DEBUG
	    if (childp -> value == destpc) {
		/*
		 *	a hit
		 */
		addarc(parentp, childp, (long) 0);
		continue;
	    }
	}
	    /*
	     *	else:
	     *	it looked like a call,
	     *	but it wasn't to anywhere.
	     */
	botched:
	    /*
	     *	something funny going on.
	     */
#	ifdef DEBUG
	    if (debug & CALLSDEBUG) {
		printf("[findcalls]\tbut it's a switch or a botch\n");
	    }
#	endif DEBUG
	continue;
    }
}

