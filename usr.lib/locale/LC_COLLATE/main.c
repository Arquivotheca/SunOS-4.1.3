/*
 * main routine colldef 
 */

/* static  char *sccsid = "@(#)main.c 1.1 92/07/30 SMI"; */

#include "colldef.h"
#include "y.tab.h"

int lineno = 1;		/* keeping lineno */
int numerrs = 0;	/* how many errors ? */
int numwarn = 0;	/* how many warning ? */

struct colldef colldef;
struct _1_to_m _1_to_m[NUM_1_TO_MANY];
struct _2_to_1 _2_to_1[NUM_2_TO_ONE];
struct charmap *charmap_head = (struct charmap *)NULL;

int no_of_1_to_m = 0;	/* number of 1 to many table */
int no_of_2_to_1 = 0;	/* number of 1 to many table */
int prime_val = DONT_CARE_P+1;	/* primary value */
int second_val = DONT_CARE_S+1;	/* secondary value */

FILE *ifp =  stdin;	/* source file */

/*
 * main routine
 */
main(argc, argv)
short argc;
char **argv;
{
	int init();
	int ofd;	/* output file */

	/*
	 * Preparations
	 */
	if (argc == 1) {
		fprintf(stderr, "usage: %s file\n", PROG);
		exit(1);
	}
	if ((ofd = init(argv[1])) == -1) {
		fprintf(stderr, "%s: can't open %s\n", PROG, argv[1]);
		exit(2);
	}
	yyparse();
	if (getc(ifp) != EOF)
		printf("Warning: any statement after the first order statement is ignored\n");
	fclose (ifp);

	/*
	 * output files
	 */
#ifdef DEBUG
	dump_1_to_m();
	dump_2_to_1();
	dump_charmap();

	dump_table(1);
	dump_table(2);
#endif
	if (numwarn == 0 && numerrs == 0) {
		save_table(ofd);
		printf("script recognized\n");
		}
	else	
		unlink (argv[1]);
	close(ofd);
	exit(0);
}

yyerror(s)
char *s;
{
	++numerrs;
	warning(s, (char *)0);
}

warning(s, t)
char *s;
char *t;
{
	++numwarn;
	fprintf(stderr, "%s: %s", PROG, s);
	if (t)
		fprintf(stderr, " %s", t);
	fprintf(stderr, " near line %d\n", lineno);
	exit(1);
}

/*
 * initialization
 */

init(s)
	char *s;
{
	int c;
	int tfp;

	tfp = creat(s, 0644);
	if (tfp == -1)
		return(-1);


	/*
	 * initialize tables
	 */
	for (c = 0; c < TABSIZE; c++) {
		colldef.primary_sort[c] = DONT_CARE_P;
		colldef.secondary_sort[c] = DONT_CARE_S;
	}
	return(tfp);
}

/*
 * dump to the table
 */

save_table(fd)
	int fd;
{
	int i;
	char *p;
	char dummy = 0;
	static struct _2_to_1 endmark = {0, 0, 0, 0};
	struct pat {
		unsigned char major_;
		unsigned char second;
	} pat;

	/*
	 * write colldef
	 */
	if (write (fd, (char *)&colldef, sizeof colldef) != sizeof colldef) {
		fprintf(stderr, "Could not write data to output file.\n");
		exit(3);
	}

	/*
	 * write 1_to_m
	 */
	for (i = 0; i < no_of_1_to_m; i++) {
		write (fd, &_1_to_m[i].one, 1);
		p = _1_to_m[i].many;
		while (*p) {
			pat.major_ = colldef.primary_sort[(unsigned char)*p];
			pat.second = colldef.secondary_sort[(unsigned char)*p];
			write (fd, (char *)&pat, sizeof pat);
			++p;
		}
		pat.major_ = pat.second = 0;
		write (fd, (char *)&pat, sizeof pat);
	}

	write (fd, &dummy, 1);
	pat.major_ = pat.second = 0;
	write (fd, (char *)&pat, sizeof pat);

	/*
	 * write 2_to_1
	 */
	for (i = 0; i < no_of_2_to_1; i++)
		write (fd, (char *)&_2_to_1[i], sizeof (struct _2_to_1));
	write (fd, (char *)&endmark, sizeof (struct _2_to_1));
}
