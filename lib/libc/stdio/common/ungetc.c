#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)ungetc.c 1.1 92/07/30 SMI"; /* from S5R2 2.1 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>

int
ungetc(c, iop)
int	c;
register FILE *iop;
{
	if(c == EOF)
		return(EOF);
	if((iop->_flag & (_IOREAD|_IORW)) == 0)
		return(EOF);

	if (iop->_base == NULL)  /* get buffer if we don't have one */
		_findbuf(iop);

	if((iop->_flag & _IOREAD) == 0 || iop->_ptr <= iop->_base)
		if(iop->_ptr == iop->_base && iop->_cnt == 0)
			++iop->_ptr;
		else
			return(EOF);
	if (*--iop->_ptr != c) *iop->_ptr = c;  /* was *--iop->_ptr = c; */
	++iop->_cnt;
	return(c);
}
