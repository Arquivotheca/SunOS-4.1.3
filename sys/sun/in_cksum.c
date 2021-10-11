#ifndef lint
static	char sccsid[] = "@(#)in_cksum.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/mbuf.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

/*
 * Checksum routine for Internet Protocol family headers.
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 */

in_cksum(m, len)
	register struct mbuf *m;
	register int len;
{
	register u_short *w;
	register int sum;
	register int mlen;
	extern void swab();

	if (len <= m->m_len && (len & 1) == 0) {
		return (0xFFFF & ~ocsum(mtod(m, u_short *), len >> 1));
	}
	sum = 0;
	mlen = 0;
	for (; ;) {
		/*
		 * Each trip around loop adds in
		 * word from one mbuf segment.
		 */
		w = mtod(m, u_short *);
		if (mlen == -1) {
			/*
			 * There is a byte left from the last segment;
			 * add it into the checksum.  Don't have to worry
			 * about a carry-out here because we make sure
			 * that high part of (32 bit) sum is small below.
			 */
#ifdef i386
			sum += *(u_char *)w << 8;
#else
			sum += *(u_char *)w;
#endif
			w = (u_short *)((char *)w + 1);
			mlen = m->m_len - 1;
			len--;
		} else
			mlen = m->m_len;
		m = m->m_next;
		if (len < mlen)
			mlen = len;
		len -= mlen;
		if (mlen > 0) {
			if (((int)w & 1) == 0) {
				sum += ocsum(w, mlen>>1);
				w += mlen>>1;
				/*
				 * If we had an odd number of bytes,
				 * then the last byte goes in the high
				 * part of the sum, and we take the
				 * first byte to the low part of the sum
				 * the next time around the loop.
				 */
				if (mlen & 1) {
#ifdef i386
					sum += *(u_char *)w;
#else
					sum += *(u_char *)w << 8;
#endif
					mlen = -1;
				}
			} else {
				u_short swsum;

#ifdef i386
				sum += *(u_char *)w;
#else
				sum += *(u_char *)w << 8;
#endif
				mlen--;
				w = (u_short *)(1 + (int)w);
				swsum = ocsum(w, mlen>>1);
				swab((char *)&swsum, (char *)&swsum,
					sizeof (swsum));
				sum += swsum;
				w += mlen>>1;
				/*
				 * If we had an even number of bytes,
				 * then the last byte goes in the low
				 * part of the sum.  Otherwise we had an
				 * odd number of bytes and we take the first
				 * byte to the low part of the sum the
				 * next time around the loop.
				 */
				if (mlen & 1)
#ifdef i386
					sum += *(u_char *)w << 8;
#else
					sum += *(u_char *)w;
#endif
				else
					mlen = -1;
			}
		}
		if (len == 0)
			break;
		/*
		 * Locate the next block with some data.
		 * If there is a word split across a boundary we
		 * will wrap to the top with mlen == -1 and
		 * then add it in shifted appropriately.
		 */
		for (; ;) {
			if (m == 0) {
				printf("cksum: out of data\n");
				goto done;
			}
			if (m->m_len)
				break;
			m = m->m_next;
		}
	}
done:
	/*
	 * Add together high and low parts of sum
	 * and carry to get cksum.
	 * Have to be careful to not drop the last
	 * carry here.
	 */
	sum = (sum & 0xFFFF) + (sum >> 16);
	sum = (sum & 0xFFFF) + (sum >> 16);
	sum = (~sum) & 0xFFFF;
	return (sum);
}
