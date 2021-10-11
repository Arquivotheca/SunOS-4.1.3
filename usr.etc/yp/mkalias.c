/* @(#)mkalias.c 1.1 92/07/30 SMI
 *
 * mkmap - program to convert the mail.aliases map into an
 * inverse map of <user@host> back to <preferred-alias>
 */

# include <sys/file.h>
# include <ndbm.h>
# include <stdio.h>
# include <ctype.h>
# include <netdb.h>

# include "ypdefs.h"
USE_YP_PREFIX
USE_YP_MASTER_NAME
USE_YP_LAST_MODIFIED

char *index(), *rindex();

int Verbose = 0;	/* to get the gory details */
int UucpOK = 0;		/* pass all UUCP names right through */
int DomainOK = 0;	/* pass all Domain names (with dots) */
int ErrorCheck = 0;	/* check carefully for errors */
int NoOutput = 0;	/* no output, just do the check */
int Simple = 0;		/* Do not do the user name preference step */
int NameMode = 0;	/* Try to capitalize as names */

DBM *Indbm, *Scandbm, *Outdbm;

IsMailingList(s)
  {
    /* 
     * returns true if the given string is a mailing list
     */
     char *p;

     if ( index(s,',') ) return(1);
     if ( index(s,'|') ) return(1);
     p = index(s,':');
     if (p && strncmp(p,":include:")) return(1);
     return(0);
  }


IsQualified(s,p,h)
     char *s;  /* input string */
     char *p;  /* output: user part */
     char *h;  /* output: host part */
  {
    /* 
     * returns true if the given string is qualified with a host name
     */
     register char *middle;

     middle = index(s,'@');
     if ( middle ) {
         for (middle=s; *middle != '@'; *p++ = *middle++) continue;
	 *p = '\0';
	 CopyName(h, middle+1);
         return(1);
       }
     middle = rindex(s,'!');
     if ( middle ) {
	 strcpy(p, middle+1);
	 *middle = '\0';
	 CopyName(h, s);
	 *middle = '!';
         return(1);
       }
     return(0);
  }


IsMaint(s)
    char *s;
  {
    /*
     * returns true if the given string is one of the maintenence
     * strings used in sendmail or NIS.
     */
    if (*s=='@') return(1);
    if (strncmp(s, yp_prefix, yp_prefix_sz)==0) return(1);
    return(0);
  }

CopyName(dst, src)
    register char *dst, *src;
  {
     /*
      * copy a string, but ignore white space
      */
     while (*src) {
	if (isspace(*src))
		src++;
	else
		*dst++ = *src++;
     }
     *dst = '\0';
  }

Compare(s1, s2)
    register char *s1, *s2;
  {
     /*
      * compare strings, but ignore white space
      */
     while (*s1 != '\0' && isspace(*s1)) s1++;
     while (*s2 != '\0' && isspace(*s2)) s2++;
     return(strcmp(s1,s2));
  }


ProcessMap()
  {
    datum key, value, part, partvalue;
    char address[PBLKSIZ];	/* qualified version */
    char user[PBLKSIZ];		/* unqualified version */
    char userpart[PBLKSIZ];	/* unqualified part of qualified addr. */
    char hostpart[PBLKSIZ];	/* rest of qualified addr. */
    
    for (key = dbm_firstkey(Scandbm); key.dptr != NULL;
      key = dbm_nextkey(Scandbm)) {
	value = dbm_fetch(Indbm, key);
	CopyName(address, value.dptr);
	CopyName(user, key.dptr);
	if (address == NULL) continue;
	if (IsMailingList(address)) continue;
	if (!IsQualified(address, userpart, hostpart)) continue;
	if (IsMaint(user)) continue;
	if (ErrorCheck && HostCheck(hostpart, address)) {
		printf("Invalid host %s in %s:%s\n", hostpart, user, address);
		continue;
	}
	part.dptr = userpart;
	part.dsize = strlen(userpart) + 1;
	if (Simple)
		partvalue.dptr = NULL;
	else
		partvalue = dbm_fetch(Indbm, part);
	value.dptr = address;
	value.dsize = strlen(address) + 1;
	if (partvalue.dptr != NULL && Compare(partvalue.dptr,user)==0) {
	    if (NameMode)
	    	DoName(userpart);
	    if (!NoOutput)
		dbm_store(Outdbm, value, part, DBM_REPLACE);
	    if (Verbose) printf("%s --> %s --> %s\n", 
	    	userpart, user, address);
	  }
	else {
	    if (NameMode)
		    	DoName(user);
	    key.dptr = user;
	    key.dsize = strlen(user) + 1;
	    if (!NoOutput)
		dbm_store(Outdbm, value, key, DBM_REPLACE);
	    if (Verbose) printf("%s --> %s\n", user, address);
	  }
    }
  }


/*
 * Returns true if this is an invalid host
 */
HostCheck(h, a)
    char *h;		/* host name to check */
    char *a;		/* full name */
{
    struct hostent *hp;
    
    if (DomainOK && index(a,'.'))
    	return(0);

    if (UucpOK && index(a,'!'))
    	return(0);

    hp = gethostbyname(h);
    return (hp == NULL);
}

/*
 * Apply some Heurisitcs to upper case-ify the name
 * If it has a dot in it.
 */
DoName(cp)
    register char *cp;
  {
    if (index(cp,'.') == NULL)
    	return;

    while (*cp) {
	UpperCase(cp);
	while (*cp && *cp != '-' && *cp != '.') 
		cp++;
	if (*cp)
		cp++;	/* skip past punctuation */
    }
  }

/*
 * upper cases one name - stops at a . 
 */
UpperCase(cp)
    char *cp;
{
    register ch = cp[0];

    if (isupper(ch))
	ch = tolower(ch);

    if (ch == 'f' && cp[1] == 'f') 
    	return; /* handle ff */

    if (ch == 'm' && cp[1] == 'c' && islower(cp[2])) 
	cp[2] = toupper(cp[2]);
    if (islower(ch)) 
	cp[0] = toupper(ch);
}



AddYPEntries()
  {
    datum key, value;
    char last_modified[PBLKSIZ];
    char host_name[PBLKSIZ];
    long now;
	    /*
	     * Add the special NIS entries. 
	     */
    key.dptr = yp_last_modified;
    key.dsize = yp_last_modified_sz;
    time(&now);
    sprintf(last_modified,"%10.10d",now);
    value.dptr = last_modified;
    value.dsize = strlen(value.dptr);
    dbm_store(Outdbm, key, value);

    key.dptr = yp_master_name;
    key.dsize = yp_master_name_sz;
    gethostname(host_name, sizeof(host_name) );
    value.dptr = host_name;
    value.dsize = strlen(value.dptr);
    dbm_store(Outdbm, key, value);
  }

main(argc, argv)
	int argc;
	char **argv;
{
    while (argc > 1 && argv[1][0] == '-') {
        switch (argv[1][1]) {
	  case 'v':
	  	Verbose = 1;
		break;

	  case 'u':
	  	UucpOK = 1;
		break;

	  case 'd':
	  	DomainOK = 1;
		break;

	  case 'e':
	  	ErrorCheck = 1;
		break;

	  case 's':
	  	Simple = 1;
		break;

	  case 'n':
	  	NameMode = 1;
		break;

	  default:
	  	printf("Unknown option %c\n", argv[1][1]);
		break;
	}
	argc--; argv++;
      }    
    if (argc < 2) {
      printf("Usage: mkalias [-e] [-v] [-u] [-d] [-s] [-n] <input> <output>\n");
      exit(1);
    }
    Indbm = dbm_open(argv[1], O_RDONLY, 0);
    if (Indbm==NULL) {
      printf("Unable to open input database %s\n", argv[1]);
      exit(1);
    }
    Scandbm = dbm_open(argv[1], O_RDONLY, 0);
    if (Scandbm==NULL) {
      printf("Unable to open input database %s\n", argv[1]);
      exit(1);
    }
    if (argv[2] == NULL)
    	NoOutput = 1;
    else {
	Outdbm = dbm_open(argv[2], O_RDWR|O_CREAT|O_TRUNC, 0644);
	if (Outdbm==NULL) {
	    printf("Unable to open output database %s\n", argv[2]);
	    exit(1);
	}
    }
    ProcessMap();
    dbm_close(Indbm);
    dbm_close(Scandbm);
    if (!NoOutput) {
	AddYPEntries();
    	dbm_close(Outdbm);
    }
    exit(0);
    /* NOTREACHED */
}
