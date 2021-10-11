/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *  Sendmail
 *  Copyright (c) 1983  Eric P. Allman
 *  Berkeley, California
 */

# include <pwd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <signal.h>
# include <errno.h>
# include "sendmail.h"
# include <sys/file.h>

SCCSID(@(#)alias.c 1.1 92/07/30 SMI); /* from UCB 5.15 4/1/88 */

/*
**  ALIAS -- Compute aliases.
**
**	Scans the alias file for an alias for the given address.
**	If found, it arranges to deliver to the alias list instead.
**	Uses libdbm database if -DDBM.
**
**	Parameters:
**		a -- address to alias.
**		sendq -- a pointer to the head of the send queue
**			to put the aliases in.
**
**	Returns:
**		none
**
**	Side Effects:
**		Aliases found are expanded.
**
**	Notes:
**		If NoAlias (the "-n" flag) is set, no aliasing is
**			done.
**
**	Deficiencies:
**		It should complain about names that are aliased to
**			nothing.
*/


#ifdef DBM
typedef struct
{
	char	*dptr;
	int	dsize;
} DATUM;
extern DATUM fetch();
#endif DBM

alias(a, sendq)
	register ADDRESS *a;
	ADDRESS **sendq;
{
	register char *p;
	extern char *aliaslookup(), *yellowlookup();

# ifdef DEBUG
	if (tTd(27, 1))
		printf("alias(%s)\n", a->q_paddr);
# endif

	/* don't realias already aliased names */
	if (bitset(QDONTSEND, a->q_flags))
		return;

	CurEnv->e_to = a->q_paddr;

	/*
	**  Look up this name
	*/

	if (NoAlias)
		p = NULL;
	else {
		p = aliaslookup(a->q_user);

		if (p == NULL)
			p = yellowlookup(a);
	}

	if (p == NULL)
		return;

	/*
	**  Match on Alias.
	**	Deliver to the target list.
	*/

# ifdef DEBUG
	if (tTd(27, 1))
		printf("%s (%s, %s) aliased to %s\n",
		    a->q_paddr, a->q_host, a->q_user, p);
# endif
	message(Arpa_Info, "aliased to %s", p);
	AliasLevel++;
	sendtolist(p, a, sendq);
	AliasLevel--;
}
/*
**  ALIASLOOKUP -- look up a name in the alias file.
**
**	Parameters:
**		name -- the name to look up.
**
**	Returns:
**		the value of name.
**		NULL if unknown.
**
**	Side Effects:
**		none.
**
**	Warnings:
**		The return value will be trashed across calls.
*/

char *
aliaslookup(name)
	char *name;
{
# ifdef DBM
	DATUM rhs, lhs;

	/* create a key for fetch */
	lhs.dptr = name;
	lhs.dsize = strlen(name) + 1;
	rhs = fetch(lhs);
	return (rhs.dptr);
# else DBM
	register STAB *s;

	s = stab(name, ST_ALIAS, ST_FIND);
	if (s == NULL)
		return (NULL);
	return (s->s_alias);
# endif DBM
}

char *ypDomain = NULL;

/*
**  YELLOWMATCH -- look up any token in the Network Information Services.
**
**	Parameters:
**		string - string to look up
**		mac - macro to use as name of map
**
**	Returns:
**		True if the value was found in the database
**
**	Side Effects:
**		If token is found, enter into cache.
**		Note the careful copying of the string, converting it
**		to lower case before doing the NIS lookup, and then converting
**		it back to original case in the symbol table.
**
**	Warnings:
**		Will hang forever if no NIS server responds
*/

yellowmatch(string,mac)
    char *string;
    char mac;
  {
  	int insize, outsize;
	register STAB *s;
	char *mapname, *result, *lowered, *macvalue();

	if (mac == 'y')
	  {
	    /*
	     * handle the special host name mapping macro.
	     */
	     struct hostinfo *h, *lookuphost();

	     h = lookuphost(string);
	     errno = 0;
	     return(h->h_exists && (h->h_down == 0 || h->h_errno != ENAMESER));
	  }
	mapname = macvalue(mac,CurEnv);
	if (mapname==NULL) return(FALSE);
	lowered = newstr(string);
	makelower(lowered);
	s = stab(lowered, ST_YP, ST_FIND);
	if (s != NULL) {
		if (bitnset(mac, s->s_value.sv_bitmap2.positive)) {
			free(lowered);
			return(TRUE);
		}
		if (bitnset(mac, s->s_value.sv_bitmap2.negative)) {
			free(lowered);
			return(FALSE);
		}
	}
	else s = stab(lowered, ST_YP, ST_ENTER);
	insize = strlen(lowered);
	if (ypDomain==NULL) {
		  yp_get_default_domain(&ypDomain);
		  if (ypDomain==NULL) return(FALSE);
	}
# ifdef DEBUG
	if (tTd(27, 1))
	    printf("NIS lookup of %s in map %s\n",lowered, mapname);
# endif DEBUG
	if (yp_match(ypDomain,mapname,lowered, insize, &result, &outsize)
	 && yp_match(ypDomain,mapname,lowered, insize+1, &result, &outsize))
	  {
		setbitn(mac, s->s_value.sv_bitmap2.negative);
	  	errno = 0;
		return(FALSE);
	  }
# ifdef DEBUG
	if (tTd(27, 1))
	    printf("NIS found %s as value for %s\n",result,lowered);
# endif DEBUG
	setbitn(mac, s->s_value.sv_bitmap2.positive);
	free(result);
	return(TRUE);
  }

/*
** MAPUSERNAME -- look up a string in the NIS with replacement
**
**	Parameters:
**		map -- name of the NIS map to use
**		buf -- input buffer
**		bufsize -- size of input buffer
**
**	Side Effects:
**		changes buffer to be its value in the map
*/
mapusername(map, buf, bufsize)
    char *map, *buf;
    int bufsize;
  {
  	int insize, outsize;
	char *result, *lowered;

	if (ypDomain==NULL)
		{
		  yp_get_default_domain(&ypDomain);
		  if (ypDomain==NULL) return;
		}
	lowered = newstr(buf);
	makelower(lowered);
	insize = strlen(lowered);
	if (yp_match(ypDomain, map, lowered, insize, &result, &outsize)
	 && yp_match(ypDomain, map, lowered, insize+1, &result, &outsize))
	  {
	  	errno = 0;
		free(lowered);
		return;
	  }
# ifdef DEBUG
	if (tTd(27, 1))
	    printf("NIS found %s as value for %s\n",result,buf);
# endif DEBUG
	free(lowered);
	if (outsize >= bufsize) outsize = bufsize - 1;
	strncpy(buf, result, outsize);
	buf[outsize] = '\0';
  }

/*
**  YELLOWLOOKUP -- look up a name in the NIS.
**
**	Parameters:
**		a -- the address to look up.
**
**	Returns:
**		the value of name.
**		NULL if unknown.
**
**	Side Effects:
**		sets 
**
**	Warnings:
**		The return value will be trashed across calls.
*/

char *
yellowlookup(a)
	register ADDRESS *a;
  {
	char *result;
	int insize, outsize;
	extern char *AliasMap;
	
		/*
		 * if we did not find a local alias, then
		 * try a remote alias through NIS.
		 */
	if (AliasMap==NULL || *AliasMap=='\0') return(NULL);

	if (ypDomain==NULL)
		{
		  yp_get_default_domain(&ypDomain);
		  if (ypDomain == NULL) return(NULL);
# ifdef DEBUG
		  if (tTd(27, 1))
	    		printf("NIS domain is %s\n",ypDomain);
# endif DEBUG
		}
	if (bitset(QWASLOCAL,a->q_flags)) return(NULL);
	insize = strlen(a->q_user);
	if (yp_match(ypDomain,AliasMap,a->q_user, insize, &result, &outsize) &&
	    yp_match(ypDomain,AliasMap,a->q_user, insize+1, &result, &outsize))
	  {
    	    errno = 0;
	    return(NULL);
	  }
  
# ifdef DEBUG
        if (tTd(27, 1))
		printf("%s maps to %s\n",a->q_user, result );
# endif DEBUG
	a->q_flags |= QDOMAIN;
	return(result);
  }
/*
**  INITALIASES -- initialize for aliasing
**
**	Very different depending on whether we are running DBM or not.
**
**	Parameters:
**		aliasfile -- location of aliases.
**		init -- if set and if DBM, initialize the DBM files.
**
**	Returns:
**		none.
**
**	Side Effects:
**		initializes aliases:
**		if DBM:  opens the database.
**		if ~DBM: reads the aliases into the symbol table.
*/

# define DBMMODE	0666

initaliases(aliasfile, init)
	char *aliasfile;
	bool init;
{
#ifdef DBM
	int atcnt;
	bool automatic = FALSE;
	char buf[MAXNAME];
#endif DBM
	time_t modtime;
	struct stat stb;
	static bool initialized = FALSE;

	if (initialized)
		return;
	initialized = TRUE;

	if (aliasfile == NULL || stat(aliasfile, &stb) < 0)
	{
		if (aliasfile != NULL && init)
			syserr("Cannot open %s", aliasfile);
		NoAlias = TRUE;
		errno = 0;
		return;
	}

# ifdef DBM
	/*
	**  Check to see that the alias file is complete.
	**	If not, we will assume that someone died, and it is up
	**	to us to rebuild it.
	*/

	if (!init)
		dbminit(aliasfile);
	atcnt = SafeAlias * 2;
	if (atcnt > 0)
	{
		while (!init && atcnt-- >= 0 && aliaslookup("@") == NULL)
		{
			/*
			**  Reinitialize alias file in case the new
			**  one is mv'ed in instead of cp'ed in.
			**
			**	Only works with new DBM -- old one will
			**	just consume file descriptors forever.
			**	If you have a dbmclose() it can be
			**	added before the sleep(30).
			*/
			dbmclose();
			sleep(30);
# ifdef NDBM
			dbminit(aliasfile);
# endif NDBM
		}
	}
	else
		atcnt = 1;

	/*
	**  See if the DBM version of the file is out of date with
	**  the text version.  If so, go into 'init' mode automatically.
	**	This only happens if our effective userid owns the DBM
	**	version or if the mode of the database is 666 -- this
	**	is an attempt to avoid protection problems.  Note the
	**	unpalatable hack to see if the stat succeeded.
	*/

	modtime = stb.st_mtime;
	(void) strcpy(buf, aliasfile);
	(void) strcat(buf, ".pag");
	stb.st_ino = 0;
	if (!init && (stat(buf, &stb) < 0 || stb.st_mtime < modtime || atcnt < 0))
	{
		errno = 0;
		if (AutoRebuild && stb.st_ino != 0 &&
		    ((stb.st_mode & 0777) == 0666 || stb.st_uid == geteuid()))
		{
			init = TRUE;
			automatic = TRUE;
			message(Arpa_Info, "rebuilding alias database");
#ifdef LOG
			if (LogLevel >= 7)
				syslog(LOG_INFO, "rebuilding alias database");
#endif LOG
		}
		else
		{
#ifdef LOG
			if (LogLevel >= 7)
				syslog(LOG_INFO, "alias database out of date");
#endif LOG
			message(Arpa_Info, "Warning: alias database out of date");
		}
	}


	/*
	**  If necessary, load the DBM file.
	**	If running without DBM, load the symbol table.
	*/

	if (init)
	{
#ifdef LOG
		if (LogLevel >= 6)
		{
			extern char *username();

			syslog(LOG_NOTICE, "alias database %srebuilt by %s",
				automatic ? "auto" : "", username());
		}
#endif LOG
		readaliases(aliasfile, TRUE, modtime);
	}
# else DBM
	readaliases(aliasfile, init, modtime);
# endif DBM
}
/*
**  READALIASES -- read and process the alias file.
**
**	This routine implements the part of initaliases that occurs
**	when we are not going to use the DBM stuff.
**
**	Parameters:
**		aliasfile -- the pathname of the alias file master.
**		init -- if set, initialize the DBM stuff.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Reads aliasfile into the symbol table.
**		Optionally, builds the .dir & .pag files.
*/

static
readaliases(aliasfile, init, modtime)
	char *aliasfile;
	bool init;
	time_t modtime;
{
	register char *p;
	char *rhs;
	bool skipping;
	int naliases, bytes, longest;
	FILE *af;
	void (*oldsigint)();
	ADDRESS al, bl;
	register STAB *s;
	char line[BUFSIZ];

	if ((af = fopen(aliasfile, "r")) == NULL)
	{
# ifdef DEBUG
		if (tTd(27, 1))
			printf("Can't open %s\n", aliasfile);
# endif
		errno = 0;
		NoAlias++;
		return;
	}

# ifdef DBM
	/* see if someone else is rebuilding the alias file already */
	if (flock(fileno(af), LOCK_EX | LOCK_NB) < 0 && errno == EWOULDBLOCK)
	{
		/* yes, they are -- wait until done and then return */
		message(Arpa_Info, "Alias file is already being rebuilt");
		if (OpMode != MD_INITALIAS)
		{
			/* wait for other rebuild to complete */
			(void) flock(fileno(af), LOCK_EX);
		}
		(void) fclose(af);
		errno = 0;
		return;
	}
# endif DBM

	/*
	**  If initializing, create the new DBM files, then reopen them
	**  in case they did not previously exist.
	*/

	if (init)
	{
		oldsigint = signal(SIGINT, SIG_IGN);
		(void) strcpy(line, aliasfile);
		(void) strcat(line, ".dir");
		if (close(creat(line, DBMMODE)) < 0)
		{
			syserr("cannot make %s", line);
			(void) signal(SIGINT, oldsigint);
			return;
		}
		(void) strcpy(line, aliasfile);
		(void) strcat(line, ".pag");
		if (close(creat(line, DBMMODE)) < 0)
		{
			syserr("cannot make %s", line);
			(void) signal(SIGINT, oldsigint);
			return;
		}
		if (dbminit(aliasfile)<0)
		{
			syserr("cannot open database %s",aliasfile);
			(void) signal(SIGINT, oldsigint);
			return;
		}
	}

	/*
	**  Read and interpret lines
	*/

	FileName = aliasfile;
	LineNumber = 0;
	naliases = bytes = longest = 0;
	skipping = FALSE;
	while (fgets(line, sizeof (line), af) != NULL)
	{
		int lhssize, rhssize;

		LineNumber++;
		p = index(line, '\n');
		if (p != NULL)
			*p = '\0';
		switch (line[0])
		{
		  case '#':
		  case '\0':
			skipping = FALSE;
			continue;

		  case ' ':
		  case '\t':
			if (!skipping)
				syserr("Non-continuation line starts with space");
			skipping = TRUE;
			continue;
		}
		skipping = FALSE;

		/*
		**  Process the LHS
		**	Find the final colon, and parse the address.
		**	It should resolve to a local name -- this will
		**	be checked later (we want to optionally do
		**	parsing of the RHS first to maximize error
		**	detection).
		*/

		for (p = line; *p != '\0' && *p != ':' && *p != '\n'; p++)
			continue;
		if (*p++ != ':')
		{
			syserr("missing colon");
			continue;
		}
		if (parseaddr(line, &al, 0, ':') == NULL)
		{
			syserr("illegal alias name");
			continue;
		}
		loweraddr(&al);

		/*
		**  Process the RHS.
		**	'al' is the internal form of the LHS address.
		**	'p' points to the text of the RHS.
		*/

		rhs = p;
		for (;;)
		{
			register char c;

			if (init && CheckAliases)
			{
				/* do parsing & compression of addresses */
				while (*p != '\0')
				{
					extern char *DelimChar;

					while (isspace(*p) || *p == ',')
						p++;
					if (*p == '\0')
						break;
					if (parseaddr(p, &bl, -1, ',') == NULL)
						usrerr("%s... bad address", p);
					p = DelimChar;
				}
			}
			else
			{
				p = &p[strlen(p)];
				if (p[-1] == '\n')
					*--p = '\0';
			}

			/* see if there should be a continuation line */
			c = fgetc(af);
			if (!feof(af))
				(void) ungetc(c, af);
			if (c != ' ' && c != '\t')
				break;

			/* read continuation line */
			if (fgets(p, sizeof line - (p - line), af) == NULL)
				break;
			LineNumber++;
		}
		if (al.q_mailer != LocalMailer)
		{
			syserr("cannot alias non-local names");
			continue;
		}

		/*
		**  Insert alias into symbol table or DBM file
		*/

		lhssize = strlen(al.q_user) + 1;
		rhssize = strlen(rhs) + 1;

# ifdef DBM
		if (init)
		{
			DATUM key, content;

			key.dsize = lhssize;
			key.dptr = al.q_user;
			content.dsize = rhssize;
			content.dptr = rhs;
			store(key, content);
			free(al.q_user);
			al.q_user = NULL;
		}
		else
# endif DBM
		{
			s = stab(al.q_user, ST_ALIAS, ST_ENTER);
			s->s_alias = newstr(rhs);
		}

		/* statistics */
		naliases++;
		bytes += lhssize + rhssize;
		if (rhssize > longest)
			longest = rhssize;
	}

# ifdef DBM
	if (init)
	{
		/* add the distinquished alias "@" */
		DATUM key, value;
		char last_modified[16];

		    /*
		     * Add the special NIS entries. We can do this
		     * without harm even if this host is not itself a NIS
		     * server.
		     */
		key.dptr = "YP_LAST_MODIFIED";
		key.dsize = strlen(key.dptr);
		sprintf(last_modified,"%10.10d",modtime);
		value.dptr = last_modified;
		value.dsize = strlen(value.dptr);
		store(key, value);

		key.dptr = "YP_MASTER_NAME";
		key.dsize = strlen(key.dptr);
		value.dptr = macvalue('w',CurEnv);
		value.dsize = strlen(value.dptr);
		store(key, value);

		key.dsize = 2;
		key.dptr = "@";
		store(key, key);

		/* restore the old signal */
		(void) signal(SIGINT, oldsigint);
	}
# endif DBM

	/* closing the alias file drops the lock */
	(void) fclose(af);
	CurEnv->e_to = NULL;
	FileName = NULL;
	message(Arpa_Info, "%d aliases, longest %d bytes, %d bytes total",
			naliases, longest, bytes);
# ifdef LOG
	if (LogLevel >= 8)
		syslog(LOG_INFO, "%d aliases, longest %d bytes, %d bytes total",
			naliases, longest, bytes);
# endif LOG
}
/*
**  FORWARD -- Try to forward mail
**
**	This is similar but not identical to aliasing.
**
**	Parameters:
**		user -- the name of the user who's mail we would like
**			to forward to.  It must have been verified --
**			i.e., the q_home field must have been filled
**			in.
**		sendq -- a pointer to the head of the send queue to
**			put this user's aliases in.
**
**	Returns:
**		none.
**
**	Side Effects:
**		New names are added to send queues.
*/

forward(user, sendq)
	ADDRESS *user;
	ADDRESS **sendq;
{
	char buf[60];
	extern bool safefile();

# ifdef DEBUG
	if (tTd(27, 1))
		printf("forward(%s)\n", user->q_paddr);
# endif DEBUG

	if (user->q_mailer != LocalMailer || bitset(QBADADDR, user->q_flags))
		return;
# ifdef DEBUG
	if (user->q_home == NULL)
		syserr("forward: no home");
# endif DEBUG

	/* good address -- look for .forward file in home */
	define('z', user->q_home, CurEnv);
	expand("\001z/.forward", buf, &buf[sizeof buf - 1], CurEnv);
	if (!safefile(buf, user->q_uid, S_IREAD))
		return;

	/* we do have an address to forward to -- do it */
	include(buf, "forwarding", user, sendq);
}
