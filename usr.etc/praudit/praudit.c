#ifndef lint
static  char sccsid[] = "@(#)praudit.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif
#include <stdio.h> 
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/vfs.h> 
#include <sys/vnode.h> 
#include <sys/pathname.h> 
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <netinet/in.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>
#include <rpc/svc.h>
#include <rpc/xdr.h>
#include <nfs/nfs.h>
#include <ufs/quota.h>
#include <sys/label.h>
#include <sys/audit.h>
#include "praudit.h"
#include <auevents.h>

/* praudit.c  -  display contents of audit_control file 
 *
 * praudit() - main control 
 * input:    - command line input:   praudit -r -s -l -ddelim. filename(s)
 * output: 
 */ 

main(argc, argv)
int argc;
char **argv;
{
	int i = 0, retstat;
	char *names[MAXFILENAMES], fname[MAXFILELEN];

	/* 
	 * get audit file names 
	 */
	if ((retstat = pa_geti(&argc, argv, names)) == 0) { 
		do {
			retstat = 0;
			/* 
			 * process each audit file 
			 */
			if (file_mode) {
				strcpy(fname, names[i]);
				if (freopen(fname, "r", stdin) == NULL) {
					fprintf(stderr, 
					"praudit: Can't assign %s to stdin.\n",
							fname);
					retstat = -1;
				}
			} else
				strcpy(fname, "stdin");
			/* 
			 * process audit file header 
			 */
			if (retstat == 0) {
				if ((retstat = pa_header(fname)) == 0) {
					/* 
					 * process records 
					 */
					while (retstat == 0)
						retstat = pa_record();
				}
			}
		} while ((++i < argc) && retstat >= 0);
	}
	if (retstat == -2) {
		printf("\nUsage: praudit [-r|-s] [-l] [-ddel] [audit file names]\n\n");
		retstat = -1;
	}
	if (retstat == 1) retstat = 0;
	exit(retstat);
	/* NOTREACHED */
}

/*
 * pa_geti() - get command line flags and file names
 * input:    - praudit [-r|-s] [-l] [-ddel] [audit file names] 
 * output:   - {audit file names} 
 * globals set:	format:		RAWM or SHORT or DEFAULT
 *		ONELINE:    1 if output is unformatted
 *	   	SEPERATOR:  default, ",", set here if 
 *                          user specified 
 */ 
pa_geti(argc, argv, names)
int *argc;
char **argv;
char **names;
{
	int i, count = 0, retstat = 0, gotone = 0;

	/* 
	 * check for flags 
	 */
	for (i=0, --(*argc); i<3 && *argc > 0 && (*++argv)[0] == '-' && 
	  retstat == 0; i++, --(*argc)) {
		switch((*argv)[1]) {
		case 'r':
			if (format == SHORTM)
				retstat = -2;
			else {
				format=RAWM;
				++gotone;
			}
			break;
		case 's':
			if (format == RAWM)
				retstat = -2;
			else {
				format=SHORTM;
				++gotone;
			}
			break;
		case 'l':
			ONELINE = 1;
			++gotone;
			break;
		case 'd':
			++gotone;
			if (strlen(*argv) != 2) {
				if (strlen(*argv+2) < sizeof(SEPERATOR))
					strcpy(SEPERATOR, *argv+2);
				else {
					fprintf(stderr,
					  "praudit: Field delimiter too long; using default.\n");
				}
			}
			break;
		default:
			if (gotone == 0)
				retstat = -2;
			break;
		}
	} 

	if (gotone != 3) --argv;

	if (retstat == 0 && *argc > 0) { 
		/* 
		 * get file names from command line 
		 */
		do {	
			if ((access(*++argv, R_OK)) == -1) {
				fprintf(stderr,"praudit: File, %s, is not accessible.\n",*argv);
				retstat = -1;
			} else if (*argc > MAXFILENAMES) {
				fprintf(stderr, "praudit: Too many file names.\n");
				retstat = -1;
			}
			if (retstat == 0) {
				count++;
				*names++ = *argv;
			}
		} while (--(*argc) > 0);

		if (retstat == 0) {
			file_mode = FILEMODE;
			*argc = count;
		}
	} else if (retstat == 0) file_mode = PIPEMODE;

	return (retstat);
}
			
/*
 * pa_header()	: process audit file header. Validate that this file is an
 *		  audit file.
 * input   	: file_name - name of current audit file 
 * output	: 
 * return codes : -1 - error
 *		:  0 - successful header process
 */

pa_header(file_name)
char *file_name;
{
	int retstat = 0;
	char *time_created, *fname;
	char cwd[MAXPATHLEN], *getwd();
	audit_header_t header;

	if ((fread(&header,sizeof(char),sizeof(audit_header_t),stdin)) 
				!= sizeof(audit_header_t)) {
		fprintf(stderr, "praudit: Error in header read.\n");
		retstat = -1;
	}
	if (retstat == 0) {
		if (header.ah_magic != AUDITMAGIC) { 
			fprintf(stderr, 
			  "praudit: %s is not an audit file.\n", file_name);
			retstat = -1;
		} else {
			uvaltype = STRING;
			if (file_name != NULL && 
			  file_name[0] != '/') {
				if (getwd(cwd) != NULL) {
					strcat(cwd, "/");
					strcat(cwd, file_name); 
					uval.string_val = cwd; 
				} else
					uval.string_val = NULL;
			} else
				uval.string_val = file_name; 
			if ((retstat = pa_print(NOINDEX, 1, 0)) == 0) {
				time_created = (char *)ctime(&(header.ah_time));
				strcpy(time_created+24, "\0"); 
				uvaltype = STRING;
				uval.string_val = time_created; 
				if(header.ah_namelen != 0) {
					if ((fname = 
					  (char *)malloc((unsigned int)header.ah_namelen)) == NULL) {
						fprintf(stderr, 
							"praudit: Error in allocating space for header field.\n");
						retstat = -1;
					} else if ((fread(fname, sizeof(char), 
							header.ah_namelen, stdin))!=
								header.ah_namelen)  {
						fprintf(stderr, "praudit: Read error in previous audit file name.\n");
						retstat = -1;
					} else {
						retstat = pa_print(NOINDEX, 1, 0);
						if (retstat == 0) {
							uvaltype = STRING;
							uval.string_val=fname; 
							retstat = 
							  retstat = pa_print(NOINDEX, 1, 1);
						}
						free(fname);
					}
					free(fname);
				} else 
					retstat = pa_print(NOINDEX, 1, 1);
			}
		}
	}
	return (retstat);
}

/*
 * pa_record()	: Read record header and display contents. 
 * input   	: 
 * output	: 
 * return codes : -1 - error
 * 		:  1 - warning, passwd entry not found
 *		:  0 - successful 
 */ 
int 
pa_record()
{
	int index, recsize, size, retstat = 0;
	short rec[2];
	char *time_created, *t_create;
	char *labeltostring();
	char *getevents();
	audit_record_t record;
	struct group *group_ptr, *getgrgid();
	struct passwd *pw; 
	
	/* 
	 * read next record size and type 
	 */
	recsize = 2 * sizeof(short); 
	if ((fread(rec, sizeof(char), recsize, stdin)) != recsize) { 
		fprintf(stderr, "praudit: Record expected but not found..\n");
		retstat = -1;
	} 
	/* 
	 * check for valid record type 
	 */
	if (PR_MAXRECTYPE != AUR_MAXRECTYPE) {
		fprintf(stderr, "praudit: Inconsistent record types.\n");
		retstat = -1;
	}

	if (rec[1] < AUR_MINRECTYPE || rec[1] > AUR_MAXRECTYPE) {
		if (rec[1] != AUR_TRAILER) { 
			fprintf(stderr, "praudit: Invalid record type.\n");
			retstat = -1;
		}
	}
	/* 
	 * check for valid record size 
	 */
	if (retstat >= 0) {
		if (rec[0] < 0) {
			fprintf(stderr, "praudit: Invalid record size.\n");
			retstat = -1;
		}
	}
	/* 
	 * check for trailor record 
	 */
	if ((retstat == 0) && (rec[1] == AUR_TRAILER)) {
		retstat = pa_trailer(rec[0]);
		if (retstat == 0) retstat = 1;
	} else if (retstat >= 0) {
		/* 
		 * if regular record, display record type 
		 */
	 	index = RECORD_TYPE;
		if (format != RAWM) {
			uvaltype = STRING;
			uval.string_val = rec_types[rec[1]];
			retstat = pa_print(index, 0, 0);
		} else {
			uvaltype = SHORT;
			uval.short_val = rec[1]; 
			retstat = pa_print(index, 0, 0);
		}
		/* 
		 * read the rest of the record header 
		 */
		recsize = sizeof(record) - (2 * sizeof(short)); 
		if ((fread(&(record.au_event), sizeof(char), 
		  recsize, stdin)) != recsize) { 
			fprintf(stderr, "praudit: corrupt audit record..\n");
			retstat = -1;
		}
		/* 
		 * display the rest of the record header 
		 */
		if (retstat >= 0) {
			/* 
			 * event type 
			 */
		 	index = RECORD_EVENT;
			if (format != RAWM) {
				uvaltype = STRING;
				if (format == SHORTM)
					uval.string_val = 
					  getevents(0, record.au_event);
				else
					uval.string_val = 
					  getevents(1, record.au_event);
				retstat = pa_print(index, 0, 0);
			} else {
				uvaltype = INT;
				uval.int_val = record.au_event;
				retstat = pa_print(index, 0, 0);
			}
		}
		if (retstat >= 0) {
			/* 
			 * record creation time 
			 */
			 
		 	index = RECORD_TIME;
			if (format != RAWM) {
				uvaltype = STRING;
				t_create = (char *)ctime(&(record.au_time));
				if ((time_created = 
				  (char *)malloc((unsigned int)strlen(t_create)+2)) == NULL) {
					fprintf(stderr, "praudit: Error in allocating space for record field.\n");
					retstat = -1;
				} else {
					strncpy(time_created, t_create, 24);
					strncpy(time_created+24, "", 1);
					uval.string_val = time_created;
					retstat = pa_print(index, 0, 0);
				}
				free(time_created);
			} else {
				uvaltype = INT;
				uval.short_val = record.au_time;
				retstat = pa_print(index, 0, 0);
			}
		}
		if (retstat >= 0) {
			/* 
			 * uid 
			 */
		 	index = RECORD_UID;
			if (format != RAWM) {
				if ((pw=getpwuid(record.au_uid))==NULL) {
					retstat = 1;
				} else {
					uvaltype = STRING;
					uval.string_val = pw->pw_name;
					retstat = pa_print(index, 0, 0);
				}
			}
			if (format == RAWM || retstat == 1) {
				uvaltype = SHORT;
				uval.short_val = record.au_uid;
				retstat = pa_print(index, 0, 0);
			}
		}
		if (retstat >= 0) {
			/* 
			 * audit uid 
			 */
		 	index = RECORD_AUID;
			if (format != RAWM) {
				if ((pw=getpwuid(record.au_auid))==NULL) {
					retstat = 1;
				} else {
					uvaltype = STRING;
					uval.string_val = pw->pw_name;
					retstat = pa_print(index, 0, 0);
				}
			}
			if (format == RAWM || retstat == 1) {
				uvaltype = SHORT;
				uval.short_val = record.au_auid;
				retstat = pa_print(index, 0, 0);
			}
		}
		if (retstat >= 0) {
		 	index = RECORD_EUID;
			if (format != RAWM) {
				if ((pw=getpwuid(record.au_euid))==NULL) {
					retstat = 1;
				} else {
					uvaltype = STRING;
					uval.string_val = pw->pw_name; 
					retstat = pa_print(index, 0, 0);
				}
			}
			if (format == RAWM || retstat == 1) {
				uvaltype = SHORT;
				uval.short_val = record.au_euid;
				retstat = pa_print(index, 0, 0);
			}
		}

		if (retstat >= 0) {
		 	index = RECORD_GID;
			if (format != RAWM) {
				if ((group_ptr =
					  getgrgid(record.au_gid)) == NULL)
					retstat = 1;
				else {
					setgrent();
					uvaltype = STRING;
					uval.string_val = group_ptr->gr_name; 
					retstat = pa_print(index, 0, 0);
				}
			}
			if (format == RAWM || retstat == 1) {
				uvaltype = SHORT;
				uval.short_val = record.au_gid;
				retstat = pa_print(index, 0, 0);
			}
		}
		if (retstat >= 0) {
		 	index = RECORD_PID;
			uvaltype = SHORT;
			uval.short_val = record.au_pid;
			retstat = pa_print(index, 0, 0);
		}
		if (retstat >= 0) {
		 	index = RECORD_ERROR;
			uvaltype = INT;
			uval.int_val = record.au_errno;
			retstat = pa_print(index, 0, 0);
		}
		if (retstat >= 0) {
		 	index = RECORD_RETURN;
			uvaltype = INT;
			uval.int_val = record.au_return;
			retstat = pa_print(index, 0, 0);
		}
		if (retstat >= 0) {
		 	index = RECORD_LABEL;
			if (format == DEFAULTM) {
				uvaltype = STRING;
				/* 
				uval.string_val = labeltostring(0, &(record.au_label), 0); 
				*/
				uval.string_val = NULL; 
				retstat = pa_print(index, 0, 0);
				/*
				free(uval.string_val);
				*/
			} else {
				uvaltype = INT;
				/*
				uval.int_val = (int) &record.au_label;
				*/
				uval.int_val = 0; 
				retstat = pa_print(index, 0, 0);
			}
		}

		/* 
		 * process variable length fields 
		 */
		if (retstat >= 0) {
			size = rec[0] - sizeof(record);
			retstat =
				pa_data(record.au_param_count, rec[1], size); 
		}
	}
	return (retstat);
}

/*
 * pa_data()	: process variable length data fields in record 
 * input   	: count - number of fields to process 
 * 		: type  - record type 
 * 		: size  - size of record - header  
 * output	: 
 * return codes : -1 - error
 *		   0 - successful 
 */

pa_data(count, type, size)
short count;
short type;
int size;
{
	int i, j, grp, group_num, retstat = 0, index, command, largebuf = 0;
	short field_counts[MAXFIELDS];
	char norm_buff[MAXAUDITDATA], *big_buff, *buffer;
	struct group *group_ptr, *getgrgid();

	/* 
	 * read data segment byte counts 
	 */
	if (count > MAXFIELDS) {
		fprintf(stderr, "praudit: Incorrect audit record format.\n");
		retstat = -1;
	}
		 
	for(i=0;i<count && retstat == 0;i++) {
		if ((fread(&(field_counts[i]),sizeof(char),
			sizeof(short),stdin)) != sizeof(short)) {
				fprintf(stderr, 
				  "praudit: Can't read data counts.\n");
				retstat = -1;
		}
	}
	/*
	 * if data contains full path name > 1023, allocate a new
	 * buffer to handle this exceptionally long audit record 
	 */
	if (retstat == 0) { 
		if (size > MAXAUDITDATA) {
			big_buff = 
			  (char *)malloc((unsigned int)size * sizeof(char));
			buffer = big_buff;
		} else
			buffer = norm_buff;

		size -= (count * sizeof(short));
		index = 0;

		/* 
		 * read the rest of the record 
		 */
		if ((fread(buffer, sizeof(char), size, stdin)) != size) {
			fprintf(stderr, 
				"praudit: Can't read variable length data.\n");
			retstat = -1;
		} else {
		    /*
		     * get groups list
		     */
		    group_num = field_counts[0] / sizeof(int);		
		    for (i=0; i<group_num && retstat == 0; i++, index+=sizeof(int)) {
			bcopy(&(buffer[index]), &grp, sizeof(int));
			if (format != RAWM) {
				if ((group_ptr =
					  getgrgid(grp)) == NULL)
					retstat = 1;
				else {
					setgrent();
					uvaltype = STRING;
					uval.string_val = group_ptr->gr_name; 
					retstat = pa_print(NOINDEX, 0, 0);
				}
			}
			if (format == RAWM || retstat == 1) {
				uvaltype = INT;
				uval.int_val = grp; 
				retstat = pa_print(NOINDEX, 0, 0);
			}
		    }	

		    /*  
		     * read the unique section of the record
		     */
		    if (retstat == 0) {
		    	i = 1;
			switch((int)type) {

			default:
				fprintf(stderr, 
				  "praudit: Unknown audit record type.\n");
				retstat = -1;
				break;

			case AUR_OPEN:
				/* 
				 * 3 str str str root cwd fname 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_CREAT:
				/* 
				 * 3 str str str root cwd fname 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_MKNOD:
				/* 
				 * 4 str str str int root cwd fname mode 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_MKDIR :
				/* 
				 * 3, str, str, str, root, cwd, fname 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_LINK :
				/* 
				 * 4, str, str, str, str, root, cwd, fname, fname 
				 */
				for (i=1; i<5 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_RENAME:
				/* 
				 * 4 str str str str root cwd fname fname 
				 */
				for (i=1; i<5 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_SYMLINK:
				/* 
				 * 4 str str str str root cwd fname fname 
				 */
				for (i=1; i<5 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_UNLINK:
				/* 
				 * 3 str str str root cwd fname 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_RMDIR:
				/* 
				 * 3 str str str root cwd fname 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_ACCESS:
				/* 
				 * 3 str str str root cwd fname 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_STAT:
				/* 
				 * 3 str str str root cwd fname 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_CHMOD:
				/* 
				 * 4 str str str int root cwd fname mode 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = pa_types(OCT, &index, field_counts[i], buffer);
				break;

			case AUR_FCHMOD:
				/* 
				 * 2 4 4 fd mode 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = pa_types(OCT, &index, field_counts[i], buffer);
				break;

			case AUR_CHOWN:
				/* 
				 * 5 str str str int int root cwd fname uid gid 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				for (i=3; i<5 && retstat == 0; i++)
					retstat = pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_FCHOWN:
				/* 
				 * 3 int int int fd uid gid 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_UTIMES:
				/* 
				 * 4 str str [2*sizeof(struct timeval)] 
				 * root cwd fname tv 
				 * timeval = 2 longs
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				for (i=0; i<4 && retstat == 0; i++)
					retstat = pa_types(LONG, &index, field_counts[2], buffer);
				break;

			case AUR_TRUNCATE:
				/* 
				 * 4 str str str int root cwd fname length 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_FTRUNCATE:
				/* 
				 * 2 int int fd length 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_PTRACE:
				/* 
				 * 5, int int int int int
				 */
				for (i=1; i<6 && retstat == 0; i++)
					retstat = pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_KILL:
				/*  
				 * 2, int int 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_KILLPG:
				/* 
				 * 2, int int  
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_SETTIMEOFDAY:
				/*  
				 * 2, sizeof(struct timeval), 
				 * sizeof(struct timezone) 
				 * timeval = 2 longs; timezone = 2 ints 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = pa_types(LONG, &index, sizeof(long), buffer);
				for (i=0; i<2 && retstat == 0; i++)
					retstat = pa_types(INT, &index, sizeof(int), buffer);
				break;

			case AUR_ADJTIME:
				/*  
				 * 2, sizeof(struct timeval), 
				 * sizeof(struct timezone) 
				 * timeval = 2 longs; timezone = 2 ints 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = pa_types(LONG, &index, sizeof(long), buffer);
				for (i=0; i<2 && retstat == 0; i++)
					retstat = pa_types(INT, &index, sizeof(int), buffer);
				break;

			case AUR_SOCKET:
				/* 
				 * 3, int int int 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_EXECV:
				/* 
				 * 3, str, str, str, root, cwd, path name 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_EXECVE:
				/* 
				 * 3, str, str, str, root, cwd, path name 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_SETHOSTNAME:
				/* 
				 * 2, str str 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_SETDOMAINNAME:
				/* 
				 * 2, str str
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_REBOOT:
				/* 
				 * 1, int 
				 */
				retstat = pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_REBOOTFAIL:
				/* 
				 * 1, int 
				 */
				retstat = pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_SYSACCT:
				/* 
				 * 1, str 
				 */
				retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_MOUNT_UFS:
				/*  
				 * 6, str, str, int, str, int, str, 
				 * root, cwd, mount type, mount dir., 
				 * flags, block special device
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = pa_types(INT, &index, field_counts[i++], buffer);
				if (retstat == 0)
					retstat = pa_types(STRING, &index, field_counts[i++], buffer);
				if (retstat == 0)
					retstat = pa_types(INT, &index, field_counts[i++], buffer);
				if (retstat == 0)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				break;


			case AUR_MOUNT_NFS:
				/*
				 *  13, str, str, int, str, int,
				 *  sizeof(struct sockaddr_in), 
				 *  sizeof(fhandle_t), int, int, int, 
				 *  int, int, str 
				 *  sockaddr_in = short, u_short, 
				 *    in_addr(total of 4 bytes), char[8]
				 *  defined in netinet/in.h
				 *  fhandle_t = NFS_FHSIZE bytes
				 *  defined in nfs.h
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = pa_types(INT, &index, field_counts[i++], buffer);
				if (retstat == 0)
					retstat = pa_types(STRING, &index, field_counts[i++], buffer);
				if (retstat == 0)
					retstat = pa_types(INT, &index, field_counts[i], buffer);
				/* 
				 * sockaddr_in structure 
				 */
				if (retstat == 0)
					retstat = pa_types(SHORT, &index, sizeof(short), buffer);
				if (retstat == 0)
					retstat = pa_types(USHORT, &index, sizeof(u_short), buffer);
				if (retstat == 0)
					retstat = pa_types(HEX, &index, sizeof(long), buffer);
				for (i=0; i<8 && retstat == 0; i++)
					retstat = pa_types(CHAR, &index, sizeof(char), buffer);
				/*  
				 * fhandle_t structure 
				 */
				for (i=0; i<4 && retstat == 0; i++)
					retstat = pa_types(HEX, &index, sizeof(long), buffer);
				/*  end of fhandle_t */

				for (i=0; i<5 && retstat == 0; i++)
					retstat = pa_types(INT, &index, sizeof(int), buffer);
				if (retstat == 0)
					retstat = pa_types(STRING, &index, field_counts[11], buffer);
				break;

			case AUR_MOUNT:
				/* 
				 * 5, str, str, int, str, int, int,
			         * root, cwd, mount type, local mount dir.
				 * flags,  
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = pa_types(INT, &index, field_counts[i++], buffer);
				if (retstat == 0)
					retstat = pa_types(STRING, &index, field_counts[i], buffer);
				for (i=0; i<2 && retstat == 0; i++)
					retstat = pa_types(INT, &index, field_counts[3], buffer);
				break;

			case AUR_UNMOUNT:
				/* 
				 * 2, str str root cwd 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = 
				  	 pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_READLINK:
				/* 
				 * 5, str, str, str, str, int 
				 * root, cwd, link name, buffer, buffer count 
				 */
				for (i=1; i<5 && retstat == 0; i++)
					retstat = 
					  pa_types(STRING, &index, field_counts[i], buffer);

				if (retstat == 0)
					retstat = 
					  pa_types(INT, &index, field_counts[i], buffer);
				 break;

			case AUR_QUOTA_ON:
				/*   
				 * 5, str, str, int, str, int, str
				 * root, cwd, quota command, device, uid,
				 * quota file name 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = 
				  	 pa_types(STRING, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(INT, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(STRING, &index, field_counts[i++], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(INT, &index, field_counts[i++], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_QUOTA_OFF:
				/* 
				 * 4, str, str, int, str, int, 
				 * root, cwd, quota command, device, uid 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = 
				  	 pa_types(STRING, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(INT, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(STRING, &index, field_counts[i++], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_QUOTA_SET:
				/*  
				 * 6, str, str, int, str, int, 
				 * sizeof(struct dqblk) = 8 u_longs 
				 * /usr/include/ufs/quota.h
				 * root, cwd, quota command, device, uid 
				 * format of disk quota file	
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = 
					pa_types(STRING, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(INT, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(STRING, &index, field_counts[i++], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(INT, &index, field_counts[i++], buffer);
				for (i=0; i<8 && retstat == 0; i++)
					retstat = 
					  pa_types(ULONG, &index, field_counts[5], buffer);
				break;

			case AUR_QUOTA_LIM:
				/* 
				 * 6, str, str, int, str, int, 
				 * sizeof(struct dqblk) 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = 
				  	 pa_types(STRING, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(INT, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(STRING, &index, field_counts[i++], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(INT, &index, field_counts[i++], buffer);
				for (i=0; i<8 && retstat == 0; i++)
					retstat = 
					  pa_types(ULONG, &index, sizeof(long), buffer);
				break;

			case AUR_QUOTA_SYNC:
				/* 
				 * 5, str, str, int, str, int, 
				 * root, cwd, quota command, device, uid 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = 
				  	 pa_types(STRING, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(INT, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(STRING, &index, field_counts[i++], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_QUOTA:
				/* 
				 * 6, str, str, int, str, int, str 
				 * root, cwd, quota command, device, uid,
				 * argument supplied to quota 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = 
					 pa_types(STRING, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(INT, &index, field_counts[i], buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(STRING, &index, field_counts[i++], buffer);
				for (i=3; i<5 && retstat == 0; i++)
					retstat = 
					  pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_STATFS:
				/* 
				 * 4, str, str,
				 * sizeof(struct statfs) 
				 * 7 longs, fsid_t=2 longs, 7 longs 
				 * /usr/include/sys/vfs.h
				 * mount dir. name, fs statistics
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = 
					  pa_types(STRING, &index, field_counts[i], buffer);
				for (i=0; i<16 && retstat == 0; i++)
					retstat = 
					  pa_types(INT, &index, sizeof(long), buffer);
				break;

			case AUR_MSGCTL:
				/* 
				 * 3, int int int 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = 
				  	 pa_types(INT, &index, field_counts[i], buffer);
				if (retstat == 0) {
					/* 
					 * read msqid_ds structure 
					 * first read ipc_perm structure 
					 * uid, gid, cuid, cgid, mode, seq, key
					 */
					for (i=0; i<6 && retstat == 0; i++)
						retstat = 
						 pa_types(USHORT, &index, sizeof(u_short), buffer);
					if (retstat == 0)
						retstat = 
						 pa_types(LONG, &index, sizeof(long), buffer);
					/* 
					 * read the rest of msqid_ds struct 
					 * *msg_first, *msg_last
					 */
					for (i=0; i<2 && retstat == 0; i++)
						retstat = 
						 pa_types(INT, &index, sizeof(int), buffer);
					/*
					 * msg_cbytes, msg_qnum, msg_qbytes 
					 * msg_lspid, msg_lrpid, msg_stime
					 */
					for (i=0; i<5 && retstat == 0; i++)
						retstat = 
						 pa_types(USHORT, &index, sizeof(u_short), buffer);
					/*
					 * msg_rtime, msg_ctime
					 */
					for (i=0; i<3 && retstat == 0; i++)
					  	retstat = 
						 pa_types(LONG, &index, sizeof(long), buffer);
				}
				break;

			case AUR_MSGGET:
				/* 
				 * 2, long int key, msgflg
				 */
				for (i=1; i<3 && retstat == 0; i++) {
					retstat = 
					  pa_types(INT, &index, sizeof(int), buffer);
				}
				break;

			case AUR_MSGRCV:
				/* 
				 * 3, int int long  msqid, msgflg, mtype 
				 */
				for (i=1; i<3 && retstat == 0; i++)
				  	retstat = 
					 pa_types(INT, &index, sizeof(int), buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(LONG, &index, sizeof(long), buffer);
				break;

			case AUR_MSGSND:
				/* 
				 * 3, int int long msqid, msgflg, mtype 
				 */
				for (i=1; i<3 && retstat == 0; i++)
				  	retstat =
					 pa_types(INT, &index, sizeof(int), buffer);
				if (retstat == 0)
					retstat = 
					  pa_types(LONG, &index, sizeof(long), buffer);
				break;

			case AUR_SEMCTL:
				/* 
				 * 4, int int int (semid_ds struct) 
				 */
				for (i=1; i<3 && retstat == 0; i++)
				  	retstat = 
					 pa_types(INT, &index, sizeof(int), buffer);
				if (retstat == 0) {
					/*
					 * first read ipc_perm structure 
					 * uid, gid, cuid, cgid, mode, seq, key
					 */
					if (retstat == 0) {
						for (i=0; i<6 && retstat == 0; i++)
							retstat = 
						 	 pa_types(USHORT, &index, sizeof(u_short), buffer);
					}
					if (retstat == 0)
						retstat = 
						 pa_types(LONG, &index, sizeof(long), buffer);
					/* 
					 * read the rest of semid_ds struct 
					 * sem_base
					 */
					if (retstat == 0)
					  	retstat = 
						  pa_types(INT, &index, sizeof(int), buffer);
					/* # of semaphores in set */
					if (retstat == 0)
						retstat = 
						 pa_types(USHORT, &index, sizeof(u_short), buffer);
					/*
					 * last semop time and change time 
					 */
					for (i=0; i<2 && retstat == 0; i++)
					  	retstat =
						 pa_types(LONG, &index, sizeof(long), buffer);
				}
				break;

			case AUR_SEMGET:
				/* 
				 * 3, int int int 
				 */
				for (i=1; i<4 && retstat == 0; i++) {
					retstat = 
				 	 pa_types(INT, &index, sizeof(int), buffer);
				}
				break;

			case AUR_SEMOP:
				/* 
				 * 2+n; n=nsops, int int  n*sembuf stuct.
				 */
				retstat = 
				 pa_types(INT, &index, field_counts[i++], buffer);
				/* 
				 * get number of sembuf structures first
				 */
				if (retstat == 0) {
					bcopy(&(buffer[index]), &command, sizeof(int));
					retstat = 
				 	 pa_types(INT, &index, field_counts[i++], buffer);
				}
				/*
				 * read nsops sembuf structures
				 * sem_num, sem_op, sem_flg
				 */
				for (i=0; retstat == 0 && i<command; i++) {
					for (j=0; retstat == 0 && j<3; j++)
						retstat = 
						 pa_types(SHORT, &index, sizeof(short), buffer);
				}
				break;
				
			case AUR_SHMCTL:
				/* 
				 * 3, int int shmid_ds struct. 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = 
					  pa_types(INT, &index, field_counts[i], buffer);
				if (retstat == 0) { 
					/* 
					 * read shmid_ds structure 
					 * first read ipc_perm structure 
				 	 * uid, gid, cuid, cgid, mode, seq, key
					 */
					for (i=0; i<6 && retstat == 0; i++)
						retstat = 
						 pa_types(USHORT, &index, sizeof(u_short), buffer);
					if (retstat == 0)
						retstat = 
						 pa_types(LONG, &index, sizeof(long), buffer);
					/* 
					 * read the rest of shmid_ds struct 
				 	 * shm_segsz 
					 */
					if (retstat == 0)
				  		retstat =
						 pa_types(UINT, &index, sizeof(uint), buffer);
				       /*
					* shm_lpid, shm_cpid, shm_nattch
					*/
					if (retstat == 0) {
						for (i=0; i<3 && retstat == 0; i++)
							retstat = 
							 pa_types(USHORT, &index, sizeof(u_short), buffer);
					}
				 	/*
					 * shm_atime, shm_dtime, shm_ctime
					 */
					if (retstat == 0) {
						for (i=0; i<3 && retstat == 0; i++)
				  			retstat =
							 pa_types(LONG, &index, sizeof(long), buffer);
					}
					if (retstat == 0) 
						retstat =
						 pa_types(INT, &index, sizeof(int), buffer);
				}
				break;

			case AUR_SHMGET:
				/* 
				 * 3, long int int 
				 */
				retstat = 
				 pa_types(LONG, &index, sizeof(long), buffer);
				if (retstat == 0) {
					for (i=0; i<2 && retstat == 0; i++)
						retstat = 
					  	  pa_types(INT, &index, sizeof(int), buffer);
				}
				break;

			case AUR_SHMAT:
				/* 
				 * 3, int int int 
				 */
				for (i=1; i<4 && retstat == 0; i++) {
				retstat = 
				  pa_types(INT, &index, field_counts[i++], buffer);
				}
				break;

			case AUR_SHMDT:
				/* 
				 * 1, INT 
				 */
				retstat = 
				  pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_CORE:
				/* 
				 * 3, str, str, str, root, cwd, corefile 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = 
					  pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_CHROOT:
				/* 
				 * 3, str, str, str, root, cwd, new root 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = 
					  pa_types(STRING, &index, field_counts[i], buffer);
				break;

			case AUR_TEXT:
				/* 
				 * #, (#) * STRING 
				 */
				for (i=1; i<count && retstat == 0; i++)
					retstat = 
					  pa_types(STRING, &index, field_counts[i], buffer);
				break;
		
			case AUR_CHDIR:
				/* 
				 * 3 str str str 
				 */
				for (i=1; i<4 && retstat == 0; i++)
					retstat = 
				  	 pa_types(STRING, &index, field_counts[i], buffer);
				 break;

			case AUR_MSGCTLRMID:
				/* 
				 * 2 int int 
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = 
				  	 pa_types(INT, &index, field_counts[i], buffer);
				break;

			case AUR_SEMCTL3:
				/* 
				 * 3, int int int 
				 */
				for (i=1; i<4 && retstat == 0; i++)
				  	retstat = 
					 pa_types(INT, &index, sizeof(int), buffer);
				break;

			case AUR_SEMCTLALL: 
				/* 
				 * 4, int int int  unsigned short 
				 */
				for (i=1; i<4 && retstat == 0; i++)
				  	retstat = 
					 pa_types(INT, &index, sizeof(int), buffer);
				if (retstat == 0)
				for (i=0; i<field_counts[4] && retstat == 0; i++) {
					retstat = 
					 pa_types(SHORT, &index, sizeof(short), buffer);
				}
				break;

			case AUR_SHMCTLRMID:
				/* 
				 * 2, int int
				 */
				for (i=1; i<3 && retstat == 0; i++)
					retstat = 
					  pa_types(INT, &index, field_counts[i], buffer);
				break;


			}
			if (retstat == 0) {
				uvaltype = OUTREC;
				retstat = pa_print(NOINDEX, 0, 1);
		    	}
		    }
		}
		/*
		 * free buffer if large buffer used
		 */
		 if (largebuf)
			free(big_buff);
	}
	return (retstat);
}
			
/*
 * pa_types()	: 
 *				  
 * input   		: 
 * output		: 
 * return codes 	: -1 - error
 *			   0 - successful 
 */

pa_types(type, index, size, buffer)
int type;
int *index;
short size;
char *buffer;
{
	short sval;
	u_short usval;
	int retstat = 0, val, n;
	long lval;
	unsigned long oval;
	u_long ulval;
	char *stringp, in_char;

	if (size != 0) {
		switch((int)type) {
		default:
			fprintf(stderr, "praudit: Unknown type.\n");
			retstat = -1;
			break;
		case INT:
			bcopy(&(buffer[*index]), &val, sizeof(int));
			uvaltype = INT;
			uval.int_val = val; 
			break;
		case UINT:
			bcopy(&(buffer[*index]), &val, sizeof(u_int));
			uvaltype = UINT;
			uval.uint_val = val; 
			break;
		case SHORT:
			bcopy(&(buffer[*index]), &sval, sizeof(short));
			uvaltype = SHORT;
			uval.short_val = sval; 
			break;
		case USHORT:
			bcopy(&(buffer[*index]), &usval, sizeof(short));
			uvaltype = USHORT;
			uval.ushort_val= usval; 
			break;
		case LONG:
			bcopy(&(buffer[*index]), &lval, sizeof(long));
			uvaltype = LONG;
			uval.long_val = lval; 
			break;
		case ULONG:
			bcopy(&(buffer[*index]), &ulval, sizeof(long));
			uvaltype = ULONG;
			uval.ulong_val= lval; 
			break;
		case CHAR:
			in_char = buffer[*index];
			uvaltype = CHAR;
			uval.char_val = in_char; 
			break;
		case STRING:
			stringp = &(buffer[*index]);
			uvaltype = STRING;
			uval.string_val = stringp; 
			break;
		case HEX:
			bcopy(&(buffer[*index]), &lval, sizeof(int));
			uvaltype = HEX;
			uval.long_val = lval; 
			break;
		case SHEX:
			bcopy(&(buffer[*index]), &sval, sizeof(int));
			uvaltype = SHEX;
			uval.short_val = sval; 
			break;
		case OCT:
			bcopy(&(buffer[*index]), &oval, sizeof(int));
			uvaltype = OCT;
			uval.ushort_val = oval & 07777; 
			break;
		}

		retstat = pa_print(NOINDEX, 1, 0);
		*index += size;
	}
	return (retstat);
}


/*
 * pa_print()	:  print as one str or formatted for easy reading. 
 * input   	: index - index into format info. array 
 * 		: flag - indicates whether to output a new line for 
 *		:        multi-line output.
 * 		:		= 0; no new line 
 *		:		= 1; new line if regular output 
 *		: endrec - indicates whether to output 
 *			   a new line for one-line output.
 * 				= 0; no new line 
 * 				= 1; new line 
 * output	: 
 * return codes : -1 - error
 *                 0 - successful 
 */

pa_print(index, flag, endrec)
int index;
int flag;
int endrec;
{
	int retstat = 0;

	if (ONELINE == 1 || index == NOINDEX) {
		if (uvaltype != OUTREC) {
			if (ONELINE == 1)
				printf("%s", SEPERATOR);

			switch(uvaltype) {
			default:
				fprintf(stderr, "praudit: Unknown type.\n");
				retstat = -1;
				break;
			case INT:
				printf("%d", uval.int_val); 
				break;
			case UINT:
				printf("%d", uval.uint_val); 
				break;
			case SHORT:
				printf("%hd", uval.short_val); 
				break;
			case USHORT:
				printf("%hd", uval.ushort_val); 
				break;
			case LONG:
				printf("%d", uval.long_val); 
				break;
			case ULONG:
				printf("%d", uval.ulong_val); 
				break;
			case CHAR:
				printf("%c", uval.char_val);
				break;
			case STRING:
				printf("%s", uval.string_val); 
				break;
			case HEX:
				printf("%lx", uval.long_val); 
				break;
			case SHEX:
				printf("%hx", uval.short_val); 
				break;
			case OCT:
				printf("%ho", uval.ushort_val); 
				break;
			}
		} 
		if (ONELINE == 1 && endrec == 1) {
			printf("%s", SEPERATOR);
			printf("%c", '\n'); 
		}

		if (index == NOINDEX && ONELINE == 0) {
			if (flag == 1)
				printf("\n");
		}
	} else 
		/* 
		 * format output 
		 */
			retstat = pa_display(index);

	return (retstat);
}

pa_display(index)
int index;
{
	int loc = 2, retstat = 0;
	char *format = "%-     ";
	static char buffer[156];

	/* 
	 * insert field width and type into format 
	 */
	strncpy(format+loc,rec_display[index].width,strlen(rec_display[index].width));
	loc += strlen(rec_display[index].width);

	/* 
	 * copy data type into format and data into buffer 
	 */
	switch(uvaltype) {
	case INT:
		strcpy(format+loc, "d");
		sprintf(buffer+(rec_display[index].pos), format, uval.int_val);
		break;
	case UINT:
		strcpy(format+loc, "d");
		sprintf(buffer+(rec_display[index].pos), format, uval.uint_val);
		break;
	case SHORT:
		strcpy(format+loc, "hd");
		sprintf(buffer+(rec_display[index].pos), format, uval.short_val);
		break;
	case USHORT:
		strcpy(format+loc, "hd");
		sprintf(buffer+(rec_display[index].pos), format, uval.ushort_val);
		break;
	case LONG:
		strcpy(format+loc, "ld");
		sprintf(buffer+(rec_display[index].pos), format, uval.long_val);
		break;
	case ULONG:
		strcpy(format+loc, "ld");
		sprintf(buffer+(rec_display[index].pos), format, uval.ulong_val);
		break;
	case CHAR:
		strcpy(format+loc, "c");
		sprintf(buffer+(rec_display[index].pos), format, uval.char_val);
		break;
	case STRING:
		strcpy(format+loc, "s");
		sprintf(buffer+(rec_display[index].pos), format, uval.string_val);
		break;
	case HEX:
		strcpy(format+loc, "lx");
		sprintf(buffer+(rec_display[index].pos), format, uval.long_val);
		break;
	case SHEX:
		strcpy(format+loc, "hx");
		sprintf(buffer+(rec_display[index].pos), format, uval.short_val);
		break;
	case OCT:
		strcpy(format+loc, "ho");
		sprintf(buffer+(rec_display[index].pos), format, uval.ushort_val);
		break;
	default:
		fprintf(stderr, "praudit: Unknown field type.\n");
		retstat = -1;
		break;
	}
	/* 
	 * if last entry, add new line and output 
	 */
	if(rec_display[index].printit)
		printf("%s\n", buffer);

	return (retstat);
}

/*
 * pa_trailer()	: process audit file trailer. Output name of next audit file. 
 * input   	: 
 * output	: 
 * return codes : -1 - error
 *		   0 - successful trailer process
 */

pa_trailer(size)
short size;
{
	int retstat = 0, recsize;
	char *time_created, *fname, temp_buff[4];
	audit_trailer_t trailer;

	/* 
	 * read rest of trailer record 
	 */
	recsize = sizeof(trailer) - (2 * sizeof(short));
	if ((fread(&(trailer.at_time), sizeof(char), 
			recsize, stdin)) != recsize) {
		fprintf(stderr, "praudit: Read error.\n");
		retstat = -1;
	}

	if (retstat == 0) {
		time_created = (char *)ctime(&(trailer.at_time));
		strcpy(time_created+24, "\0"); 
		uvaltype = STRING;
		uval.string_val = time_created; 
		if(trailer.at_namelen != 0) { 
			if ((fname = (char *)malloc(((unsigned int)trailer.at_namelen) * sizeof(char))) == NULL) {
				fprintf(stderr, "praudit: Error in allocating space for trailer field.\n");
				retstat = -1;
			/*
			 * read next audit file name 
			 */
			} else if ((fread(fname, sizeof(char), 
				  trailer.at_namelen, stdin)) != 
				  trailer.at_namelen) {
				fprintf(stderr, 
				  "praudit: Read error in next audit file name.\n");
				retstat = -1;
			} else if ((retstat = 
			  pa_print(NOINDEX, 1, 0)) == 0) {
				uvaltype = STRING;
				uval.string_val = fname; 
				if ((retstat = 
		  		  pa_print(NOINDEX, 1, 1)) == 0) {
					/*
					 * if extra bytes, read them 
				 	 */
					recsize = 
					size - (sizeof(trailer) + 
				  	  trailer.at_namelen);
					if (recsize > sizeof(temp_buff)) {
						fprintf(stderr, 
					 	  "praudit: Read error.\n");
						retstat = -1;
					} else if (recsize > 0) {
						if ((fread(temp_buff,
						  sizeof(char), recsize, 
						  stdin)) != recsize) {
							fprintf(stderr, 
							  "praudit: Read error.\n");
							retstat = -1;
						}
					}
				}
			}
			if (fname != NULL) free(fname);
		} else
			retstat = pa_print(NOINDEX, 1, 1);
	}
	return (retstat);
}

char *
getevents(verbose, event_val)
int verbose;
unsigned int event_val;
{
	int i, event, event_list[MAXEVENT], count = 0;
	static char event_string[MAXEVENTSTR];

	/*
	 * collect enabled events
	 */
	for (event = 0; event_val != 0; event++, event_val >>= 1) {
		if ((event_val & 1) != 0) {
			event_list[count++] = event;
		}
	}
	/*
	 * build displayable event str 
	 */
	event_string[0] = '\0';
	for (i=0; i<count; i++) {
		if (verbose)
			strcat(event_string, event_class[event_list[i]].event_lname);
		else
			strcat(event_string, event_class[event_list[i]].event_sname);
		if (i < count-1)
			strcat(event_string, "|");
	}
	return (event_string);
}
