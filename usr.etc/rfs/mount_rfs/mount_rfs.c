#ifndef lint
static	char sccsid[] = "@(#)mount_rfs.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

/*
 * RFS mount utility -- Basically this is the S5R3 mount command
 * stripped to support only RFS.
 * This command is intended to be exec'd by the sun mount(1) command
 * and its argument syntax must conform with that specified in the
 * mount.c code.
 */
#include <sys/param.h>
#include <rpc/rpc.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/vfs.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/mount.h>
#include <mntent.h>
#include <sys/fcntl.h>
#include <rfs/nserve.h>
#include <tiuser.h>
#include <sys/stropts.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <rfs/cirmgr.h>
#include <rfs/message.h>
#include <rfs/rfsys.h>
#include <string.h>
#include <rfs/nsaddr.h>
#include <rfs/pn.h>
#include <rfs/hetero.h>

#define NETSPEC		"/usr/nserve/netspec"

extern int errno;
extern int   optind;
extern char *optarg;
extern char *strtok();
extern struct address *ns_getaddr();
static	char  *getnetspec();
char *findaddr();
char *sopt();
char *index();

main(argc,argv)
int  argc;
char **argv;
{
	char    *advname;
	char	*directory;
	char 	*machname = (char *) NULL;
	char 	*cmd = argv[0];
	int flags = M_NEWTYPE;
	char c;
	struct mntent mnt;
	char *opts;

	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT,  SIG_IGN);

	if  (argc < 3) {
		fprintf(stderr,
		"Usage: %s <adv-name> <dir> ['rfs' [options (ro,rw,machine=<name>)]\n",
				cmd);
		exit(-1);
	}
	/* Get resource name and directory */
	advname = argv[1];
	argv++; argc--;
	directory = argv[1];
	argv++; argc--;
	argv++; argc--;
	opts = argv[1];

	mnt.mnt_opts = opts;
	flags |= M_NOSUB; 	/* No mounts allowed in an RFS filesystem */
	if (hasmntopt(&mnt, MNTOPT_RO))
		flags |= M_RDONLY;
	if (hasmntopt(&mnt, "nomulti"))	/* Multi-component lookup is default */
		flags &= ~M_MULTI;
	else
		flags |= M_MULTI;
	machname = sopt(&mnt, "machine");

	if (*directory != '/') {
		fprintf(stderr, "mount_rfs: directory argument <%s> must be a full path name\n",directory);
		exit(2);
	}

	exit(mount_rfs(advname, directory, machname, flags));
	/* NOTREACHED */
}

	
int
mount_rfs(advname, dir, machname, flags)
	char *advname, *dir, *machname;
	int flags;
{
	struct address *addr;
	struct token	token;
	char dname[MAXDNAME];
	extern ndata_t ndata;
	struct rfs_args m_args;
	extern struct address *ns_getaddr(), *astoa();
	int rflag = flags & M_RDONLY ? RO_BIT : 0;

	/* set id to CLIENT; later getoken() will clear it.  */
	token.t_id = RFS_CLIENT;

	/* Machine name specified: get corresponding address from file */
	if (machname) {
		char *tbuf = findaddr(machname);
		if ((tbuf == (char *)NULL) || 
		    (addr = astoa(tbuf, NULL)) == (struct address *)NULL) {
			fprintf(stderr,"mount_rfs: invalid address specified: %s\n", 
				machname);
			return(EINVAL);
		}
		strncpy(token.t_uname, machname, MAXDNAME);
	/* Get resource address from name server */
	} else { 
		/* Determine if RFS status. If first call fails, RFS has not 
		 been installed.  If second call succeeds, RFS is not running. */
		if (rfsys(RF_GETDNAME, dname, MAXDNAME) < 0) {
			perror("mount_rfs");
			return(errno);
		}
		if (rfsys(RF_SETDNAME, dname, strlen(dname)+1) >= 0) {
			fprintf(stderr, "mount_rfs: RFS not running\n");
			return(EINVAL);
		}
		if ((addr = ns_getaddr(advname, rflag, token.t_uname)) 
		      == (struct address *)NULL) {
			fprintf(stderr,"mount_rfs: %s not available\n", advname);
			nserror("mount_rfs");
			return(ECONNREFUSED); /* This causes mount to retry */
		}
	}

	m_args.rmtfs = advname;
	m_args.token = &token;

	/* Mount the device */
	if (mount("rfs", dir, flags, &m_args) < 0) {
		if (errno != ENOLINK) {
			perror("mount_rfs");
			return(errno);
		}
		if (u_getckt(addr,&token) < 0) {
			errno = ECONNREFUSED;
			perror("mount_rfs");
			return(errno);
		}
		/*
		 *  Perform user and group id mapping for the host.
		 *  NOTE: ndata.n_netname is set via negotiate() in u_getckt().
		 */
		uidmap(0, (char *)NULL, (char *)NULL, &ndata.n_netname[0], 0);
		uidmap(1, (char *)NULL, (char *)NULL, &ndata.n_netname[0], 0);

		if (mount("rfs", dir, flags, &m_args) < 0) {
			perror("mount_rfs");
			return(errno);
		}
	}
	return(0);
}


static int
u_getckt(addr,token)
struct address	*addr;
struct token	*token;
{
	int fd;
	char mypasswd[20];
	struct gdpmisc gdpmisc;
	int pfd;
	int num;
	char modname[FMNAMESZ+1];
	extern ndata_t ndata;

	gdpmisc.hetero = gdpmisc.version = 0;

	if (((pfd = open(PASSFILE, O_RDONLY)) < 0)
	  || ((num = read(pfd, &mypasswd[0], sizeof(mypasswd)-1)) < 0)) {
		strcpy(mypasswd, "np");
	} else { 
		mypasswd[num] = '\0';
		(void) close(pfd);
	}

	if ((fd = att_connect(addr, RFSD)) == -1) {
		return(-1);
	}
	if (rf_request(fd, RF_RF) == -1) {
		t_cleanup(fd);
		return(-1);
	}

	/* Hack -- wait a bit so server parent has time to close its fd for 
	  for the connection, so that only one person will have the 
	  connection open when the child goes to do the FWFD */
	sleep(1);

	if ((gdpmisc.version = negotiate(fd,&mypasswd[0],RFS_CLIENT))<0) {
		(void) fprintf(stderr,"mount: negotiations failed\n");
		(void) fprintf(stderr,"mount: possible cause: machine password incorrect\n");
		t_cleanup(fd);
		return(-1);
	}
	gdpmisc.hetero = ndata.n_hetero;
	if (ioctl(fd, I_LOOK, modname) >= 0) {
		if (strcmp(modname, TIMOD) == 0)
			if (ioctl(fd, I_POP) < 0)
				perror("mount: warning");
	}
	if (rfsys(RF_FWFD, fd, token, &gdpmisc) <0) {
		perror("mount");
		(void) t_close(fd);
		return(-1);
	}
	return(0);
}

static
int
t_cleanup(fd)
int fd;
{
	(void) t_snddis(fd, (struct t_call *)NULL);
	(void) t_unbind(fd);
	(void) t_close(fd);
}

char *
findaddr(mach_name)
char *mach_name;
{
	char	*file[2];
	char	*f_name, *f_cmd, *f_addr;
	char	*netspec;
	FILE	*fd;
	int	i;
	char	abuf[BUFSIZ];
	static	char	retbuf[BUFSIZ];

	file[0] = NETMASTER;
	file[1] = DOMMASTER;

	/*
	 *	Create a string of the form "netspec machaddr"
	 *	and return that string or NULL if error.
	 */

	if ((netspec = getnetspec()) == NULL) {
		fprintf(stderr, "mount: cannot obtain network specification\n");
		return(NULL);
	}

	for (i = 0; i < 2; i ++) {
		if ((fd = fopen(file[i], "r")) == NULL)
			continue;
		while (fgets(abuf, sizeof(abuf), fd) != NULL) {
			f_name = strtok(abuf, " \t");
			f_cmd  = strtok(NULL, " \t");
			if ((strcmp(f_cmd, "a") == 0 || strcmp(f_cmd, "A") == 0)
			  && (strcmp(f_name, mach_name) == 0)) {
				strncpy(retbuf, netspec, sizeof(retbuf));
				strncat(retbuf, " ", sizeof(retbuf)-strlen(retbuf));
				if ((f_addr = strtok(NULL, "\n")) != NULL)
					strncat(retbuf, f_addr, sizeof(retbuf)-strlen(retbuf));
				fclose(fd);
				return(retbuf);
			}
		}
	}
	fclose(fd);
	return(NULL);
}

static
char *
getnetspec()
{
	static char netspec[BUFSIZ];
	FILE   *fp;

	if (((fp = fopen(NETSPEC, "r")) == NULL)
	 || (fgets(netspec, BUFSIZ, fp) == NULL))
		return(NULL);
	/*
	 *	get rid of training newline if present.
	 */
	if (netspec[strlen(netspec)-1] == '\n')
		netspec[strlen(netspec)-1] = '\0';

	fclose(fp);
	return(netspec);
}

/*
 * Return the value of a string option of the form foo=<string>, if
 * option is not found or is malformed, return 0.
 */
char *
sopt(mnt, opt)
	struct mntent *mnt;
	char *opt;
{
	char *ret = (char *) NULL;
	char *equal;
	char *tmp;
	char *tmp2;
	char *str;
	char *malloc();

	if (str = hasmntopt(mnt, opt)) {
		if (!(tmp = malloc(strlen(str))))
			return((char *) NULL);
		strcpy(tmp, str);
		if (equal = index(tmp, '=')) {
			ret = &equal[1];
			for (tmp2 = ret; *tmp2 && *tmp2 != ','; tmp2++);
			*tmp2 = '\0';
		} else
			fprintf(stderr, "mount_rfs: bad option '%s'\n", str);
	}
	return(ret);
}
