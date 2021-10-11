/*
 * @(#)pmem.h - Rev 1.1 - 7/30/92
 *
 * pmem.h:  header file for pmem.c.
 *
 */

/*
 * error code for this test.
 */
#define BAD_NAMELIST            4
#define BAD_PHYSMEM_VALUE       5
#define NO_PHYSMEM              6
#define NO_MEM_FILE             8
#define BAD_VALLOC              9
#define BAD_MMAP                10
#define ID_MACH_SHIFT           24      /* machine id in MSB of gethostid() */
#define ID_MACH_MASK            0xf0

#ifndef MAP_FIXED
#define MAP_FIXED               0
#endif  

#ifndef SUN4C_ARCH
#define SUN4C_ARCH		0x50
#endif

struct nlist    nl[] = {
#define NL_PHYSMEM 0
#ifndef SVR4
    {"_physmem"},
#else
    {"physmem"},
#endif
#define NL_ROMP 1
    {"_romp"},
#define NL_PHYSMEMORY 2
    {"_physmemory"},
    {""}
};
