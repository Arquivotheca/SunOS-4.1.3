/*	@(#)bitmask.h 1.1 92/07/30 SMI */

/*
 *  Bitmask handling declarations
 */

#ifndef bitmask_h
#define bitmask_h

typedef struct bm_ {
    unsigned int *bm_mask;
    int bm_max_bits;
    int bm_mask_size;
} Bitmask;

#ifndef BITSPERBYTE
#define BITSPERBYTE 8
#endif

#ifndef bitmask_c

extern Bitmask * sv_bits_new_mask();
extern void sv_bits_dispose_mask();
extern Bitmask * sv_bits_set_mask();
extern unsigned int sv_bits_get_mask();
extern int sv_bits_cmp_mask();
extern Bitmask * sv_bits_copy_mask();

#endif bitmask_c

#endif bitmask_h
