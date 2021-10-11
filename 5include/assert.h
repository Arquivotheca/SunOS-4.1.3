/*	@(#)assert.h 1.1 92/07/30 SMI; from S5R2 1.4	*/

#ifndef _assert_h
#define _assert_h

#ifdef NDEBUG
#define assert(EX)
#else
extern void _assert();
#define assert(EX) if (EX) ; else _assert("EX", __FILE__, __LINE__)
#endif

#endif /*!_assert_h*/
