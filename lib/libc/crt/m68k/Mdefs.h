/*      @(#)Mdefs.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#define FMOVEDIN \
	moveml	d0/d1,sp@- ; \
	fmoved	sp@,fp0

#define FMOVEDINC \
	moveml	d0/d1,sp@- ; \
	fmoved	sp@+,fp0

#define FMOVEDOUT \
	fmoved	fp0,sp@ ; \
	moveml	sp@+,d0/d1

#define FMOVEDEC \
	fmoved	fp0,sp@- ; \
	moveml	sp@+,d0/d1

#define STACKANDRZERO \
	fmovel	fpc,d0 ; \
	movel	d0,sp@- ; \
	andb	#ROUNDMASK,d0 ; \
	orb	#RZERO,d0 ; \
	fmovel	d0,fpc

