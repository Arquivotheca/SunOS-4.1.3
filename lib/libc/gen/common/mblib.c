/*
 * misc routines
 */


#if !defined(lint) && defined(SCCSIDS)
static  char *sccsid = "@(#)mblib.c 1.1 92/07/30 SMI";
#endif 

#include <stdio.h>
#include <sys/types.h>
#include "codeset.h"
#include "mbextern.h"
#include <dlfcn.h>

static void *handle = (void *)NULL;	/* initialize it with -1 */

/*
 * Close current library library
 */
_ml_close_library()
{
#ifdef PIC
	if (handle == (void *)NULL) {
		_code_set_info.open_flag = NULL;
		return;
	}

	dlclose(handle);  
	_code_set_info.open_flag = NULL;
	handle = (void *)NULL;
#endif PIC
	return(0);
}

/*
 * Open the given library
 */
void *
_ml_open_library()
{
	char buf[BUFSIZ];

#ifdef PIC
	if (handle != (void *)NULL) /* This library is already opened */
		return(handle);

	/*
	 * Open the given library
	 */
	strcpy(buf, LIBRARY_PATH);
	strcat(buf, _code_set_info.code_name);
	strcat(buf, ".so");
#ifdef DEBUG
	printf ("ml_open_library: buf = '%s'\n", buf);
#endif
	handle = dlopen(buf, 1);
	if (handle != (void *)NULL)
		_code_set_info.open_flag = 1;
#ifdef DEBUG
	else
		printf ("_ml_open_library: dlopen failed\n");
#endif
#endif PIC
	return(handle);
}
