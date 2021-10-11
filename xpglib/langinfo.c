/*	@(#)langinfo.c 1.1 92/07/30 SMI; from S5R2 1.1	*/

#include <locale.h>
#include <stdio.h>
#include <nl_types.h>
#include <langinfo.h>
#include <string.h>
#include <malloc.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAXANSLENGTH 128

extern char *getlocale_time();
extern char *getlocale_monetary();
extern char *getlocale_linfo();
extern int  openlocale();

static char *getstr();

extern struct dtconv *_dtconv;	/* CTE - sidneyho 8/2/91 */
extern struct lconv lconv;
extern struct langinfo _langinfo;
extern char _locales[MAXLOCALE-1][MAXLOCALENAME+1];

static char yes[MAXANSLENGTH+1] = "yes";
static char no[MAXANSLENGTH+1] = "no";
static char langinfo_locale[MAXLOCALENAME+1] = "C";
static char *langinfo_str = NULL;


char *
nl_langinfo(item)
	nl_item item;
{
	switch (item) {
	case D_T_FMT:
		if (getlocale_time() == NULL)
			return (NULL);
		return(_dtconv->dtime_format);
		break;
	case DAY_1: case DAY_2: case DAY_3: case DAY_4:
	case DAY_5: case DAY_6: case DAY_7:
		if (getlocale_time() == NULL)
			return (NULL);
		return(_dtconv->weekday_names[item - DAY_1]);
		break;
	case ABDAY_1: case ABDAY_2: case ABDAY_3: case ABDAY_4:
	case ABDAY_5: case ABDAY_6: case ABDAY_7:
		if (getlocale_time() == NULL)
			return (NULL);
		return(_dtconv->abbrev_weekday_names[item - ABDAY_1]);
		break;
	case MON_1: case MON_2: case MON_3: case MON_4:
	case MON_5: case MON_6: case MON_7: case MON_8:
	case MON_9: case MON_10: case MON_11: case MON_12:
		if (getlocale_time() == NULL)
			return (NULL);
		return(_dtconv->month_names[item - MON_1]);
		break;
	case ABMON_1: case ABMON_2: case ABMON_3: case ABMON_4:
	case ABMON_5: case ABMON_6: case ABMON_7: case ABMON_8:
	case ABMON_9: case ABMON_10: case ABMON_11: case ABMON_12:
		if (getlocale_time() == NULL)
			return (NULL);
		return(_dtconv->abbrev_month_names[item - MON_1]);
		break;
	case CRNCYSTR:
		return(lconv.currency_symbol);
		break;
	case RADIXCHAR:
		return(lconv.decimal_point);
		break;
	case THOUSEP:
		return(lconv.thousands_sep);
		break;
	case YESSTR:
		if (getlocale_linfo(_locales[LANGINFO - 1]) == NULL)
			return (NULL);
		return(_langinfo.yesstr);
		break;
	case NOSTR:
		if (getlocale_linfo(_locales[LANGINFO - 1]) == NULL)
			return (NULL);
		return(_langinfo.nostr);
		break;
	default:
		return (NULL);
		break;
	}
}


char *
getlocale_linfo(my_locale)
char *my_locale;
{
	register int fd;
	struct stat buf;
	char *str;
	register char *p;
	struct langinfo langinfop;

	if ((fd = openlocale("LANGINFO", LANGINFO, _locales[LANGINFO - 1], my_locale)) < 0)
		return (NULL);

	if (fd == 0)
		return "";
	if ((fstat(fd, &buf)) != 0)
		return (NULL);
	if ((str = malloc((unsigned)buf.st_size + 2)) == NULL) {
		close(fd);
		return (NULL);
	}

	if ((read(fd, str, (int)buf.st_size)) != buf.st_size) {
		close(fd);
		free(str);
		return (NULL);
	}

	/* Set last character of str to '\0' */
	p = &str[buf.st_size];
	*p++ = '\n';
	*p = '\0';

	/* p will "walk thru" str */
	p = str;  	

	p = getstr(p, &langinfop.yesstr);
	if (p == NULL)
		goto fail;
	p = getstr(p, &langinfop.nostr);
	if (p == NULL)
		goto fail;
	(void) close(fd);

	/*
	 * set info.
	 */
	if (langinfo_str != NULL)
		free(langinfo_str);

	langinfo_str = str;
	_langinfo = langinfop;
	return (langinfo_str);

fail:
	(void) close(fd);
	free(str);
	return (NULL);
}

static char *
getstr(p, strp)
        register char *p;
        char **strp;
{
        *strp = p;
        p = strchr(p, '\n');
        if (p == NULL)
                return (NULL);  /* no end-of-line */
        *p++ = '\0';
        return (p);
}
