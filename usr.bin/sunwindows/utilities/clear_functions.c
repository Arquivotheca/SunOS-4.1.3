#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)clear_functions.c 1.1 92/07/30";
#endif
#endif

#include <sys/types.h>
#include <stdio.h>
#include <sunwindow/notify.h>
#include <suntool/selection_svc.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

/*ARGSUSED*/
#ifdef STANDALONE
main(argc, argv)
#else
int clear_functions_main(argc, argv)
#endif STANDALONE
{
	char	*handle;

	handle = seln_create((void (*) ())0,(Seln_result (*) ())0,(char *) &handle);
	if (handle == (char *) NULL)  {
		(void)fprintf(stderr, "Can't find selection service to clear.\n");
		EXIT(1);
	}
	seln_clear_functions();
	seln_destroy(handle);
	EXIT(0);
}
