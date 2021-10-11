	.data
|	.asciz	"@(#)fp_commons.s 1.1 92/07/30 Copyr 1983 Sun Micro"
	.even

	.globl __fpabase
__fpabase = 0xe0000000

	.globl __skybase	| sky access address
__skybase : .long 0		| 0 if fp_state_skyffp != fp enabled
				| 1 if fp_state_skyffp = fp enabled

