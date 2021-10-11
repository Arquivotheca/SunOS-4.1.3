
#ifndef lint
static  char sccsid[] = "@(#)chksum.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <pixrect/pixrect_hs.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <esd.h>

#define TAR		"tar"
#define TAR_ARG		"xf"
#define CG12_DATA	"cg12.data"
#define CG12_DATA_GSXR	"cg12.data.gsxr"
#define ICON_SIZE	8
#define TOLERANCE	4
#define MAX_TRIALS	10

extern
int		chksum_gen;

extern
int		chksum_dump;

extern
char		*device_name;

extern
char		*getfname();



/**********************************************************************/
char *
chksum_verify(test)
/**********************************************************************/
char *test;

{
    static char errtxt[256];
    Pixrect *device_pr;
    Pixrect *pr;
    Pixrect *mpr;
    int reduced_width;
    int reduced_height;
    int icon_width = ICON_SIZE;
    int icon_height = ICON_SIZE;
    int red_err = 0;
    int green_err = 0;
    int blue_err = 0;

    func_name = "chksum_verify";
    TRACE_IN

#ifdef ALWAYS_PASSED	/* for testing purposes */
    TRACE_OUT
    sleep(3);
    return (char *)0;
#else if !ALWAYS_PASSED

    device_pr = pr_open(device_name);
    if (device_pr == NULL) {
	(void)sprintf(errtxt, errmsg_list[4], device_name);
	TRACE_OUT
	return(errtxt);
    }
    /* set 24-bit plane group and attributes */
    (void)pr_set_planes(device_pr, PIXPG_24BIT_COLOR, -1);

    /* no matter what the screen size really is, we will work
     * only in a region of 1152x900.
     */
    pr = pr_region(device_pr, 0, 0, 1152, 900);
    if (pr == NULL) {
	(void)pr_close(device_pr);
	(void)sprintf(errtxt, errmsg_list[4], device_name);
	TRACE_OUT
	return(errtxt);
    }

    reduced_width = pr->pr_width / icon_width;
    reduced_height = pr->pr_height / icon_height;

    mpr = mem_create(reduced_width, reduced_height, 8);
    if (mpr == NULL) {
	(void)pr_close(pr);
	(void)pr_close(device_pr);
	TRACE_OUT
	return("unable to create memory pixrect.\n");
    }

    if (reduce_output_image(pr, mpr, icon_width, icon_height, RED_CHK)){
	(void)pr_close(pr);
	(void)pr_close(device_pr);
	(void)pr_close(mpr);
	TRACE_OUT
	return("Can't reduce result image, RED component.\n");
    }

    if (verify_with_file(getfname(test, RED_CHK), mpr, chksum_gen)) {
	red_err = 1;
    }

    if (reduce_output_image(pr, mpr, icon_width, icon_height,
							GREEN_CHK)) {
	(void)pr_close(pr);
	(void)pr_close(device_pr);
	(void)pr_close(mpr);
	TRACE_OUT
	return("Can't reduce result image, GREEN component.\n");
    }
    if (verify_with_file(getfname(test, GREEN_CHK), mpr, chksum_gen)) {
	green_err = 1;
    }

    if (reduce_output_image(pr, mpr, icon_width, icon_height,
							    BLUE_CHK)) {
	(void)pr_close(pr);
	(void)pr_close(device_pr);
	(void)pr_close(mpr);
	TRACE_OUT
	return("Can't reduce result image, BLUE component.\n");
    }
    if (verify_with_file(getfname(test, BLUE_CHK), mpr, chksum_gen)) {
	blue_err = 1;
    }


    (void)pr_close(pr);
    (void)pr_close(mpr);
    (void)pr_close(device_pr);

    if (red_err | green_err | blue_err) {
	if (chksum_dump) {
	    FILE *fd;
	    char fname[256];

	    device_pr = pr_open(device_name);
	    (void)pr_set_planes(device_pr, PIXPG_24BIT_COLOR, -1);

	    strcpy(fname, "/tmp/");
	    strcat(fname, test);
	    strcat(fname, ".dump");
	    fd = fopen(fname, "w");
	    if (fd == NULL) {
		(void)fb_send_message(SKIP_ERROR, CONSOLE, "Can't open %s to dump pixrect.\n", fname);
	    } else {
		(void)pr_dump(device_pr, fd, NULL, RT_BYTE_ENCODED, 0);
		(void)fclose(fd);
	    }

	    (void)pr_close(device_pr);
	}

	(void)strcpy(errtxt, "Error(s) found in ");
	if (red_err) (void)strcat(errtxt, "RED ");
	if (green_err) (void)strcat(errtxt, "GREEN ");
	if (blue_err) (void)strcat(errtxt, "BLUE ");
	(void)strcat(errtxt, "component(s).\n");
	TRACE_OUT
	return errtxt;
    } else {
	TRACE_OUT
	return (char *)0;
    }

#endif ALWAYS_PASSED

}
	
/**********************************************************************/
static int
verify_with_file(testname, mpr, gen)
/**********************************************************************/
char *testname;
Pixrect *mpr;
int gen;

{
    FILE *file;
    Pixrect *fpr;
    unsigned char *md;
    unsigned char *fd;
    register unsigned char *mp;
    register unsigned char *fp;
    int npixels;
    int i;
    int err;
    int trials;

    func_name = "verify_with_file";
    TRACE_IN

    (void)check_key();

    if (gen) {	/* generate test result file rather than checking */
	file = fopen(testname, "w");
	if (file == NULL) {
	    (void)perror(testname);
	    TRACE_OUT
	    return 1;
	}
	err = pr_dump(mpr, file, NULL, RT_BYTE_ENCODED, 0);
	(void)fclose(file);
	if (err) {
	    (void)perror(testname);
	    TRACE_OUT
	    return 1;
	} else {
	    TRACE_OUT
	    return 0;
	}
    } else {	/* verify content of result file against mpr */

	trials = MAX_TRIALS;
	do {
	    file = fopen(testname, "r");
	    if (file) break;
	    sleep(1);
	} while (file == NULL && trials--);
	if (file == NULL) {
	    (void)perror(testname);
	    TRACE_OUT
	    return 1;
	}
	fpr = pr_load(file, NULL);
	(void)fclose(file);
	(void)unlink(testname);

	if (fpr == NULL) {
	    (void)perror(testname);
	    TRACE_OUT
	    return 1;
	}
	md = (unsigned char *)mpr_d(mpr)->md_image;
	fd = (unsigned char *)mpr_d(fpr)->md_image;
	npixels = mpr->pr_width * mpr->pr_height;
	for (mp = md, fp = fd, i = 0 ; i < npixels ; fp++, mp++, i++) {
	    register fmax = *fp + TOLERANCE;
	    register fmin = *fp - TOLERANCE;

	    if (fmax > 255) fmax = 255;
	    if (fmin < 0) fmin = 0;

	    if (*mp > fmax || *mp < fmin) {
		if (debug) {
		    FILE *fd;
		    char fname[256];

		    strcpy(fname, testname);
		    strcat(fname, ".dump");
		    fd = fopen(fname, "w");
		    if (fd == NULL) {
			(void)fb_send_message(SKIP_ERROR, CONSOLE, "Can't open %s to dump pixrect.\n", fname);
		    } else {
			(void)pr_dump(mpr, fd, NULL, RT_BYTE_ENCODED, 0);
			(void)fclose(fd);
		    }
		}

		(void)pr_close(fpr);
		TRACE_OUT
		return 1;
	    }
	}
	(void)pr_close(fpr);
	TRACE_OUT
	return 0;
    }
}

/**********************************************************************/
static int
reduce_output_image(spr, dpr, icon_width, icon_height, type)
/**********************************************************************/
Pixrect *spr;
Pixrect *dpr;
int icon_width;
int icon_height;
int type;

{
    Pixrect *ipr;
    unsigned char *dd;
    register unsigned int *id;
    register int x;
    register int y;
    register int i;
    register int shift;
    int npixels;
    double total;
    int value;
    int mask;
    int xrun;
    int yrun;

    func_name = "reduce_output_image";
    TRACE_IN

    ipr = mem_create(icon_width, icon_height, 32);
    if (ipr == NULL) {
	TRACE_OUT
	return 1;
    }

    dd = (unsigned char *)mpr_d(dpr)->md_image;
    npixels = icon_width * icon_height;
    xrun = spr->pr_width-icon_width;
    yrun = spr->pr_height-icon_height;

    if (type == RED_CHK) {
	mask = 0xff;
	shift = 0;
    } else if (type == GREEN_CHK) {
	mask = 0xff00;
	shift = 8;
    } else if (type == BLUE_CHK) {
	mask = 0xff0000;
	shift = 16;
    } else {
	mask = 0;
    }

    for (y = 0 ; y <= yrun ; y += icon_height) {
	(void)check_key();
	for (x = 0 ; x <= xrun ; x += icon_width) {
	    (void)pr_rop (ipr, 0, 0, icon_width, icon_height,
				PIX_SRC | PIX_DONTCLIP, spr, x, y);
	    id = (unsigned int *)mpr_d(ipr)->md_image;
	    for (i = 0, total = 0.0 ; i < npixels ; i++) {
		total += (double)((*id++ & mask) >> shift);
	    }
	    value = (unsigned char)(total / (double)npixels);
	    *dd++ = value;

	    if (debug) { /* write back to the frame buffer */
		id = (unsigned int *)mpr_d(ipr)->md_image;
		for (i = 0 ; i < npixels ; i++) {
		    *id &= ~mask;
		    *id++ |= (value << shift);
		}
		(void)pr_rop (spr, x, y, icon_width, icon_height,
			PIX_SRC | PIX_DONTCLIP, ipr, 0, 0);
	    }
	}
    }

    TRACE_OUT
    return 0;
}

/**********************************************************************/
xtract(file)
/**********************************************************************/
char *file;

{

    char file_red[128], file_green[128], file_blue[128];

    func_name = "xtract";
    TRACE_IN

    strcpy(file_red, getfname(file, RED_CHK));
    strcpy(file_green, getfname(file, GREEN_CHK));
    strcpy(file_blue, getfname(file, BLUE_CHK));

    (void)tarxf(file_red, file_green, file_blue);

    TRACE_OUT
}

/**********************************************************************/
tarxf(testname_red, testname_green, testname_blue)
/**********************************************************************/
char *testname_red;
char *testname_green;
char *testname_blue;

{
    extern int sys_nerr;
    extern int errno;
    extern char *sys_errlist[];

    int child_pid;
    int count;

    func_name = "tarxf";
    TRACE_IN

    child_pid = vfork();
    if (child_pid == 0) {
	Pixrect *pr;

	pr = pr_open(device_name);
	execlp(TAR, TAR, TAR_ARG,
	    (pr->pr_size.x == 1280) ? CG12_DATA_GSXR : CG12_DATA,
		testname_red, testname_green, testname_blue, (char *)0);
        (void)fb_send_message(SKIP_ERROR, CONSOLE, "tar: %s\n",
							    sys_errlist[errno]);
	_exit(-1);
    } else if (child_pid < 0) {
	(void)fb_send_message(SKIP_ERROR, CONSOLE,
				"vfork: %s\n", sys_errlist[errno]);
    }

    /* wait for the tar program to complete */
    count = 1000000;
    while ((wait4(child_pid, NULL, WNOHANG | WUNTRACED, NULL) == NULL) &&
								    count--);

    if (count == 0) {
        (void)fb_send_message(SKIP_ERROR, WARNING, "'tar' never finished.\n");
    }

    TRACE_OUT
}
		
