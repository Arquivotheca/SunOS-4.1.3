/*	@(#)logmalloc.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*  #ident	"@(#)libns:logmalloc.c	1.3" */
#include <stdio.h>
#include "nslog.h"
#undef free
#undef malloc
#undef calloc
#undef realloc
#include <malloc.h>

void
xfree(p)
char	*p;
{
	LOG2(L_MALLOC,"free(0x%x)\n",p);
	fflush(Logfd);
	free(p);
	return;
}
malloc_t
xmalloc(size)
unsigned size;
{
	malloc_t ret;
	LOG2(L_MALLOC,"malloc(%d)",size);
	fflush(Logfd);
	ret = malloc(size);
	LOG2(L_MALLOC,"returns 0x%x\n",ret);
	return (ret);
}
malloc_t
xrealloc(p,size)
malloc_t p;
unsigned size;
{
	malloc_t ret;
	LOG3(L_MALLOC,"realloc(0x%x,%d) ",p,size);
	fflush(Logfd);
	ret = realloc(p,size);
	LOG2(L_MALLOC,"returns 0x%x\n",ret);
	return (ret);
}
malloc_t 
xcalloc(n,size)
unsigned n,size;
{
	malloc_t ret;
	LOG3(L_MALLOC,"calloc(%d,%d) ",n,size);
	fflush(Logfd);
	ret = calloc(n,size);
	LOG2(L_MALLOC,"returns 0x%x\n",ret);
	return (ret);
}
