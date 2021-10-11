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
#include <gttest.h>
#include <errmsg.h>

#define TAR		"tar"
#define TAR_ARG		"-xBef"
#define ICON_SIZE	8
#define TOLERANCE	4
#define MAX_TRIALS	10

extern
int		sys_nerr;
extern
int		errno;
extern
char		*sys_errlist[];

extern
int		chksum_gen, test_debug;

extern
char		*device_name;

extern
char		*getfname();

extern
Pixrect		*open_pr_device();



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

    device_pr = open_pr_device();
    if (device_pr == NULL) {
	(void)sprintf(errtxt, DLXERR_OPEN, device_name);
	TRACE_OUT
	return(errtxt);
    }
    /* no matter what the screen size really is, we will work
     * only in a region of 1024x900.
     */
    pr = pr_region(device_pr, 0, 0, 1024, 900);
    if (pr == NULL) {
	(void)sprintf(errtxt, DLXERR_OPEN, device_name);
	TRACE_OUT
	return(errtxt);
    }

    reduced_width = pr->pr_width / icon_width;
    reduced_height = pr->pr_height / icon_height;

    mpr = mem_create(reduced_width, reduced_height, 8);
    if (mpr == NULL) {
	(void)close_pr_device();
	TRACE_OUT
	return DLXERR_OPEN_MEM_PIXRECT;
    }

    /* set 24-bit plane group and attributes, read from buffer A */
    (void)pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
    (void)pr_dbl_set(pr, PR_DBL_READ, PR_DBL_A, 0);

    if (reduce_output_image(pr, mpr, icon_width, icon_height, RED_CHK)){
	(void)close_pr_device();
	(void)pr_close(mpr);
	TRACE_OUT
	return DLXERR_REDUCE_RED;
    }

    if (verify_with_file(getfname(test, RED_CHK), mpr, chksum_gen)) {
	red_err = 1;
    }

    if (reduce_output_image(pr, mpr, icon_width, icon_height,
							GREEN_CHK)) {
	(void)close_pr_device();
	(void)pr_close(mpr);
	TRACE_OUT
	return DLXERR_REDUCE_GREEN;
    }
    if (verify_with_file(getfname(test, GREEN_CHK), mpr, chksum_gen)) {
	green_err = 1;
    }

    if (reduce_output_image(pr, mpr, icon_width, icon_height,
							    BLUE_CHK)) {
	(void)close_pr_device();
	(void)pr_close(mpr);
	TRACE_OUT
	return DLXERR_REDUCE_BLUE;
    }
    if (verify_with_file(getfname(test, BLUE_CHK), mpr, chksum_gen)) {
	blue_err = 1;
    }


    (void)close_pr_device();
    (void)pr_close(mpr);

    if (red_err | green_err | blue_err) {
	(void)strcpy(errtxt, DLXERR_FOUND_IN);
	if (red_err) (void)strcat(errtxt, DLXERR_RED);
	if (green_err) (void)strcat(errtxt, DLXERR_GREEN);
	if (blue_err) (void)strcat(errtxt, DLXERR_BLUE);
	(void)strcat(errtxt, DLXERR_COMPONENTS);
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
	    (void)fb_send_message(SKIP_ERROR, WARNING, "%s: %s\n",
					testname, sys_errlist[errno]);
	    TRACE_OUT
	    return 1;
	}
	err = pr_dump(mpr, file, NULL, RT_BYTE_ENCODED, 0);
	(void)fclose(file);
	if (err) {
	    (void)fb_send_message(SKIP_ERROR, WARNING, "%s: %s\n",
					testname, sys_errlist[errno]);
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
	    (void)fb_send_message(SKIP_ERROR, WARNING, "%s: %s\n",
					testname, sys_errlist[errno]);
	    TRACE_OUT
	    return 1;
	}
	fpr = pr_load(file, NULL);
	(void)fclose(file);
	(void)unlink(testname);

	if (fpr == NULL) {
	    (void)fb_send_message(SKIP_ERROR, WARNING, "%s: %s\n",
					testname, sys_errlist[errno]);
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
		if (test_debug) {
		    FILE *fdb;
		    char fname[256];

		    strcpy(fname, testname);
		    strcat(fname, ".dump");
		    fdb = fopen(fname, "w");
		    if (fdb == NULL) {
			fb_send_message(SKIP_ERROR, CONSOLE, DLXERR_OPEN_FILE_TO_DUMP, fname);
		    } else {
			(void)pr_dump(mpr, fdb, NULL, RT_BYTE_ENCODED, 0);
			(void)fclose(fdb);
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

	    if (test_debug) {
		/*write back to the frame buffer*/
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
xtract_chk(file)
/**********************************************************************/
char *file;

{

    char file_1[128], file_2[128], file_3[128], file_4[128];

    func_name = "xtract";
    TRACE_IN

    strcpy(file_1, getfname(file, RED_CHK));
    strcpy(file_2, getfname(file, GREEN_CHK));
    strcpy(file_3, getfname(file, BLUE_CHK));

    (void)tarxf(file_1, file_2, file_3, (char *)0);

    TRACE_OUT
}

/**********************************************************************/
xtract_hdl(file)
/**********************************************************************/
char *file;

{

    char file_1[128];

    func_name = "xtract";
    TRACE_IN

    strcpy(file_1, getfname(file, HDL_CHK));

    (void)tarxf(file_1, (char *)0, (char *)0, (char *)0);

    TRACE_OUT
}

/**********************************************************************/
tarxf(testname_1, testname_2, testname_3, testname_4)
/**********************************************************************/
char *testname_1;
char *testname_2;
char *testname_3;
char *testname_4;

{
    extern int sys_nerr;
    extern int errno;
    extern char *sys_errlist[];
    extern char testdata_filename[];

    int child_pid;
    int count;
    int status;


    func_name = "tarxf";
    TRACE_IN

    (void)check_key();

    child_pid = vfork();
    if (child_pid == 0) {
	execlp(TAR, TAR, TAR_ARG, testdata_filename, testname_1,
		testname_2, testname_3, testname_4, (char *)0);
	(void)fb_send_message(SKIP_ERROR, WARNING, DLXERR_TAR,
						    sys_errlist[errno]);
	_exit(0);
    } else if (child_pid < 0) {
	(void)fb_send_message(SKIP_ERROR, WARNING, DLXERR_VFORK,
						    sys_errlist[errno]);
	return 0;
    }

    /* wait for the tar program to complete */
    /********
    count = 0x3ffffff;
    while ((wait4(child_pid, &status, WNOHANG | WUNTRACED,
					NULL) != child_pid) && count--);

    if (count <= 0) {
	(void)fb_send_message(SKIP_ERROR, WARNING, DLXERR_TAR_NEVER_FINISHED);
    } else
    *********/

    if (wait(&status) != child_pid) {
	(void)fb_send_message(SKIP_ERROR, WARNING, DLXERR_TAR_NEVER_FINISHED);
    }

    if (status & 0xff) {
	(void)fb_send_message(SKIP_ERROR, WARNING, DLXERR_TAR_KILLED,
					(int)((status & 0xff00) >> 8));
    } else if (status & 0xff00) {
	/* tar fail */
	(void)fb_send_message(SKIP_ERROR, WARNING, DLXERR_TAR_ERROR);
	(void)unlink(testname_1);
	if (testname_2) {
	    (void)unlink(testname_2);
	}
	if (testname_3) {
	    (void)unlink(testname_3);
	}
	if (testname_4) {
	    (void)unlink(testname_4);
	}
    }

    TRACE_OUT
}
