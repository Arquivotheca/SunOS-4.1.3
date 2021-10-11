#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)getacinfo.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif
/* getacinfo.c  -  get audit control info */

#include <stdio.h>
#include <string.h>

#define DIROP 0
#define OTHEROP 1

#define LEN 360		/* maximum audit control entry length */

#define SUCCESS 0
#define EOF_WARN 1 
#define REW_WARN 2
#define EOF_ERR -1 
#define ERROR   -2 
#define FORMAT_ERR -3


static char *AUDIT_CTRL  = "/etc/security/audit/audit_control";
static char *MINLABEL    = "minfree:";
static char *DIRLABEL    = "dir:";
static char *FLGLABEL    = "flags:";
static int  LASTOP;
static int  DIRINIT;
static FILE *acf;    /* pointer into /etc/security/audit/audit_control */

/* getacinfo.c  -  get audit control info
 *
 *	getacdir() - get audit control directories, one at a time
 *	getacflg() - get audit control flags 
 *	getacmin() - get audit control directory min. fill value
 *	setac()    -  rewind the audit control file 
 *	endac()    -  close the audit control file 
 */


/* getacdir() - get audit control directories, one at a time
 *
 * input: len  - size of dir buffer
 *
 * output: dir - directory string
 *
 * returns:  0 - entry read ok
 *          -1 - end of file
 *          -2 - error - can't open audit control file for read 
 *          -3 - error - directory entry format error 
 *           1 - directory search started from beginning again 
 *
 * notes:    It is the responsibility of the calling function to 
 * 		check the status of the directory entry.
 */

int
getacdir(dir, len)
char *dir;
int len;
{
	int retstat = SUCCESS, gotone = 0, dirlen, dirst;
	char entry[LEN];
	void setac();
	
	/* 
	 * open file if it is not already opened
	 */
	if (acf == NULL && (acf = fopen(AUDIT_CTRL, "r")) == NULL)
		retstat = ERROR;
	else if (LASTOP != DIROP && DIRINIT == 1) {
		 retstat = REW_WARN;
		 setac();
	} else {
		DIRINIT = 1;
		LASTOP == DIROP;
	}
	if (retstat >= SUCCESS) {
  		do {
    			if (fgets(entry, LEN, acf) != NULL) {
				switch(*entry) {
        			case '#':
           				break;
        			case 'd':
				/*
            	 		 * return directory entry 
			 	 */
            			if (!strncmp(entry,DIRLABEL,strlen(DIRLABEL))) {
                			if ((strlen(entry)+1) > len)
                   				retstat = FORMAT_ERR;
                			else {
					/* 
				 	 * allow zero or one blank 
					 * between colon and directory 
				 	 */
                     			if (entry[strlen(DIRLABEL)] == ' ') {
                            			dirst = strlen(DIRLABEL)+1;
                      				dirlen = 
					  	  strlen(entry) - 
						    (strlen(DIRLABEL)+2); 
					} else {
                           			dirst = strlen(DIRLABEL);
                           			dirlen = 
						  strlen(entry) - 
						    (strlen(DIRLABEL)+1); 
                       			}
                			strcpy(dir, entry+dirst);
                			strcpy(dir+dirlen, "\0");
                			gotone = 1;    
                   			}
				} else
               				retstat = FORMAT_ERR;
                		break;
            			case 'm':
               				break;
            			case 'f':
                			break;
            			default:
                			break;
            			}
        		} else if ((feof(acf)) == 0)
        			retstat = ERROR;
        		else
            			retstat = EOF_ERR;

    		} while (gotone == 0 && retstat >= SUCCESS);
	}
    	return (retstat);
}

/*
 * getacmin() - get audit control directory min. fill value
 *
 * output: min_val - percentage of directory fill allowed 
 *
 * returns:  0 - entry read ok
 *           1 - end of file
 *          -2 - error; errno contains error number 
 *          -3 - error - directory entry format error 
 */

int
getacmin(min_val)
int *min_val;
{
	int retstat = SUCCESS, gotone = 0;
	char entry[LEN];
	void endac();

	/* 
	 * open file if it is not already opened 
	 */
	if (acf == NULL && (acf = fopen(AUDIT_CTRL, "r")) == NULL)
	    retstat = ERROR;
	else
	    rewind(acf);

	if (retstat == SUCCESS) {
      		do {
        		if (fgets(entry, LEN, acf) != NULL) {
            			switch(*entry) {
	    			case '#':
					break;
				case 'd':
					break;
				case 'm':
					if (!strncmp(entry, MINLABEL, strlen(MINLABEL))) {
		    			sscanf(entry+strlen(MINLABEL), "%d", min_val);
		    			gotone = 1;
					} else
		    			retstat = FORMAT_ERR;
					break;
				case 'f':
					break;
				default:
					break;
				}
			} else if ((feof(acf)) == 0)
				retstat = ERROR;
			else
				retstat = EOF_WARN;

		} while (gotone == 0 && retstat == SUCCESS);
	}

	if (LASTOP == DIROP)
		LASTOP = OTHEROP;
	else
		endac();

	return (retstat);
}

/* getacflg() - get audit control flags 
 *
 * output: auditstring - character representation of system audit flags 
 *
 * returns:  0 - entry read ok
 *           1 - end of file
 *          -2 - error - errno contains error number 
 *          -3 - error - directory entry format error 
 */

getacflg(auditstring, len)
char *auditstring;
int len;		
{
	int retstat = SUCCESS, gotone = 0, minst, minlen;
	char entry[LEN];
	void endac();

	/* 
	 * open file if it is not already opened 
	 */
	if (acf == NULL && (acf = fopen(AUDIT_CTRL, "r")) == NULL)
		retstat = ERROR;
	else
		rewind(acf);

	if (retstat == SUCCESS) {
		do {
			if (fgets(entry, LEN, acf) != NULL) {
				switch(*entry) {
				case '#':
					break;
				case 'd':
					break;
				case 'm':
					break;
				case 'f':
					if ((strncmp(entry, FLGLABEL, strlen(FLGLABEL))) == 0) {
						if (entry[strlen(FLGLABEL)] == ' ') {
							minst = strlen(FLGLABEL)+1;
							minlen = strlen(entry)-(strlen(FLGLABEL)+2); 
						} else {
							minst = strlen(FLGLABEL);
							minlen = strlen(entry)-(strlen(FLGLABEL)+1); 
						}
						if (minlen > len)
							retstat = FORMAT_ERR;
						else {
							strcpy(auditstring, entry+minst);
							strcpy(auditstring+minlen, "\0");
							gotone = 1;	
						}
					} else 
						retstat = FORMAT_ERR;
					break;
				default:
					break;
				}
			} else if ((feof(acf)) == 0)
				retstat = ERROR;
			else
				retstat = EOF_WARN;

		} while (gotone == 0 && retstat == SUCCESS);
	}
	if (LASTOP == DIROP)
		LASTOP = OTHEROP;
	else
		endac();

	return (retstat);
}

/* rewind the audit control file */
void
setac()
{
	if (acf == NULL)
		acf = fopen(AUDIT_CTRL, "r");
	else
		rewind(acf);
	LASTOP = DIROP;
	DIRINIT = 0;
}


/* close the audit control file */
void
endac()
{
	if (acf != NULL) {
		fclose(acf);
		acf = NULL;
	}
	LASTOP = DIROP;
	DIRINIT = 0;
}
