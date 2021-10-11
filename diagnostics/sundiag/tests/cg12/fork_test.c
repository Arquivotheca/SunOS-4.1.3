
#ifndef lint
static  char sccsid[] = "@(#)fork_test.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <strings.h>
#include <esd.h>

extern int non_interactive;
extern char *sprintf();

static
char	*test_name[] = {	"",
		       		"esd_int",
		       		"esd_sb",
			};

/**********************************************************************/
char *
fork_test(fildes, test_type, blck_cnt, brd_cnt, test_select, rpid)
/**********************************************************************/
int fildes[], test_type, blck_cnt, brd_cnt, test_select, *rpid;

{
    extern char *getenv();
    char errtxt[512];
    int pid=-1;

    *rpid = -1;

    if ((pid = fork()) == -1) {
	perror(errmsg_list[9]);
	return(errmsg_list[10]);
    }

    if (pid == 0) {
	char arg0[256], arg1[10], arg2[10], arg3[10], arg4[10], arg5[10];
	char *envptr;

	    envptr = getenv("ESD_PATH");
            if (envptr) {
                (void)strcpy(arg0, envptr);
                (void)strcat(arg0, "/");
                (void)strcat(arg0, test_name[test_type]);
            } else {
                (void)strcpy(arg0, test_name[test_type]);
            }
 
            /* pipe file descriptor */
            (void)sprintf(arg1, "%d", fildes[1]);

	    /* # of loops per functional block */
	    (void)sprintf(arg2, "%d", blck_cnt);

	    /* # of loops per board */
	    (void)sprintf(arg3, "%d", brd_cnt);

	    /* functional block selection */
	    (void)sprintf(arg4, "%d", test_select);

	    /* checksum generation flag , alsway 0 */
	    (void)sprintf(arg5, "%d", 0);
  
	    execlp(arg0, arg0, arg1, arg2, arg3, arg4, arg5, 0);

            /* if we come here, then execl has failed */
	    (void)sprintf(errtxt, errmsg_list[11], test_name[test_type]);
	    perror(errtxt);
	    (void)write(fildes[1], errtxt, strlen(errtxt));
	    (void)write(fildes[1], ".\n", 2);
	    exit(-1);
    }

    register_forked_process(pid);

    *rpid = pid;
    return((char *)0);
}

/**********************************************************************/
register_forked_process(pid)
/**********************************************************************/
int pid;

{
    if (non_interactive) {
	register_signal(pid);
    } else {
	register_notifier(pid);
    }
}
