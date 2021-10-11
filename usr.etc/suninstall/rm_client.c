#ifndef lint
#ifdef SunB1
#ident			"@(#)rm_client.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)rm_client.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/***************************************************************************
**
** File :  rm_client.c
**
**
** Description : This handy utility allows the user to remove a client with
**		 ease. The user is, by default prompted for each client
**		 entry deletion.  If the user specifies "-y", it means
**		 answer yes to each question and don't prompt each time.
**
****************************************************************************
*/


#include "install.h"
#include <stdio.h>
#include <strings.h>
#include <sysexits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>


#define USAGE \
"usage:  %s [-y] client_list\n\
   where 'client' is a list of client names to remove and, \n\
	-y      \tforces \"yes\" responses to all questions asked.\n\n"



char	*progname;		/* this program name */


static int	yflag;	/* flag true if -y (yes) specified */


main(argc, argv)
	int             argc;
	char          **argv;
{
	sys_info	sys;		/* for system data */

	/*
	 * get program name
	 */
	progname = basename(argv[0]);

	/*
	 * only superuser can do damage with this command
	 */
	if (getuid() != 0) {
		(void) fprintf(stderr, "%s: must be superuser\n", progname);
		exit(EX_NOPERM);
	}

	if (argc < 2) {
		(void) fprintf(stderr, USAGE, progname);
		exit(EX_USAGE);
	}

	/*
	 * parse arguments
	 */

	if (strcmp(argv[1], "-y") == 0) {
		yflag++;
		argv++;
		argc--;
	}

	/*
	 *	Now, let's get the system data
	 */
	if (read_sys_info(SYS_INFO, &sys) != 1) {
                menu_log("%s: Error in %s.", progname, SYS_INFO);
                menu_abort(1);
        }

	for(; argc > 1 ; argc--, argv++)
		(void) remove_client(argv[1], &sys);

	exit(EX_OK);

	/* NOTREACHED */
}


remove_client(clientname, sys_p)
	char 		*clientname;
	sys_info	*sys_p;
{
	struct 	ether_addr 	ether_addr;
	clnt_info 		client;
	struct 	hostent 	*client_hostent;
	int 			r;
	char 			cmd[MAXPATHLEN];

        (void) sprintf(cmd, "%s.%s", CLIENT_STR, clientname);

	if (r = read_clnt_info(cmd, &client) != 1) {
		fprintf(stderr, "%s: %s: no such client.\n", 
			progname, clientname);
		return(1);
	}

	/*
	 * get ether addr (for warning), ipaddr(for boot filename)
	 */

	if ((client_hostent = gethostbyname(clientname)) == NULL) {
	    (void) fprintf(stderr, 
		    "%s: warning: host entry not found for %s\n", 
		       progname, clientname);
       }

	(void) delete_entry(clientname, HOSTS);
	(void) delete_entry(clientname, ETHERS);

	/*
	 * delete boot file link from /tftpboot
	 */

	if (client_hostent)
		(void) delete_tftplink(client_hostent);

	/*
	 * delete client's entry to bootparams (use NIS if available)
	 */

	(void) delete_bootparams(clientname);

	/*
	 * delete client's root directory tree and swapfile
	 */
	if (!access(client.root_path, F_OK)) {
		if (ask ("client's root directory", client.root_path)) {
			(void) printf("removing %s...\n", client.root_path);
			(void) unexport(client.root_path);
			(void) sprintf (cmd, "rm -rf %s%s", 
				dir_prefix(), client.root_path);
			(void) system (cmd);
		} else {
			(void) unexport(client.root_path);
		}
	} else {
		(void) fprintf (stderr, 
			"%s: warning: no such path -> %s\n", 
				progname, client.root_path);
	}

	if (!access(client.swap_path, F_OK)) {
		(void) printf("removing %s...\n", client.swap_path);
		(void) unexport(client.swap_path);
		(void) sprintf (cmd, "rm -f %s%s", 
			dir_prefix(), client.swap_path);
		(void) system (cmd);
	} else {
		(void) fprintf (stderr, 
			"%s: warning: no such path -> %s\n", 
				progname, client.swap_path);
	}


	(void) printf("removing database entries for %s...\n", clientname);

	(void) sprintf(cmd, "%s.%s", CLIENT_STR,  client.hostname);
	(void) unlink(cmd);
	
	delete_client_from_list(&client);


	/*
	 * 	Now that everthing was removed, update the NIS's
	 */
	update_yp(&client);

	(void) fprintf (stdout, "client %s removed.\n", clientname);
	
	return(0);

} /* end of remove_client() */


/*
 * delete client entry from /etc/bootparams
 */

delete_bootparams(clientname)
	char	*clientname;
{
	FILE	*ifile;
	FILE	*ofile;
	char	fname[MAXPATHLEN];
	char	line[LARGE_STR * 2];
	char	*cmd = fname;
	char	*str = fname;
	char	*p;

	/*
	*	open the two files
	*/
	(void) sprintf(fname, "%s%s", dir_prefix(), BOOTPARAMS);
	if ((ifile	= fopen(fname, "r")) == NULL) {
		(void) fprintf(stderr, "%s: could not open %s\n", 
			progname, fname);
		return(-1);
	}
	if ((ofile = fopen("/tmp/bootparams", "w")) == NULL) {
		(void) fprintf(stderr, "%s: could not open %s\n", 
			progname, fname);
		fclose(ifile);
		return(-1);
	}

	/*
	*	walk through the input file and copy only those 
	*	lines which don't match our clientname to the output file
	*/
	(void) printf("removing %s's bootparams entry...\n", clientname);
	bzero(line, sizeof(line));

	while (fgets(line, sizeof(line), ifile) != NULL) {
		(void) sscanf(line, "%s", str);
		if (strcmp(str, clientname) != 0) {
			(void) fputs(line, ofile);
			while ((p = rindex(line, '\\')) != NULL) {
				bzero(line, sizeof(line));
				(void) fgets(line, sizeof(line), ifile);
				(void) fputs(line, ofile);
			}
			
		} else {
			/*
			 * 	grab all additional items as well
			 */
			while ((p = rindex(line, '\\')) != NULL) {
				bzero(line, sizeof(line));
				(void) fgets(line, sizeof(line), ifile);
			}
		}
		bzero(line, sizeof(line));
	}
	
	fclose(ifile);
	fclose(ofile);

	(void) sprintf(cmd, "mv -f /tmp/bootparams %s%s", 
		       dir_prefix(), BOOTPARAMS);

	(void) system(cmd);

	(void) unlink("/tmp/bootparams");	
	
	return(0);
	
} /* end of delete_bootparams() */


/*
 * delete client's entry from the hosts and ethers databases
 */

delete_entry(clientname, file)
	char	*clientname;
	char	*file;
{
	FILE	*ifile;
	FILE	*ofile;
	char	fname[MAXPATHLEN];
	char	line[LARGE_STR * 2];
	char	ip[32];
	char	*cmd = fname;
	char	*str = fname;
	int 	found;
	int 	pid;

	pid = getpid();

	(void) sprintf(fname, "%s%s", dir_prefix(), file);
	if ((ifile = fopen(fname, "r")) == NULL) {
		(void) fprintf(stderr, "%s: could not open file %s\n", 
			progname, fname);
		return(-1);
	}
	(void) sprintf(fname, "/tmp/rm_client.%d", pid);
	if ((ofile = fopen(fname, "w")) == NULL) {
		(void) fprintf(stderr,
			       "%s: could not open temporary file %s\n", 
			progname, fname);
		fclose(ifile);
		return(-1);
	}

	/*
	*	walk through the file and copy only those 
	*	lines which don't match our pathname
	*/
	(void) printf("removing %s's %s entry...\n", clientname, file);
	found = 0;
	while (fgets(line, sizeof(line), ifile) != NULL) {
		(void) sscanf(line, "%s %s", ip, str);
		if (strcmp(str, clientname))
			fputs(line, ofile);
		else {
			found++;
			(void) sprintf(str, "%s entry ->", file);
			if (!ask(str, line))
				fputs(line, ofile);
		}
	}

	fclose(ifile);
	fclose(ofile);

	if (!found) {
		(void) fprintf(stderr, "%s: warning: entry for %s not found in %s file\n", progname, clientname, file);
		(void) sprintf(cmd, "rm /tmp/rm_client.%d", pid);
	} else
		(void) sprintf(cmd, "mv -f /tmp/rm_client.%d %s%s", 
			pid, dir_prefix(), file);

	(void) system(cmd);
	return(0);

} /* end of delete_entry() */


delete_tftplink(he)
	struct hostent * he;
{
	char		cmd[256];
	unsigned long 	inet_addr();
	char		*inet_ntoa();
	char		hexip[16];
	char		*path[MAXPATHLEN];

	(void) sprintf (hexip, "%08lX", inet_addr(inet_ntoa
	    (*(struct in_addr *)he->h_addr)));

	(void) sprintf (path, "%s/tftpboot/%s", dir_prefix(), hexip);
	if (access(path, F_OK)) {
		(void) fprintf (stderr, 
			"%s: warning: /tftpboot/%s link not found\n", 
			progname, hexip);
	} else {
		(void) printf("removing /tftpboot link %s...\n", hexip);
		(void) sprintf (cmd, "rm -f %s/tftpboot/%s* 2>/dev/null", 
			dir_prefix(), hexip);
		(void) system(cmd);
	}

	return (0);

} /* end of delete_tftplink() */


unexport(path)
	char	*path;
{
	FILE	*ifile;
	FILE	*ofile;
	char	fname[MAXPATHLEN];
	char	line[LARGE_STR * 2];
	char	*cmd = fname;
	char	*str = fname;
	int found;

	(void) sprintf(fname, "%s%s", dir_prefix(), EXPORTS);

	if ((ifile = fopen(fname, "r")) == NULL) {
		(void) fprintf(stderr, "%s: could not open %s file\n", 
			progname, fname);
		return(-1);
	}
	strcpy(fname, "/tmp/exports");
	if ((ofile = fopen(fname, "w")) == NULL) {
		(void) fprintf(stderr, "%s: could not open temporary file %s\n", 
			progname, fname);
		fclose(ifile);
		return(-1);
	}

	/*
	*	attempt to unexport these paths
	*/
	(void) sprintf(cmd, "/usr/etc/exportfs -u %s 2>/dev/null", path);
	(void) system(cmd);

	/*
	*	walk through the file and copy only those 
	*	lines which don't match our pathname
	*/
	found = 0;
	while (fgets(line, sizeof(line), ifile) != NULL) {
		(void) sscanf(line, "%s", str);
		if (strcmp(str, path))
			(void) fputs(line, ofile);
		else
			found++;
	}
	fclose(ifile);
	fclose(ofile);

	if (!found) {
		(void) fprintf (stderr,
			"%s: warning: entry for %s not found in %s file\n",
				progname, path, EXPORTS);
		(void) sprintf(cmd, "rm /tmp/exports");
	} else
		(void) sprintf(cmd, "mv -f /tmp/exports %s%s", 
			dir_prefix(), EXPORTS);

	(void) system(cmd);

	return(0);

} /* end of unexport() */


ask(s1, s2)
	char	*s1;
	char	*s2;
{
	char	line [256];

	if (yflag)
		return (1);

	(void) printf ("Remove %s %s (y/n)? ", s1, s2);
	(void) fflush (stdout);
	line [0] = '\0';
	(void) gets (line);
	if ((line [0] == 'y') || (line [0] == 'Y'))
		return (1);
	else
		return (0);
	
} /* end of ask() */

