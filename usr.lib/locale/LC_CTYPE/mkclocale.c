#ifndef lint
static	char sccsid[] = "@(#)mkclocale.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <ctype.h>
#include <locale.h>

/*ARGSUSED*/
int
main(argc, argv)
	int argc;
	char **argv;
{

	if (write(1, _ctype_, 514) < 0) {
		perror("mkclocale");
		return (1);
	} else
		return (0);
}
