/ 
/	.data
/	.asciz	"@(#)log10.s 1.1 92/07/30 SMI"
/	.even
/	.text
/
/	Copyright (c) 1987 by Sun Microsystems, Inc.

	.text
	.globl	log10
log10:
	fldl	4(%esp)
	fldlg2	
	fxch			/ st = arg, st(1) = log10(2)
	fyl2x			/ st = log10(arg) = log10(2)*log2(arg)
	ret

