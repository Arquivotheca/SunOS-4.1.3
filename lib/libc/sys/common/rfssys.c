#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)rfssys.c 1.1 92/07/30 SMI";
#endif

#include	<syscall.h>
#include	<varargs.h>
#include	<sys/errno.h>
#include	<sys/types.h>

/* RFS system calls */

/*  
 *  opcodes for rfsys system calls.
 *  Note: These are also defined in /usr/include/rfs/rfsys.h
 *  for unbundling reasons, and the definitions must match.
 */

#define RF_FUMOUNT	1	/* forced unmount */
#define RF_SENDUMSG	2	/* send buffer to remote user-level */
#define RF_GETUMSG	3	/* wait for buffer from remote user-level */
#define RF_LASTUMSG	4	/* wakeup from GETUMSG */
#define RF_SETDNAME	5	/* set domain name */
#define RF_GETDNAME	6	/* get domain name */
#define RF_SETIDMAP	7	/* Load a uid/gid map */
#define RF_FWFD		8 	/* Stuff a TLI circuit into the kernel */
#define RF_VFLAG	9	/* Handle verification option */
#define RF_DISCONN	10	/* return value for link down */
#define RF_VERSION	11 	/* Handle version information */
#define RF_RUNSTATE	12	/* See if RFS is running */
#define RF_ADVFS	13	/* Advertise a resource */
#define RF_UNADVFS	14 	/* Unadvertise a resource */
#define RF_RFSTART	15	/* Start up RFS */
#define RF_RFSTOP	16 	/* Stop RFS */
#define RF_MISC		17 	/* Miscellaneous -- reserved */

extern errno;

advfs(fs, svcnm, rwflag, clist)
char	*fs;		/* root of file system */
char	*svcnm;		/* global name given to name server */
int	rwflag;		/* readonly/read write flag	*/
char	**clist;	/* client list			*/
{
	return(syscall(SYS_rfssys, RF_ADVFS, fs, svcnm, rwflag, clist));
}

unadvfs(svcnm)
char	*svcnm;		/* global name given to name server */
{
	return(syscall(SYS_rfssys, RF_UNADVFS, svcnm));
}

rfstart()
{
	return(syscall(SYS_rfssys, RF_RFSTART));
}

rfstop()
{
	return(syscall(SYS_rfssys, RF_RFSTOP));
}

rfsys(va_alist)
va_dcl
{
	va_list ap;
	int opcode;

	va_start(ap);
	opcode = va_arg(ap, int);
	va_end(ap);

	switch (opcode) {
	case RF_FUMOUNT: {
		char *resource;

		va_start(ap);
		opcode = va_arg(ap, int);
		resource = va_arg(ap, char *);
		va_end(ap);
		return(syscall(SYS_rfssys, opcode, resource));
	}
	case RF_SENDUMSG: {
		int cl_sysid;
		char *buf;
		unsigned size;

		va_start(ap);
		opcode = va_arg(ap, int);
		cl_sysid = va_arg(ap, int);
		buf = va_arg(ap, char *);
		size = va_arg(ap, unsigned);
		va_end(ap);
		return(syscall(SYS_rfssys, opcode, cl_sysid, buf, size));
	}
	case RF_GETUMSG: {
		char *buf;
		unsigned size;

		va_start(ap);
		opcode = va_arg(ap, int);
		buf = va_arg(ap, char *);
		size = va_arg(ap, unsigned);
		va_end(ap);
		return(syscall(SYS_rfssys, opcode, buf, size));
	}
	case RF_LASTUMSG:
	case RF_RUNSTATE:
		return(syscall(SYS_rfssys, opcode));
	case RF_FWFD: {
		int fd;
		char *token;
		char *gdpmisc;

		va_start(ap);
		opcode = va_arg(ap, int);
		fd = va_arg(ap, int);
		token = va_arg(ap, char *);
		gdpmisc = va_arg(ap, char *);
		va_end(ap);
		return(syscall(SYS_rfssys, opcode, fd, token, gdpmisc));
	}
	case RF_SETDNAME: {
		char *dname;
		int size;

		va_start(ap);
		opcode = va_arg(ap, int);
		dname = va_arg(ap, char *);
		size = va_arg(ap, int); 
		va_end(ap);
		return(syscall(SYS_rfssys, opcode, dname, size));
	}
	case RF_GETDNAME: {
		char *dname;
		int size;

		va_start(ap);
		opcode = va_arg(ap, int);
		dname = va_arg(ap, char *);
		size = va_arg(ap, int); 
		va_end(ap);
		return(syscall(SYS_rfssys, opcode, dname, size));
	}
	case RF_SETIDMAP: {
		char *name;
		int flag;
		char *map;

		va_start(ap);
		opcode = va_arg(ap, int);
		name = va_arg(ap, char *);
		flag = va_arg(ap, int); 
		map = va_arg(ap, char *); 
		va_end(ap);
		return(syscall(SYS_rfssys, opcode, name, flag, map));
	}
	case RF_VFLAG: {
		int vcode;

		va_start(ap);
		opcode = va_arg(ap, int);
		vcode = va_arg(ap, int);
		va_end(ap);
		return(syscall(SYS_rfssys, opcode, vcode));
	}
	case RF_VERSION: {
		int vcode;
		int *vhigh;
		int *vlow;

		va_start(ap);
		opcode = va_arg(ap, int);
		vcode = va_arg(ap, int);
		vhigh = va_arg(ap, int *);
		vlow = va_arg(ap, int *);
		va_end(ap);
		return(syscall(SYS_rfssys, opcode, vcode, vhigh, vlow));
	}
	case RF_MISC: {
		int subcode;
		caddr_t args;

		va_start(ap);
		opcode = va_arg(ap, int);
		subcode = va_arg(ap, int);
		args = va_arg(ap, caddr_t);
		va_end(ap);
		return(syscall(SYS_rfssys, opcode, subcode, args));
	}
	default:
		errno = EINVAL;
		return(-1);
	}
}
