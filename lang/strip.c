#ifndef lint
static	char sccsid[] = "@(#)strip.c 1.1 92/07/30 SMI"; /* from UCB 4.5 83/07/06 */
#endif
/*
 * strip
 */
#include <a.out.h>
#include <signal.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

struct	exec head;
int	pagesize;

main(argc, argv)
	char *argv[];
{
	register i;
	int rc = 0;

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	for (i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-x")==0 || strcmp(argv[i],"-r")==0) {
			printf("strip: %s is not currently supported\n", argv[i]);
			rc = 1;
			break;
		}
		rc |= strip(argv[i]);
	}
	exit(rc);
	/* NOTREACHED */
}

int
strip(name)
	char *name;
{
	register f;
	long size;
	int status = 0;
	struct stat stb;
	int extra = 0;
	struct extra_sections xtra;
	unsigned str_size, xtra_size;
	char *p;
	int magic;

	f = open(name, O_RDWR);
	if (f < 0) {
		fprintf(stderr, "strip: "); perror(name);
		status = 1;
		goto out;
	}
	if (read(f, (char *)&head, sizeof (head)) < 0 || N_BADMAG(head)) {
		printf("strip: %s not in a.out format\n", name);
		status = 1;
		goto out;
	}
	if ((head.a_syms == 0) && (head.a_trsize == 0) && (head.a_drsize ==0)) {
		printf("strip: %s already stripped\n", name);
		goto out;
	}
	/* check if the extra section exists */
	fstat(f, &stb);
	lseek(f, N_STROFF(head), L_SET);
	read(f, &str_size, sizeof(str_size));
	/* check if EXTRA_MAGIC is right after the string table */
	if( stb.st_size > (N_STROFF(head)+str_size)){
		lseek(f, N_STROFF(head)+str_size, L_SET);
		read(f, &magic, sizeof(magic));
		extra = (magic == EXTRA_MAGIC);
	}
	if (extra){
		lseek(f, N_STROFF(head)+str_size, L_SET);
		read(f, &xtra, sizeof(xtra));
		read(f, &xtra_size, sizeof(xtra_size));
		p = (char*)malloc(xtra_size);
		read(f, p, xtra_size);
	}
	head.a_syms = head.a_trsize = head.a_drsize = 0;
	size = N_SYMOFF(head);
	if (ftruncate(f, size) < 0) {
		fprintf(stderr, "strip: "); perror(name);
		status = 1;
		goto out;
	}
	(void) lseek(f, (long)0, L_SET);
	(void) write(f, (char *)&head, sizeof (head));

	/* put out the extra section after the truncation */
	if (extra){
		lseek(f, size, L_SET);
		write(f, &xtra, sizeof(xtra));
		write(f, &xtra_size, sizeof(xtra_size));
		write(f, p, xtra_size);
	}
out:
	close(f);
	return status;
}
