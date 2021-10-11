/* Copyright (C) 1986 Sun Microsystems Inc. */
/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this source code without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * THIS PROGRAM CONTAINS SOURCE CODE COPYRIGHTED BY SUN MICROSYSTEMS, INC.
 * AND IS LICENSED TO SUNSOFT, INC., A SUBSIDIARY OF SUN MICROSYSTEMS, INC.
 * SUN MICROSYSTEMS, INC., MAKES NO REPRESENTATIONS ABOUT THE SUITABLITY
 * OF SUCH SOURCE CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT
 * EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  SUN MICROSYSTEMS, INC. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO SUCH SOURCE CODE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
 * NO EVENT SHALL SUN MICROSYSTEMS, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT,
 * INCIDENTAL, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM USE OF SUCH SOURCE CODE, REGARDLESS OF THE THEORY OF LIABILITY.
 * 
 * This source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction, 
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS
 * SOURCE CODE OR ANY PART THEREOF.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California 94043
 */
#include "common.h"
#include "queue.h"
#include "asynch.h"
#include "machsig.h"
#include "machdep.h"
#include "queue.h"
#include "cntxt.h"
#include "lwperror.h"
#include "message.h"
#include "process.h"
#ifndef lint
SCCSID(@(#) checkassym.c 1.1 92/07/30 Copyr 1987 Sun Micro);
#endif lint

/*
 * generate the CHECK macro.
 */
main()
{
	printf("extern char *__CurStack;\n");

	printf("#define CHECK(location, result) {\t\t\t\\\n");
	printf("\tasm (\"movl ___CurStack, d0\");\t\t\t\\\n");
	printf("\tasm (\"addl #%d, d0\");\t\t\t\t\\\n", STKOVERHEAD);
	printf("\tasm (\"cmpl d0, sp\");\t\t\t\t\\\n");
	printf("\tasm (\"jhi 1f\");\t\t\t\t\t\\\n");
	printf("\tlocation = result;\t\t\t\t\\\n");
	printf("\tasm (\"unlk a6\");\t\t\t\t\\\n");
	printf("\tasm (\"rts\");\t\t\t\t\t\\\n");
	printf("\tasm (\"1:\");\t\t\t\t\t\\\n");
	printf("}\n");
	exit(0);
}
