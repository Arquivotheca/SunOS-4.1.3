/*
 *
 * getcons - print the console type
 *
 * made in several flavours
 *	OPENBOOT: use ioctl call to obtain info
 *
 *	!OPENBOOT: depending on machine, "know" the location of romp
 *
 */
#ident  "@(#)getcons.c 1.1 92/07/30 SMI";

#if	defined(sun4c) || defined(sun4m)
#define OPENPROMS
#endif

#include <sys/types.h>
#include <sys/file.h>
#include <mon/sunromvec.h>
#include <stdio.h>

#ifdef OPENPROMS
#include <sundev/openpromio.h>
#include <fcntl.h>
#endif OPENPROMS

#ifndef	OPENPROMS
extern	off_t	lseek();
void	grabit();
#endif	!OPENPROMS
int	fd;			/* fd of /dev/kmem or /dev/openprom */

#ifndef	OPENPROMS
unsigned char *insourcep;
unsigned char *outsinkp;
#else
#define	ARRAY_SIZE	((sizeof(char))*2)
#define	BUFSIZE		(ARRAY_SIZE + sizeof(u_int))
typedef union {
	char buf[BUFSIZE];
	struct openpromio opp;
} Oppbuf;
#endif	!OPENPROMS
unsigned char insource;
unsigned char outsink;

main(argc, argv)
int	argc;
char	*argv[];
{
#ifdef	OPENPROMS
	Oppbuf	oppbuf;
	register struct openpromio *opp = &(oppbuf.opp);
#else	
	struct sunromvec *rp;
#endif	OPENPROMS

#ifdef OPENPROMS
	if ((fd = open("/dev/openprom", O_RDONLY)) < 0) {
		perror("can't open /dev/openprom");
		exit(-1);
	}
	/* zero out the structure */
	bzero(oppbuf.buf, BUFSIZE);
	/*
	 * Now use ioctl to get the info that used to be available
         * in the v_insource/outsink fields of the romp.  Newer 
         * OBP doesnt have them.  We will get back an array of
         * of two chars representing v_insource and v_outsink
         * respectively.  The promlib handles the case where we
         * do have an OBP with these fields available.
	 */
	opp->oprom_size = ARRAY_SIZE;

	if (ioctl(fd, OPROMGETCONS, opp) < 0) {
		perror("ioctl to /dev/openprom failed");
		exit(-1);
	}
	insource = opp->oprom_array[0];
	outsink = opp->oprom_array[1];
#else
	if ((fd = open("/dev/kmem", 0)) < 0) {
		perror("can't open /dev/kmem");
		exit(-1);
	}
	/* 
	 * grab the romp value
	 * known value defined in include sunromvec.h
	 */
	rp = romp;

	/* grab v_insource pointer from sunromvec */
	grabit((off_t)(&(rp->v_insource)), sizeof(unsigned char *), &insourcep,
	    "insourcep");

	/* grab v_insource itself */
	grabit((off_t)(insourcep), sizeof(unsigned char), &insource,
	    "insource");

	/* grab v_outsink pointer from sunromvec */
	grabit((off_t)(&(rp->v_outsink)), sizeof(unsigned char *), &outsinkp,
	    "outsink");

	/* grab v_outsink itself */
	grabit((off_t)(outsinkp), sizeof(unsigned char), &outsink, "outsink");
#endif	OPENPROMS

	printf("in %d out %d\n", insource, outsink);

	exit(outsink);
}

#ifndef	OPENPROMS
void
grabit(offset, size, to, what)
off_t	offset;
int	size;
caddr_t	to;
char	*what;
{
	char	buf[80];

#ifdef NEVER
/*DDD*/ printf("seeking to 0x%x\n", offset);
#endif NEVER
	if (lseek(fd, (off_t)offset, L_SET) != (off_t)offset) {
		sprintf(buf, "can't seek to %s", what);
		perror(buf);
		exit(1);
	}

	if (read(fd, to, size) != size) {
		sprintf(buf, "can't read %s", what);
		perror(buf);
		exit(1);
	}
}
#endif	!OPENPROMS
