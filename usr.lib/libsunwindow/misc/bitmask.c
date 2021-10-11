#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)bitmask.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 *  Bitmask handling
 */

#define bitmask_c
#include <sunwindow/bitmask.h>
#include <sunwindow/sv_malloc.h>

Bitmask *
sv_bits_new_mask(max_bits)
    int max_bits;
{
    Bitmask *m;
    int n;

    m = (Bitmask *)sv_malloc(sizeof(Bitmask));
    m->bm_max_bits = max_bits;

    /* Compute number of bytes needed */

    n = (max_bits + BITSPERBYTE - 1) / BITSPERBYTE;

    /* Compute number of ints needed */

    m->bm_mask_size = (n + sizeof(unsigned int) - 1) / sizeof(unsigned int);

    m->bm_mask = (unsigned int *)sv_malloc(m->bm_mask_size * sizeof(unsigned int));
    for (n = 0; n < m->bm_mask_size; n++)
	m->bm_mask[n] = 0;

    return m;
}

Bitmask *
sv_bits_copy_mask(m)
    Bitmask *m;
{
    Bitmask *p;
    register int i;

    p = (Bitmask *)sv_malloc(sizeof(Bitmask));
    p->bm_max_bits = m->bm_max_bits;
    p->bm_mask_size = m->bm_mask_size;
    p->bm_mask = (unsigned int *)sv_malloc(p->bm_mask_size * sizeof(unsigned int));
    for (i = 0; i < p->bm_mask_size; i++)
	p->bm_mask[i] = m->bm_mask[i];

    return p;
}

void
sv_bits_dispose_mask(m)
    Bitmask *m;
{
    free(m->bm_mask);
    free(m);
}

Bitmask *
sv_bits_unset_mask(m, bit)
    register Bitmask *m;
    register int bit;
{
    register int n, rbit;

    if (bit >= m->bm_max_bits) return (Bitmask *)0;

    n = bit / (sizeof(unsigned int) * BITSPERBYTE);
    rbit = bit - n * (sizeof(unsigned int) * BITSPERBYTE);

    m->bm_mask[n] &= ~((unsigned) 1 << rbit);

    return m;
}

Bitmask *
sv_bits_set_mask(m, bit)
    register Bitmask *m;
    register int bit;
{
    register int n, rbit;

    if (bit >= m->bm_max_bits) return (Bitmask *)0;

    n = bit / (sizeof(unsigned int) * BITSPERBYTE);
    rbit = bit - n * (sizeof(unsigned int) * BITSPERBYTE);

    m->bm_mask[n] |= (unsigned) 1 << rbit;

    return m;
}

unsigned int
sv_bits_get_mask(m, bit)
    register Bitmask *m;
    register int bit;
{
    register int n, rbit;

    if (bit >= m->bm_max_bits) return 0;

    n = bit / (sizeof(unsigned int) * BITSPERBYTE);
    rbit = bit - n * (sizeof(unsigned int) * BITSPERBYTE);

    return (m->bm_mask[n] & ((unsigned) 1 << rbit));
}

int
sv_bits_cmp_mask(m1, m2)
    register Bitmask *m1, *m2;
{
    register int i;

    if (m1->bm_mask_size != m2->bm_mask_size)
        return m1->bm_mask_size - m2->bm_mask_size;

    for (i = 0; i < m1->bm_mask_size; i++)
        if (m1->bm_mask[i] != m2->bm_mask[i])
            return -1;

    return 0;
}

Bitmask *
sv_bits_and_mask(m1, m2, m3)  /* m3 <- m1 intersection m2 */
    Bitmask *m1, *m2, *m3;
{
    int max_bits;
    int max_words;
    register int i;

    if (!m1 || !m2)
        return (Bitmask *)0;

    max_bits = (m1->bm_max_bits > m2->bm_max_bits)?
            m1->bm_max_bits: m2->bm_max_bits;

    max_words = (m1->bm_mask_size > m2->bm_mask_size)?
            m1->bm_mask_size: m2->bm_mask_size;

    if (!m3)
        /* if third arg is null mask then create a new mask */
        m3 = sv_bits_new_mask(max_bits);
    else if (m3->bm_mask_size < max_words)
        /* if third arg not null and not big enough then give up */
        return (Bitmask *)0;

    m3->bm_max_bits = max_bits;

    for (i = 0; i < max_words; i++)
        m3->bm_mask[i] = m1->bm_mask[i] & m2->bm_mask[i];

    return m3;
}

Bitmask *
sv_bits_or_mask(m1, m2, m3)  /* m3 <- m1 union m2 */
    Bitmask *m1, *m2, *m3;
{
    int max_bits;
    int max_words;
    register int i;

    if (!m1 || !m2)
        return (Bitmask *)0;

    max_bits = (m1->bm_max_bits > m2->bm_max_bits)?
            m1->bm_max_bits: m2->bm_max_bits;

    max_words = (m1->bm_mask_size > m2->bm_mask_size)?
            m1->bm_mask_size: m2->bm_mask_size;

    if (!m3)
        /* if third arg is null mask then create a new mask */
        m3 = sv_bits_new_mask(max_bits);
    else if (m3->bm_mask_size < max_words)
        /* if third arg not null and not big enough then give up */
        return (Bitmask *)0;

    m3->bm_max_bits = max_bits;

    for (i = 0; i < max_words; i++)
        m3->bm_mask[i] = m1->bm_mask[i] | m2->bm_mask[i];

    return m3;
}


Bitmask *
sv_bits_not_mask(m1, m2)  /* m2 <- not m1 */
    Bitmask *m1, *m2;
{
    register int i;

    if (!m1)
        return (Bitmask *)0;

    if (!m2)
        /* if second arg is null mask then create a new mask */
        m2 = sv_bits_new_mask(m1->bm_max_bits);
    else if (m1->bm_mask_size > m2->bm_mask_size)
        /* if second arg is not null and not big enough then give up */
        return (Bitmask *)0;

    for (i = 0; i < m1->bm_mask_size; i++)
        m2->bm_mask[i] = ~m1->bm_mask[i];

    return m2;
}
