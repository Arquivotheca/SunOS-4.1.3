#ifndef lint
static  char sccsid[] = "@(#)xdrio.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "toc.h"

/*
 * local data 
 */

static FILE *file_ptr = NULL;	/* file pointer for actual i/o */
static XDR xdrstruct;		/* actual xdr structure */

/*
 * open routine 
 */

XDR *
xdr_open(ptr,writeflag)
FILE *ptr;
bool_t writeflag;
{
	extern int io_read(), io_write();

	if(file_ptr)
		return((XDR *) 0);
	else
		file_ptr = ptr;

	xdrstruct.x_op = (writeflag)?XDR_ENCODE:XDR_DECODE;
	xdrrec_create(&xdrstruct, 512, 512,
		(char *) file_ptr, io_read, io_write);
	if(!writeflag)
		xdrrec_skiprecord(&xdrstruct);

	return (&xdrstruct);
}

/*
 * close xdr stream
 */

bool_t
xdr_close(xp)
XDR *xp;
{
	xdr_destroy(xp);
	file_ptr = NULL;
	return (TRUE);
}

/*
 * low level (xdr called) read & write routines.....
 */

static
int io_read(handle,buf,nbytes)
char *handle;
char *buf;
int nbytes;
{
	int nr;
	/*
	 * Read a bunch of bytes. Presumably any reasonable amount will do.
	 */

	if((nr = fread(buf, sizeof (char), nbytes, (FILE *) handle)) == 0)
		return(-1);
	else
		return(nr);
}

static
int io_write(handle,buf,nbytes)
char *handle;
char *buf;
int nbytes;
{
	if(fwrite(buf, sizeof (char), nbytes, (FILE *) handle) != nbytes)
		return(0);	
	else
		return(nbytes);
}


/*
 * For convenience in packaging ....
 *
 * read_xdr_toc() - decode xdr table of contents
 * into an array of toc_entry structures....
 *
 * Arguments:	FILE *ip --> Already opened stdio input stream
 *		toc_entry tab[] -> base of array
 *		int max -> # of elements (max) +1 in array
 *	Returns # of entries read
 */


int
read_xdr_toc(ip,dinfo,vinfo,etab,max)
FILE *ip;
distribution_info *dinfo;
volume_info *vinfo;
toc_entry etab[];
int max;
{
	auto u_int magic, version;
	XDR *xdr_open();
	register XDR *xp;
	register toc_entry *tp;

	bzero((char *) etab, max * sizeof (etab[0]));
	xp = xdr_open(ip,FALSE);
	if(!xdr_u_int(xp,&magic) || magic != TOCMAGIC)
	{
		(void) fprintf(stderr,
			"read_xdr_toc: magic number wrong (%#x)\n",magic);
		(void) xdr_close(xp);
		return(0);
	}
	else if(!xdr_u_int(xp,&version) || version != TOCVERSION)
	{
		(void) fprintf(stderr,
			"read_xdr_toc: version number wrong (%#x)\n",version);
		(void) xdr_close(xp);
		return(0);
	}
	else if(!xdr_distribution_info(xp,dinfo))
	{
		(void) fprintf(stderr,
			"read_xdr_toc: failed to read distribution header\n");
		(void) xdr_close(xp);
		return(0);
	}
	else if(!xdr_volume_info(xp,vinfo))
	{
		(void) fprintf(stderr,
			"read_xdr_toc: failed to read volume header\n");
		(void) xdr_close(xp);
		return(0);
	}

	if(dinfo->nentries > max)
	{
		(void) fprintf(stderr,
			"read_xdr_toc: etab not big enough\n");
		(void) xdr_close(xp);
		return(0);
	}
	
	tp = etab;

	while( xdr_toc_entry(xp,tp) )
	{
		tp++;
		if( tp >= &etab[dinfo->nentries] )
			break;
		xdrrec_skiprecord(xp);
	}

	(void) xdr_close(xp);
	return(tp-etab);
}

bool_t
write_xdr_toc(op,dinfo,vinfo,etab,nentries)
FILE *op;
distribution_info *dinfo;
volume_info *vinfo;
toc_entry etab[];
int nentries;
{
	auto u_int magic, version;
	XDR *xdr_open();
	register XDR *xp;
	register toc_entry *tp;

	dinfo->nentries = nentries;
	magic = TOCMAGIC;
	version = TOCVERSION;
	xp = xdr_open(op,TRUE);
	if(!xdr_u_int(xp,&magic))
	{
		(void) fprintf(stderr,
			"write_xdr_toc: magic number xfer failed\n");
		(void) xdr_close(xp);
		return(FALSE);
	}
	else if(!xdr_u_int(xp,&version))
	{
		(void) fprintf(stderr,
			"write_xdr_toc: version number xfer failed\n");
		(void) xdr_close(xp);
		return(FALSE);
	}
	else if(!xdr_distribution_info(xp,dinfo))
	{
		(void) fprintf(stderr,
			"write_xdr_toc: failed to write distribution header\n");
		(void) xdr_close(xp);
		return(FALSE);
	}
	else if(!xdr_volume_info(xp,vinfo))
	{
		(void) fprintf(stderr,
			"write_xdr_toc: failed to write volume header\n");
		(void) xdr_close(xp);
		return(FALSE);
	}

	for(tp = etab; tp < &etab[nentries]; tp++)
	{
		if(!xdr_toc_entry(xp,tp))
		{
			fprintf(stderr,
				"xdr_write_toc: failed to write entry\n");
			(void) xdr_close(xp);
			return(FALSE);
		}
		if(!xdrrec_endofrecord(xp, TRUE))
		{
			fprintf(stderr,"xdr_write_toc: flush rec failed\n");
			(void) xdr_close(xp);
			return(FALSE);
		}
	}

	(void) xdr_close(xp);
	return(TRUE);
}
