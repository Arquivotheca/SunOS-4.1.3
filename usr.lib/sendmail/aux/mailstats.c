# include <sys/types.h>
# include <sys/stat.h>
# include <stdio.h>
# include "conf.h"
# include "mailstats.h"

#ifndef lint
static char sccsid[] = "@(#)mailstats.c 1.1 92/07/30 SMI"; /* From UCB 4.1 7/25/83 */
#endif

/*
**  MAILSTATS -- print mail statistics.
**
**	Arguments: 
**		file		Name of statistics file.
**
**	Exit Status:
**		zero.
*/

main(argc, argv)
	char  **argv;
{
	register int fd;
	struct statistics st;
	char *sfile = "/etc/sendmail.st";
	register int i;
	struct stat sbuf;
	extern char *ctime();

	if (argc > 1) sfile = argv[1];

	fd = open(sfile, 0);
	if (fd < 0)
	{
		perror(sfile);
		exit(1);
	}
	fstat(fd, &sbuf);
	if (read(fd, &st, sizeof st) != sizeof st ||
	    st.stat_size != sizeof st)
	{
		(void) sprintf(stderr, "File size change\n");
		exit(1);
	}

	printf("Mail statistics from %24.24s", ctime(&st.stat_itime));
	printf(" to %s\n", ctime(&sbuf.st_mtime));
	printf("  Mailer   msgs from  bytes from    msgs to    bytes to\n");
	for (i = 0; i < MAXMAILERS; i++)
	{
		if (st.stat_nf[i] == 0 && st.stat_nt[i] == 0)
			continue;
		printf("%2d %-10s", i, st.stat_names[i]);
		if (st.stat_nf[i])
		  printf("%6ld %10ldK ", st.stat_nf[i], st.stat_bf[i]);
		else
		  printf("                   ");
		if (st.stat_nt[i])
		  printf("%10ld %10ldK\n", st.stat_nt[i], st.stat_bt[i]);
		else
		  printf("\n");
	}
	exit(0);
}
