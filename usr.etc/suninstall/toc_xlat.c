#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)toc_xlat.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)toc_xlat.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */


/*
 *	Name:		toc_xlat.c
 *
 *	Description:	Translate table of contents into a media file list.
 *
 *	Exit codes:	 0 - everything is okay
 *			 1 - reserved for execl() error in caller
 *			-1 - fatal error
 */

#include <ctype.h>
#include "mktp.h"
#include "install.h"


/*
 *	External references:
 */
extern	void		flash_off();
extern	void		flash_on();
extern	void		menu_abort();
extern	void		menu_log();
extern	char *		sprintf();
extern	char *		strstr();
extern	char *		strncat();




/*
 *	Local functions:
 */
static	char *		get_media_arch();
static	void		tape_to_os();
static	char *		gtypnam();

/*
 *	Global variables:
 */
#ifdef SunB1
int		old_mac_state;			/* old mac-exempt state */
#endif /* SunB1 */
char *		progname;


main(argc, argv)
	int		argc;
	char *		argv[];
{
	distribution_info d_info;		/* distribution information */
	toc_entry *	ep;			/* TOC entry pointer */
	FILE *		fp;			/* ptr to XDRTOC */
	Os_ident	os_id_buf;		/* OS id buffer */
	Installable *	install_p;		/* ptr to Installable info */
	char *		media_arch;		/* media architecture */
	media_file	media_list[NENTRIES];	/* media file list */
	media_file *	mf;			/* ptr to media_list */
	int		n_entries;		/* # entries in table */
	char *		name;			/* name of file to translate */
	char		pathname[MAXPATHLEN];	/* path to soft_info file */
	soft_info	soft;			/* software info buffer */
	char *		sys_arch;		/* system architecture */
	toc_entry	table[NENTRIES];	/* table of contents */
	tapeaddress *	tape_p;			/* ptr to Tape size info */
	volume_info	v_info;			/* volume information */
	int 		showflag = 0;		/* show simple toc form */
	char *		device;			/* tape device name */

	progname = argv[0];

	(void) umask(UMASK);			/* set file creation mask */

	bzero((char *) media_list, sizeof(media_list));
	bzero((char *) &os_id_buf, sizeof(os_id_buf));

	if (strcmp(basename(progname), "xdrtoc") == 0 || argc == 1) {
		showflag = 1;
		fp = stdin;
		sys_arch = "";
#ifdef lint 
		sys_arch = sys_arch;
#endif
		name = "";
	}
	else	{
		switch (argc)	{
		    case 2:
			showflag = 1;			/* show TOC */
		        device = argv[1];
			name = XDRTOC;
			sys_arch = "";
			break;
		    case 3:
			name = argv[1];
			sys_arch = argv[2];
			break;
		    default:
			(void) fprintf(stderr, 
				"Usage: %s: [DeviceName e.g. st1, mt0]\n",
				progname);
			exit(-1);
		}
		if (showflag)	{
			(void) sprintf(soft.media_path, "/dev/nr%s", device);
			(void) cv_str_to_media(device, &soft.media_dev);
			(void) media_set_soft(&soft,soft.media_dev);
			soft.media_loc = LOC_LOCAL;
			flash_off();
			if (read_file(&soft, 1, name) != 1)
				exit(-1);
			flash_on();
		}
		fp = fopen(name, "r");
	}
	if (fp == NULL) {
		log("%s: %s: cannot open file for reading.\n",
		    progname, name);
		exit(-1);
	}

	bzero((char *) &d_info, sizeof(d_info));
	bzero((char *) &v_info, sizeof(v_info));

	n_entries = read_xdr_toc(fp, &d_info, &v_info, table, NENTRIES);

	(void) fclose(fp);

	if (n_entries == 0) {
		(void) fprintf(stderr, "%s: %s: no entries.\n",
			       progname, name);
		exit(-1);
	}

	/*
	 *	Make sure this is the right architecture tape.  Exit with a
	 *	fatal error if we can't get the media architecture.
	 */
	media_arch = get_media_arch(table, n_entries);
	if (media_arch == NULL) {
		(void) fprintf(stderr,
			       "%s: %s: cannot get media architecture.\n",
			       progname, name);
		exit(-1);
	}

	/*
	 *	Convert the tape information into an APR id.
	 */
	tape_to_os(&d_info, media_arch, &os_id_buf);
	(void) os_aprid(&os_id_buf, soft.arch_str);

	/*
	 *	Copy the relevant entries from the table of contents.
	 */


	if (showflag)	{
		(void) fprintf(stdout,"%s %s of %s from %s\n",d_info.Title,
			d_info.Version,d_info.Date,d_info.Source);
		(void) fprintf(stdout,"ARCH %s\n", media_arch);
		(void) fprintf(stdout,"VOLUME %d\n",
			v_info.vaddr.Address_u.tape.volno);
	        (void) fprintf(stdout,
			" Vol File                 Name       Size\tType\n");
	}

	mf = media_list;
	for (ep = table; ep < &table[n_entries]; ep++) {
		install_p = &ep->Info.Information_u.Install;
		tape_p = &ep->Where.Address_u.tape;

		/*
		 *	Don't want table of contents, images or unknowns
		 */
		if (showflag)	{
			(void) fprintf(stdout,"%4d %4d %20s %10d\t%s\n",
				tape_p->volno, tape_p->file_number,
				ep->Name, tape_p->size,
				gtypnam(ep->Type));
		}

		/*
		 *	Let's process the image only if it is of KIND
		 *	INSTALLABLE or INSTALL_TOOL
		 */
		if (ep->Type == TOC 	||
		    ep->Type == IMAGE 	||
		    ep->Type == UNKNOWN ||
		    (ep->Info.kind != INSTALL_TOOL &&
		     ep->Info.kind != INSTALLABLE))
			continue;

		mf->media_no = tape_p->volno;
		mf->file_no = tape_p->file_number;
		mf->mf_name = ep->Name;
		mf->mf_type = ep->Type;

		switch (ep->Info.kind) {
		case INSTALLABLE:
			/*
			 *	Do not use Where.tape.size here since it
			 *	is not always valid.
			 */
			mf->mf_size = ep->Info.Information_u.Install.size;
			mf->mf_kind = KIND_INSTALLABLE;
			if (install_p->required)
				mf->mf_iflag = IFLAG_REQ;

			else if (install_p->desirable)
				mf->mf_iflag = IFLAG_DES;

			else if (install_p->common)
				mf->mf_iflag = IFLAG_COMM;

			else
				mf->mf_iflag = IFLAG_OPT;
			mf->mf_loadpt = install_p->loadpoint;
			mf->mf_depcount =
				       install_p->soft_depends.string_list_len;
			mf->mf_deplist =
				       install_p->soft_depends.string_list_val;
			break;

		case INSTALL_TOOL:
			mf->mf_size = ep->Info.Information_u.Tool.size;
			mf->mf_kind = KIND_INSTALL_TOOL;
			mf->mf_loadpt = ep->Info.Information_u.Tool.loadpoint;
			break;
		} /* end switch */

		mf++;
	}

	soft.media_vol = v_info.vaddr.Address_u.tape.volno;
	soft.media_count = mf - media_list;
	soft.media_files = media_list;

	(void) sprintf(pathname, "%s.tmp", MEDIA_FILE);
	if (save_media_file(pathname, &soft) != 1)
		exit(-1);

	exit(0);
	/*NOTREACHED*/
} /* end main() */
 



/*
 *	Name:		get_media_arch()
 *
 *	Description:	Get the media architecture from the table of contents.
 */

static char *
get_media_arch(table, n_entries)
	toc_entry	table[NENTRIES];
	int		n_entries;
{
	toc_entry *	ep;			/* table entry pointer */
	string_list *	sp = 0;			/* string list pointer */


	/*
	 *	This code is stolen from the xdrtoc program.  This code
	 *	should be a subroutine in a library since getting the
	 *	architecture from a table of contents is generally useful.
	 */

	for (ep = table; ep < &table[n_entries]; ep++) {
		if (ep->Info.kind == STANDALONE) {
			if (ep->Info.Information_u.Standalone.arch.string_list_len != 0) {
				sp = &ep->Info.Information_u.Standalone.arch;
				break;
			}
		}
		else if (ep->Info.kind == EXECUTABLE) {
			if (ep->Info.Information_u.Exec.arch.string_list_len != 0) {
				sp = &ep->Info.Information_u.Exec.arch;
				break;
			}
		}
		else if (ep->Info.kind == INSTALLABLE) {
			if (ep->Info.Information_u.Install.arch_for.string_list_len != 0) {
				sp = &ep->Info.Information_u.Install.arch_for;
				break;
			}
		}
	}

	if (sp)
		return(*sp->string_list_val);

	return(NULL);
} /* end get_media_arch() */




/*
 *	Name:		tape_to_os()
 *
 *	Description:	Convert information about the tape into an Os_ident
 *		structure.
 */

static void
tape_to_os(info_p, media_arch, buf_p)
	distribution_info * info_p;
	char *		media_arch;
	Os_ident *	buf_p;
{
	char *		cp;			/* scratch char pointer */
	int		len;

	/*
	 *	Create an Os_ident structure from the mktape information
	 */

	/*
	 *	The Title of the info_p structure now contains both the
	 *	os_name and release.  The os_name ends at the first blank
	 *	and the release is everything else
	 */
	len = strcspn(info_p->Title," ");
	buf_p->os_name[0] = 0;
	(void)strncat(buf_p->os_name,info_p->Title,len);

	/*
	 *	Translate the Title into a lower case string and
	 *	do a little bit of sanity checking for whitespace.
	 */
	for (cp = buf_p->os_name; *cp; cp++) {
		switch (*cp) {
		case ' ':
		case '\t':
			*cp = '_';
			break;

		default:
			if (isupper(*cp))
				*cp = tolower(*cp);
			break;
		}
	}

	/*
	 * If the title only contained the os name then it must be an old
	 * tape.  Since the information is the same we copy it from the
	 * Version.  This also protects us in case people don't do the
	 * changes to the mktp entries in time as well as backward
	 * compatibility
	 */
	if (len < strlen(info_p->Title))
		(void)strcpy(buf_p->release,&info_p->Title[len + 1]);
	else
		(void) strcpy(buf_p->release, info_p->Version);

	/*
	 *	Check for a realization string.
	 *	If found, copy it into "realization", and truncate
	 *	"release" at that point.
	 */

	cp = strstr(buf_p->release, PSR_IDENT);
	if (cp != NULL) {
		strcpy(buf_p->realization, cp);
		*cp = '\0';
	}

	(void) strcpy(buf_p->impl_arch, media_arch);
	(void) appl_arch(media_arch, buf_p->appl_arch);
} /* end tape_to_os() */

static char *
gtypnam(type)
	file_type type;
{
	switch(type)
	{
	    default:
	    case UNKNOWN :
		return "unknown";
	    case TOC:
		return "toc";
	    case IMAGE:
		return "image";
	    case TAR:
		return "tar";
	    case CPIO:
		return "cpio";
	    case TARZ:
		return "tarZ";
	}
}

