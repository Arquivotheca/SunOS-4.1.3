
#ifndef lint
static  char sccsid[] = "@(#)main.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/* Include files */

#include <stdio.h>
#include <sys/file.h>
#include <esd.h>


extern	struct test	sb_testlist[];

static
int			fildes[2];

static
int			logfile_fd,
			no_log = 0;
			non_interactive = 0;

/**********************************************************************/
main(argc, argv)
/**********************************************************************/
int argc;
char **argv;

{
    static char usage_msg[] = "usage: %s [-l logfile] [-n] [-t [-i [#loops]] | -s [#loops/block] [#loops/board] [test_select] ].\n";
    static char test_type_msg[] = "Use only one of the options -i, -s.\n";
    static char default_logfile_name[] = "/usr/adm/cg12diaglog";

    char *logfile_name = (char *)0;
    char *cp;
    int test_type = NO_TEST;
    int blck_cnt = 1;
    int brd_cnt = 1;
    int test_select=-1;
    int i;


    /* process argc and argv */
    for (i = 1 ; i < argc ; i++) {
	cp = argv[i];
	if (*cp == '-') {
	    cp++;
	    switch (*cp) {

		case 'l' :	/* log file name */
		    i++;
		    if (i >= argc) goto usage;
		    logfile_name = argv[i];
		    break;

		case 'n' :	/* no log file */
		    no_log = 1;
		    break;

		case 't' :	/* glass TTY terminal */
		    non_interactive = 1;
		    break;

		case 'i' :	/* Integration Test */
		    if (test_type == SINGLE_BLOCK_TEST) {
			(void)fprintf(stderr, test_type_msg);
			exit(-1);
		    }
		    test_type = INTEGRATION_TEST;
		    i++;
		    if (i >= argc) break;
		    if (*(argv[i]) == '-') {
			i--;
		        break;
		    }
		    brd_cnt = atoi(argv[i]);
		    break;

		case 's' :	/* Functinal Blocks Test */
		    if (test_type == INTEGRATION_TEST) {
			(void)fprintf(stderr, test_type_msg);
			exit(-1);
		    }
		    test_type = SINGLE_BLOCK_TEST;
		    test_select = get_test_selection(sb_testlist, LIST_DEFAULT);
		    i++;
		    if (i >= argc) break;
		    if (*(argv[i]) == '-') {
			i--;
		        break;
		    }
		    blck_cnt = atoi(argv[i]);
		    i++;
		    if (i >= argc) break;
		    if (*(argv[i]) == '-') {
			i--;
		        break;
		    }
		    brd_cnt = atoi(argv[i]);
		    i++;
		    if (i >= argc) break;
		    if (*(argv[i]) == '-') {
			i--;
		        break;
		    }
		    test_select = atoi(argv[i]);
		    break;

		default :
		    usage:
		    (void)fprintf(stderr, usage_msg, argv[0]);
		    exit(-1);
	    }
	} else {
	    goto usage;
	}
    }
    /* default is Integration Test */
    if (non_interactive && test_type == NO_TEST) {
	test_type = INTEGRATION_TEST;
    }

    /* open the logfile */
    if (logfile_name == (char *)0)
        logfile_name = default_logfile_name;
 
    if (!no_log) {
        if ((logfile_fd = open(logfile_name,
                            O_WRONLY|O_APPEND|O_CREAT, 0644)) == -1) {
            (void)fprintf(stderr, errmsg_list[28], logfile_name);
            exit(-1);
        }
    }    
 
    /* open the pipe for communication between main and tests */
    if(pipe(fildes) == -1) {
        perror(errmsg_list[29]);
        exit(-1);
    }   

    if (non_interactive) {
	(void)exec_tests(fildes, test_type, blck_cnt, brd_cnt, test_select);
	exit(0);
    }

    (void)exec_menu(fildes);
    exit(0);

}

/**********************************************************************/
void
pmessage(text)
/**********************************************************************/
char *text;

{
    if (non_interactive) {
	(void)fprintf(stdout, "%s", text);
    } else {
	show_msg(text);
    }
    
    if (no_log)
	return;

    if(write(logfile_fd, text, strlen(text)) != strlen(text)) {
        (void)fprintf(stderr, errmsg_list[30]);
        return;
    } 

}
