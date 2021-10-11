static char sccsid[] = "@(#)fparel_util.c 1.1 7/30/92 Copyright Sun Microsystems"; 

/*
 * ****************************************************************************
 * Source File     : fparel_util.c
 * Original Engr   : Nancy Chow
 * Date            : 10/12/88
 * Function        : This file contains the general utility routines used by
 *		   : the fparel test.
 * Revision #1 Engr: 
 * Date            : 
 * Change(s)       :
 * ****************************************************************************
 */

/* ***************
 * Include Files
 * ***************/

#include <sys/types.h>
#include "fpa3x.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sundev/fpareg.h>

/* ***************
 * fpa_open_fail
 * ***************/

/* This routine handles the error logging of an unsuccessful attempt to open
   the fpa device file for a context */

fpa_open_fail(fail_code)
int fail_code;			/* system error code */
{

char mesg[120];			/* message str */
FILE *input_file;		/* diag log file pointer */

    mesg[0] = '\0';		/* init str */
    time_of_day(mesg);		/* get time of day */

				/* attempt to open diag log file */
    if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL) {
	printf("FPA Reliability Test Error.\n");
	printf("    Could not access /usr/adm/diaglog file, can be accessed only under root\n");
	return;
    }
    
    switch (fail_code) {
	case ENETDOWN :		/* if FPA not communicating */  
	    strcat(mesg," : FPA was disabled by kernal while doing "); 
	    strcat(mesg,"reliability test, may be due to h/w problem.");
	    break;
	case EBUSY :		/* cannot open /dev/fpa */
	    strcat(mesg," : No FPA context is Available.");
	    break;
	case EEXIST :		
	    strcat(mesg," : Duplicate open on FPA context.");
	    break;
    }

    strcat(mesg," FPA reliability test is terminated.\n");
    fputs(mesg,input_file);	/* log diag message */
    fclose(input_file);		/* close diag log file */
}

/* ***********
 * fail_close    
 * ***********/
    
/* This routine handles the reporting of an unsuccessful test during the 
   reliability test */

fail_close(test_nam, contexts)
char   *test_nam;		/* name of test which failed */
u_long contexts;		/* contexts during failure */
{

char mesg[180];			/* message str */

				/* log test failure */
    if (make_diaglog_msg(contexts, FAIL, test_nam, mesg))
	return;	
    make_broadcast_msg();	/* broadcast message to users logged on */
    make_unix_msg(mesg);	/* disable FPA and generate system message */
}

/* *************
 * time_of_day
 * *************/

/* This routine is used to retrieve the time of day into an array */

time_of_day(mesg)
char mesg[];			/* array for time of day */
{
char *tempptr;			/* ptr to ascii time of day */
long temptime;			/* time in seconds */

    time(&temptime);		/* get time in seconds */
    tempptr = ctime(&temptime);	/* convert time to ascii */

    strcat(mesg, "\n");
    strncat(mesg, tempptr, 24);	/* append time of day to  message */
}

/* ***************
 * make_unix_msg
 * ***************/

/* This routine is used to disable the FPA and generate a system message */

make_unix_msg(mesg)
char mesg[];			/* system message */
{

    if (ioctl(dev_no,FPA_FAIL,(char *)mesg) < 0) {	/* disable FPA */
	if (errno == EPERM)
	    printf("Not a Super User :Fparel unable to disable FPA.\n"); 
	else
	    perror("fparel");
    }
}

/* ******************
 * make_diaglog_msg
 * ******************/

/* This routine is used to log a diagnostic message (Pass or Fail) */

make_diaglog_msg(conts, suc_err, test_nam, mesg)
u_long conts;			/* contexts used */
int    suc_err;			/* PASS or FAIL */
char   *test_nam;		/* test name for report */
char   *mesg;			/* message to be logged */
{

FILE   *input_file;		/* diag log file pointer */
int    i;			
char   cxt_str[180];		/* str for context numbers */
u_long cur_cxt;			/* current context */
u_long mask;			/* context mask */

    mesg[0] = '\0';		/* init str */
    time_of_day(mesg);
				/* open diag log file */
    if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL) {	
	printf("FPA Reliability Test Error.\n");
	printf("    Could not access /usr/adm/diaglog file, can be accessed only under root permission.\n");
	return(FAIL);
    }

    if (suc_err == FAIL) {
	strcat(mesg," : Sun FPA Reliability Test Failed, Sun FPA DISABLED -  Service Required.\n");
	sprintf(cxt_str, "%s", " Sun FPA : ");
	strcat(cxt_str, test_nam);
	strcat(cxt_str, " Test Failed, Context Number = ");

	mask = FIRST_BIT;		/* initial mask */
	for (i=0; i < MAX_CXTS; i++) {
	    if (conts & mask)		/* if context tested */
		cur_cxt = i;
	    mask = mask << 1;	/* adjust mask for next context */
	}
	apnd_cxt(cxt_str, cur_cxt);	/* append context number */
    }
    else {
	strcat(mesg," : Sun FPA Reliability Test Passed.\n");
	sprintf(cxt_str, "%s", " Sun FPA : Contexts tested are : ");

	mask = FIRST_BIT;		/* initial mask */
	for (i=0; i < MAX_CXTS; i++) {	/* append each context to str */
	    if (conts & mask) {		/* if context tested */
		if (mask != FIRST_BIT)		/* 1st context appended? */
		    strcat(cxt_str, ", \0");	/* append delimiter */
		apnd_cxt(cxt_str, i);		/* append context number */ 
	    }
	    mask = mask << 1;   	/* adjust mask for next context */
	}
    }

    fputs(mesg, input_file);
    fputs(cxt_str, input_file);
    fclose(input_file);
    return(0);
}

/* **********
 * apnd_cxt
 * **********/

/* This routine is used to append the ascii characters of a numeric context
   to the end of a message string */

apnd_cxt(cxt_str, cur_cxt)
char   *cxt_str;		/* message string */
u_long cur_cxt;			/* current context number */
{

char num_str[4];		/* holds ascii chars of context */

    if (cur_cxt <= 9) {	
	num_str[0] = cur_cxt + '0';
	num_str[1] = '\0';
    }
    else {
	if (cur_cxt <= 19) {
	    num_str[0] = '1';
	    num_str[1] = (cur_cxt - 10) + '0';
	}
	else if (cur_cxt <= 29) {
	    num_str[0] = '2';
	    num_str[1] = (cur_cxt - 20) + '0';
	}
	else {
	    num_str[0] = '3';
	    num_str[1] = (cur_cxt - 30) + '0';
	}
	num_str[2] = '\0';
    }

    strcat(cxt_str, num_str);		/* append context to message */
}

/* ********************
 * make_broadcast_msg
 * ********************/

/* This routine is used to generate a message to all users logged on */

make_broadcast_msg()
{

FILE    *input_file;			/* temporary file pointer */
char    mesg[120];			/* broadcast message */
char    file_name[20];			/* temporary filename */
char    cmd_str[40];			/* command string */

    mesg[0] = '\0';			/* init str */
    time_of_day(mesg);			/* get time of day */

    strcat(mesg, " : Sun FPA Reliability Test Failed, Sun FPA DISABLED - Service Required.\n");
    tmpnam(file_name);			/* get temporary file name */
    input_file = fopen(file_name,"w");	/* open file for write access */
    fputs(mesg, input_file);		/* log message to temp file */
    fclose(input_file);			/* close temp file */
    
					/* generate wall system command */
    cmd_str[0] = '\0';			/* init str */
    strcat(cmd_str, "wall ");
    strcat(cmd_str, file_name);
    system(cmd_str);

					/* generate remove system command */
    cmd_str[0] = '\0';                  /* init str */ 
    strcat(cmd_str, "rm -f ");
    strcat(cmd_str, file_name); 
    system(cmd_str);
}


