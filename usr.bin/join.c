#ifndef lint
static	char sccsid[] = "@(#)join.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

/*	join F1 F2 on stuff */

#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>

#define F1 0
#define F2 1
#define	NFLD	20	/* number fields per line */
#define	NOFLD	(2*NFLD)	/* number arguments to -o */
#define comp() cmp(ppi[F1][j1],ppi[F2][j2])
#define putfield(string) if(*string == NULL) fputs(null, stdout); \
			else fputs(string, stdout)
#define max(a,b)	(a >= b ? a : b)

int CNFLD;		/* current number of fields per line */
int CNOFLD;		/* current number of output fields */
int CBUFSIZE[2];	/* current size of input buffers */

FILE *f[2];
char *buf[2];		/* input lines */
char **ppi[2];		/* pointers to fields in lines */
int	j1	= 1;	/* join of this field of file 1 */
int	j2	= 1;	/* join of this field of file 2 */
int	*olist;		/* output these fields */
int	*olistf;	/* from these files */
int	no;	/* number of entries in olist */
int	sep1	= ' ';	/* default field separator */
int	sep2	= '\t';
char*	null	= "";
int	aflg;

extern char *calloc();

main(argc, argv)
char *argv[];
{
	int i;
	int n1, n2;
	long top2, bot2;
	long ftell();

	init_buf();

	while (argc > 1 && argv[1][0] == '-') {
		if (argv[1][1] == '\0')
			break;
		switch (argv[1][1]) {
		case 'a':
			switch(argv[1][2]) {
			case '1':
				aflg |= 1;
				break;
			case '2':
				aflg |= 2;
				break;
			default:
				aflg |= 3;
			}
			break;
		case 'e':
			null = argv[2];
			argv++;
			argc--;
			break;
		case 't':
			sep1 = sep2 = argv[1][2];
			break;
		case 'o':
			no = 0;
			while (argv[2][0] == '1' || argv[2][0] == '2' &&
			       argv[2][1] == '.') {
				/* Check for overflow of olist */
				if (no > CNOFLD - 1) {
					/* Get larger table for outlists */
					/* (CNOFLD is updated.) */
					get_olist(CNOFLD + NOFLD);
				}
				if (argv[2][0] == '1' && argv[2][1] == '.') {
					olistf[no] = F1;
					olist[no] = atoi(&argv[2][2]);
				} else if (argv[2][0] == '2' && argv[2][1] == '.') {
					olist[no] = atoi(&argv[2][2]);
					olistf[no] = F2;
				} else
					break;

				if (olist[no] < 1)
					error("argument to -o should be greater than zero: %s", &argv[2][2]);
				/* Check for overflow of ppi (CNFLD updated) */
				if (olist[no] > CNFLD)
					get_ppi(olist[no]);
				argc--;
				argv++;
				no++;
			}
			break;
		case 'j':
			if (argv[1][2] == '1')
				j1 = atoi(argv[2]);
			else if (argv[1][2] == '2')
				j2 = atoi(argv[2]);
			else
				j1 = j2 = atoi(argv[2]);

			if (j1 < 1 || j2 < 1)
				error("argument to -j should be greater than zero: %s", argv[2]);
			/* Check for overflow of ppi (CNFLD updated) */
			if (j1 > CNFLD || j2 > CNFLD)
				get_ppi(max(j1, j2));
			argc--;
			argv++;
			break;
		}
		argc--;
		argv++;
	}
	for (i = 0; i < no; i++)
		olist[i]--;	/* 0 origin */
	if (argc != 3)
		error("usage: join [-an] [-e s] [-jn m] [-tc] [-o list] file1 file2");
	j1--;
	j2--;	/* everyone else believes in 0 origin */
	if (argv[1][0] == '-')
		f[F1] = stdin;
	else if ((f[F1] = fopen(argv[1], "r")) == NULL)
		error("can't open %s", argv[1]);
	if ((f[F2] = fopen(argv[2], "r")) == NULL)
		error("can't open %s", argv[2]);

#define get1() n1=input(F1)
#define get2() n2=input(F2)
	get1();
	bot2 = ftell(f[F2]);
	get2();
	while(n1>0 && n2>0 || aflg!=0 && n1+n2>0) {
		if(n1>0 && n2>0 && comp()>0 || n1==0) {
			if(aflg&2) output(0, n2);
			bot2 = ftell(f[F2]);
			get2();
		} else if(n1>0 && n2>0 && comp()<0 || n2==0) {
			if(aflg&1) output(n1, 0);
			get1();
		} else /*(n1>0 && n2>0 && comp()==0)*/ {
			while(n2>0 && comp()==0) {
				output(n1, n2);
				top2 = ftell(f[F2]);
				get2();
			}
			(void)fseek(f[F2], bot2, 0);
			get2();
			get1();
			for(;;) {
				if(n1>0 && n2>0 && comp()==0) {
					output(n1, n2);
					get2();
				} else if(n1>0 && n2>0 && comp()<0 || n2==0) {
					(void)fseek(f[F2], bot2, 0);
					get2();
					get1();
				} else /*(n1>0 && n2>0 && comp()>0 || n1==0)*/{
					(void)fseek(f[F2], top2, 0);
					bot2 = top2;
					get2();
					break;
				}
			}
		}
	}
	return(0);
}

input(n)		/* get input line and split into fields */
{
	register int i, c;
	char *bp;
	char **pp;
	int nread;
	char *tbp;

	bp = buf[n];
	pp = ppi[n];

	if (fgets(bp, CBUFSIZE[n], f[n]) == NULL) {
		return(0);
	/* Check for long lines */
	} else if ((strchr(bp, '\n') == NULL) && 
		    strlen(bp) == CBUFSIZE[n] - 1) {
		tbp = bp;
		nread = CBUFSIZE[n] - 1;
		while (strchr(tbp, '\n') == NULL) {
			/* Increase buffer by BUFSIZ */
			/* (CBUFSIZE[n] is updated.) */
			get_buf(n, CBUFSIZE[n] + BUFSIZ);
			bp = buf[n];
			tbp = buf[n] + nread;

			/* Read rest of line */
			if (fgets(tbp, BUFSIZ + 1, f[n]) == NULL) {
				break;
			}
			nread += strlen(tbp);
		}
	} 

	i = 0;
	do {
		i++;
		/* Check for overflow of ppi[n] */
		if (i > CNFLD - 1) {	/* account for 0 termination */
			/* Reallocate larger buffers (CNFLD is updated.) */
			get_ppi(CNFLD + NFLD);
			/* Restore local pointer */
			pp = &ppi[n][i-1];
		}
		if (sep1 == ' ')	/* strip multiples */
			while ((c = *bp) == sep1 || c == sep2)
				bp++;	/* skip blanks */
		*pp++ = bp;	/* record beginning */
		while ((c = *bp) != sep1 && c != '\n' && c != sep2 && c != '\0')
			bp++;
		*bp++ = '\0';	/* mark end by overwriting blank */
			/* fails badly if string doesn't have \n at end */
	} while (c != '\n' && c != '\0');

	*pp = 0;
#ifdef DEBUG
	fprintf(stderr, "input: n %d i %d\n", n, i);
#endif
	return(i);
}

output(on1, on2)	/* print items from olist */
int on1, on2;
{
	int i;

	if (no <= 0) {	/* default case */
		if (on1)
			putfield(ppi[F1][j1]);
		else
			putfield(ppi[F2][j2]);
		for (i = 0; i < on1; i++)
			if (i != j1) {
				(void)putchar(sep1);
				putfield(ppi[F1][i]);
			}
		for (i = 0; i < on2; i++)
			if (i != j2) {
				(void)putchar(sep1);
				putfield(ppi[F2][i]);
			}
		(void)putchar('\n');
	} else {
		for (i = 0; i < no; i++) {
			if(olistf[i]==F1 && on1<=olist[i] ||
			   olistf[i]==F2 && on2<=olist[i])
				fputs(null, stdout);
			else
				putfield(ppi[olistf[i]][olist[i]]);
			if (i < no - 1)
				(void)printf("%c", sep1);
			else
				(void)putchar('\n');
		}
	}
}

/*VARARGS1*/
error(s1, s2, s3, s4, s5)
char *s1;
{
	(void)fprintf(stderr, "join: ");
	(void)fprintf(stderr, s1, s2, s3, s4, s5);
	(void)fprintf(stderr, "\n");
	exit(1);
}

cmp(s1, s2)
char *s1, *s2;
{
	if (s1 == NULL) {
		if (s2 == NULL)
			return(0);
		else
			return(-1);
	} else if (s2 == NULL)
		return(1);
	return(strcmp(s1, s2));
}

/*
 *  Allocate memory for buffers.
 */
init_buf()
{
	get_buf(F1, BUFSIZ);
	get_buf(F2, BUFSIZ);
	get_ppi(NFLD);
	get_olist(NOFLD);
}

get_ppi(nfld)
int nfld;
{
	int i;

	for (i = 0; i < 2; i++) {
		if (ppi[i]) {
			if ((ppi[i] = (char **) 
			     realloc((char *) ppi[i], (unsigned) (nfld * sizeof(char *)))) 
			     == NULL)
				error("realloc pointer table failed");
		} else {
			if ((ppi[i] = (char **) 
			     calloc((unsigned) nfld, 
				    (unsigned) sizeof(char *))) == NULL)
				error("calloc pointer table failed");
		}
	}

	CNFLD = nfld;
#ifdef DEBUG
	fprintf(stderr, "get_ppi ppi 0x%x nfld %d CNFLD %d\n", 
				ppi, nfld, CNFLD);
#endif
}

get_buf(i, size)
int i;
int size;
{
	if (buf[i]) {
		if ((buf[i] = (char *)
		     realloc(buf[i], (unsigned) (size * sizeof(char)))) 
		     == NULL)
			error("realloc input buffer failed");
	} else {
		if ((buf[i] = (char *)
		     calloc((unsigned) size, (unsigned) sizeof(char))) == NULL)
			error("calloc input buffer failed");
	}

	CBUFSIZE[i] = size;
#ifdef DEBUG
	fprintf(stderr, "get_buf i %d buf 0x%x size %d CBUFSIZE %d\n", 
				i, buf[i], size, CBUFSIZE[i]);
#endif
}

get_olist(onfld)
int onfld;
{
	if (olist) {
		if ((olist = (int *) realloc((char *) olist,
		     (unsigned) (onfld * sizeof(int)))) == NULL)
			error("realloc olist failed");

	} else {
		if ((olist = (int *) 
		     calloc((unsigned) onfld, (unsigned) sizeof(int))) == NULL)
			error("calloc olist failed");
	}

	if (olistf) {
		if ((olistf = (int *) realloc((char *) olistf,
		     (unsigned) (onfld * sizeof(int))))
		     == NULL)
			error("realloc olistf failed");
	} else {
		if ((olistf = (int *) 
		     calloc((unsigned) onfld, (unsigned) sizeof(int))) == NULL)
			error("calloc olistf failed");
	}

	CNOFLD = onfld;
#ifdef DEBUG
	fprintf(stderr, "get_olist olist 0x%x olistf 0x%x nofld %d CNOFLD %d\n",
				olist, olistf, onfld, CNOFLD);
#endif
}

