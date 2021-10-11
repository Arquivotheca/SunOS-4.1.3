#ifndef lint
static	char sccsid[] = "@(#)skyrc.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * push ascii microcode at the sky board for starting it up.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sundev/skyreg.h>

char * ufilename;
char * devname = "/dev/sky";
int skyfd;
FILE *ufile;
struct skyreg *sp;
int nerrs;

int
mapsky()
{
    /*
     * try to map the sky board into our address space.
     * the only allowable error is to not be able to open
     * the device. this causes us to return 0.
     * on any other error, exit noisily. 
     * otherwise return 1;
     */
    int ps = getpagesize();
    
    if ((skyfd = open(devname, O_RDWR         ))>=0){
	fprintf(stderr, "%s ALREADY INITIALIZED\n", devname);
	close(skyfd);
	exit(3);
    }
    if ((sp = (struct skyreg *)valloc( ps ))==NULL){
	fprintf(stderr, "cannot allocate space\n");
	exit(3);
    }
    if ((skyfd = open(devname, O_RDWR|O_NDELAY))<0)
	return 0;
    if (mmap( sp, ps, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED, skyfd, 0)<0){
	perror(devname);
	exit(3);
    }
    return 1;
}

int
microcram()
{
    /*
     * cram microcode into sky board.
     */
    register struct skyreg *s = sp;
    union swap {
	short	s[2];
	long	l;
    } inword, outword;
    register unsigned word;
    int uaddr;
    int lineno = 0;
    int nwords = 0;

    /* halt the board, in case its running */
    s->sky_status = SKY_IHALT;
    s->sky_status = SKY_IHALT;
    /* cram microcode at it -- read it back each time to check */
    while( (word=fscanf( ufile, "%x %x ", &uaddr, &inword.l)) != EOF){
	lineno++;
	if (word!=2){
	    fprintf(stderr,"error reading line %d\n", lineno);
	    nerrs++;
	    continue;
	}
	outword.s[1] = inword.s[0];
	outword.s[0] = inword.s[1];
	s->sky_command = uaddr;
	s->sky_ucode =   outword.l;
	word = s->sky_ucode;
	if (word != outword.l){
	    fprintf(stderr,"microcode verify error at address %#X\n",uaddr);
	    nerrs++;
	}
	nwords++;
    }
    /* start the thing up */
    s->sky_status = SKY_RESET;
    s->sky_command = SKY_START0;
    s->sky_command = SKY_START0;
    s->sky_command = SKY_START1;
    s->sky_status  = SKY_RUNENB;
    return nwords;
}

skytest()
{
    static float fdata[8] = { 1., 2., 3., 4., 5., 6., 7., 8. };
    int sdata;  
    register i;
    register struct skyreg *s = sp;
    register int *rdata;

    rdata = (int *)fdata;
    /* do a restore/save */
    s->sky_command = SKY_RESTOR;
    for (i=0; i<8; i++)
	s->sky_data = rdata[i];
    s->sky_command = SKY_SAVE;
    for (i=0; i<8; i++){
	sdata = s->sky_data;
	if (sdata != rdata[i]){
	    fprintf( stderr, "sky save/restore failure\n");
	    nerrs++;
	}
    }
    /* test log(1.0) */
    s->sky_command = 0x102d;
    s->sky_data = rdata[0];
    /* wail for result */
    for (i=0; i<40; i++)
	if (s->sky_status < 0 ) break;
    sdata = s->sky_data;
    if (sdata != 0){
	fprintf( stderr, "sky arithmetic failure\n");
	nerrs++;
    }
}

main( argc, argv )
    int argc;
    char **argv;
{
    int i;
    char *ufilename = NULL;
    int nword, notst = 0;

    for (i=1; i<argc; i++){
	if (!strcmp( argv[i], "-d")){
	    devname = argv[i+1];
	    i += 1;
	} else if (!strcmp( argv[i], "-q")){
	    notst=1;
	} else if (ufilename!=NULL){
	    fprintf(stderr,"warning: filename is %s\n", ufilename);
	    nerrs++;
	} else
	    ufilename = argv[i];
    }
    if (ufilename==NULL)
	ufilename = "sky.ucode";
    if (mapsky()==0)
	return 0;
    ufile = fopen( ufilename, "r");
    if (ufile==NULL){
	perror( ufilename );
	exit(3);
    }
    nword = microcram();
    printf("loaded %d words of sky microcode\n", nword);
    if (!notst)
	skytest();
    return nerrs;
}
