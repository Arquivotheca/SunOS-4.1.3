#ifndef lint
static	char sccsid[] = "@(#)chsh.c 1.1 92/07/30 SMI"; /* from UCB 4.7 07/10/83 */
#endif

/*
 * chsh
 */
#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>

char	PASSWD[] = "/etc/passwd";
char	temp[]	 = "/etc/ptmp";
struct	passwd *pwd;
struct	passwd *getpwent();
int	endpwent();
char	*strcat();
char	buf[BUFSIZ];

main(argc, argv)
register char *argv[];
{
	register int u, fd;
	register FILE *tf;

	if (argc < 2 || argc > 3) {
		printf("Usage: chsh user [ /bin/csh ]\n");
		exit(1);
	}
	if (argc == 2)
		argv[2] = "";
	else {
		if (argv[2][0] != '/')
			argv[2] = strcat(
			    "/bin/\0 12345678901234567890123456789", argv[2]);
		if (strcmp(argv[2], "/bin/csh") &&
		    strcmp(argv[2], "/bin/oldcsh") &&
		    strcmp(argv[2], "/bin/newcsh") &&
		    strcmp(argv[2], "/usr/new/csh") &&
			/* and, for cretins who can't read the manual */
		    strcmp(argv[2], "/bin/sh") &&
		    getuid()) {
			printf(
			    "Only /bin/csh may be specified\n"
			);
			exit(1);
		}
		if (access(argv[2], 1) < 0) {
			printf("%s is not available\n", argv[2]);
			exit(1);
		}
	}
	unlimit(RLIMIT_CPU);
	unlimit(RLIMIT_FSIZE);
	while ((pwd=getpwent()) != NULL) {
		if (strcmp(pwd->pw_name, argv[1]) == 0) {
			u = getuid();
			if (u!=0 && u != pwd->pw_uid) {
				printf("Permission denied.\n");
				exit(1);
			}
			break;
		}
	}
	endpwent();

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	(void) umask(0);
	if ((fd = open(temp, O_CREAT|O_EXCL|O_RDWR, 0644)) < 0) {
		printf("Temporary file busy -- try again\n");
		exit(1);
	}
	if ((tf=fdopen(fd, "w")) == NULL) {
		printf("Absurd fdopen failure - seek help\n");
		goto out;
	}
	/*
	 * Copy passwd to temp, replacing matching lines
	 * with new shell.
	 */
	while ((pwd=getpwent()) != NULL) {
		if (strcmp(pwd->pw_name, argv[1]) == 0) {
			u = getuid();
			if (u != 0 && u != pwd->pw_uid) {
				printf("Permission denied.\n");
				goto out;
			}
			pwd->pw_shell = argv[2];
		}
		if (strcmp(pwd->pw_shell, "/bin/sh") == 0)
			pwd->pw_shell = "";
		fprintf(tf, "%s:%s:%d:%d:%s:%s:%s\n"
			, pwd->pw_name
			, pwd->pw_passwd
			, pwd->pw_uid
			, pwd->pw_gid
			, pwd->pw_gecos
			, pwd->pw_dir
			, pwd->pw_shell
		);
	}
	endpwent();
	if (rename(temp, PASSWD) < 0) {
		fprintf(stderr, "chsh: "); perror("rename");
  out:
		unlink(temp);
		exit(1);
	}
	fclose(tf);
	exit(0);
}

unlimit(lim)
{
	struct rlimit rlim;

	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	setrlimit(lim, &rlim);
}

static char EMPTY[] = "";
static FILE *pwf = NULL;
static char line[BUFSIZ+1];
static struct passwd passwd;

setpwent()
{
	if( pwf == NULL )
		pwf = fopen( PASSWD, "r" );
	else
		rewind( pwf );
}

endpwent()
{
	if( pwf != NULL ){
		fclose( pwf );
		pwf = NULL;
	}
}

static char *
pwskip(p)
register char *p;
{
	while( *p && *p != ':' && *p != '\n' )
		++p;
	if( *p ) *p++ = 0;
	return(p);
}

struct passwd *
getpwent()
{
	register char *p;

	if (pwf == NULL) {
		if( (pwf = fopen( PASSWD, "r" )) == NULL )
			return(0);
	}
	p = fgets(line, BUFSIZ, pwf);
	if (p==NULL)
		return(0);
	passwd.pw_name = p;
	p = pwskip(p);
	passwd.pw_passwd = p;
	p = pwskip(p);
	passwd.pw_uid = atoi(p);
	p = pwskip(p);
	passwd.pw_gid = atoi(p);
	passwd.pw_quota = 0;
	passwd.pw_comment = EMPTY;
	p = pwskip(p);
	passwd.pw_gecos = p;
	p = pwskip(p);
	passwd.pw_dir = p;
	p = pwskip(p);
	passwd.pw_shell = p;
	while(*p && *p != '\n') p++;
	*p = '\0';
	return(&passwd);
}
