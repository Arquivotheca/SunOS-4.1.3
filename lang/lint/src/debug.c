#ifndef lint
static	char sccsid[] = "@(#)debug.c 1.1 92/07/30 SMI";
#endif

/* debuging functions for use with dbx */

#ifdef DEBUG

#include "cpass1.h"

        /* macros for doing double indexing */
# define R2PACK(x,y,z) (0200*((x)+1)+y+040000*z)
# define R2UPK1(x) ((((x)>>7)-1)&0177)
# define R2UPK2(x) ((x)&0177)
# define R2UPK3(x) (x>>14)
# define R2TEST(x) ((x)>=0200)

typedef enum { F_hexfloat, F_immed, F_loworder, F_highorder } Floatform;

char *
rnames[] = {  /* keyed to register number tokens */
        "*",   "%g1", "%g2", "%g3", "%g4", "%g5", "%g6", "%g7",
        "%o0", "%o1", "%o2", "%o3", "%o4", "%o5", "%o6", "%o7",
        "%l0", "%l1", "%l2", "%l3", "%l4", "%l5", "%l6", "%l7",
        "%i0", "%i1", "%i2", "%i3", "%i4", "%i5", "%i6", "%i7",
        "%f0", "%f1", "%f2", "%f3", "%f4", "%f5", "%f6", "%f7",
        "%f8",  "%f9",  "%f10", "%f11", "%f12", "%f13", "%f14", "%f15",
        "%f16", "%f17", "%f18", "%f19", "%f20", "%f21", "%f22", "%f23",
        "%f24", "%f25", "%f26", "%f27", "%f28", "%f29", "%f30", "%f31",
};

extern void eprint();

epr( p )
	NODE *p;
{
	fwalk( p, eprint, 0 );
}

static void
adrput( p )
	register NODE *p; 
{
	/* the 68k code saves lval and restores after the switch */
	register int r;
	/* output an address, with offsets, from p */

	if( p->in.op == FLD ){
	    p = p->in.left;
	}
	switch( p->in.op ){

	case NAME:
	case ICON:
		putchar('[');
		acon( p );
		putchar(']');
		return;

	case FCON:
		fcon( p, F_highorder );
		return;

	case REG:
		r = p->tn.rval;
		printf( "%s", rnames[r] );
		return;

	case OREG:
		oregput(p);
		break;

	case UNARY MUL:
		putchar('[');
		adrput( p->in.left);
		putchar(']');
		return;

	default:
		cerror( "illegal address" );
		return;

	}

}

/* print out a constant */
acon( p ) register NODE *p; 
{ 
	if( p->in.name[0] == '\0' ){
		printf( "0x%x", p->tn.lval );
	} else {
		printf( "%s", p->in.name );
		if( p->tn.lval != 0 ){
			putchar('+');
			printf( "0x%x", p->tn.lval );
		}
	}
}

oregput(p)
        register NODE *p;
{
        int base = p->tn.rval;
        int val;
 
        base = p->tn.rval;
        if (R2TEST(base)){
                val = R2UPK2(base);
                base = R2UPK1(base);
                printf("[%s+%s]", rnames[base], rnames[val]);
        } else {
                putchar( '[' );
                printf( "%s", rnames[base] );
                if( p->tn.lval != 0 || p->in.name[0] != '\0' ) {
                        putchar('+');
                        acon( p );
                }
                putchar(']');
        } /* if */
} /* oregput */

static char floatfmt[] = "0f%.7e";
static char doublefmt[] = "0f%.15e";

static
fcon( p, form )
	register NODE *p;
	Floatform form;
{
	float x;
	long *lp;

	switch(form) {
	case F_hexfloat:
		/* put out single precision representation in hex */
		x = p->fpn.dval;
		lp = (long*)&x;
		printf("0x%x", *lp);
		return;
	case F_immed:
		/*
		 * put out the constant in e-floating point format;
		 * for INIT nodes and coprocessor instructions.
		 */
		if (p->in.type == FLOAT) {
			printf(floatfmt, p->fpn.dval);
		} else {
			printf(doublefmt, p->fpn.dval);
		}
		return;
	}
}

#endif /* DEBUG */
