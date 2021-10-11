
/* @(#)PIC.h 1.1 92/07/30 SMI; */

#define PIC_SAVE(r1,r2) \
		moveml	r1/r2,sp@-;	
#define PIC_RESTORE(r1,r2) \
		moveml	sp@+,r1/r2;
#ifdef PIC 
#define PIC_SETUP(r1)\
	movl #__GLOBAL_OFFSET_TABLE_, r1;lea pc@(0, r1:L),r1;

#define JBSR(sym, free_reg) jbsr sym,free_reg 

#else 
#define PIC_SETUP(r1)

#define JBSR(sym, free_reg) jbsr sym;
#endif 

