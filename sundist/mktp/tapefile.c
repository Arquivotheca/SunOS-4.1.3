#ifndef lint
static  char sccsid[] = "@(#)tapefile.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * file - determine type of file for making tapes
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <a.out.h>
#include <sys/core.h>

int	errno;
int in;
int i  = 0;
char buf[BUFSIZ];
main(argc, argv)
char **argv;
{
	FILE *fl;
	register char *p;
	char ap[128];
	int i;

	if (argc > 2)		/* only do one file */
		exit(-1);
	exit (type(argv[1]));
	/* NOTREACHED */
}

type(file)
char *file;
{
	int code;		/* return code */
	struct stat mbuf;
	struct exec ex;
	int executable;
	int ifile;

	ifile = -1;
/* following was lstat. changed to stat by mjacob Sun Sep  6 1987 */
	if (stat(file, &mbuf) < 0) {
		return(-1);
	}
	switch (mbuf.st_mode & S_IFMT) {

	case S_IFREG:
		break;	/* Regular file */

	case S_IFSOCK:
	case S_IFLNK:
	case S_IFDIR:
	case S_IFCHR:
	case S_IFBLK:
	default:
spcl:
		return(-1);

	}

	ifile = open(file, 0);
	if(ifile < 0) 
		return(-1);
	in = read(ifile, buf, BUFSIZ);
	if(in == 0)
		return(-1);

	ex = *((struct exec*)buf);
	if(!N_BADMAG(ex)) {
#ifdef sun
		switch(ex.a_machtype) {
		case M_OLDSUN2:
			code = M_OLDSUN2 << 4;
			break;
		case M_68010:
			code = M_68010 << 4;
			break;
		case M_68020:
			code = M_68020 << 4;
			break;
		case M_SPARC:
			code = M_SPARC << 4;
			break;
		default:
			return (-1);
		}
#else
		return(0);
#endif sun
		switch(ex.a_magic) {
		case ZMAGIC:
			code = code | 2;
			break;

		case NMAGIC:
			code = code | 1;
			break;
		case OMAGIC:
			code = code | 0;
			break;
		default:
			return (-1);
		} /*switch*/
		return (code);
	} /* !N_BADMAG(ex) */
	return (-1);
}

