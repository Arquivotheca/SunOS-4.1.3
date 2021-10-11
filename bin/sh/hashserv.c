/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)hashserv.c 1.1 92/07/30 SMI"; /* from S5R3 1.10 */
#endif

/*
 *	UNIX shell
 */

#include	"hash.h"
#include	"defs.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>

static char	cost;
static int	dotpath;
static int	multrel;
static struct entry	relcmd;

int		argpath();

short
pathlook(com, flg, arg)
	char	*com;
	int		flg;
	register struct argnod	*arg;
{
	register char	*name = com;
	register ENTRY	*h;

	ENTRY		hentry;
	int		count = 0;
	int		i;
	int		pathset = 0;
	int		oldpath = 0;
	struct namnod	*n;


	hentry.data = 0;

	if (any('/', name))
		return(COMMAND);

	h = hfind(name);

	if (h)
	{
		if (h->data & (BUILTIN | FUNCTION))
		{
			if (flg)
				h->hits++;
			return(h->data);
		}

		if (arg && (pathset = argpath(arg)))
			return(PATH_COMMAND);

		if ((h->data & DOT_COMMAND) == DOT_COMMAND)
		{
			if (multrel == 0 && hashdata(h->data) > dotpath)
				oldpath = hashdata(h->data);
			else
				oldpath = dotpath;

			h->data = 0;
			goto pathsrch;
		}

		if (h->data & (COMMAND | REL_COMMAND))
		{
			if (flg)
				h->hits++;
			return(h->data);
		}

		h->data = 0;
		h->cost = 0;
	}

	if (i = syslook(name, commands, no_commands))
	{
		hentry.data = (BUILTIN | i);
		count = 1;
	}
	else
	{
		if (arg && (pathset = argpath(arg)))
			return(PATH_COMMAND);
pathsrch:
			count = findpath(name, oldpath);
	}

	if (count > 0)
	{
		if (h == 0)
		{
			hentry.cost = 0;
			hentry.key = make(name);
			h = henter(hentry);
		}

		if (h->data == 0)
		{
			if (count < dotpath)
				h->data = COMMAND | count;
			else
			{
				h->data = REL_COMMAND | count;
				h->next = relcmd.next;
				relcmd.next = h;
			}
		}


		h->hits = flg;
		h->cost += cost;
		return(h->data);
	}
	else 
	{
		return(-count);
	}
}
			

static void
zapentry(h)
	ENTRY *h;
{
	h->data &= HASHZAP;
}

void
zaphash()
{
	hscan(zapentry);
	relcmd.next = 0;
}

void 
zapcd()
{
	ENTRY *ptr = relcmd.next;
	
	while (ptr)
	{
		ptr->data |= CDMARK;
		ptr = ptr->next;
	}
	relcmd.next = 0;
}


static void
hashout(h)
	ENTRY *h;
{
	sigchk();

	if (hashtype(h->data) == NOTFOUND)
		return;

	if (h->data & (BUILTIN | FUNCTION))
		return;

	prn_buff(h->hits);

	if (h->data & REL_COMMAND)
		prc_buff('*');


	prc_buff(TAB);
	prn_buff(h->cost);
	prc_buff(TAB);

	pr_path(h->key, hashdata(h->data));
	prc_buff(NL);
}

void
hashpr()
{
	prs_buff("hits	cost	command\n");
	hscan(hashout);
}


set_dotpath()
{
	register char	*path;
	register int	cnt = 1;

	dotpath = 10000;
	path = getpath("");

	while (path && *path)
	{
		if (*path == '/')
			cnt++;
		else
		{
			if (dotpath == 10000)
				dotpath = cnt;
			else
			{
				multrel = 1;
				return;
			}
		}
	
		path = nextpath(path);
	}

	multrel = 0;
}


hash_func(name)
	char *name;
{
	ENTRY	*h;
	ENTRY	hentry;

	h = hfind(name);

	if (h)
	{

		if (h->data & (BUILTIN | FUNCTION))
			return;
		else
			h->data = FUNCTION;
	}
	else
	{
		int i;

		if (i = syslook(name, commands, no_commands))
			hentry.data = (BUILTIN | i);
		else
			hentry.data = FUNCTION;

		hentry.key = make(name);
		hentry.cost = 0;
		hentry.hits = 0;
	
		henter(hentry);
	}
}

func_unhash(name)
	char *name;
{
	ENTRY 	*h;

	h = hfind(name);

	if (h && (h->data & FUNCTION))
		h->data = NOTFOUND;
}


short
hash_cmd(name)
	char *name;
{
	ENTRY	*h;

	if (any('/', name))
		return(COMMAND);

	h = hfind(name);

	if (h)
	{
		if (h->data & (BUILTIN | FUNCTION))
			return(h->data);
		else if ((h->data & REL_COMMAND) == REL_COMMAND)
		{ /* unlink h from relative command list */
			ENTRY *ptr = &relcmd;
			while(ptr-> next != h)
				ptr = ptr->next;
			ptr->next = h->next;
		}
		zapentry(h);
	}

	return(pathlook(name, 0, 0));
}


what_is_path(name)
	register char *name;
{
	register ENTRY	*h;
	int		cnt;
	short	hashval;

	h = hfind(name);

	prs_buff(name);
	if (h)
	{
		hashval = hashdata(h->data);

		switch (hashtype(h->data))
		{
			case BUILTIN:
				prs_buff(" is a shell builtin\n");
				return;
	
			case FUNCTION:
			{
				struct namnod *n = lookup(name);

				prs_buff(" is a function\n");
				prs_buff(name);
				prs_buff("(){\n");
				prf(n->namenv);
				prs_buff("\n}\n");
				return;
			}
	
			case REL_COMMAND:
			{
				short hash;

				if ((h->data & DOT_COMMAND) == DOT_COMMAND)
				{
					hash = pathlook(name, 0, 0);
					if (hashtype(hash) == NOTFOUND)
					{
						prs_buff(" not found\n");
						return;
					}
					else
						hashval = hashdata(hash);
				}
			}

			case COMMAND:					
				prs_buff(" is hashed (");
				pr_path(name, hashval);
				prs_buff(")\n");
				return;
		}
	}

	if (syslook(name, commands, no_commands))
	{
		prs_buff(" is a shell builtin\n");
		return;
	}

	if ((cnt = findpath(name, 0)) > 0)
	{
		prs_buff(" is ");
		pr_path(name, cnt);
		prc_buff(NL);
	}
	else
		prs_buff(" not found\n");
}


findpath(name, oldpath)
	register char *name;
	int oldpath;
{
	register char 	*path;
	register int	count = 1;

	char	*p;
	int	ok = 1;
	int 	e_code = 1;
	
	cost = 0;
	path = getpath(name);

	if (oldpath)
	{
		count = dotpath;
		while (--count)
			path = nextpath(path);

		if (oldpath > dotpath)
		{
			catpath(path, name);
			p = curstak();
			cost = 1;

			if (chk_access(p, S_IEXEC, 1) == 0)
				return(dotpath);
			else
				return(oldpath);
		}
		else 
			count = dotpath;
	}

	while (path)
	{
		path = catpath(path, name);
		cost++;
		p = curstak();

		if ((ok = chk_access(p, S_IEXEC, 1)) == 0)
			break;
		else
			e_code = max(e_code, ok);

		count++;
	}

	return(ok ? -e_code : count);
}

/*
 * Determine if file given by name is accessible with permissions
 * given by mode.  Return 0 if it's accessible, 3 if the permissions don't
 * allow access, or 1 if some other error occurred.
 *
 * Regflag argument non-zero means that the test is for whether an "exec" will
 * really succeed; as such, a non-regular file gets an error return of 2, and
 * a file with none of its "exec" bits set gets an error return of 3.
 *
 * The checks should be done using the effective user and group ID, not the
 * real user and group ID, when this is being used to do command hashing,
 * since the command will be run with the effective user's and group's
 * permissions.  This is done by temporarily switching the real and effective
 * user IDs, and the effective user and group IDs if they differ, and switching
 * them back when done.
 *
 * This routine could be used by the "test" command, if it is decided that the
 * "test" commands should use the effective user and group IDs (because the
 * work done by a shell script will generally be done with the effective user's
 * and group's permissions).  For now, we don't use it for that, for
 * compatibility with older shells.
 */

chk_access(name, mode, regflag)
register char	*name;
int mode, regflag;
{	
	static int flag;
	static int ruid;
	static int euid;
	static int rgid; 
	static int egid;
	struct stat statb;
	register int retval;

	if(flag == 0) {
		ruid = getuid();
		euid = geteuid();
		rgid = getgid();
		egid = getegid();
		flag = 1;
	}
	if (ruid != euid)
		(void) setreuid(euid, ruid);
	if (rgid != egid)
		(void) setregid(egid, rgid);
	switch (mode) {

	case S_IREAD:
		if (access(name, 4) == -1)
			retval = (errno == EACCES ? 3 : 1);
		else
			retval = 0;
		break;

	case S_IWRITE:
		if (access(name, 2) == -1)
			retval = (errno == EACCES ? 3 : 1);
		else
			retval = 0;
		break;

	case S_IEXEC:
		if (access(name, 1) == -1)
			retval = (errno == EACCES ? 3 : 1);
		else {
			if (regflag) {
				if (stat(name, &statb) == 0) {
					if ((statb.st_mode & S_IFMT) != S_IFREG)
						retval = 2;
					else if ((statb.st_mode &
					    (S_IEXEC|(S_IEXEC>>3)|(S_IEXEC>>6))) == 0)
						retval = 3;
					else
						retval = 0;
				} else
					retval = (errno == EACCES ? 3 : 1);
			}
		}
		break;
	}
	if (ruid != euid)
		(void) setreuid(ruid, euid);
	if (rgid != egid)
		(void) setregid(rgid, egid);
	return(retval);
}


pr_path(name, count)
	register char	*name;
	int count;
{
	register char	*path;

	path = getpath(name);

	while (--count && path)
		path = nextpath(path, name);

	catpath(path, name);
	prs_buff(curstak());
}


static
argpath(arg)
	register struct argnod	*arg;
{
	register char 	*s;
	register char	*start;

	while (arg)
	{
		s = arg->argval;
		start = s;

		if (letter(*s))		
		{
			while (alphanum(*s))
				s++;

			if (*s == '=')
			{
				*s = 0;

				if (eq(start, pathname))
				{
					*s = '=';
					return(1);
				}
				else
					*s = '=';
			}
		}
		arg = arg->argnxt;
	}

	return(0);
}
