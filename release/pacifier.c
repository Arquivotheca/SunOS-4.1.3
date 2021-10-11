/*
 * pacifier - program used by pieupgrade to write a pacifier
 * to the console.  Requires 1 filename as input.
 */
#include <stdio.h>

#define BS 8 

extern void exit();
char ttname[] = "/dev/console";
char buffer[]  = "|/-\\";

main(argc, argv)
char **argv;
{
	FILE *fp, *wtty;
	int i=0, count=0;
	register c;

	if (argc <= 1) exit(0);

	if ((fp = fopen(argv[1], "r")) == NULL) {
		(void) fprintf(stderr, "pacifier: ");
		perror(argv[1]);
		exit(1);
	}

	while ((c = getc(fp)) != EOF) {
		putchar(c);
		if (++i > 0x10000) {
        		if ((wtty = fopen(ttname, "w")) != NULL) {
                        	(void) putc(buffer[count], wtty);
                        	(void) putc(BS, wtty);
                        	(void) fclose(wtty);
				if (++count > 3) count=0;
				i=0;
			}
		}

	}
	(void) fclose(fp);
	return(0);
}
