static char     fmhsccsid[] = "@(#)fp.h 1.1 92/07/30 SMI";

/*
 * ========================================
 * sundiag related standard define
 * ========================================
 */

#define PROBE_DEV_ERROR         3

#ifdef SOFT
#define Device                  "softfp"
#define TEST_NAME               "softfp"
#endif

/*******/
#ifdef sun3
#ifdef MC68881
#define Device                  "MC68881"
#define TEST_NAME               "mc68881"
#endif
#endif sun3
/*******/

/*******/
#ifdef sun4
#ifdef FPU
#define Device                  "FPU"
#define TEST_NAME               "fputest"
#endif FPU
#endif sun4
/*******/
