
/*      @(#)do_diskinfo.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 *	this copy commented for educational purposes  -fred
 */


#include "install.h"

extern char *sprintf();

/*
*	do_diskinfo()
*
*	This routine looks at a the 'targetdisk' to see if
*	an arbitrary 'targetsize' will fit in the 'target'
*	partition.
*
*	If 'targetsize' can be acommodated then fine... the
*	file disk_info.disk is updated to show the correct
*	remaining space in the 'target' partition after 'targetsize'
*	has been subtracted from it.
*
*	If 'targetsize' cannot be accomodated by the space
*	remaining in 'target', then, if ample space exists in
*	the partition designated as the FREE_HOG, then the 
*	'target' partition is enlargened by an ample number
*	of whole disk cylinders and the FREE_HOG partition is
*	made smaller by the identical amount.
*
*	If, on the other hand, insufficient space exists to
*	accomodate 'targetsize' using both the remaining space
*	in the 'target' partition combined with the remaining
*	space in the FREE_HOG, then nothing is changed and
*	an error (-1) is returned.
*
*/

do_diskinfo(targetdisk, target, targetsize, progname)
char *targetdisk;	/* the disk name w/o part (i.e. xy0) */
char *target;		/* the partition to check for (i.e. xy0a) */
long targetsize;	/* the size of what we're trying to shove into it */
char *progname;
{
	char freeflag[10], part[10];
	long total_size, avail_size;
	long freehog_total, freehog_avail, target_total, target_avail, value;
	long cylsize;
	int error, i;
	struct dk_geom g;
	char FREE_HOG;
	FILE *infile, *outfile;
	char filename[MAXPATHLEN];
	char buf[BUFSIZE];

	/*
	 *	open the raw device and get it's geometry
	 */
	(void) sprintf(filename, "/dev/r%sc", targetdisk);
        if ( (i = open(filename, 0)) != -1 ) {
                if (ioctl(i, DKIOCGGEOM, &g) != 0) {
			(void) close(i);
                        (void) fprintf(stderr,
			    "%s:\tUnable to read disk geometry for %s\n",
			    progname, targetdisk);
                        return(-1);
                }
		/*
		*	compute the cylinder size in bytes
		*/
		cylsize = g.dkg_nhead * g.dkg_nsect * 512;
                (void) close(i);
        } else {    
                (void) fprintf(stderr,"%s:\tUnable to access raw device %s\n",
		    progname, filename);
                return(-1);
        }

	/*
	*	open the infamous 'disk_info.disk' file
	*/
        (void) sprintf(filename,"%sdisk_info.%s",INSTALL_DIR,targetdisk);
        if((infile = fopen(filename,"r")) == NULL) {
                (void) fprintf(stderr,
		    "%s:\tUnable to open %s\n", progname, filename);
                return(-1);
        }

	freehog_total=0;
	freehog_avail=0;
	target_total=0;
	target_avail=0;

	/* 
	*	read in the disk_info file, 
	*	find out which partition is the free hog,
	*	get the total and avail size for the free hog,
	*	get the total and avail size for the target partition,
	*/
	while (fgets(buf,BUFSIZ,infile) != NULL) {
                bzero(freeflag,sizeof(freeflag));
                (void) sscanf(buf,"%s %d %d %s",
			part,&total_size,&avail_size,freeflag);
                if (!strcmp(freeflag,"free")) {
                        FREE_HOG = part[strlen(part)-1];
                        freehog_total = total_size;
                        freehog_avail = avail_size;
                }
		if (!strcmp(target,part) ) {
			target_total = total_size;
			target_avail = avail_size;
		}
	}
	(void) fclose(infile);

	/*
	*	if the target size is more than what's available,
	*	and the target part. is not the free hog, then,
	*	the *value* (in bytes) to be added to the target is 
	*	equal to the difference in whole cylinders
	*/
	if ( (target_avail < targetsize) && 
	     FREE_HOG != target[strlen(target)-1] ) {

		/* 
		* first, find needed value in whole cylinders
		*/
		value = (targetsize-target_avail) / cylsize;
		/*
		*	bump by one if remainder
		*/
		if ((targetsize-target_avail) % cylsize)
			value++;
		/*
		*	convert to bytes
		*/
		value *= cylsize;

		/*
		*	if the freehog has it available then
		*	pump up the target and deplete the free hog
		*/
		if ( (freehog_total - value) > 0) {
                	freehog_total -= value;
                	freehog_avail = freehog_total;
                	target_total += value;
                	target_avail = 0;
		}
	}

	/*
	*	reopen the disk_info.disk as the input file
	*/
	(void) sprintf(filename,"%sdisk_info.%s",INSTALL_DIR,targetdisk);
        if((infile = fopen(filename,"r")) == NULL) {
                (void) fprintf(stderr,
		    "%s:\tUnable to open %s\n",progname, filename);
                return(-1);
        }
	/*
	*	open a new disk_info.disk.tmp output file
	*/
        (void) sprintf(filename,"%sdisk_info.%s.tmp",INSTALL_DIR,targetdisk);
        if((outfile = fopen(filename,"w")) == NULL) {
                (void) fprintf(stderr,
		    "%s:\tUnable to open %s\n", progname, filename);
		(void) fclose(infile);
                return(-1);
        }

	error = 0;
        while (fgets(buf,BUFSIZ,infile) != NULL) {
		bzero(freeflag,sizeof(freeflag));
                (void) sscanf(buf,"%s %d %d %s",part,&total_size,
			&avail_size,freeflag);

		/*
		*	if available space is (or was made)
		*	large enough...
		*/
		if (target_avail >= targetsize) {
               		if ( strcmp(target,part) )
                               	(void) fputs(buf,outfile);
                       	else
                               	(void) fprintf(outfile,"%s %d %d %s\n",part,
				    total_size,avail_size-targetsize,freeflag);
		} else {
			/*
			*	if the target is not the free hog...
			*/
			if ( target[strlen(target)-1] != FREE_HOG ) {
				/*
				*	if the current part is the target...
				*/
				if ( !strcmp(target,part) )
					(void) fprintf(outfile,"%s %d %d\n",
						part,target_total,target_avail);
				/*
				*	if the current part is the free hog...
				*/
				else if ( part[strlen(part)-1] == FREE_HOG )
                                       (void) fprintf(outfile,"%s %d %d free\n",
					    part, freehog_total,freehog_avail);
				/*
				*	if the current part is unrelated
				*	to either...
				*/
	                        else
                                     (void) fprintf(outfile,"%s %d %d\n",part,
					    total_size,avail_size);
			} else {
				/*
				*	...otherwise, target was the
				*	free hog so then it's an error...
				*/
				freehog_total = total_size;
				freehog_avail = avail_size;
				(void) fprintf(outfile,"%s %d %d free\n",part,
				    total_size, avail_size);
				error = 1;
			}
                }
	}

        (void) fclose(infile);
        (void) fclose(outfile);
	if (!error)
               	(void) sprintf(buf,"mv %sdisk_info.%s.tmp %sdisk_info.%s\n",
			INSTALL_DIR,targetdisk,INSTALL_DIR,targetdisk);
	else
		(void) sprintf(buf,"rm -f %sdisk_info.%s.tmp %sdisk_info.%s\n",
                       	INSTALL_DIR,targetdisk,INSTALL_DIR,targetdisk);
        (void) system(buf);
	return(0);
}
