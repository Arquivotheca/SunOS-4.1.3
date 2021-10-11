#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)mntent.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <ctype.h>
#include <mntent.h>
#include <sys/file.h>

static	struct mntent *mntp;
char	*calloc();

struct mntent *
_mnt()
{

	if (mntp == 0)
		mntp = (struct mntent *)calloc(1, sizeof (struct mntent));
	return (mntp);
}

static char *
mntstr(p)
	register char **p;
{
	unsigned char *cp = (unsigned char *) *p;
	unsigned char *retstr;

	while (*cp && isspace(*cp))
		cp++;
	retstr = cp;
	while (*cp && !isspace(*cp))
		cp++;
	if (*cp) {
		*cp = '\0';
		cp++;
	}
	*p = (char *) cp;
	return ((char *)retstr);
}

static int
mntdigit(p)
	register char **p;
{
	register int value = 0;
	unsigned char *cp = (unsigned char *) *p;

	while (*cp && isspace(*cp))
		cp++;
	for (; *cp && isdigit(*cp); cp++) {
		value *= 10;
		value += *cp - '0';
	}
	while (*cp && !isspace(*cp))
		cp++;
	if (*cp) {
		*cp = '\0';
		cp++;
	}
	*p =  (char *) cp;
	return (value);
}

static
mnttabscan(mnttabp, mnt)
	FILE *mnttabp;
	struct mntent *mnt;
{
	static	char *line = NULL;
	char *cp;

	if (line == NULL)
		line = (char *)malloc(BUFSIZ+1);
	do {
		cp = fgets(line, BUFSIZ, mnttabp);
		if (cp == NULL) {
			return (EOF);
		}
	} while (*cp == '#');
	mnt->mnt_fsname = mntstr(&cp);
	if (*cp == '\0')
		return (1);
	mnt->mnt_dir = mntstr(&cp);
	if (*cp == '\0')
		return (2);
	mnt->mnt_type = mntstr(&cp);
	if (*cp == '\0')
		return (3);
	mnt->mnt_opts = mntstr(&cp);
	if (*cp == '\0')
		return (4);
	mnt->mnt_freq = mntdigit(&cp);
	if (*cp == '\0')
		return (5);
	mnt->mnt_passno = mntdigit(&cp);
	return (6);
}
	
FILE *
setmntent(fname, flag)
	char *fname;
	char *flag;
{
	FILE *mnttabp;

	if ((mnttabp = fopen(fname, flag)) == NULL) {
		return (NULL);
	}
	for (; *flag ; flag++) {
		if (*flag == 'w' || *flag == 'a' || *flag == '+') {
			if (flock(fileno(mnttabp), LOCK_EX) < 0) {
				fclose(mnttabp);
				return (NULL);
			}
			break;
		}
	}
	return (mnttabp);
}

int
endmntent(mnttabp)
	FILE *mnttabp;
{

	if (mnttabp) {
		fclose(mnttabp);
	}
	return (1);
}

struct mntent *
getmntent(mnttabp)
	FILE *mnttabp;
{
	int nfields;

	if (mnttabp == 0)
		return ((struct mntent *)0);
	if (_mnt() == 0)
		return ((struct mntent *)0);
	nfields = mnttabscan(mnttabp, mntp);
	if (nfields == EOF || nfields != 6)
		return ((struct mntent *)0);
	return (mntp);
}

addmntent(mnttabp, mnt)
	FILE *mnttabp;
	register struct mntent *mnt;
{
	if (fseek(mnttabp, 0L, 2) < 0)
		return (1);
	if (mnt == (struct mntent *)0)
		return (1);
	if (mnt->mnt_fsname == NULL || mnt->mnt_dir  == NULL ||
	    mnt->mnt_type   == NULL || mnt->mnt_opts == NULL)
		return (1);

	mntprtent(mnttabp, mnt);
	return (0);
}

static char *
mntopt(p)
	char **p;
{
	unsigned char *cp = (unsigned char *) *p;
	unsigned char *retstr;

	while (*cp && isspace(*cp))
		cp++;
	retstr = cp;
	while (*cp && *cp != ',')
		cp++;
	if (*cp) {
		*cp = '\0';
		cp++;
	}
	*p =  (char *) cp;
	return ((char *)retstr);
}

char *
hasmntopt(mnt, opt)
	register struct mntent *mnt;
	register char *opt;
{
	char *f, *opts;
	static char *tmpopts;

	if (tmpopts == 0) {
		tmpopts = (char *)calloc(256, sizeof (char));
		if (tmpopts == 0)
			return (0);
	}
	strcpy(tmpopts, mnt->mnt_opts);
	opts = tmpopts;
	f = mntopt(&opts);
	for (; *f; f = mntopt(&opts)) {
		if (strncmp(opt, f, strlen(opt)) == 0)
			return (f - tmpopts + mnt->mnt_opts);
	} 
	return (NULL);
}

static
mntprtent(mnttabp, mnt)
	FILE *mnttabp;
	register struct mntent *mnt;
{
	fprintf(mnttabp, "%s %s %s %s %d %d\n",
	    mnt->mnt_fsname,
	    mnt->mnt_dir,
	    mnt->mnt_type,
	    mnt->mnt_opts,
	    mnt->mnt_freq,
	    mnt->mnt_passno);
	return(0);
}
