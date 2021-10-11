	.data
|	.asciz	"@(#)ptwo.s 1.1 92/07/30 Copyr 1983 Sun Micro"
	.even
	.text

|	Copyright (c) 1983 by Sun Microsystems, Inc.

| ptwo: 
| used by division routines to turn small powers of two into shift counts.
	.globl	ptwo
	.text
ptwo:	| table indexed by a small power of two to yield shift count
	.byte	0, 0, 1, 1, 2, 2, 2, 2	|   1, 2, 4
	.byte	3, 3, 3, 3, 3, 3, 3, 3	|   8
	.byte	4, 4, 4, 4, 4, 4, 4, 4	|  16
	.byte	4, 4, 4, 4, 4, 4, 4, 4
	.byte	5, 5, 5, 5, 5, 5, 5, 5	|  32
	.byte	5, 5, 5, 5, 5, 5, 5, 5
	.byte	5, 5, 5, 5, 5, 5, 5, 5
	.byte	5, 5, 5, 5, 5, 5, 5, 5
	.byte	6, 6, 6, 6, 6, 6, 6, 6	|  64
	.byte	6, 6, 6, 6, 6, 6, 6, 6
	.byte	6, 6, 6, 6, 6, 6, 6, 6
	.byte	6, 6, 6, 6, 6, 6, 6, 6
	.byte	6, 6, 6, 6, 6, 6, 6, 6
	.byte	6, 6, 6, 6, 6, 6, 6, 6
	.byte	6, 6, 6, 6, 6, 6, 6, 6
	.byte	6, 6, 6, 6, 6, 6, 6, 6
	.byte	7, 7, 7, 7, 7, 7, 7, 7	| 128
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
	.byte	7, 7, 7, 7, 7, 7, 7, 7
