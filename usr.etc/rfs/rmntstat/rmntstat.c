/*	@(#)rmntstat.c 1.1 92/07/30 SMI					*/

/*	Copyright (c) 1987 Sun Microsystems			*/
/*	ported from System V.3.1				*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rmntstat:rmntstat.c	1.4"
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <rfs/rfs_misc.h>
#include <rfs/adv.h>
#include <rfs/nserve.h>
#include <sys/stat.h>
#include <rfs/rfs_mnt.h>
#include <rfs/fumount.h>

extern	int	nadvertise;
extern	struct	advertise	*Advtab;
extern	char	*malloc();
extern	char	*strtok();

struct	clnts	*client;
static	int	 header;

static	char	*adv;
static	int	 adv_err;

main(argc, argv)
	int    argc;
	char **argv;
{
	int advx;
	int num_args;

	num_args = argc;

	if (argc != 1 && strcmp(argv[1], "-h") == 0) {
		header = 0;
		num_args --;
	} else {
		header = 1;
	}

	if (num_args > 2) {
		fprintf(stderr, "%s: usage: %s [-h] [resource]\n", argv[0], argv[0]);
		exit(1);
	}

	if (getuid() != 0) {
		fprintf(stderr, "%s: must be super-user\n", argv[0]);
		exit(1);
	}

	if (nlload() != 0)
		exit(1);

	if (num_args == 2) {
		if (getnodes(argv[argc-1], 0) == 1) {
			fprintf(stderr, "%s: %s not known\n", argv[0], argv[argc-1]);
			exit(1);
		}
		pr_info(argv[argc-1]);
		exit(0);
	}

	for (advx = 0; advx < nadvertise; advx ++) {
		if (Advtab[advx].a_flags & A_INUSE) {
			getnodes(Advtab[advx].a_name, 0);
			pr_info(Advtab[advx].a_name);
		}
	}
	exit(0);
	/* NOTREACHED */
}

pr_info(res)
char	*res;
{
	static int already_read = 0;
	int i = 1;
	char	*pathname();

	if (header) {
		printf("RESOURCE       PATH                             HOSTNAMES\n");
		header = 0;
	}

	if (!already_read)
		read_advtab();

	already_read = 1;

	printf("%-14s", res);
	printf(" %-32s", pathname(res));

	if (client[0].flags != EMPTY)
		printf(" %s", client[0].node);

	while (client[i].flags != EMPTY) {
		printf(",%s", client[i].node);
		i ++;
	}
	printf("\n");
}

read_advtab()
{
	int fd;
	struct stat sbuf;

	adv_err =0;

	if ((stat("/etc/advtab", &sbuf) == -1)
	|| ((adv = malloc(sbuf.st_size + 1)) == NULL)
	|| ((fd = open("/etc/advtab", O_RDONLY)) == -1)
	|| (read(fd, adv, sbuf.st_size) != sbuf.st_size)) {
		printf("rmntstat: warning: cannot get information from /etc/advtab; pathnames will not be printed\n");
		adv_err = 1;
	}

	if (!adv_err)
		*(adv + sbuf.st_size) = '\0';
}

static char *
pathname(res)
char	*res;
{
	char *name, *path;
	char *gettok();

	if (adv_err)
		return ("unknown");

	if ((name = gettok(adv, " ")) == NULL)
		return ("unknown");

	while (strcmp(name, res) != 0) {
		if (gettok(NULL, "\n") == NULL
		|| (name = gettok(NULL, " ")) == NULL)
			return ("unknown");
	}

	if ((path = gettok(NULL, " ")) == NULL)
		path = "unknown";

	return (path);
}

static
char *
gettok(string, sep)
char  *string;
char  *sep;
{
	register char	*ptr;
	static	 char	*savept;
	static	 char	buf[512];
	char	 t;

	char	*strpbrk();

	ptr = (string == NULL)? savept: string;

	ptr = ptr + strspn(ptr, sep);

	if (*ptr == '\0')
		return (NULL);

	if ((savept = strpbrk(ptr, sep)) == NULL)
		savept = ptr + strlen(ptr);

	t = *savept;
	*savept = '\0';
	strncpy(buf, ptr, 512);
	*savept = t;
	buf[511] = '\0';
	if (*savept != '\0')
		savept ++;
	return (buf);
}
