#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)fgetpwaent.c 1.1 92/07/30 Copyr 1990 Sun Micro"; /* c2 secure */
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <pwdadj.h>


extern int strcmp();
extern int strlen();
extern char *strncpy();

static struct passwd_adjunct *interpret();


struct passwd_adjunct *
fgetpwaent(f)
	FILE *f;
{
	char line1[BUFSIZ+1];

	if(fgets(line1, BUFSIZ, f) == NULL)
		return(NULL);
	return (interpret(line1, strlen(line1)));
}

static char *
pwskip(p)
	register char *p;
{
	while(*p && *p != ':' && *p != '\n')
		++p;
	if (*p == '\n')
		*p = '\0';
	else if (*p != '\0')
		*p++ = '\0';
	return(p);
}

static struct passwd_adjunct *
interpret(val, len)
	char *val;
{
	register char *p;
	static struct passwd_adjunct *pwadjp;
	static char *line;
	char *field;

	if (pwadjp == 0) {
		pwadjp = (struct passwd_adjunct *)calloc(1,sizeof(struct passwd_adjunct));
		if (pwadjp == 0)
			return (0);
	}
	if (line == NULL) {
		line = (char *)calloc(1,BUFSIZ+1);
		if (line == 0)
			return (0);
	}
	(void) strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	line[len+1] = 0;

	pwadjp->pwa_name = p;
	p = pwskip(p);
	if (strcmp(pwadjp->pwa_name, "+") == 0) {
		/* we are going to the NIS -- fix the
		 * rest of the struct as much as is needed
		 */
		pwadjp->pwa_passwd = "";
		return (pwadjp);
	}
	pwadjp->pwa_passwd = p;
	p = pwskip(p);
	field = p;
	p = pwskip(p);
	/*
	 * Zero labels since they are meaningless in this release.
	 */
	bzero((char *)&pwadjp->pwa_minimum, sizeof (blabel_t));
	field = p;
	p = pwskip(p);
	bzero((char *)&pwadjp->pwa_maximum, sizeof (blabel_t));
	field = p;
	p = pwskip(p);
	bzero((char *)&pwadjp->pwa_def, sizeof (blabel_t));
	field = p;
	p = pwskip(p);
	pwadjp->pwa_au_always.as_success = 0;
	pwadjp->pwa_au_always.as_failure = 0;
	if (getauditflagsbin(field, &pwadjp->pwa_au_always) != 0)
		return NULL;
	field = p;
	while(*p && *p != '\n') p++;
	*p = '\0';
	pwadjp->pwa_au_never.as_success = 0;
	pwadjp->pwa_au_never.as_failure = 0;
	if (getauditflagsbin(field, &pwadjp->pwa_au_never) != 0)
		return NULL;
	return(pwadjp);
}
