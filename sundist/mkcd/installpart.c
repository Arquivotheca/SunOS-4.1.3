/*      @(#)installpart.c 1.1 92/07/30 SMI      */
/*
 * installpart cdimage partition bootfile
 *
 */

#ifndef lint
static  char sccsid[] = "@(#)installpart.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif  lint

#include <fcntl.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "hsfs_spec.h"
#include "iso_spec.h"
#include "iso_impl.h"
#include "label.h"

#define PUTSECTOR(buf, secno, nosec) (putdisk(buf, (secno)*ISO_SECTOR_SIZE, \
	(nosec)*ISO_SECTOR_SIZE))
#define GETSECTOR(buf, secno, nosec) (getdisk(buf, (secno)*ISO_SECTOR_SIZE, \
		(nosec)*ISO_SECTOR_SIZE))

int cdfd;
int fd;

char buf[64 * 1024];

main(argc, argv)
	int argc;
	char **argv;
{
	int rsize;
	struct stat stb;
	int i;
	int filesize, blocksize;
	int secno;

	argc--; 
	argv++;
	
	if (argc != 3) usage();

	if ((cdfd = open(argv[0], O_RDWR)) < 0) {	
		fprintf(stderr, "installpart: open: ");
		perror(argv[0]);
		exit(0);
	}

	/* cdimage must be created using -B npart option */
	secno = cklabel(cdfd, argv[1]);

	/* check for the boot file */
	fd = open(argv[2], 0);
	if (fd < 0) {
		fprintf(stderr, "installpart: cannot open ");
		perror(argv[1]);
		exit(1);
	}

	if (fstat(fd, &stb) < 0) {
		fprintf(stderr, "installpart: cannot stat ");
		perror(argv[1]);
		close (fd);
		close (cdfd);
		exit(1);
	}

	filesize = stb.st_size;
	blocksize = stb.st_blksize;

	if (filesize > MAXBOOTSIZE) {
		fprintf(stderr,"installpart: WARNING boot program > %d bytes.\n",MAXBOOTSIZE);
		exit(1);
	}

	/* seek to the appropriate location */
	if (lseek(cdfd, secno*512, L_SET) < 0) {
		perror("installpart: cannot seek cd image file ");
		close (cdfd);
		close (fd);
		exit(1);
	}

	/* do the copy */
	/* assume MAXBOOTSIZE must be divisible by blocksize */
	for (i=0; i < MAXBOOTSIZE/blocksize; i++) {
		if ((rsize = read(fd, buf, blocksize)) < 0) {
			perror("installpart: error in reading boot file");
			close (cdfd);
			close (fd);
			exit(1);
		}
		else if (rsize == 0) break; /* EOF */
		else if (rsize != blocksize) {
			/* zero out the remaining byte */
			bzero(&buf[rsize], blocksize-rsize);
		}
		if ((write (cdfd, buf, blocksize)) != blocksize) {
			perror("installpart: error in writing cd image file");
			close (cdfd);
			close (fd);
		}
	}
	close(cdfd);
}

usage()
{
	fprintf(stderr, "Usage: installpart cdimage partition bootfile\n");
	fprintf(stderr, "where: partition must be between 'b' to 'h'\n");
	exit(1);
}

int 
cklabel(cdfd, partp)
int cdfd;
char *partp;
{
int partition;
struct cd_map map;
int i;

	/* make sure this is an cdrom image file */
	if (lseek(cdfd, ISO_VOLDESC_SEC* ISO_SECTOR_SIZE, L_SET) < 0) {
		perror("installpart: cannot seek cd image file ");
		close (cdfd);
		exit(1);
	}
	/* read standard volume descriptor */
	if (read(cdfd, buf, ISO_SECTOR_SIZE) != ISO_SECTOR_SIZE) {
		perror("installpart: cannot read cd image file");
		close (cdfd);
		exit(1);
	}

	/* quick check for iso volume*/
	for (i = 0; i < ISO_ID_STRLEN; i++)
		if (ISO_STD_ID(buf)[i] != ISO_ID_STRING[i])
			goto notiso;
	if (ISO_STD_VER(buf) != ISO_ID_VER)
		goto notiso;

	if (ISO_DESC_TYPE(buf) != ISO_VD_PVD) 
		goto notiso;

	/* check user input */
	if ((strlen(partp) != 1) || 
	    ((*partp < 'b') || (*partp > 'h'))) {
		close(cdfd);
		usage();
	}
	
	partition = (int) (*partp - 'b');

	if (lseek(cdfd, 0, L_SET) < 0) {
		perror("installpart: cannot seek cd image file ");
		close (cdfd);
		exit(1);
	}
	/* read sector zero */
	if (read(cdfd, buf, 512) != 512) {
		perror("installpart: cannot read cd image file");
		close (cdfd);
		exit(1);
	}

	if (cd_getlabel(&map, buf) != 0) {
		fprintf(stderr, "installpart: cd image file does not contain valid label");

		close (cdfd);
		exit(1);
	}

	if ((partition > map.cdm_npart) ||
		(map.cdm_paddr[partition+1] == 0)) {
		fprintf(stderr, "installpart: cd image file does not contain the valid partition\n");
		exit(1);

	}
	
	return(map.cdm_paddr[partition+1]);

notiso:
	fprintf(stderr, "installpart: cd image file is not an ISO 9660 format file\n");
	close (cdfd);
	
	exit(1);
}
