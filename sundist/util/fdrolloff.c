/*
 *
 * fdrolloff - to rapidly roll of data from a floppy -  to load miniroot
 * a simple stripped down dd(1), that accepts an absolute byte count
 *
 * the disk format used is: for fd0c (all the disk)
 *	1st 4 blocks - Copyright	
 *	next block - simple label
 *	rest of disk - compressed miniroot image
 */

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sun/dkio.h>

/*
 * argv[0] - cmdname
 * argv[1] - label letter
 * argv[2] - device
 */

char	*nullstring = "";
char	buf[ (18*512) ];
char	lc;			/* label character */
int	count;			/* how many is on the diskette */
int	towrite;		/* to read from diskette, write to pipe */

main( argc, argv )
int	argc;
char	**argv;
{
	int	fd;
	int	chgd;
	int	first = 1;

	if ( argc != 3 ) {
		fprintf(stderr, "Usage: fdrolloff vol device\n" );
		exit( 1 );
	}

retry:
	if( !first ) {
		(void)close(fd);
		fprintf(stderr, "please insert volume \"%c\" and then press return\n",
		    *argv[1] );
		(void)fgets( buf, 512, stdin );
	}
	first = 0;
	if( (fd = open( argv[2], O_NDELAY|O_RDONLY)) < 0 ) {
		fprintf(stderr, "fdrolloff: couldn't open %s",  argv[2] );
		perror( nullstring );
		exit( 1 );
	}
	/* see if a diskette is in the drive */
	if( ioctl( fd, FDKGETCHANGE, &chgd ) == -1 ) {
		fprintf(stderr, "fdrolloff: couldn't FDKGETCHANGE %s", argv[2]);
		perror( nullstring );
		goto retry;
	}
	/* sense of signal is low true - so set means no diskette */
	if( chgd & FDKGC_CURRENT ) {
		fprintf(stderr, "fdrolloff: no diskette in drive!\n" );
		goto retry;
	}
	if( lseek(fd, 4*512, 0) != (4*512) ) {
		fprintf(stderr, "fdrolloff: couldn't seek on %s", argv[2] );
		perror( nullstring );
		exit( 1 );
	}
	if( read(fd, buf, 512) != 512 ) {
		fprintf(stderr, "fdrolloff: couldn't read label on %s",argv[2]);
		perror( nullstring );
		(void)system( "eject" );
		goto retry;
	}
	/*
	 * first char ought to be label, 2nd thing is byte count
	 */
	if( sscanf( buf, "%c %d", &lc, &count ) != 2 ) {
		fprintf(stderr, "warning: fdrolloff: bad scanf of label: got \"%c\" and \"%d\"\n", lc, count );
	}

#ifdef DEBUG
/*DDD*/fprintf(stderr, "fdrolloff: found got \"%c\" and \"%d\"\n", lc, count );
#endif DEBUG

	if( lc != *argv[1] ) {
		fprintf(stderr, "wrong volume, expected \"%c\", found \"%c\"\n",
		    *argv[1], lc );
		(void)system( "eject" );
		goto retry;
	}

/* XXX check for preposterous counts? */
#ifdef XXX
	DKIOCGPART() to get partition size
	compare
#endif XXX

	/*
	 * found correct diskette,
	 * read it in and stuff "count" bytes onto stdout
	 */
	towrite = (18*512) - (5*512);	/* first time is short read */
	while ( count ) {
		if( read(fd, buf, towrite) != towrite ) {
			fprintf(stderr, "fdrolloff: read error on %s", argv[2]);
			perror( nullstring );
			exit( 1 );
		}
		if( towrite > count )
			towrite = count;
		/* must have been adios on pipe? */
		if( write( 1, buf, towrite ) != towrite ) {
			if( errno == EPIPE )
				exit( 0 );
			fprintf(stderr, "fdrolloff: write error ");
			perror( nullstring );
			exit( 1 );
		}
		count -= towrite;
		/* progress message - down counter */
		fprintf(stderr,"%d \r", count / (18*512) );
		towrite = (18*512);	/* reload read/write count 2nd+ times */
	}
	fprintf(stderr,"\n");
	exit( 0 );
}
