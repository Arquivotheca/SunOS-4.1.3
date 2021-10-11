#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)rpc_dtablesize.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Cache the result of getdtablesize(), so we don't have to do an
 * expensive system call every time.
 */
_rpc_dtablesize()
{
	static int size;

	if (size == 0) {
		size = getdtablesize();
	}
	return (size);
}
