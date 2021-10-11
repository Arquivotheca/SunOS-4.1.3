/*      @(#)idload.c 1.1 92/07/30 SMI      */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idload:idload.c	1.9.1.1"
#include <stdio.h>
#include <fcntl.h>
#include <nlist.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <rfs/idtab.h>
#include <rfs/rfsys.h>
#include <rfs/nserve.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include <rfs/pn.h>
#include <rfs/cirmgr.h>

#define N_MGDP		0
#define N_GDP		1
#define N_GLOBAL	2

#define MGDP_STR	"maxgdp"
#define GDP_STR		"gdp"
#define GLOBAL_STR	"Global"

#define MEMF		"/dev/kmem"
#define SYSTEM		"/unix"
#define ID_DATA		"/etc/iddata"

#define	ERROR		-1
#define	NO_ERROR	0
#define	NO_MAPPING	1

struct nlist nl[] = {
	{MGDP_STR},
	{GDP_STR},
	{GLOBAL_STR},
	{""}
};

typedef struct  rem_id {
	char	r_idbuf[BUFSIZ];
	struct	rem_id	*r_idnext;
};

extern char *malloc();
extern long  lseek();
extern char *optarg;
extern int   optind;

char	*cmd_name;

main(argc, argv)
int   argc;
char *argv[];
{
	int	n_update = 0;
	int	error = 0;
	int	num_args = 0;
	int	gflag = 0, uflag = 0;
	int	kflag = 0;
	int	usr_map_stat = 0, grp_map_stat = 0;
	int	c;

	char	*u_rules = "/usr/nserve/auth.info/uid.rules";
	char	*g_rules = "/usr/nserve/auth.info/gid.rules";
	char	*passdir = NULL;

	cmd_name = argv[0];

	/*
	 *	Parse arguments.
	 */

	while ((c = getopt(argc, argv, "ng:u:k")) != EOF) {
		switch (c) {
			case 'n':
				if (n_update || kflag)
					error = 1;
				else
					n_update = 1;
				break;
			case 'g':
				if (gflag || kflag)
					error = 1;
				else {
					g_rules = optarg;
					gflag = 1;
				}
				break;
			case 'u':
				if (uflag || kflag)
					error = 1;
				else {
					u_rules = optarg;
					uflag = 1;
				}
				break;
			case 'k':
				if (kflag || n_update || uflag || gflag)
					error = 1;
				else {
					kflag = 1;
				}
				break;
			case '?':
				error = 1;
		}
	}

	num_args = argc - optind;

	if (num_args == 1)
		passdir = argv[optind];

	if (num_args == 1 && kflag)
		error = 1;

	if (num_args > 1 || error) {
		fprintf(stderr, "%s: usage: %s [-n] [-g gid_rules] [-u uid_rules] [directory]\n", argv[0], argv[0]);
		fprintf(stderr, "%s:        %s -k\n", argv[0], argv[0]);
		exit(1);
	}

	if (geteuid() != 0) {
		fprintf(stderr, "%s: must be super-user\n", argv[0]);
		exit(1);
	}

	/*
	 *	If the "-k" flag is set, then print out the mapping
	 *	information in the kernel.  Otherwise, parse the user-
	 *	level mapping files (the code to do that is in
	 *	libns:uidmap.c).
	 */

	if (kflag) {
		if (getmap() == ERROR) {
			fprintf(stderr, "%s: could not obtain idmapping information from the kernel\n", argv[0]);
			exit(1);
		} else {
			exit(0);
		}
	}

	/*
	 *	Clear the translation tables and
	 *	perform user id mapping for all machines.
	 */

	if (n_update == 0)
		rfsys(RF_SETIDMAP, NULL, UID_MAP, NULL);

	usr_map_stat = uidmap(UID_MAP, u_rules, passdir, NULL, n_update);

	/*
	 *	Perform group id mapping for all machines.
	 */

	grp_map_stat = uidmap(GID_MAP, g_rules, passdir, NULL, n_update);

	if (grp_map_stat == 0 || usr_map_stat == 0)
		exit(1);
	else
		exit(0);
	/* NOTREACHED */
}

/*
 *	Get the idmapping that is currently in the kernel
 */

getmap()
{
	int	mem, gdpsiz, size, i;
	int	table, global, n_elements;
	int	errorstate;
	int	nlfd;
	int	gotem = 0;
	struct	gdp *gdpspace;
	struct	stat iddata, uxdata;

	/*
	 *	open file to access memory
	 */

	if ((mem = open(MEMF, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: cannot open %s\n", cmd_name, MEMF);
		return(ERROR);
	}

	/*
	 *	Get values of system variables
	 *	If the file that contains the address of the required
	 *	system variables exists (i.e., the file exists and
	 *	is newer than the kernel) then read in the address from
	 *	the file.  Otherwise get the addresses from the kernel
	 *	and write it into the file.
	 */

	if((stat(ID_DATA, &iddata) >= 0)
	   && (stat(SYSTEM, &uxdata) >= 0)
	   && (iddata.st_mtime > uxdata.st_mtime)
	   && ((nlfd = open(ID_DATA, O_RDONLY)) >= 0)) {
		gotem = 1;
		n_elements = sizeof(nl) / sizeof(struct nlist);
		for(i = 0; i < n_elements; i++) {
			if(read(nlfd, &nl[i].n_value, sizeof(long)) != sizeof(long)) {
				gotem = 0;
				break;
			}
		}
		close(nlfd);
	}

	/*
	 *	If the information couldn't be obtained from the
	 *	file, get the information from the kernel and
	 *	write it into the file so next time the command will
	 *	run much faster.
	 */

	if (!gotem) {
		if (nlist(SYSTEM, nl) < 0) {
			fprintf(stderr, "%s: cannot get system variables\n", cmd_name);
			return(ERROR);
		}
		if((nlfd = open(ID_DATA, O_WRONLY | O_CREAT)) >= 0 ) {
			chmod(ID_DATA, 0664);
			n_elements = sizeof(nl) / sizeof(struct nlist);
			for(i = 0; i < n_elements; i++)
				write(nlfd, &nl[i].n_value, sizeof(long));
			close(nlfd);
		}
	}

	/*
	 *	read in the value of maxgdp to get the size of the
	 *	gdp table.
	 */

	if (rread(mem, nl[N_MGDP].n_value, &gdpsiz, sizeof(gdpsiz)) == ERROR) {
		fprintf(stderr, "%s: cannot read kernel-level data\n", cmd_name);
		return(ERROR);
	}

	size = gdpsiz * sizeof(struct gdp);

	/*
	 *	read in the gdp table.
	 */

	if (((gdpspace = (struct gdp *)malloc(size)) == NULL)
	 || (rread(mem, nl[N_GDP].n_value, gdpspace, size) == ERROR)) {
		fprintf(stderr, "%s: cannot read kernel-level data\n", cmd_name);
		return(ERROR);
	}

	/*
	 *	The following loop prints out idtable information
	 *	for uid mappings (i == 0) and gid mappings (i == 1).
	 */

	printf("%-5s %-14s %-11s %-14s %-14s %-14s\n", "TYPE",
		"MACHINE", "REM_ID", "REM_NAME", "LOC_ID", "LOC_NAME");

	errorstate = NO_ERROR;

	for (i = 0; i < 2; i ++) {
		/*
		 *	Print the global specification.
		 */

		printf("\n");
		global = pr_global(i, mem);

		/*
		 *	Print out the values of the idtables associated
		 *	with each gdp structure.
		 */

		table = pr_gdp(i, gdpspace, gdpsiz, mem);

		if (global == NO_MAPPING && table == NO_MAPPING)
			no_mappings(i);

		if (global == ERROR || table == ERROR) {
			fprintf(stderr, "%s: error in accessing the kernel-level idtables\n", cmd_name);
			errorstate = ERROR;
		}
	}

	return(errorstate);
}

pr_global(flag, mem)
int flag, mem;
{
	char	*globp[2];

	/*
	 *	Read in the value of the pointers to the idtables
	 *	for the global specification.
	 */

	if (rread(mem, nl[N_GLOBAL].n_value, globp, sizeof(globp)) == ERROR)
		return(ERROR);

	if (globp[flag] == 0)
		return(NO_MAPPING);
	else
		return(pr_table(flag, "GLOBAL", mem, globp[flag]));
}

pr_gdp(flag, gdpspace, gdpsiz, mem)
int	flag;
struct	gdp *gdpspace;
int	gdpsiz;
int	mem;
{
	struct	gdp *gdpp, *lastgdp, *temp;
	int	already_done;
	int	count = 0;

	lastgdp = gdpspace + gdpsiz;

	/*
	 *	Go through the gdp table and get the idmap table
	 *	this machine has created for each remote machine.
	 */

	for (gdpp = gdpspace; gdpp < lastgdp; gdpp++) {
		if (!gdpp->flag)
			continue;

		/*
		 *	If gdpp->idmap[flag] is 0, then no specific
		 *	mapping for the remote machine was specified.
		 */

		if (gdpp->idmap[flag] == 0)
			continue;
	
		/*
		 *	Check to make sure that this machine has
		 *	not already been processed.
		 */

		count ++;
		already_done = 0;
		for (temp = gdpspace; temp < gdpp; temp++) {
			if (temp->flag && strcmp(temp->token.t_uname, gdpp->token.t_uname) == 0) {
				already_done = 1;
				break;
			}
		}

		if (already_done)
			continue;

		if (pr_table(flag, gdpp->token.t_uname, mem, gdpp->idmap[flag]) == ERROR)
			return(ERROR);
	}
	if (count == 0)
		return(NO_MAPPING);
	else
		return(NO_ERROR);
}

pr_table(flag, name, mem, place)
int flag;
char *name;
int mem;
struct idhead *place;
{
	struct	idhead headbuf;
	struct	idtab *where, *idtable, *ptr;
	char	*type = (flag == 0)? "USR": "GRP";
	int	i, size;
	char	rem_str[16], temp_str[16];

	/*
	 *	Read in the header of the idmap table and
	 *	print out thr default values.
	 */

	if (rread(mem, place, &headbuf, sizeof(headbuf)) == ERROR)
		return(ERROR);

	print_def(type, headbuf.i_default, name);

	/*
	 *	headbuf.i_cend is the offset
	 *	(in struct idtabs) to the actual idtable.
	 */

	where = (struct idtab *)(place) + headbuf.i_cend;
	size = sizeof(struct idtab) * headbuf.i_size;

	if (size == 0)
		return(NO_ERROR);

	if (((idtable = (struct idtab *)malloc(size)) == NULL)
	 || (rread(mem, where, idtable, size) == ERROR))
		return(ERROR);

	/*
	 *	Go through the idtable and print out each entry.
	 *	Note: if the local value (ptr->i_loc) is a -1,
	 *	then the entry represents a range of remote
	 *	values.  Therefore, print out something like:
	 *		0       -1
	 *		100     999
	 *	as:
	 *		0-100   999
	 */

	rem_str[0] = '\0';
	for (i = 0; i < headbuf.i_size; i++) {
		ptr = idtable + i;
		sprintf(temp_str, "%d", ptr->i_rem);
		strcat(rem_str, temp_str);
		if (ptr->i_loc == (unsigned short)(-1)) {
			strcat(rem_str, "-");
			continue;
		}
		printf("%-5s %-14s %-11s %-14s %-14d %-14s\n",
		  type, name, rem_str, "n/a", ptr->i_loc,
		  ptr->i_loc == MAXUID + 1? "guest_id":"n/a");
		rem_str[0] = '\0';
	}
	return(NO_ERROR);
}

print_def(type, def, name)
char *type;
int   def;
char *name;
{
	char def_str[15];

	/*
	 *	Print out the default value for the remote
	 *	machine.
	 */

	sprintf(def_str, "%d", def);
	printf("%-5s %-14s %-11s %-14s %-14s %-14s\n",
		type, name, "DEFAULT", "n/a",
		def == 0? "transparent": def_str,
		def == OTHERID? "guest_id":"n/a");
}

no_mappings(flag)
int flag;
{
	printf("%-5s %-14s %-11s %-14s %-14d %-14s\n",
		flag == 0? "USR":"GRP", "GLOBAL", "DEFAULT", "n/a",
		OTHERID, "guest_id");
}

rread(device, position, buffer, count)
char *buffer;
int count, device;
long position;
{
	/*
	 *	Seeks to "position" on device "device" and reads "count"
	 *	bytes into "buffer".
	 */

	if ((lseek(device, position, 0) == (long) -1)
	 || (read(device, buffer, (unsigned) count) == ERROR)) {
		fprintf(stderr, "idload: error in reading %s\n",MEMF);
		return(ERROR);
	}
	return(NO_ERROR);
}
