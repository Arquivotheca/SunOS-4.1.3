#ifndef lint
static	char sccsid[] = "@(#)newtest_msg.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * All error messages that will be seen and concerned by the user
 * should be put in this "message" file.
 * The "message" file should always be called testname_msg.c.
 * Also, a separate testname_msg.h is needed so that the "message"
 * pointer can be used by more than one source files by including it.
 */

char	*failed_msg = "failed.";
char	*err_limit_msg= "failed due to exceeding error limit.";
