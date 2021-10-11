/*
 * @(#)mptest.h - Rev 1.1 - 7/30/92
 *
 * mptest.h:  header file for mptest.c.
 *
 */

#define ONE_TEST                    1
#define TWO_TESTS                   2
#define THREE_TESTS                 3
#define FOUR_TESTS                  4
#define ALL_TESTS_ENABLED           0xf
#define CACHE_TIMEOUT               2000000

/* error code */
#define OPEN_FILE_ERR               20
#define MMAP_ERR                    21
#define SHMGET_ERR                  22
#define RDM_COMPARE_ERR             23
#define FORK_ERR	            24
#define LINPACK_ERR	            25
#define OPEN_KMEM_ERR               26
#define OPEN_MEM_ERR                27
#define IOCTL_ERR                   28
#define CACHE_TEST_MMAP_ERR         29
#define NUMBER_TESTS_ERR            30
#define NUM_PROCESSORS_ENABLED_ERR  31
#define CANNOT_RUN_MP               32
#define NOT_MP_SYSTEM               33
#define OPEN_DEV_NULL_ERR           34
#define MIOCSPAM_ERR                35
#define GETNEXTBITMSK_ERR           36
#define ONE_LINE_CACHE_ERR          37
#define MULTI_LINE_CACHE_ERR        38
#define SUBTEST_SELECT_ERR          39
#define MPLOCK_FAIL                 40

/* shared memory and i/o tests define */
#define MAX_STRING                  50 
#define CACHE_BLOCK                 512
#define BLOCK_SIZE                  16
#define CLEAR                       0
#define FALSE                       0
#define PATTERN                     0x10
#define FLAG_SIZE                   16   
#define DONE_READING                2
#define DONE_WRITING                1
#define CACHE_LINE                  32

/* linpack test define */
/* Double or single precision defines */
#ifdef  DP 
#define PREC                "double"
#define LINPACK             dlinpack_test
#define LINSUB              dlinsub
#define MATGEN              dmatgen
#define GEFA                dgefa
#define GESL                dgesl
#define AXPY                daxpy
#define SCAL                dscal
#define IAMAX               diamax
#define EPSLON              depslon
#define MXPY                dmxpy
#define REAL                double
#define ZERO                0.0e0
#define ONE                 1.0e0
#define RESIDN              1.6711730035118721e+00
#define RESID               7.4162898044960457e-14
#define EPS                 2.2204460492503131e-16
#define X11                 -1.4988010832439613e-14
#define XN1                 -1.8984813721090177e-14
#else
#define PREC                "single"
#define LINPACK             slinpack_test
#define LINSUB              slinsub
#define MATGEN              smatgen
#define GEFA                sgefa
#define GESL                sgesl
#define AXPY                saxpy
#define SCAL                sscal
#define IAMAX               siamax
#define EPSLON              sepslon
#define MXPY                smxpy
#define REAL                float
#define ZERO                0.0
#define ONE                 1.0
#define RESIDN              1.8984881639480591e+00
#define RESID               4.5233617129269987e-05
#define EPS                 1.1920928955078125e-07
#define X11                 -1.3113021850585938e-05
#define XN1                 -1.3053417205810547e-05
#endif

REAL EPSLON();
double fabs();
