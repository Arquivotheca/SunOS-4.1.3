/*	%Z%%M% %I% %E% SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:ind_data.c	1.7.2.1"
#include <stdio.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <rfs/ns_xdr.h>
#include <rfs/nserve.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include "nsdb.h"
#include "stdns.h"
#include "nslog.h"
#include "string.h"
/*******************************************************************
 *
 *	These functions convert data from a machine independent
 *	external format to internal values.
 *
 ******************************************************************/
char	*prplace();
#define c_sizeof(s)	align(L_SIZE + ((s)?strlen(s)+1:1))

/* initialize place in a block	*/
place_p
setplace(bptr,size)
char	*bptr;
int	size;
{
	place_p	retp;

	LOG4(L_CONV,"(%5d) setplace(bptr=%x,size=%d)\n",
		Logstamp,bptr,size);
	if ((retp = (place_p) malloc(sizeof(place_t))) == NULL) {
		PLOG3("(%5d) setplace: malloc(%d) failed\n",
			Logstamp,sizeof(place_t));
		return(NULL);
	}
	retp->p_start = retp->p_ptr = bptr;
	retp->p_end = bptr + size;

	return(retp);
}
/*	realloc pp to a new size	*/
int
explace(pp,size)
place_p	pp;
int	size;
{
	char	*bptr;

	LOG4(L_CONV,"(%5d) explace(pp=%s,size=%d)\n",
		Logstamp,prplace(pp),size);
	bptr = pp->p_start;

	if (size == 0)	/* increase by DBLKSIZ	*/
		size = (pp->p_end - bptr) + DBLKSIZ;

	if ((pp->p_end - bptr) > size)
		return(RFS_SUCCESS);	/* don't reduce size	*/

	free(bptr);
	if ((bptr = realloc(bptr,size)) == NULL) {
		PLOG4("(%5d) explace: realloc(%d,%d) failed\n",
			Logstamp,bptr,size);
		return(RFS_FAILURE);
	}
	/* adjust p_ptr and p_end	*/
	pp->p_end = bptr + size;
	pp->p_ptr = bptr + (pp->p_ptr - pp->p_start);
	pp->p_start = bptr;
	LOG4(L_CONV,"(%5d) explace succeeds, returns pp=%s, size=%d\n",
		Logstamp,prplace(pp),size);
	return(RFS_SUCCESS);
}
/*
 * read a string from block
 */
char	*
getstr(pp,str,size)
place_p	pp;
char	*str;
int	size;
{
	int	rsize;	/* real size of string in block	*/
	char	buffer[BUFSIZ];

	LOG5(L_CONV,"(%5d) getstr(pp=%s,str=%x,size=%d)\n",
		Logstamp,prplace(pp),(str)?str:"NULL",size);

	if (!bump(pp))
		return NULL;
	fcanon(rfs_string,pp->p_ptr,buffer);
	pp->p_ptr += c_sizeof(buffer);
	rsize = strlen(buffer);
	if (str == NULL) {
		if ((str=malloc(rsize+1)) == 0) {
			PLOG3("(%5d) getstr: malloc(%d) failed\n",
				Logstamp,rsize+1);
			return(NULL);
		}
		size = rsize;
	}
	else if (rsize > size)
		size--;	/* leave room for null	*/

	strncpy(str,buffer,size);
	str[size] = NULL;
	LOG3(L_CONV,"(%5d) getstr returns str=%s\n",
		Logstamp,(str)?str:"NULL");
	return(str);
}
/* negative ret could be valid or error, for now assume only positive numbers */
long
getlong(pp)
place_p	pp;
{
	long	ret=0;

	LOG3(L_CONV,"(%5d) getlong(pp=%s)\n",
		Logstamp,prplace(pp));

	if (overbyte(pp,L_SIZE))
		return(-1);

	fcanon(rfs_long,pp->p_ptr,(char *) &ret);
	pp->p_ptr += L_SIZE;

	LOG3(L_CONV,"(%5d) getlong returns %ld\n",Logstamp,ret);
	return(ret);
}
/*
 *	now the put routines
 */
int
putlong(pp,value)
place_p	pp;
long	value;
{
	int	size;

	LOG4(L_CONV,"(%5d) putlong(pp=%s, value=%ld)\n",
		Logstamp,prplace(pp),value);

	if (overbyte(pp,L_SIZE))
		return(RFS_FAILURE);

	bump(pp);
	size = tcanon(rfs_long,(char *) &value,pp->p_ptr);
	pp->p_ptr += size;
	return(RFS_SUCCESS);
}
/* put a string into block, return # of bytes copied (including null) */
int
putstr(pp,str)
place_p	pp;
char	*str;
{
	int	i;
	char	*nstr = "";

	LOG4(L_CONV,"(%5d) putstr(pp=%s, str=%s)\n",
		Logstamp,prplace(pp),(str)?str:"NULL");

	bump(pp);
	if (!str)
		str = nstr;

	i = c_sizeof(str);
	if (overbyte(pp,i)) {
		LOG3(L_ALL,"(%5d) putstr: overran end size=%d\n",Logstamp,i);
		return(0);
	}

	i = tcanon(rfs_string,str,pp->p_ptr);
	pp->p_ptr += i;
	LOG3(L_CONV,"(%5d) putstr puts %d bytes in block\n",Logstamp,i);

	return(i);
}
char	*
prplace(pp)
place_p	pp;
{
	static	char	buf[64];

	sprintf(buf,"{s=%x, p=%x, e=%x}",pp->p_start,pp->p_ptr,pp->p_end);
	return(buf);
}
int
dumpblock(block,size)
char	*block;
int	size;
{
	int	i;
	int	*x;

	for (i=0; i <= size; i += 4) {
		if (i % 16 == 0)
			printf("\n");
		x = (int *) &block[i];
		printf("%08x ",*x);
	}
	printf("\n");
}
int
bump(pp)
place_p	pp;
{
	pp->p_ptr = (char *) align(pp->p_ptr);

	if (pp->p_ptr >= pp->p_end)
		return(0);
	else
		return(1);
}
