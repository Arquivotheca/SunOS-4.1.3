#ifndef lint
static  char sccsid[] = "@(#)bootparam_lib.c 1.1 92/07/30 SMI";
#endif

/*
 * Library routines of the bootparam server.
 */

#include <stdio.h>
#include <rpcsvc/ypclnt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#define iseol(c)	(c == '\0' || c == '\n' || c == '#')
#define issep(c)	(c == ' ' || c == '\t')

#define BOOTPARAMS	"/etc/bootparams"
#define	LINESIZE	512
#define NAMESIZE	256
#define DOMAINSIZE	256

static char domain[DOMAINSIZE];			/* NIS domain name */

struct stat laststat;
static char *bpbuf;

char *
read_bootparams()
{
	int fd;
	struct stat statb;

	if (stat(BOOTPARAMS, &statb) < 0) {
		fprintf(stderr, "rpc.bootparamsd: can't stat %s\n", BOOTPARAMS);
		return (NULL);
	}

	if (statb.st_mtime != laststat.st_mtime) {
		if ((fd = open(BOOTPARAMS, O_RDWR, 0)) < 0) {
			perror(BOOTPARAMS);
			return (NULL);
		}
		if (bpbuf) {
			free(bpbuf);
		}
		bpbuf = (char *)malloc(statb.st_size + 1);
		if (bpbuf == NULL) {
			fprintf("rpc.bootparamd: out of memory\n");
			return (NULL);
		}
		if (read(fd, bpbuf, statb.st_size) < 0) {
			perror(BOOTPARAMS);
			return (NULL);
		}
		bpbuf[statb.st_size] = '\0';
		laststat = statb;
	}
	return (bpbuf);
}

/*
 * getline()
 * Read a line from a buffer.
 * Fill in line buffer, and update lpp, return number of chars.
 */
getline(line, maxlinelen, lpp)
	char *line;
	int maxlinelen;
	char **lpp;
{
	int count;
	char *p;
	char *linestart;

	p = *lpp;
	if (p == NULL || *p == '\0') {
		return (0);
	}
	count = 0;
	linestart = line;

	while (iseol(*p)) {
		p++;
	}

	while (*p) {
		if (*p == '\n') {
			if (count && p[-1] == '\\') {
				/*
				 * Continuation line, back up one char
				 * and get next line from buf.
				 */
				line--;
				p++;
			} else {
				break;
			}
		}
		count++;
		*line++ = *p++;
	}
	*line = '\0';
	*lpp = p;

	return (count);
}

/*
 * getname()
 * Get the next entry from the line.
 * You tell getname() which characters to ignore before storing them 
 * into name, and which characters separate entries in a line.
 * The cookie is updated appropriately.
 * return:
 *	   1: one entry parsed
 *	   0: partial entry parsed, ran out of space in name
 *    -1: no more entries in line
 */
int
getname(name, namelen, linep)
	char *name;
	int namelen;
	char **linep;
{
	char c;
	char *lp;
	char *np;
	int maxnamelen;

	lp = *linep;
	do {
		c = *lp++;
	} while (issep(c) && !iseol(c));

	if (iseol(c)) {
		*linep = lp - 1;
		return(0);
	}

	np = name;
	while (! issep(c) && ! iseol(c) && np - name < namelen - 1) {	
		*np++ = c;	
		c = *lp++;
	} 
	*np = '\0';
	lp--;
	*linep = lp;
	return (np - name);
}

/*
 * getclntent reads the line buffer and returns a string of client entry
 * in NIS or the "/etc/bootparams" file. Called by getclntent.
 */
static int
getvalue(lpp, clnt_entry)
	register char **lpp;			/* function input */
	register char *clnt_entry;		/* function output */
{
	char name[NAMESIZE];
	int append = 0;

	while (getname(name, sizeof(name), lpp) > 0) {
		if (!append) {
			strcpy(clnt_entry, name);
			append++;
		} else {
			strcat(clnt_entry, " ");
			strcat(clnt_entry, name);
		}
	}
	return (0);
}	

/*
 * getfileent returns the client entry in the "/etc/bootparams"
 * file given the client name.
 */
int
bp_getclntent(clnt_name, clnt_entry)
	register char *clnt_name;		/* function input */
	register char *clnt_entry;		/* function output */
{
	FILE *fp; 
	char line[LINESIZE];
	char name[NAMESIZE];
	char *lp;
	char *linep;
	char *val;
	int vallen;
	int reason;
 
	reason = -1;
	lp = read_bootparams();
	while (reason && getline(line, sizeof(line), &lp)) {
		linep = line;
		val = NULL;

		if (*linep == '+' && useyp()) {
			linep++;
			if (getname(name, sizeof(name), &linep)) {
				reason = yp_match(domain, name, clnt_name,
				    strlen(clnt_name), &val, &vallen);
			} else {
				reason = yp_match(domain, "bootparams",
				    clnt_name, strlen(clnt_name), &val,&vallen);
			}
			if (!reason) {
				linep = val;
			}
		}

		if (!reason || (getname(name, sizeof(name), &linep) > 0
		    && strcmp(name,clnt_name) == 0)) {
			reason = getvalue(&linep, clnt_entry);
		}

		if (val) {
			free(val);
		}
	}
	return (reason);
}

/*
 * return the value string associated with clnt_key (key=value) in
 * the line buffer lpp. Update lpp.
 */
static int
getkeyval(lpp, clnt_key, clnt_entry)
	register char **lpp;			/* function input */
	register char *clnt_key;		/* function input */
	register char *clnt_entry;		/* function output */
{
	char name[NAMESIZE];
	char *cp;

	while (getname(name, sizeof(name), lpp) > 0) {
		if ((cp = (char *)index(name, '=')) == 0)
			return (-1);
		*cp++ = '\0';
		if (strcmp(name, clnt_key) == 0) {
			strcpy(clnt_entry, cp);
			return (0);
		}
	}
	if (strcmp(clnt_key, "dump") == 0) {
		/*
		 * This is a gross hack to handle the case where 
		 * no dump file exists in bootparams. The null
		 * server and path names indicate this fact to the
		 * client.
		 */
		strcpy(clnt_entry, ":");
		return (0);
	}
	return (-1);
}

/*
 * getclntkey returns the client's server name and its pathname from either
 * the NIS or the "/etc/bootparams" file given the client name and
 * the key.
 */
int
bp_getclntkey(clnt_name, clnt_key, clnt_entry)
	register char *clnt_name;		/* function input */
	register char *clnt_key;		/* function input */
	register char *clnt_entry;		/* function output */
{
	char line[LINESIZE];
	char name[NAMESIZE];
	char *lp;
	char *linep;
	char *val;
	int reason;
	int vallen;
 
	reason = -1;
	lp = read_bootparams();
	while (reason && getline(line, sizeof(line), &lp)) {
		linep = line;
		val = NULL;

		if (*linep == '+' && useyp()) {
			linep++;
			if (getname(name, sizeof(name), &linep)) {
				reason = yp_match(domain, name, clnt_name,
				    strlen(clnt_name), &val, &vallen);
			} else {
				reason = yp_match(domain, "bootparams",
				    clnt_name, strlen(clnt_name), &val,&vallen);
			}
			if (!reason) {
				linep = val;
			}
		}

		if (!reason || ( getname(name, sizeof(name), &linep) > 0
		    && strcmp(name,clnt_name) == 0) ) {
			reason = getkeyval(&linep, clnt_key, clnt_entry);
		}

		if (val) {
			free(val);
		}
	}
	return (reason);
}

/*
 * Determine whether or not to use the NIS service to do lookups.
 */
static int initted;
static int usingyp;
static int
useyp()
{
	if (!initted) {
		getdomainname(domain, sizeof(domain));
		usingyp = !yp_bind(domain);
		initted = 1;
	}
	return (usingyp);
}

/*
 * Determine if a descriptor belongs to a socket or not
 */
issock(fd)
	int fd;
{
	struct stat st;

	if (fstat(fd, &st) < 0) {
		return (0);
	} 
	/*	
	 * SunOS returns S_IFIFO for sockets, while 4.3 returns 0 and
	 * does not even have an S_IFIFO mode.  Since there is confusion 
	 * about what the mode is, we check for what it is not instead of 
	 * what it is.
	 */
	switch (st.st_mode & S_IFMT) {
	case S_IFCHR:
	case S_IFREG:
	case S_IFLNK:
	case S_IFDIR:
	case S_IFBLK:
		return (0);
	default:	
		return (1);
	}
}
