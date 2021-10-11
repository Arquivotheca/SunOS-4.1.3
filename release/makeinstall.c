
#ifndef lint
static	char sccsid[] = "@(#)makeinstall.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

main(a,v)
char **v;
{
	char *nvecs[200];
	register int uid, i;

	uid = geteuid();
	setuid(uid);
	
	nvecs[0] = "make";
	nvecs[1] = "-e";
	nvecs[2] = "install";
	--a;
	i = 3;
	while (a-- > 0)
		nvecs[i++] = *++v;
	nvecs[i] = 0;
	execvp("make",nvecs);
	printf("exec of make fails\n");
	exit(1);
}
