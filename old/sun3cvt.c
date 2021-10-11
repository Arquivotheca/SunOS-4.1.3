
#ifndef lint
static char sccsid[] = "@(#)sun3cvt.c	1.1 (Sun) 8/5/92";
#endif lint

/*
 * conversion tool for sun-3 executable files
 *
 *	This program converts a sun-2 executable file into a sun-3
 *	executable.  The program will attempt to create a file of the
 *	same type (magic number).  However, for "small" pure-text files
 *	(magic number 0410 or 0413, less than 96k of text) the new
 *	file must be impure text (0407) since Sun-3 data segments must
 *	begin on 128kb boundaries.
 */

#include <stdio.h>
#include <sys/file.h>
#include <a.out.h>

#define round(x, y)   ((((x)+((y)-1))/(y))*(y))
#define BSIZE 8192

void	error();
long	lseek();
char	*program= "sun3cvt";
struct exec oldheader,newheader;
int	oldfile, newfile;
char	*oldfilename, *newfilename;

char	buffer[BSIZE];

struct jump {			/* to get to the old starting address */
	short op;
	long  target;
} jump = { 0x4ef9, 0 };		/* jmp xxx.l  (absolute) */


/*VARARGS1*/
void
error(s,arg1,arg2,arg3,arg4)
	char *s;
{
	(void)fprintf(stderr,"%s: ", program);
	(void)fprintf(stderr, s, arg1, arg2, arg3, arg4);
	exit(1);
}

int
readblock(buffer, size)
	char *buffer;
	int size;
{
	int nbytes;

	nbytes = read(oldfile, buffer, size);
	if (nbytes == -1) {
		error("cannot read from %s\n", oldfilename);
		/* NOTREACHED */
	}
	return(nbytes);
}

void
writeblock(buffer, size)
	char *buffer;
	int size;
{
	if (write(newfile, buffer, size) == -1) {
		error("cannot write to %s\n", newfilename);
		/* NOTREACHED */
	}
}

void
fillblock(gap)
	long gap;
{
	bzero(buffer, sizeof(buffer));
	while (gap >= sizeof(buffer)) {
		writeblock(buffer, sizeof(buffer));
		gap -= sizeof(buffer);
	}
	writeblock(buffer, (int)gap);
}

void
copyblock(size)
	long size;
{
	int nbytes;

	if (size == -1) {
		/* copy to EOF */
		do {
			nbytes = readblock(buffer, sizeof(buffer)); 
			writeblock(buffer, nbytes);
		} while (nbytes != 0);
	} else {
		while (size >= sizeof(buffer)) {
			nbytes = readblock(buffer, sizeof(buffer)); 
			writeblock(buffer, nbytes);
			size -= nbytes;
		}
		nbytes = readblock(buffer, (int)size); 
		writeblock(buffer, nbytes);
	}
}

/*
 * unsharedtext()
 *	handles OMAGIC(0407) files, in which data begins at end-of-text.
 *	All we have to do is pad out the header so that text begins where
 *	it's supposed to.
 */
void
unsharedtext()
{
	long gap;

	newheader = oldheader;
	newheader.a_machtype = M_68010;
	newheader.a_text = N_DATADDR(oldheader) - N_TXTADDR(newheader);
	newheader.a_entry = N_TXTADDR(newheader);
	writeblock((char*)&newheader, sizeof(newheader));
	jump.target = N_TXTADDR(oldheader);
	gap = (long)(N_TXTADDR(oldheader)-N_TXTADDR(newheader) - sizeof(jump));
	writeblock((char*)&jump, sizeof(jump));
	fillblock(gap);
	copyblock((long)-1);
}

/*
 * sharedtext()
 *	For NMAGIC(0410) and ZMAGIC(0413) files; these require data
 *	to be aligned at segment boundaries. Since these are larger
 *	in Sun-3 object files, we must check that the data segment
 *	begins above the first nonzero segment boundary.  If it
 *	doesn't, the output file cannot have shared text.
 */
void
sharedtext()
{
	unsigned end_text, end_data, end_shared_text;
	long gap;
	long offset;

	/*
	 * initialize the new exec structure
	 */
	newheader = oldheader;
	newheader.a_machtype = M_68010;

	/*
	 * compute boundaries of text and data from the old header
	 */
	end_text = N_TXTADDR(oldheader) + oldheader.a_text;
	end_data = N_BSSADDR(oldheader);

	/*
	 * check whether the text segment is big enough to be
	 * shared.
	 */
	end_shared_text = end_text - (end_text % SEGSIZ);
	if (end_shared_text == 0) {
		/*
		 * not enough text to make a shared segment
		 */
		newheader.a_magic = OMAGIC;
		/*
		 * compute the size of the text segment
		 */
		newheader.a_text = N_DATADDR(oldheader) - N_TXTADDR(newheader);
	} else {
		/*
		 * Sharable text segments must end at SEGSIZ boundaries.
		 * The remainder of the text goes into data space (bletch).
		 */
		newheader.a_text = end_shared_text - N_TXTADDR(newheader);
		newheader.a_data = end_data - end_shared_text;
		/*
		 * For demand paged (0413) files, round the data segment
		 * up to a multiple of PAGSIZ. The text segment is already
		 * taken care of, since OLD_SEGSIZ is a multiple of PAGSIZ.
		 */
		if (newheader.a_magic == ZMAGIC) {
			newheader.a_data = round(newheader.a_data,PAGSIZ);
		}
	}
	/*
	 * copy header
	 */
	newheader.a_entry = N_TXTADDR(newheader);
	if (newheader.a_magic == ZMAGIC) {
		newheader.a_entry += sizeof(newheader);
	}
	writeblock((char*)&newheader, sizeof(newheader));
	gap = N_TXTADDR(oldheader) - N_TXTADDR(newheader);
	if (newheader.a_magic == ZMAGIC) {
		/*
		 * for 0413 files, the exec structure is in text space.
		 * so take it out of the fudge factor.
		 */
		gap -= sizeof(struct exec);
	}
	jump.target = N_TXTADDR(oldheader);
	gap -= sizeof(jump);
	writeblock((char*)&jump, sizeof(jump));
	fillblock(gap);
	/*
	 * copy text
	 */
	offset = N_TXTOFF(oldheader);
	if (lseek(oldfile, offset, L_SET) != offset) {
		error("cannot seek to offset %d in file %s\n",
			offset, oldfilename);
		/* NOTREACHED */
	}
	copyblock((long)oldheader.a_text);
	fillblock((long)(N_DATADDR(oldheader) - end_text));
	/*
	 * copy data
	 */
	offset = N_TXTOFF(oldheader)+oldheader.a_text;
	if (lseek(oldfile, offset, L_SET) != offset) {
		error("cannot seek to offset %d in file %s\n",
			offset, oldfilename);
		/* NOTREACHED */
	}
	copyblock((long)oldheader.a_data);
	fillblock((long)(N_BSSADDR(newheader) - end_data));
	/*
	 * copy the rest of the file
	 */
	copyblock((long)-1);
}

main(argc,argv)
	char *argv[];
{
	switch(argc) {
	case 1:
		oldfilename = "a.out";
		newfilename = "sun3.out";
		break;
	case 2:
		oldfilename = argv[1];
		newfilename = "sun3.out";
		break;
	case 3:
		oldfilename = argv[1];
		newfilename = argv[2];
		break;
	default:
		(void)fprintf(stderr,"usage: sun3cvt [oldfile [newfile]]\n");
		exit(1);
	}
	oldfile = open(oldfilename, O_RDONLY, 0666);
	if (oldfile < 0) {
		error("cannot open %s\n", oldfilename);
		/*NOTREACHED*/
	}
	newfile = open(newfilename, O_TRUNC|O_CREAT|O_WRONLY, 0666);
	if (newfile < 0) {
		error("cannot open %s\n", newfilename);
		/*NOTREACHED*/
	}
	if (read(oldfile, (char*)&oldheader, sizeof(oldheader)) == -1) {
		error("cannot read %s\n", oldfilename);
		/*NOTREACHED*/
	}
	switch(oldheader.a_machtype) {
	case M_OLDSUN2:
		break;
	case M_68010:
	case M_68020:
		error("%s is already a sun-3 program file\n", oldfilename);
		/*NOTREACHED*/
	default:
		error("file %s has unknown machine type\n", oldfilename);
		/*NOTREACHED*/
	}
	if (oldheader.a_trsize || oldheader.a_drsize) {
		error("%s is relocatable -- no need to convert\n",
			oldfilename);
		/*NOTREACHED*/
	}
	switch(oldheader.a_magic) {
	case OMAGIC:
		unsharedtext();
		break;
	case NMAGIC:
	case ZMAGIC:
		sharedtext();
		break;
	default:
		error("%s is not a program file\n", oldfilename);
		/*NOTREACHED*/
	}
	(void)chmod(newfilename,0775);
	return(0);
}
