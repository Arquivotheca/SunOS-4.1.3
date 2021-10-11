#ifndef lint
static	char sccsid[] = "@(#)prmail.c 1.1 92/08/05 SMI"; /* from UCB 4.2 02/09/83 */
#endif

#include <pwd.h>
/*
 * prmail
 */
struct	passwd *getpwuid();
char	*getenv();

main(argc, argv)
	int argc;
	char **argv;
{
	register struct passwd *pp;

	--argc, ++argv;
	if (chdir("/usr/spool/mail") < 0) {
		perror("/usr/spool/mail");
		exit(1);
	}
	if (argc == 0) {
		char *user = getenv("USER");
		if (user == 0) {
			pp = getpwuid(getuid());
			if (pp == 0) {
				printf("Who are you?\n");
				exit(1);
			}
			user = pp->pw_name;
		}
		prmail(user, 0);
	} else
		while (--argc >= 0)
			prmail(*argv++, 1);
	exit(0);
	/* NOTREACHED */
}

#include <sys/types.h>
#include <sys/stat.h>

prmail(user, other)
	char *user;
{
	struct stat stb;
	char cmdbuf[256];

	if (stat(user, &stb) < 0) {
		printf("No mail for %s.\n", user);
		return;
	}
	if (access(user, 4) < 0) {
		printf("Mailbox for %s unreadable\n", user);
		return;
	}
	if (other)
		printf(">>> %s <<<\n", user);
	sprintf(cmdbuf, "more %s", user);
	system(cmdbuf);
	if (other)
		printf("-----\n\n");
}
