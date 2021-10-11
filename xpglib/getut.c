/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#if !defined(lint) && defined(SCCSIDS)
static  char *sccsid = "@(#)getut.c 1.1 92/07/30 SMI";
#endif


/*	Routines to read and write the /etc/utmp file.		
 * 	Modified to simulate the SysV operation on top of the real
 *	berkeley utmp structure. Will fake some fields if we are
 *	using the real utmp file as described in UTMP_FILE.
 * 	If we are using a non standard file we make assumptions that 
 * 	it is a non-berkeley file and use the sysV format accordingly
 */

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	<fcntl.h>

#define	MAXFILE	79	/* Maximum pathname length for "utmp" file */

/* 
 *	This is the real McCoy which we will find in /etc/utmp
 */


void 	copyutmp();
struct xpg87_utmp *xpg87_getutent();
struct xpg87_utmp *sys5_getutent();
struct xpg87_utmp *xpg87_getutid();
struct xpg87_utmp *sys5_getutid();
struct xpg87_utmp *xpg87_getutline();
struct xpg87_utmp *sys5_getutline();
struct xpg87_utmp *xpg87_pututline();
struct xpg87_utmp *sys5_pututline();

void 	endutent(), setutent();

static struct utmp *getutent(), *getutid(), *pututline();

/*
 *	Local xpg87 copy of utmp which cannot be confused with the 
 *	real utmp.h file which would define a different structure
 */

#define	UTMP_FILE	"/etc/utmp"
#define	WTMP_FILE	"/etc/wtmp"
#define	ut_name	ut_user

struct xpg87_utmp
  {
	char ut_user[8] ;		/* User login name */
	char ut_id[4] ; 		/* /etc/lines id(usually line #) */
	char ut_line[12] ;		/* device name (console, lnxx) */
	short ut_pid ;			/* process id */
	short ut_type ; 		/* type of entry */
	struct exit_status
	  {
	    short e_termination ;	/* Process termination status */
	    short e_exit ;		/* Process exit status */
	  }
	ut_exit ;			/* The exit status of a process
					 * marked as DEAD_PROCESS.
					 */
	time_t ut_time ;		/* time entry was made */
  } ;
/*
 * Structure of real SunOS utmp and wtmp files.
 *
 * XXX - Assuming the number 8 is unwise (still).
 */
struct utmp {
	char	ut_line[8];		/* tty name */
	char	ut_name[8];		/* user id */
	char	ut_host[16];		/* host name, if remote */
	long	ut_time;		/* time on */
};

/*	Definitions for xpg87 ut_type						*/

#define	EMPTY		0
#define	RUN_LVL		1
#define	BOOT_TIME	2
#define	OLD_TIME	3
#define	NEW_TIME	4
#define	INIT_PROCESS	5	/* Process spawned by "init" */
#define	LOGIN_PROCESS	6	/* A "getty" process waiting for login */
#define	USER_PROCESS	7	/* A user process */
#define	DEAD_PROCESS	8
#define	ACCOUNTING	9

#define	UTMAXTYPE	ACCOUNTING	/* Largest legal value of ut_type */

/*	Special strings or formats used in the "ut_line" field when	*/
/*	accounting for something other than a process.			*/
/*	No string for the ut_line field can be more than 11 chars +	*/
/*	a NULL in length.						*/

#define	RUNLVL_MSG	"run-level %c"
#define	BOOT_MSG	"system boot"
#define	OTIME_MSG	"old time"
#define	NTIME_MSG	"new time"

static int fd = -1;	/* File descriptor for the utmp file. */

static char utmpfile[MAXFILE+1] = UTMP_FILE;    /* Name of the current
						 * "utmp" like file.
						 */
static long loc_utmp;	/* Where in "utmp" the current "ubuf" was
			 * found.
			 */
static struct utmp ubuf;		/* Copy of last entry read in. */
static struct xpg87_utmp xpg87_ubuf;	


/* 
 *	"xpg87_getutent" gets the next entry in the utmp file.
 */


struct xpg87_utmp *xpg87_getutent()
{

	if (strcmp(utmpfile, UTMP_FILE))
		return (sys5_getutent());
	else {
		(void) getutent();
		copyutmp();
		return &xpg87_ubuf;
	}
}

struct xpg87_utmp *sys5_getutent()
{
	extern int fd;
	extern char utmpfile[];
	extern struct xpg87_utmp xpg87_ubuf;
	extern long loc_utmp,lseek();
	extern int errno;
	register char *u;
	register int i;
	struct stat stbuf;

/* If the "utmp" file is not open, attempt to open it for
 * reading.  If there is no file, attempt to create one.  If
 * both attempts fail, return NULL.  If the file exists, but
 * isn't readable and writeable, do not attempt to create.
 */
{

	if (fd < 0) {

/* Make sure file is a multiple of 'utmp' entries long */
		if (stat(utmpfile,&stbuf) == 0) {
			if((stbuf.st_size % sizeof(struct xpg87_utmp)) != 0) {
				errno = EINVAL;
				return(NULL);
			}
		}
		if ((fd = open(utmpfile, O_RDWR|O_CREAT, 0644)) < 0) {

/* If the open failed for permissions, try opening it only for
 * reading.  All "pututline()" later will fail the writes.
 */
			if (errno == EACCES
			    && (fd = open(utmpfile, O_RDONLY)) < 0)
				return(NULL);
		}
	}

/* Try to read in the next entry from the utmp file.  */
	if (read(fd,&xpg87_ubuf,sizeof(xpg87_ubuf)) != sizeof(xpg87_ubuf)) {

/* Make sure xpg87_ubuf is zeroed. */
		for (i=0,u=(char *)(&xpg87_ubuf); i<sizeof(xpg87_ubuf); i++) *u++ = '\0';
		loc_utmp = 0;
		return(NULL);
	}

/* Save the location in the file where this entry was found. */
	loc_utmp = lseek(fd,0L,1) - (long)(sizeof(struct xpg87_utmp));
	return(&xpg87_ubuf);
}
}

static struct utmp *getutent()
{
	extern int fd;
	extern long loc_utmp,lseek();
	register char *u;
	register int i;
	struct stat stbuf;

	if (fd < 0) {
		if (stat(utmpfile,&stbuf) == 0) {
			if((stbuf.st_size % sizeof(struct utmp)) != 0) {
				errno = EINVAL;
				return(NULL);
			}
		}
		if ((fd = open(utmpfile, O_RDWR|O_CREAT, 0644)) < 0) {
			if (errno == EACCES
			    && (fd = open(utmpfile, O_RDONLY)) < 0)
				return(NULL);
		}
	}

	if (read(fd,&ubuf,sizeof(ubuf)) != sizeof(ubuf)) {
		for (i=0,u=(char *)(&ubuf); i<sizeof(ubuf); i++) *u++ = '\0';
		loc_utmp = 0;
		return(NULL);
	}

	loc_utmp = lseek(fd,0L,1) - (long)(sizeof(struct utmp));
	return(&ubuf);
}

struct xpg87_utmp *xpg87_getutid(entry)
register struct xpg87_utmp *entry;
{

	if (strcmp(utmpfile,UTMP_FILE))
		return sys5_getutid(entry);
	else {
		getutid(entry); 
		copyutmp();
                return &xpg87_ubuf;
	}
}


struct xpg87_utmp *sys5_getutid(entry)
register struct xpg87_utmp *entry;
{
	extern struct xpg87_utmp xpg87_ubuf;
	struct xpg87_utmp *xpg87_getutent();
	register short type;

/* Start looking for entry.  Look in our current buffer before */
/* reading in new entries. */
	do {

/* If there is no entry in "ubuf", skip to the read. */
		if (xpg87_ubuf.ut_type != EMPTY) {
			switch(entry->ut_type) {

/* Do not look for an entry if the user sent us an EMPTY entry. */
			case EMPTY:
				return(NULL);

/* For RUN_LVL, BOOT_TIME, OLD_TIME, and NEW_TIME entries, only */
/* the types have to match.  If they do, return the address of */
/* internal buffer. */
			case RUN_LVL:
			case BOOT_TIME:
			case OLD_TIME:
			case NEW_TIME:
				if (entry->ut_type == xpg87_ubuf.ut_type) return(&xpg87_ubuf);
				break;

/* For INIT_PROCESS, LOGIN_PROCESS, USER_PROCESS, and DEAD_PROCESS */
/* the type of the entry in "xpg87_ubuf", must be one of the above and */
/* id's must match. */
			case INIT_PROCESS:
			case LOGIN_PROCESS:
			case USER_PROCESS:
			case DEAD_PROCESS:
				if (((type = xpg87_ubuf.ut_type) == INIT_PROCESS
					|| type == LOGIN_PROCESS
					|| type == USER_PROCESS
					|| type == DEAD_PROCESS)
				    && xpg87_ubuf.ut_id[0] == entry->ut_id[0]
				    && xpg87_ubuf.ut_id[1] == entry->ut_id[1]
				    && xpg87_ubuf.ut_id[2] == entry->ut_id[2]
				    && xpg87_ubuf.ut_id[3] == entry->ut_id[3])
					return(&xpg87_ubuf);
				break;

/* Do not search for illegal types of entry. */
			default:
				return(NULL);
			}
		}
	} while (xpg87_getutent() != NULL);

/* Return NULL since the proper entry wasn't found. */
	return(NULL);
}

/* Treat getutid and getutline the same as we do not store
 * id information, only line is really meaningful information 
 */

static struct utmp *getutid(entry)
register struct utmp *entry;
{

	do {
		if (strcmp(ubuf.ut_line, entry->ut_line))
			continue;
		else
			return(&ubuf);
	} while (getutent() != NULL);

	return(NULL);
}



struct xpg87_utmp *xpg87_getutline(entry)
register struct xpg87_utmp *entry;
{

	if (strcmp(utmpfile,UTMP_FILE))
		return (sys5_getutline(entry));
	else {
		if (getutid(entry) == NULL)
			return ((struct xpg87_utmp *) NULL);
		copyutmp();
		return &xpg87_ubuf;
	}
}


/*******************************************************************
 * "sys5_getutline" searches the "utmp" file for a LOGIN_PROCESS or
 * USER_PROCESS with the same "line" as the specified "entry".
 */

struct xpg87_utmp *sys5_getutline(entry)
register struct xpg87_utmp *entry;
{
	register struct xpg87_utmp *cur;

/* Start by using the entry currently incore.  This prevents */
/* doing reads that aren't necessary. */
	cur = &xpg87_ubuf;
	do {

/* If the current entry is the one we are interested in, return */
/* a pointer to it. */

		if (cur->ut_type != EMPTY && (cur->ut_type == LOGIN_PROCESS
		    || cur->ut_type == USER_PROCESS) && strncmp(&entry->ut_line[0],
		    &cur->ut_line[0],sizeof(cur->ut_line)) == 0) return(cur);
	} while ((cur = xpg87_getutent()) != NULL);

/* Since entry wasn't found, return NULL. */
	return(NULL);
}

/*	"pututline" writes the structure sent into the utmp file.	*/
/*	If there is already an entry with the same id, then it is	*/
/*	overwritten, otherwise a new entry is made at the end of the	*/
/*	utmp file.							*/

struct xpg87_utmp *xpg87_pututline(entry)
register struct xpg87_utmp *entry;
{

	if (strcmp(utmpfile,UTMP_FILE))
		return (sys5_pututline(entry));
	else {
		pututline(entry);
		copyutmp();
		return &xpg87_ubuf;
	}
}

struct xpg87_utmp *sys5_pututline(entry)
struct xpg87_utmp *entry;
{
	int fc;
	struct xpg87_utmp *answer;
	extern long time();
	extern long loc_utmp,lseek();
	extern int fd,errno;
	struct xpg87_utmp tmpbuf;

/* Copy the user supplied entry into our temporary buffer to */
/* avoid the possibility that the user is actually passing us */
/* the address of "ubuf". */
	tmpbuf = *entry;
	xpg87_getutent();
	if (fd < 0) {
		return((struct xpg87_utmp *)NULL);
	}
/* Make sure file is writable */
	if ((fc=fcntl(fd, F_GETFL, NULL)) == -1
	    || (fc & O_RDWR) != O_RDWR) {
		return((struct xpg87_utmp *)NULL);
	}

/* Find the proper entry in the utmp file.  Start at the current */
/* location.  If it isn't found from here to the end of the */
/* file, then reset to the beginning of the file and try again. */
/* If it still isn't found, then write a new entry at the end of */
/* the file.  (Making sure the location is an integral number of */
/* utmp structures into the file incase the file is scribbled.) */

	if (xpg87_getutid(&tmpbuf) == NULL) {
		setutent();
		if (xpg87_getutid(&tmpbuf) == NULL) {
			fcntl(fd, F_SETFL, fc | O_APPEND);
		} else {
			lseek(fd, -(long)sizeof(struct xpg87_utmp), 1);
		}
	} else {
		lseek(fd, -(long)sizeof(struct xpg87_utmp), 1);
	}

/* Write out the user supplied structure.  If the write fails, */
/* then the user probably doesn't have permission to write the */
/* utmp file. */
	if (write(fd,&tmpbuf,sizeof(tmpbuf)) != sizeof(tmpbuf)) {
		answer = NULL;
	} else {
/* Copy the user structure into ubuf so that it will be up to */
/* date in the future. */
		xpg87_ubuf = tmpbuf;
		answer = &xpg87_ubuf;

	}
	fcntl(fd, F_SETFL, fc);
	return(answer);
}


static struct utmp *pututline(entry)
struct utmp *entry;
{
	int fc;
	struct utmp *answer;
	extern long time();
	extern long loc_utmp,lseek();
	extern int fd,errno;
	struct utmp tmpbuf;

	tmpbuf = *entry;
	getutent();
	if (fd < 0) {
		return((struct utmp *)NULL);
	}
	if ((fc=fcntl(fd, F_GETFL, NULL)) == -1
	    || (fc & O_RDWR) != O_RDWR) {
		return((struct utmp *)NULL);
	}

	if (getutid(&tmpbuf) == NULL) {
		setutent();
		if (getutid(&tmpbuf) == NULL) {
			fcntl(fd, F_SETFL, fc | O_APPEND);
		} else {
			lseek(fd, -(long)sizeof(struct utmp), 1);
		}
	} else
		lseek(fd, -(long)sizeof(struct utmp), 1);

	if (write(fd,&tmpbuf,sizeof(tmpbuf)) != sizeof(tmpbuf)) {
		answer = NULL;
	} else {
		ubuf = tmpbuf;
		answer = &ubuf;

	}
	fcntl(fd, F_SETFL, fc);
	return(answer);
}



/*	"setutent" just resets the utmp file back to the beginning.	*/

void setutent()
{
	register char *ptr;
	register int i;

	if (strcmp(utmpfile,UTMP_FILE)) {
		if (fd != -1) lseek(fd,0L,0);
		for (i=0,ptr=(char*)&xpg87_ubuf; i < sizeof(xpg87_ubuf);i++) *ptr++ = '\0';
		loc_utmp = 0L;
	}
	else {
		if (fd != -1) lseek(fd,0L,0);
		for (i=0,ptr=(char*)&ubuf; i < sizeof(ubuf);i++) *ptr++ = '\0';
		loc_utmp = 0L;
	}
		
}


/*	"endutent" closes the utmp file.				*/
void endutent()
{
	register char *ptr;
	register int i;

	if (strcmp(utmpfile,UTMP_FILE)) {
		if (fd != -1) 
			close(fd);
		fd = -1;
		loc_utmp = 0;
		for (i=0,ptr= (char *)(&xpg87_ubuf); i < sizeof(xpg87_ubuf);i++)
			*ptr++ = '\0';
	}
	else {
		if (fd != -1) close(fd);
		fd = -1;
		loc_utmp = 0;
		for (i=0,ptr= (char *)(&ubuf); i < sizeof(ubuf);i++) 
			*ptr++ = '\0';

	}

}

/*	"utmpname" allows the user to read a file other than the	*/
/*	normal "utmp" file.						*/

utmpname(newfile)
char *newfile;
{
	extern char utmpfile[];

/* Determine if the new filename will fit.  If not, return 0. */
	if (strlen(newfile) > MAXFILE) return (0);

/* Otherwise copy in the new file name. */
	else strcpy(&utmpfile[0],newfile);

/* Make sure everything is reset to the beginning state. */
	endutent();
	return(1);
}

void copyutmp() 

{
	register short i;
	
	memcpy(xpg87_ubuf.ut_user, ubuf.ut_line, 8); /* note no macro for 8 */
	memcpy(xpg87_ubuf.ut_line, ubuf.ut_line, 8); /* ditto */
	xpg87_ubuf.ut_time = ubuf.ut_time;
	for(i=0;i<4;i++) {
		xpg87_ubuf.ut_id[i] = 0;
		xpg87_ubuf.ut_line[8+i] = 0;
	}
	xpg87_ubuf.ut_type = USER_PROCESS;
	xpg87_ubuf.ut_exit.e_termination  = 0;	
	xpg87_ubuf.ut_exit.e_exit = 0;
}



