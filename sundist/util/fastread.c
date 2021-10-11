/*
 * @(#)fastread.c 1.1 92/07/30 Copyright 1989 Sun Micro
 *
 * fastread - used to read the CDROM real fast, does
 * skipping with lseek(2)
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sun/dklabel.h>

#define BLOCK_SIZE (32*1024)

char buf[BLOCK_SIZE];
char display[4] = {"-\|/"};
char *nullstring = "";

main (argc, argv) 
int argc;
char *argv[];
{
	int fd, skip, size_in_bytes, i, trail;
	extern int errno;
	off_t lseek();
	int	partno;
	struct dk_label *lp;

	if (argc != 5) {
		fprintf(stderr,
	"usage: fastread <disk> <part_no> <skipinbytes> <sizeinbytes>\n");
		fprintf(stderr, "exiting\n");
		exit(1);
	}
	
	if ((fd=open(argv[1], O_RDONLY)) == -1) {
		fprintf(stderr,"%s: failed to open %s", argv[0], argv[1]);
		perror(nullstring);
		exit (1);
	}
	partno = atoi(argv[2]);
	skip = atoi(argv[3]);
	size_in_bytes = atoi(argv[4]);
	trail = size_in_bytes % BLOCK_SIZE;

	size_in_bytes = size_in_bytes - trail;

	/*
	 * read 1st block of device to get the label
	 */
	if (read(fd, buf, 512) != 512) {
		fprintf(stderr,"%s: can't read label on %s:", argv[0], argv[1]);
		perror(nullstring);
		exit (1);
	}
	lp = (struct dk_label *)buf;
	skip += (lp->dkl_map[partno].dkl_cylno *
	    (512 * lp->dkl_nsect * lp->dkl_nhead));

	
	if (lseek(fd, (off_t)skip, L_SET) == (off_t)-1) {
		fprintf(stderr," seek failed errno %x exiting \n", errno);
		exit(1);
	}
	
	for (i =0; i< size_in_bytes; i += BLOCK_SIZE) {
		if (read(fd, buf, BLOCK_SIZE) == -1) {
			fprintf(stderr, " read failed  at %d, errno %d\n", i, errno);
		 fprintf(stderr, " exiting\n");
			exit(1);
		}
		if (fwrite(buf, BLOCK_SIZE, 1, stdout) == -1) {
			fprintf(stderr, " write failed at  %d errno %d\n", i, errno);
			fprintf(stderr, " exiting\n");
			exit(1);
		}
		fprintf(stderr,"%c\r", display[(i/BLOCK_SIZE)%4]);
	}
	if (trail) { 		/*trailing bytes*/
		if (read(fd, buf, trail) == -1) {
			fprintf(stderr, " read failed  at %d, errno %d\n", i, errno);
			fprintf(stderr, " exiting\n");
			exit(1);
		}
		if (fwrite(buf, trail, 1, stdout) == -1) {
			fprintf(stderr, " write failed at  %d errno %d\n", i, errno);
			fprintf(stderr, " exiting\n");
			exit(1);
		}
	}

	fprintf(stderr, "\n");
	exit(0);
}
