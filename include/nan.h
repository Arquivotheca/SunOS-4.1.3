/*	@(#)nan.h 1.1 92/07/30 SMI; from S5R2 1.3	*/

/* Handling of Not_a_Number's (only in IEEE floating-point standard) */

#ifndef _nan_h
#define _nan_h

#include <signal.h>
#define KILLFPE()	(void) kill(getpid(), SIGFPE)
#if u3b || u3b5 || sun
#define NaN(X)	(((union { double d; struct { unsigned :1, e:11; } s; } \
			*)&X)->s.e == 0x7ff)
#define KILLNaN(X)	if (NaN(X)) KILLFPE()
#else
#define Nan(X)	0
#define KILLNaN(X)
#endif

#endif /*!_nan_h*/
