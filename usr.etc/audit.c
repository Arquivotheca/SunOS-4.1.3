#ifndef lint
static	char *sccsid = "@(#)audit.c 1.1 92/07/30 SMI"; /* Copywr SunMicro */
#endif

#include <stdio.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/label.h> 
#include <sys/audit.h>
#include <pwdadj.h>
#include <pwd.h>


#define USERTEMP 0	/*  change kernel audit flags - only temp. */ 
#define SIGREAD  1	/*  notify audit kernel to read audit_control file */ 
#define SIGNEXT  2	/*  notify audit kernel to use next audit directory */ 
#define USERPERM 3	/*  change kernel audit flags based on passwd.adjunct 
			     and audit_control for inputted user names */
#define AUDITOFF 4	/*  disable auditing */ 
#define UARGNUM  4	/*  number of -u arguments */
#define MAXARG 10   	/*  max. number of users specified on command line */

static char AUDITDATAFILE[]    = "/etc/security/audit/audit_data";

/* GLOBALS */
int operation = -1;
int audit_error = 0;
char *sprintf();
char *audit_argv[] = { "audit", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
extern int errno;
extern char *sys_errlist[];

/* audit.c  -  audit control file operations */ 

/*
 * audit() - This program serves as a general administrator's interface to
 *           the audit trail.  
 *
 * input:   audit -d username(s)  - change kernel version of event flags
 *                                  for "username{s}" based on passwd
 *                                  adjunct and audit_control file.
 *          audit -s              - signal audit daemon to read 
 *				    audit_control file.
 *          audit -n              - signal audit daemon to use next 
 *				    audit_control audit directory.
 *          audit -u username auditflags - change kernel version of users 
 *                                         audit flags.
 *          audit -t              - signal audit daemon to disable auditing. 
 *
 * output:    
 *
 * returns:  0 - entry read ok
 *          -1 - error 
 *           1 - end of file
 */
	
main(argc, argv)
int argc;
char **argv;
{
	int i, j, max, retstat = 0, saveargc;
	char **initargv, *names[MAXARG];

	/*
	 * set up audit record
	 */
	saveargc = argc+1;
	max = MAXARG+2;
	initargv = argv;
	for (i=1, j=2; i<argc && j<max; i++, j++)
		audit_argv[j] = argv[i]; 
	argv = initargv;

	if (i != argc) {
		retstat = -2;
		saveargc = max;
	}
	/* 
	 * get input 
	 */
	if ((retstat != -2 && 
	  (retstat = ac_getinput(&argc, argv, names)) == 0)) {
	    	switch(operation) {
	    	case USERTEMP:
			retstat = ac_utemp(names);
	        	break;
	    	case SIGREAD:
	        	retstat = ac_read();
	        	break;
	    	case SIGNEXT:
	        	retstat = ac_next();
	        	break;
	    	case USERPERM:
	        	retstat = ac_uperm(argc, names);
	        	break;
	    	case AUDITOFF:
	        	retstat = ac_auditoff();
	        	break;
	    	default:
	        	retstat = -2;
	        	break;
	    	}
	}

	if (retstat == -2) {
		retstat = -1;
		audit_argv[1] = "Invalid input";
		fprintf(stderr, 
	"usage: audit [-s] | [-n] | [-t] | [-d username{s}] | [-u username auditflags]\n");
	}
	/* 
	 * output audit record 
	 */
	if (retstat == 0)
		audit_argv[1] = "Successful";
	audit_text(AU_ADMIN, audit_error, retstat, saveargc, audit_argv);

	exit (retstat);
	/* NOTREACHED */
}

/*
 * getacinput(): Reads command line arguments.
 * input:        argc - number of arguments in argv 
 *               argv - command line flags and data items. 
 * output:       names - data items. 
 */

ac_getinput(argc, argv, names)
int *argc;
char **argv;
char **names;
{
	int count = 0, retstat = 0; 

	/* 
	 * check for flags 
	 */
	if ((--(*argc) > 0) && ((*++argv)[0] == '-')) {
	    	switch((*argv)[1]) {
	    	case 'u':
	        	/* 
			 * get username and new kernel audit flags 
			 */
	        	if (*argc == UARGNUM - 1) {
	            		operation = USERTEMP;
	            		while (--(*argc) > 0)
	                		*names++ = *++argv;
	        	} else retstat = -2;
	        	break;

	    	case 's':
	        	/* 
			 * signal audit daemon to read audit_control file 
			 */ 
	        	operation = SIGREAD;
	        	break;

	    	case 'n':
	        	/* 
			 * signal audit daemon to read audit_control file 
			 */ 
	        	operation = SIGNEXT;
	        	break;

	    	case 't':
	        	/* 
			 * signal audit daemon to disable auditing 
			 */ 
	        	operation = AUDITOFF;
	        	break;

	    	case 'd':
	        	/* 
			 * get user name(s) 
			 */ 
	        	operation = USERPERM;
	        	while (--(*argc) > 0) {
	            		*names++ = *++argv;
	            		count++;    
	        	}
	        	if (count <= 0)
	            		retstat = -2;
	        	else
	            		*argc = count;
	        	break;
	    
	    	default:
	        	retstat = -2;
	        	break;
	    	}
	} else
		retstat = -2;

	if (operation == -1) retstat = -2;
	return (retstat);
}

/*
 * ac_utemp(): 
 * purpose: set kernel audit event flags to user specified values 
 * input:    *name[0]    - user name        
 *           *name[1]    - kernel event flags     
 *             
 * returns:  0 - successful 
 *          -1 - error in getting passwd entry or converting flags 
 */

ac_utemp(names)
char **names;
{
	int retstat = 0;
	audit_state_t astate;
	struct passwd *pw, *getpwnam();

	astate.as_success = 0;
	astate.as_failure = 0;
	
	if ((pw = getpwnam(*names)) == NULL) {
		fprintf(stderr, "audit: user, %s, invalid.\n", names[0]);
		audit_argv[1] = "Invalid user name";
	    	retstat = -1;
	}

	if (retstat == 0) {
	    	if ((retstat = getauditflagsbin(*++names, &astate)) == 0) {
	        	if ((retstat = 
			    setuseraudit(pw->pw_uid, &astate)) != 0) {
				fprintf(stderr, 
				 "audit: Error in setting state of processes "); 
				fprintf(stderr, "owned by %d.\n", pw->pw_uid);
				audit_argv[1] = sys_errlist[errno]; 
				audit_error = errno;
	            		retstat = -1;
			}
		} else {
	        	retstat = -1;
			fprintf(stderr, "audit: error in converting audit flags.\n");
			audit_argv[1] = 
			  "Error in converting audit flags."; 
		}
	}
	return (retstat);
}

/*
 * ac_read(): 
 * purpose: notify audit daemon to read audit control file 
 *             
 * returns:  0 - successful 
 *          -1 - error in opening audit_data file 
 */

ac_read()
{
	int pid, retstat = 0;
	FILE *adp;

	/* 
	 * open audit_data file 
	 */
	if ((adp = fopen(AUDITDATAFILE, "r")) == NULL) {
	    	fprintf(stderr, "audit: Can't open audit_data file.\n");
		audit_argv[1] = "Error in audit_data open.";
	    	retstat = -1;
	} else {
	    	/* 
		 * get pid of audit daemon and send signal
		 */
	    	fscanf(adp, "%d", &pid);
	    	if ((retstat = kill(pid, SIGHUP)) != 0) {
	    		fprintf(stderr, 
			  "audit: Can't send signal to audit daemon.\n");
			audit_argv[1] = sys_errlist[errno]; 
			audit_error = errno;
	            	retstat = -1;
		}
		fclose(adp);
	}
	return (retstat);
}

/*
 * ac_next(): 
 * purpose: notify audit daemon to use next audit directory 
 *             
 * returns:  0 - successful 
 *          -1 - error in opening audit_data file 
 */

ac_next()
{
	int pid, retstat = 0;
	FILE *adp;

	/* 
	 * get audit daemon pid 
	 */
	if ((adp = fopen(AUDITDATAFILE, "r")) == NULL) {
	    	fprintf(stderr, "audit: Can't open audit_data file.\n");
		audit_argv[1] = "Error in audit_data open.";
	    	retstat = -1;
	} else {
	    	/* 
		 * get pid of audit daemon 
		 */
	    	fscanf(adp, "%d", &pid);
	    	if ((retstat = kill(pid, SIGUSR1)) != 0) {
	    		fprintf(stderr, 
			  "audit: Can't send signal to audit daemon.\n");
			audit_argv[1] = sys_errlist[errno]; 
			audit_error = errno;
			retstat = -1;
		}
		fclose(adp);
	}
	return (retstat);
}

/*
 * ac_uperm(): 
 * purpose: notify audit daemon to read audit control file 
 * input  : count - number of users             
 *          names - array of names of users             
 *             
 * returns:  0 - successful 
 *          -1 - error 
 */

ac_uperm(count, names)
int count;
char **names;
{
	int i, retstat = 0;
	audit_state_t astate;
	struct passwd *pw;
	struct passwd_adjunct *apw;
	
	astate.as_success = 0;
	astate.as_failure = 0;

	/* 
	 * for each user 
	 */
	for (i=0; i < count; i++, ++names) {
		if ((pw = getpwnam(*names)) == NULL) {
	        	fprintf(stderr,"audit: user, %s, invalid.\n", names[0]);
			audit_argv[1] = "Invalid user name";
	        	retstat = -1;
	    	} else if ((apw = getpwanam(*names)) == NULL) {
	        	fprintf(stderr, "audit: user, %s, invalid.\n", names[0]);
			audit_argv[1] = "Invalid user name";
	        	retstat = -1;
	    	}

		if (retstat == 0) {
			if ((getfauditflags(&apw->pwa_au_always, 
				 &apw->pwa_au_never, &astate)) == 0) {
	            		if ((retstat = 
				  setuseraudit(pw->pw_uid, &astate)) != 0) {
		    			fprintf(stderr, 
				  	  "audit: Error in setting state of processes "); 
					fprintf(stderr, "owned by %d.\n", pw->pw_uid);
					audit_argv[1] = sys_errlist[errno]; 
					audit_error = errno;
	            			retstat = -1;
				}
	    		} else {
				fprintf(stderr, 
				  "audit: error in getting event state.\n");
				audit_argv[1] = 
				  "Error in getting event state.."; 
			}
		}
	}
	return (retstat);
}

/*
 * ac_auditoff(): 
 * purpose: disable auditing 
 *             
 * returns:  0 - successful 
 *          -1 - error in opening audit_data file 
 */

ac_auditoff()
{
	int pid, retstat = 0;
	FILE *adp;

	/* 
	 * get audit daemon pid 
	 */
	if ((adp = fopen(AUDITDATAFILE, "r")) == NULL) {
	    	fprintf(stderr, "audit: Can't open audit_data file.\n");
		audit_argv[1] = "Error in audit_data open.";
	    	retstat = -1;
	} else {
	    	/* 
		 * get pid of audit daemon 
		 */
	    	fscanf(adp, "%d", &pid);
	    	if ((retstat = kill(pid, SIGTERM)) != 0) {
	    		fprintf(stderr, 
			  "audit: Can't send signal to audit daemon.\n");
			audit_argv[1] = sys_errlist[errno]; 
			audit_error = errno;
			retstat = -1;
		}
		fclose(adp);
	}
	return (retstat);
}
