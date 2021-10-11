#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)getgraent.c 1.1 92/07/30 Copyr 1990 Sun Micro"; /* c2 secure */
#endif

#include <stdio.h>
#include <grp.h>
#include <grpadj.h>
#include <rpcsvc/ypclnt.h>

extern void rewind();
extern long strtol();
extern int strlen();
extern int strcmp();
extern int fclose();
extern char *strcpy();
extern char *calloc();
extern char *malloc();

void setgraent(), endgraent();

#define MAXINT 0x7fffffff;
#define	MAXGRP	200

static struct gradata {
	char	*domain;
	FILE	*grfa;
	char	*yp;
	int	yplen;
	char	*oldyp;
	int	oldyplen;
	struct list {
		char *name;
		struct list *nxt;
	} *minuslist;			/* list of - items */
	struct	group_adjunct interpgra;
	char	interpline[BUFSIZ+1];
	struct	group_adjunct *sv;
} *gradata, *_gradata();

static char *GROUPADJ = "/etc/security/group.adjunct";
static	struct group_adjunct *interpret();
static	struct group_adjunct *interpretwithsave();
static	struct group_adjunct *save();
static	struct group_adjunct *getnamefromyellow();
static	struct group_adjunct *getgidfromyellow();

static struct gradata *
_gradata()
{
	register struct gradata *g = gradata;

	if (g == 0) {
		g = (struct gradata *)calloc(1, sizeof (struct gradata));
		gradata = g;
	}
	return (g);
}

#ifdef	NOT_DEFINED
struct group_adjunct *
getgragid(gid)
register gid;
{
	struct group *getgrgid();
	struct group *gr;

	if ((gr = getgrgid(gid)) == NULL)
		return NULL;
	return (getgranam(gr->gr_name));
}
#endif	NOT_DEFINED

struct group_adjunct *
getgranam(name)
register char *name;
{
	register struct gradata *g = _gradata();
	struct group_adjunct *gra;
	char line[BUFSIZ+1];

	setgraent();
	if (g == 0)
		return (0);
	if (!g->grfa)
		return NULL;
	while (fgets(line, BUFSIZ, g->grfa) != NULL) {
		if ((gra = interpret(line, strlen(line))) == NULL)
			continue;
		if (matchname(line, &gra, name)) {
			endgraent();
			return (gra);
		}
	}
	endgraent();
	return (NULL);
}

void
setgraent()
{
	register struct gradata *g = _gradata();

	if (g == NULL)
		return;
	if (g->domain == NULL)
		(void) yp_get_default_domain(&g->domain);
	if (!g->grfa)
		g->grfa = fopen(GROUPADJ, "r");
	else
		rewind(g->grfa);
	if (g->yp)
		free(g->yp);
	g->yp = NULL;
	freeminuslist();
}

void
endgraent()
{
	register struct gradata *g = _gradata();

	if (g == 0)
		return;
	if (g->grfa) {
		(void) fclose(g->grfa);
		g->grfa = NULL;
	}
	if (g->yp)
		free(g->yp);
	g->yp = NULL;
	freeminuslist();
}

struct group_adjunct *
fgetgraent(f)
	FILE *f;
{
	char line1[BUFSIZ+1];

	if(fgets(line1, BUFSIZ, f) == NULL)
		return(NULL);
	return (interpret(line1, strlen(line1)));
}

static char *
grskip(p,c)
	register char *p;
	register c;
{
	while(*p && *p != c && *p != '\n') ++p;
	if (*p == '\n')
		*p = '\0';
	else if (*p != '\0')
		*p++ = '\0';
	return(p);
}

struct group_adjunct *
getgraent()
{
	register struct gradata *g = _gradata();
	char line1[BUFSIZ+1];
	static struct group_adjunct *savegra;
	struct group_adjunct *gra;

	if (g == 0)
		return (0);
	if (g->domain == NULL) {
		(void) yp_get_default_domain(&g->domain);
	}
	if(!g->grfa && !(g->grfa = fopen(GROUPADJ, "r")))
		return(NULL);
  again:
	if (g->yp) {
		gra = interpretwithsave(g->yp, g->yplen, savegra);
		free(g->yp);
		if (gra == NULL)
			return(NULL);
		getnextfromyellow();
		if (onminuslist(gra))
			goto again;
		else
			return (gra);
	}
	else if (fgets(line1, BUFSIZ, g->grfa) == NULL)
		return(NULL);
	if ((gra = interpret(line1, strlen(line1))) == NULL)
		return(NULL);
	switch(line1[0]) {
		case '+':
			if (strcmp(gra->gra_name, "+") == 0) {
				getfirstfromyellow();
				savegra = save(gra);
				goto again;
			}
			/* 
			 * else look up this entry in NIS
			 */
			savegra = save(gra);
			gra = getnamefromyellow(gra->gra_name+1, savegra);
			if (gra == NULL)
				goto again;
			else if (onminuslist(gra))
				goto again;
			else
				return (gra);
			break;
		case '-':
			addtominuslist(gra->gra_name+1);
			goto again;
			break;
		default:
			if (onminuslist(gra))
				goto again;
			return (gra);
			break;
	}
	return (NULL);
}

static struct group_adjunct *
interpret(val, len)
	char *val;
{
	register struct gradata *g = _gradata();
	register char *p;

	if (g == 0)
		return (0);
	strncpy(g->interpline, val, len);
	p = g->interpline;
	g->interpline[len] = '\n';
	g->interpline[len+1] = 0;
	g->interpgra.gra_name = p;
	p = grskip(p,':');
        if (strcmp(g->interpgra.gra_name, "+") == 0) {
                /* we are going to the NIS - fix the
                 * rest of the struct as much as is needed
                 */
                g->interpgra.gra_passwd = "";
		return (&g->interpgra);
        }
	g->interpgra.gra_passwd = p;
        while(*p && *p != '\n') p++;
        *p = '\0';
	return (&g->interpgra);
}

static
freeminuslist() {
	register struct gradata *g = _gradata();
	struct list *ls;
	
	if (g == 0)
		return;
	for (ls = g->minuslist; ls != NULL; ls = ls->nxt) {
		free(ls->name);
		free(ls);
	}
	g->minuslist = NULL;
}

static struct group_adjunct *
interpretwithsave(val, len, savegra)
	char *val;
	struct group_adjunct *savegra;
{
	register struct gradata *g = _gradata();
	struct group_adjunct *gra;
	
	if (g == 0)
		return (0);
	if ((gra = interpret(val, len)) == NULL)
		return (NULL);
	if (savegra->gra_passwd && *savegra->gra_passwd)
		gra->gra_passwd =  savegra->gra_passwd;
	return (gra);
}

static
onminuslist(gra)
	struct group_adjunct *gra;
{
	register struct gradata *g = _gradata();
	struct list *ls;
	register char *nm;
	
	if (g == 0)
		return 0;
	nm = gra->gra_name;
	for (ls = g->minuslist; ls != NULL; ls = ls->nxt)
		if (strcmp(ls->name, nm) == 0)
			return 1;
	return 0;
}

static
getnextfromyellow()
{
	register struct gradata *g = _gradata();
	int reason;
	char *key = NULL;
	int keylen;
	
	if (g == 0)
		return;
	if (reason = yp_next(g->domain, "group.adjunct.byname",
	    g->oldyp, g->oldyplen, &key, &keylen,
	    &g->yp, &g->yplen)) {
#ifdef DEBUG
fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		g->yp = NULL;
	}
	if (g->oldyp)
		free(g->oldyp);
	g->oldyp = key;
	g->oldyplen = keylen;
}

static
getfirstfromyellow()
{
	register struct gradata *g = _gradata();
	int reason;
	char *key = NULL;
	int keylen;
	
	if (g == 0)
		return;
	if (reason =  yp_first(g->domain, "group.adjunct.byname",
	    &key, &keylen, &g->yp, &g->yplen)) {
#ifdef DEBUG
fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
		g->yp = NULL;
	}
	if (g->oldyp)
		free(g->oldyp);
	g->oldyp = key;
	g->oldyplen = keylen;
}

static struct group_adjunct *
getnamefromyellow(name, savegra)
	char *name;
	struct group_adjunct *savegra;
{
	register struct gradata *g = _gradata();
	struct group_adjunct *gra;
	int reason;
	char *val;
	int vallen;
	
	if (g == 0)
		return (NULL);
	if (reason = yp_match(g->domain, "group.adjunct.byname",
	    name, strlen(name), &val, &vallen)) {
#ifdef DEBUG
fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		return NULL;
	}
	else {
		gra = interpret(val, vallen);
		free(val);
		if (gra == NULL)
			return NULL;
		if (savegra->gra_passwd && *savegra->gra_passwd)
			gra->gra_passwd =  savegra->gra_passwd;
		return gra;
	}
}

static
addtominuslist(name)
	char *name;
{
	register struct gradata *g = _gradata();
	struct list *ls;
	char *buf;
	
	if (g == 0)
		return;
	ls = (struct list *)malloc(sizeof(struct list));
	buf = (char *)malloc(strlen(name) + 1);
	(void) strcpy(buf, name);
	ls->name = buf;
	ls->nxt = g->minuslist;
	g->minuslist = ls;
}

/* 
 * save away psswd field, which is the only
 * one which can be specified in a local + entry to override the
 * value in the NIS
 */
static struct group_adjunct *
save(gra)
	struct group_adjunct *gra;
{
	register struct gradata *g = _gradata();
	
	if (g == 0)
		return 0;
	/* 
	 * free up stuff from last time around
	 */
	if (g->sv) {
		free(g->sv->gra_passwd);
		free(g->sv);
	}
	g->sv = (struct group_adjunct *)calloc(1, sizeof(struct group_adjunct));
	g->sv->gra_passwd = (char *)malloc(strlen(gra->gra_passwd) + 1);
	(void) strcpy(g->sv->gra_passwd, gra->gra_passwd);
	return g->sv;
}

static
matchname(line1, grap, name)
	char line1[];
	struct group_adjunct **grap;
	char *name;
{
	struct group_adjunct *savegra;
	struct group_adjunct *gra = *grap;

	switch(line1[0]) {
		case '+':
			if (strcmp(gra->gra_name, "+") == 0) {
				savegra = save(gra);
				gra = getnamefromyellow(name, savegra);
				if (gra) {
					*grap = gra;
					return 1;
				}
				else
					return 0;
			}
			if (strcmp(gra->gra_name+1, name) == 0) {
				savegra = save(gra);
				gra = getnamefromyellow(gra->gra_name+1, savegra);
				if (gra) {
					*grap = gra;
					return 1;
				}
				else
					return 0;
			}
			break;
		case '-':
			if (strcmp(gra->gra_name+1, name) == 0) {
				*grap = NULL;
				return 1;
			}
			break;
		default:
			if (strcmp(gra->gra_name, name) == 0)
				return 1;
	}
	return 0;
}
