#ident "@(#)exc_handlers.s	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */


/* 	@(#)exc_handlers.s	1.3	2/17/88
	Fault handlers for the cruel system simulation mode
*/

	/* ASI values used in fault handlers for cruel system model */
	ASI_IFAULT = 126
	ASI_DFAULT = 127
	ASI_CLEARINTS = 128

#define iae_handler lda [%l1] ASI_IFAULT, %g0; jmp %l1; rett %l2; nop
#define dae_handler lda [%l1] ASI_DFAULT, %g0; jmp %l1; rett %l2; nop
#define int_handler jmp %l1; rett %l2; nop; nop


