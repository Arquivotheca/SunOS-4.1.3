|	"@(#)libg.s 1.1 92/07/30

|	Copyright (c) 1983 by Sun Microsystems, Inc.

	.data
|	this is a special line marker to denote the end
|	of the user's program, and the beginning of the library.
|	Its purpose is to make adb single-line stepping act rationally
|	around library and system calls. That is, we should only single-
|	step instructions through things which don't have line numbers
|	in them.

	.text
	.stabs	"libg.s",0x64,0,0,.	|	file name
	.stabd	0x44,0,-1		|	phoney line number
