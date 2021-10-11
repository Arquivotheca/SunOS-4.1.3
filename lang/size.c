#ifndef lint
static	char sccsid[] = "@(#)size.c 1.1 92/07/30 SMI"; /* from UCB 4.4 4/22/85 */
#endif

/*
 * size
 */

#include	<stdio.h>
#include 	<a.out.h>

int	header;

main(argc, argv)
char **argv;
{
	struct exec buf;
	long sum;
	int gorp,i;
	int err = 0;
	FILE *f;

	if (argc==1) {
		*argv = "a.out";
		argc++;
		--argv;
	}
	gorp = argc;
	while(--argc) {
		++argv;
		if ((f = fopen(*argv, "r"))==NULL) {
			printf("size: %s not found\n", *argv);
			err++;
			continue;
		}
		if (fread((char *)&buf, sizeof(buf), 1, f) != 1 ||
		    N_BADMAG(buf)) {
			printf("size: %s not an object file\n", *argv);
			fclose(f);
			err++;
			continue;
		}
		if (header == 0) {
			printf("text\tdata\tbss\tdec\thex\n");
			header = 1;
		}
		printf("%u\t%u\t%u\t", buf.a_text,buf.a_data,buf.a_bss);
		sum = (long) buf.a_text + (long) buf.a_data + (long) buf.a_bss;
		printf("%ld\t%lx", sum, sum);
		if (gorp>2)
			printf("\t%s", *argv);
		printf("\n");
		fclose(f);
	}
	exit(err);
	/* NOTREACHED */
}
