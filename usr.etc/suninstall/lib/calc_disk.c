#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)calc_disk.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)calc_disk.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		calc_disk()
 *
 *	Description:	Calculate the amount of disk space used by the current
 *		configuration.  Calls calc_client() and calc_software() to
 *		do the real work for each supported architecture.
 *
 *		Returns 1 if everything is okay, 0 if there is insufficent
 *		disk space and -1 if there was an error.
 */

#include <sys/file.h>
#include <stdio.h>
#include "install.h"
#include "menu.h"




/*
 *	External functions:
 */
extern	char *		sprintf();


/*
 *	Local functions:
 */
static	int		df_to_avail();
static	int		do_df();
static	char *		new_appl_arch();

/*
 *	Local constants:
 */
static	char		DF_FILE[] = "/tmp/df_file";

int
calc_disk(sys_p)
	sys_info *	sys_p;
{
	arch_info	*arch, *ap;		/* architecture info buffer */
	char		buf[BUFSIZ];		/* I/O buffer */
	char		cmd[BUFSIZ];		/* command buffer */
	FILE *		fp;			/* scratch file pointer */
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	int		ret_code;		/* return code */
	char *		arid;

	if (fp = fopen(DISK_LIST, "r")) {
                while (fgets(buf, sizeof(buf), fp)) {
			delete_blanks(buf);

			/*
			 *	Skip this disk if there is no data file.
			 */
			(void) sprintf(pathname, "%s.%s.original", DISK_INFO,
				       buf);
			if (access(pathname, R_OK) != 0)
				continue;

        		(void) sprintf(cmd, "cp %s.%s.original %s.%s 2>> %s",
				       DISK_INFO, buf, DISK_INFO, buf,
				       LOGFILE);

			x_system(cmd);

			if (!is_miniroot()) {
				(void) sprintf(pathname, "%s.%s",
					       DISK_INFO, buf);
				ret_code = df_to_avail(DF_FILE, pathname);
				if (ret_code != 1)
					return(ret_code);
			}
                }
                (void) fclose(fp);
        }
	if (!is_miniroot())
		return(1);

	switch (read_arch_info(ARCH_INFO, &arch)) {
	case -1:
		return(-1);

	case 0:					/* missing file is okay here */
		break;

	case 1:
                for (ap = arch; ap ; ap = ap->next) {
			ret_code = calc_software(ap,ap->arch_str,sys_p,"impl");
			if (ret_code != 1)  {
				free_arch_info(arch);
				return(ret_code);
			}
			if (arid = new_appl_arch(arch, ap, buf))    {
				if ((ret_code = 
				    calc_software(ap,arid,sys_p,"appl")) != 1
				    || (ret_code =
				    calc_software(ap,arid,sys_p,"root")) != 1
				    || (ret_code =
				    calc_software(ap,arid,sys_p,"share")) != 1)
				    {
					free_arch_info(arch);
					return(ret_code);
				}
			}

			if (sys_p->sys_type == SYS_SERVER &&
			    (ret_code = calc_client(ap->arch_str)) != 1) {
				free_arch_info(arch);
				return(ret_code);
			    }
                } 
		break;
        }

	free_arch_info(arch);
	return(1);
} /* end calc_disk() */


static char *
new_appl_arch(root, ap, buf)
	arch_info *	root;
	arch_info *	ap;
	char *		buf;
{
	char		arid[MEDIUM_STR];
	arch_info *	tmp;
	(void) aprid_to_arid(ap->arch_str, buf);
	for (tmp = root; tmp && tmp != ap; tmp = tmp->next)  {
		if (strcmp(os_arid(&tmp->os, arid), buf) == 0)
			return(NULL);
	}
	return(buf);
}


/*
 *	Name:		df_to_avail()
 *
 *	Description:	Use df(1) to get available byte information and
 *		update the appropriate field in the given disk_info file.
 */

static	int
df_to_avail(df_file, info_file)
	char *		df_file;
	char *		info_file;
{
	long		avail;			/* available K bytes */
	char		device[SMALL_STR];	/* device name */
	disk_info	disk;			/* scratch disk information */
	FILE *		fp;			/* ptr to df_file */
	int		part;			/* partition code */
	int		ret_code;		/* return code */

	static	char		did_df = 0;	/* have we done df(1) yet? */

	extern	int		errno;		/* system error number */


	bzero((char *) &disk, sizeof(disk));

	if (did_df == 0) {
		ret_code = do_df(df_file);
		if (ret_code != 1)
			return(ret_code);

		did_df = 1;
	}

	switch (read_disk_info(info_file, &disk)) {
	case 1:
		break;

	case 0:
		menu_log("%s: %s: cannot read disk information.", progname,
			 info_file);
		/*
		 *	Fall through here
		 */
	case -1:
		return(-1);
	}

	fp = fopen(df_file, "r");
	if (fp == NULL) {
		menu_log("%s: %s: %s", progname, df_file, err_mesg(errno));
		return(-1);
	}

	while (fscanf(fp, "%s %ld", device, &avail) == 2) {
		part = device[strlen(device) - 1];

		if (strncmp(disk.disk_name, device,
			    strlen(disk.disk_name)) == 0)
			disk.partitions[part - 'a'].avail_bytes = avail * 1024;
	}

	(void) fclose(fp);

	if (save_disk_info(info_file, &disk) != 1)
		return(-1);

	return(1);
} /* end df_to_avail() */




/*
 *	Name:		do_df()
 *
 *	Description:	Do a df(1) and put the relevant data into a temp file.
 *		Returns 1 if everything is okay and -1 otherwise.
 */

static	int
do_df(pathname)
	char *		pathname;
{
	long		avail;			/* available K bytes */
	char		buf[BUFSIZ];		/* scratch buffer */
	char		device[SMALL_STR];	/* device name */
	FILE *		fp;			/* ptr to pathname */
	FILE *		pp;			/* ptr to df(1) process */

	extern	int		errno;		/* system error number */


	fp = fopen(pathname, "w");
	if (fp == NULL) {
		menu_log("%s: %s: %s", progname, pathname, err_mesg(errno));
		return(-1);
	}

	(void) sprintf(buf, "df -t %s", DEFAULT_FS);
#ifdef SunB1
	menu_flash_on("Using df(1) to get available byte information");
#endif SunB1

	macex_on();
	pp = popen(buf, "r");
	macex_off();

	if (pp == NULL) {
		menu_log("%s: '%s': command failed.", progname, buf);
		return(-1);
	}

	while (fgets(buf, sizeof(buf), pp)) {
		if (sscanf(buf, "/dev/%s %*s %*s %ld", device, &avail) == 2)
			(void) fprintf(fp, "%s %ld", device, avail);
	}

	(void) pclose(pp);
	(void) fclose(fp);

#ifdef SunB1
	menu_flash_off(REDISPLAY);
#endif /* SunB1 */

	return(1);
} /* end do_df() */
