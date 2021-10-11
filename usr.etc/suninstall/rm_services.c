#ifndef lint
static  char *sccsid = "@(#)rm_services.c 1.1 92/07/30 SMI";
#endif lint

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"
#include "sysexits.h"
#include "dirent.h"
#include "errno.h"

#ifdef lint
int	BACKWARD,FORWARD;
struct loc map[];
int mapentries;
#endif lint

char		*export="export";	/* /export for real */
char		*arch;
char		*proto;
char		*exec;
char		*kvm;		
char		*home;
char		*mail;
char		*share;
char		*progname;
char		*mount_pt;
int             eflag, hflag, iflag, kflag, mflag, pflag, qflag;
int		vflag, xflag;
int             dataerr, err, c;
char		servername[STRSIZE];

extern char	*optarg;
extern int      optind;
extern char	domain[];
extern int 	errno;
extern char	*sprintf(), *getarch(), *where_are_we(), *rindex();

/*
 * return true if this is the last architecture removed
 */

last_arch()
{
	return (0);		/* TBD */
}


/*
 * string match s2 in s1
 */

match (s1, s2)
char *s1, *s2;
{
#ifdef lint
s1 = s2;
s2 = s1;
#endif lint
	return (0);		/* TBD */
}


main(argc, argv)
int	argc;
char	**argv;
{
	struct server_info server;
	char buf[BUFSIZ];

	/*
	 * get program name
	 */

	if ((progname = rindex(argv[0], '/')) == NULL)
		progname = argv[0];
	else
		progname++;

	/*
	*	start logger
	*/
	(void) log_start(progname);

	/*
	 * only superuser can do damage with this command
	 */

	if (suser() == -1) {
		(void) fprintf(stderr, "%s: must be superuser\n", progname);
		exit(EX_NOPERM);
	}

	/*
	 *	get server's arch, use as default
	 */

	arch = getarch(progname);
	(void) strcpy(server.arch, arch);

	/*
	 * parse arguments
	 */

	while ((c = getopt(argc, argv, "a:de:h:ikm::p:s:vx:")) != -1) {
		switch (c) {

		case 'e':
			eflag++;
			exec = optarg;
			fullpath(exec,dataerr);
			break;
		case 'h':
			hflag++;
			home = optarg;
			fullpath(home,dataerr);
			break;
		case 'i':	/* interactive */
			iflag++;
			break;
		case 'k':
			kflag++;
			kvm = optarg;
			fullpath(kvm,dataerr);
			break;
		case 'm':
			mflag++;
			mail = optarg;
			fullpath(mail,dataerr);
			break;
		case 'p':
			pflag++;
			proto = optarg;
			fullpath(proto,dataerr);
			break;
		case 'q':
			qflag++;
			share = optarg;
			fullpath(share,dataerr);
			break;
		case 'v':	/* verbose */
			vflag++;
			break;
		case 'x':
			xflag++;
			export = optarg;
			fullpath(export,dataerr);
			break;
		case '?':
			err++;
			break;
		}
	}
	if (err || (argc != optind +1)) {
		(void) fprintf(stderr,
		    "usage:  %s [-iy][-axehhkm] arg\n",
		    progname);
		exit(EX_USAGE);
	} else if (dataerr) {
		(void) fprintf(stderr, "%s: data error\n", progname);
		exit(EX_DATAERR);
	}

	(void) gethostname(servername, sizeof(servername));

	 /* 
	  *	sort out arguments
	  */

	set_args(&server);

	/*
	 * are we in miniroot ? (sets up mount_pt string)
	 */

	mount_pt = where_are_we();

	/*
	 * delete boot file from tftpboot directory
	 */

	rm_tftpboot();

       /* 
	* remove arch and kvm directory trees
	*/

	(void) sprintf (buf, "rm -rf %s\n", server.exec_path);
	(void) system (buf);
	(void) sprintf (buf, "rm -rf %s\n", server.kvm_path);
	(void) system (buf);

	if (last_arch()) {
		/*
		 * remove share export tree
		 */
		(void) sprintf (buf, "rm -rf %s\n", server.share_path);
		(void) system (buf);
	}

	/* 
	 * edit exports to remove lines for things we are unexporting
	 */

	change_exports(&server);

	/* 
	 * un-export the things we just edited out of the exports file
	 */

	if (!MINIROOT) {
		/* XXX - will this do the trick? */
		(void) system("/usr/etc/exportfs -a\n");
	}
	exit(0);
	/* NOTREACHED */
}

/*
 *	sort out legality of input and figure default values
 */

set_args(server)
struct server_info *server;
{

	/*
	 *	check arch for legality
	 */
	if (strcmp(arch,SUN4) && strcmp(arch,SUN3) &&
	    strcmp(arch,SUN3X) && strcmp(arch,SUN2)) {
		(void) fprintf(stderr,"%s: %s illegal architecture\n", 
			progname, arch);
		exit(EX_DATAERR);
	}

	/* 
	 * Set up directory pathnames - root, exec, kvm
	 */
	if (!(xflag && !strcmp(export,NONE))) {

		if (eflag) {
			(void) strcpy(server->exec_path,exec);
		} else {
			(void) strcpy(server->exec_path,export);
			(void) strcat(server->exec_path,"/exec/");
			(void) strcat(server->exec_path,arch);
	        }


		if (kflag) {
			(void) strcpy(server->exec_path,kvm);
		} else {
			(void) strcpy(server->kvm_path,export);
			(void) strcat(server->kvm_path,"/exec/kvm/");
			(void) strcat(server->kvm_path,arch);
		}

		if (qflag) {
			(void) strcpy(server->share_path, share);
		} else {
			(void) strcpy(server->kvm_path,export);
			(void) strcat(server->kvm_path,"share");
		}

	} else {
		(void) strcpy(server->exec_path,export);
		(void) strcpy(server->kvm_path,export);
	}

	if (hflag)
		(void) strcpy(server->home_path,home);
	else {
		(void) strcpy(server->home_path,"/home/");
		(void) strcat(server->home_path,servername);
	}

	if (mflag)
		(void) strcpy(server->mail_path,mail);
	else 
		(void) strcpy(server->mail_path,"/var/spool/mail");
}


rm_tftpboot()
{
	char pathname[STRSIZE];
	char buf[BUFSIZ];

	/*
	 * delete bootfile from tftpboot directory
	 */

	(void) sprintf(pathname,"%s/tftpboot/boot.%s", mount_pt, arch);
	if (unlink (pathname) == -1) {
		(void) sprintf (buf, "%s: couldn't unlink %s", progname,
			 pathname);
		perror (buf);
	}
}


change_exports(server)
struct server_info *server;
{
	int last = last_arch();
	int fd;
	char *template="rm_execXXXXXX";
	char **getline();
	FILE *temp, *exports;
	static char line [256];
	char filename[MAXPATHLEN];
	char buf[BUFSIZ];

	(void) sprintf(filename,"%s%s",mount_pt, EXPORTS);
	if((exports = fopen(EXPORTS,"r")) == NULL) {
		(void) fprintf(stderr, "%s: unable to open %s\n",progname, EXPORTS);
		return;
	}

	if ((fd = mkstemp (template)) == -1) {
		(void) fprintf (stderr, "%s: couldn't make tempfile\n", progname);
		return;
	}

	if ((temp = fdopen (fd, "w")) == NULL) {
		(void) fprintf (stderr, "%s: couldn't fdopen %s\n", progname, 
			 template);
		return;
	}
	
	/*
	 * copy line for line entries from exports file to tempfile,
	 * filtering out lines for directory trees we want to un-export.
	 */

	while (getline(line, 256, exports) != NULL) {
		if (last) {
			if (!match (line, server->exec_path) &&
			    !match (line, server->kvm_path) &&
			    !match (line, server->share_path) &&
			    !match (line, server->home_path) &&
			    !match (line, server->mail_path))
				fputs (line, temp);
		} else {
			if (!match (line, server->exec_path) &&
			    !match (line, server->kvm_path))
				fputs (line, temp);
		}
	}

	if (rename (template, filename) == -1) {
		(void) sprintf (buf, "%s: couldn't rename %s to %s", 
			 progname, template, filename);
		perror (buf);
	}
	(void) fclose(exports);
}
