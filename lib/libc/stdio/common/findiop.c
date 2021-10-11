#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)findiop.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <errno.h>
#include "iob.h"

#define active(iop)	((iop)->_flag & (_IOREAD|_IOWRT|_IORW))

extern	char	*calloc();

static unsigned char sbuf[NSTATIC][_SBFSIZ];
unsigned char (*_smbuf)[_SBFSIZ] = sbuf;
static	FILE	**iobglue;
static	FILE	**endglue;

/*
 * Find a free FILE for fopen et al.
 * We have a fixed static array of entries, and in addition
 * may allocate additional entries dynamically, up to the kernel
 * limit on the number of open files.
 * At first just check for a free slot in the fixed static array.
 * If none are available, then we allocate a structure to glue together
 * the old and new FILE entries, which are then no longer contiguous.
 */
FILE *
_findiop()
{
	register FILE **iov, *iop;
	register FILE *fp;

	if(iobglue == NULL) {
		for(iop = _iob; iop < _iob + NSTATIC; iop++)
			if(!active(iop))
				return(iop);

		if(_f_morefiles() == 0) {
			errno = ENOMEM;
			return(NULL);
		}
	}

	iov = iobglue;
	while(*iov != NULL && active(*iov))
		if (++iov >= endglue) {
			errno = EMFILE;
			return(NULL);
		}

	if(*iov == NULL)
		*iov = (FILE *)calloc(1, sizeof **iov);

	return(*iov);
}

_f_morefiles()
{
	register FILE **iov;
	register FILE *fp;
	register unsigned char *cp;
	int nfiles;

	nfiles = getdtablesize();

	iobglue = (FILE **)calloc(nfiles, sizeof *iobglue);
	if(iobglue == NULL)
		return(0);

	if((_smbuf = (unsigned char (*)[_SBFSIZ])malloc(nfiles * sizeof *_smbuf)) == NULL) {
		free((char *)iobglue);
		iobglue = NULL;
		return(0);
	}

	endglue = iobglue + nfiles;

	for(fp = _iob, iov = iobglue; fp < &_iob[NSTATIC]; /* void */)
		*iov++ = fp++;

	return(1);
}

f_prealloc()
{
	register FILE **iov;
	register FILE *fp;

	if(iobglue == NULL && _f_morefiles() == 0)
		return;

	for(iov = iobglue; iov < endglue; iov++)
		if(*iov == NULL)
			*iov = (FILE *)calloc(1, sizeof **iov);
}

void
_fwalk(function)
register int (*function)();
{
	register FILE **iov;
	register FILE *fp;

	if(function == NULL)
		return;

	if(iobglue == NULL) {
		for(fp = _iob; fp < &_iob[NSTATIC]; fp++)
			if(active(fp))
				(*function)(fp);
	} else {
		for(iov = iobglue; iov < endglue; iov++)
			if(*iov && active(*iov))
				(*function)(*iov);
	}
}
