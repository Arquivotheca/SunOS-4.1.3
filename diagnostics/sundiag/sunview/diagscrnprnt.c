static char     sccsid[] = "@(#)@(#)diagscrnprnt.c 1.1 92/07/30 Sun Micro";

#include        <fcntl.h>
#include        <rasterfile.h>

int     ifd = -1;
char    fbuf[512];
char    printbuf[120];
int     fbsz;

main(argc, argv)
	int  argc;
	char **argv;
{
	char  *tmpname="/tmp/sundiag.XXXXXX";
	printbuf[0]= '\0'; 

	mktemp(tmpname);
        (void)sprintf(printbuf, "screendump > %s ", tmpname);
	system(printbuf);

        ifd = open(tmpname, O_RDONLY);
         fbsz = read(ifd, fbuf, 512);
 
#       define ras ((struct rasterfile *)fbuf)
	/*
        printf("rasterfile, %dx%dx%d\n", ras->ras_width,
                ras->ras_height, ras->ras_depth);
	*/
	printbuf[0]= '\0'; 
	if (ras->ras_depth == 1){
		(void)sprintf(printbuf, "screendump | lpr -v -P%s ", argv[1]);
		system(printbuf);
	} else {
		(void)sprintf(printbuf, "screendump | rasfilter8to1 | lpr -v -P%s ", argv[1]);
		system(printbuf);
	}

	printbuf[0]= '\0'; 
	(void)sprintf(printbuf, "/bin/rm -rf %s", tmpname);
	system(printbuf);
	close(ifd);
}

