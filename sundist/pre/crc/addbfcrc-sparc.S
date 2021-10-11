	.seg    "data"
_sccsid:
	.asciz  "@(#)addbfcrc-sparc.S 1.1 92/07/30"

#ifdef USE_ASM
!
! tcrc addbfcrc(buf, size, crc)
!	register char *buf;
!	register int size;
!       register tcrc crc;
! {
!       register int i;
!
!       for (i = 0; i < size; ++i)
!               crc = crctab[(int)(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
!       return(crc);
! }
!
	.seg	"text"
	.proc	14
	.global	_addbfcrc
_addbfcrc:
	tst	%o1
	ble	out
	sethi	%hi(_crctab),%o4
	or	%o4,%lo(_crctab),%o4	! %o4 = crctab
	add	%g0,%o0,%o5
	add	%o1,%o0,%o0		! %o0 = buf + size
again:
	ldsb	[%o5],%o3		! %o3 = buf[i]
	srl	%o2,8,%o1		! %o1 = crc >> 8
	xor	%o2,%o3,%o3		! %o3 = crc ^ buf[i]
	and	%o3,255,%o3		! mask...
	sll	%o3,2,%o3		!  ...and scale to table index
	ld	[%o3+%o4],%o3		! %o3 = crctab[%o3]
	inc	%o5
	cmp	%o5,%o0
	blu	again
	xor	%o3,%o1,%o2		! crc = crctab[%o3] ^ (crc >> 8)
out:
	retl
	mov	%o2,%o0
#endif
