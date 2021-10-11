#ifndef lint
static	char sccsid[] = "@(#)mkclocale.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <locale.h>

/*ARGSUSED*/
int
main(argc, argv)
	int argc;
	char **argv;
{
	register struct dtconv *dtcp;
	register int i;

	dtcp = localdtconv();	/* get C locale's strings */

	for (i = 0; i < 12; i++)
		(void) printf("%s\n", dtcp->abbrev_month_names[i]);
	for (i = 0; i < 12; i++)
		(void) printf("%s\n", dtcp->month_names[i]);
	for (i = 0; i < 7; i++)
		(void) printf("%s\n", dtcp->abbrev_weekday_names[i]);
	for (i = 0; i < 7; i++)
		(void) printf("%s\n", dtcp->weekday_names[i]);
	(void) printf("%s\n", dtcp->time_format);
	(void) printf("%s\n", dtcp->sdate_format);
	(void) printf("%s\n", dtcp->dtime_format);
	(void) printf("%s\n", dtcp->am_string);
	(void) printf("%s\n", dtcp->pm_string);
	(void) printf("%s\n", dtcp->ldate_format);

	if (ferror(stdout)) {
		perror("mkclocale");
		return (1);
	} else
		return (0);
}
