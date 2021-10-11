#ifndef lint
static  char sccsid[] = "@(#)integration_test.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <gttest.h>
#include <errmsg.h>

extern
struct test		isubtests[];

extern
char			*device_name;

static
char			textbuf[512];

static
char			errtxt[128];

/**********************************************************************/
char *
integration_test(isub)
/**********************************************************************/
int isub;

{
    struct test *tl;
    int it;
    int errors=0;
    char *errmsg;

    func_name = "integration_test";
    TRACE_IN

    /* lock the desktop completely during test */
    /*
    (void)lock_desktop(device_name);
    */

    /* Initialize the system */
    if (!open_hawk()) {
	TRACE_OUT
	return DLXERR_SYSTEM_INITIALIZATION;
    }

    for (tl = isubtests, it = 1; tl ; tl = tl->nexttest, it *=2) {
	if (it & isub) {
	    /* wacht out for user interrupt */
	    (void)check_key();

	    (void)sprintf(textbuf, "Integration subtest '%s': ", tl->testname);
	    (void)strcat(textbuf, "started.\n");
	    pmessage(textbuf);

	    errmsg = (tl->proc)();

	    (void)sprintf(textbuf, "Integration subtest '%s': ", tl->testname);
	    (void)strcpy(textbuf, textbuf);
	    if (errmsg) {
		errors++;
		(void)strcat(textbuf, "*** ");
		(void)strcat(textbuf, errmsg);
		error_report(textbuf);
	    } else {
		(void)strcat(textbuf, "OK.\n");
		pmessage(textbuf);
	    }

	}
    }

    /* close system */
    (void)close_hawk();

    /* release window system */
    /*
    unlock_desktop();
    */

    if (errors) {
	sprintf(errtxt, DLXERR_INTEGRATION_TEST_FAILED, errors);
	TRACE_OUT
	return errtxt;
    } else {
	TRACE_OUT
	return (char *)0;
    }
}
    

