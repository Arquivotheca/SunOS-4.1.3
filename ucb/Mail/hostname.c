#ifndef lint
static	char *sccsid = "@(#)hostname.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif
/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Code to figure out what host we are on.
 */

#include "rcv.h"
#include "configdefs.h"

char host[64];
char domain[128];
/*
 * Initialize the network name of the current host.
 */
inithost()
{
	register struct netmach *np;

	gethostname(host, sizeof host);
	strcpy(domain, host);
	strcat(domain, MYDOMAIN);
	for (np = netmach; np->nt_machine != 0; np++)
		if (strcmp(np->nt_machine, EMPTY) == 0)
			break;
	if (np->nt_machine == 0) {
		printf("Cannot find empty slot for dynamic host entry\n");
		exit(1);
	}
	np->nt_machine = host;
	np++;
	np->nt_machine = domain;
	if (debug) fprintf(stderr, "host '%s', domain '%s'\n", host, domain);
}

#ifdef UNAME

#ifdef USG
#include <sys/utsname.h>
#else USG
/* @(#)utsname.h	1.1	*/
struct utsname {
	char	sysname[9];
	char	nodename[9];
	char	release[9];
	char	version[9];
	char	machine[9];
};
#endif USG

gethostname(host, size)
char *host;
int size;
{
	struct utsname name;
	uname(&name);
	strcpy(host, name.nodename);
}

#ifndef USG
/*
 * This routine is compatible with the USG system call uname(2),
 * which figures out the name of the local system.
 * However, we do it by reading the file /usr/include/whoami.h.
 * This avoids having to recompile programs for each site.
 * The other strings are currently compiled in, since there is
 * no standard place to get them.  In 4.2BSD the gethostname call
 * is used instead.
 */
#include <stdio.h>
#define	HDRFILE "/usr/include/whoami.h"

#define SYS "vmunix"
#define REL "4.1BSD"
#define VER "vax"
#define MACH "vax-780"

uname(uptr)
struct utsname *uptr;
{
	char buf[BUFSIZ];
	FILE *fd;
	
	fd = fopen(HDRFILE, "r");
	if (fd == NULL) {
		fprintf(stderr, "Cannot open %s\n", HDRFILE);
		exit(1);
	}
	
	for (;;) {	/* each line in the file */
		if (fgets(buf, sizeof buf, fd) == NULL) {
			fprintf(stderr, "no sysname in %s\n", HDRFILE);
			exit(2);
		}
		if (sscanf(buf, "#define sysname \"%[^\"]\"",
				uptr->nodename) == 1) {
			strcpy(uptr->sysname, SYS);
			strcpy(uptr->release, REL);
			strcpy(uptr->version, VER);
			strcpy(uptr->machine, MACH);
			return;
		}
	}
}
#endif UNAME
#endif USG
