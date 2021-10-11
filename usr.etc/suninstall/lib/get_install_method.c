#ifndef lint
#ifdef SunB1
#ident 			"@(#)get_install_method.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)get_install_method.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/****************************************************************************
**  File : get_install_method.h
**
** Function :  (int) get_install_method()
**
** Return Value : the install method to use :
**		MANUAL_INSTALL for normal installation
**		EASY_INSTALL   for easy   installation
**
** Description : this function queries the user for the type of installation
**		 method to use.
**
****************************************************************************
*/

#include <stdio.h>
#include "install.h"		/* for installation type definitions */
#include "unpack_help.h"
 

int
get_install_method()
{
	char buf[BUFSIZ];
#ifdef REPREINSTALLED
	FILE *in, *popen();
	int sun4c_flag=0;

	in = popen("arch -k", "r");
	fscanf(in,"%s",buf);
	if ( strcmp(buf, "sun4c") == 0 ) {
         sun4c_flag=1;
	}
    pclose(in);
#endif REPREINSTALLED
	while(1) {
#ifdef REPREINSTALLED
		if ( sun4c_flag == 1) {
			(void) printf(INTRO);
			(void) printf(METHOD_0);
		} else 
#endif REPREINSTALLED
#ifndef REPREINSTALLED
			(void) printf(INTRO_ORG);
#endif !REPREINSTALLED
	    (void) printf(METHOD_1);
	    (void) printf(METHOD_2);
	    (void) printf(METHOD_3);
	    get_stdin(buf);
	    if (strlen(buf) == 1)
		    switch(buf[0])  {
#ifdef REPREINSTALLED
			case '0':	if (sun4c_flag == 1) {
							return(RE_PREINSTALLED);
						} else {
							(void) putchar('\n');
							return(EXIT_INSTALL);
						}
#endif REPREINSTALLED
            case '1':   return(EASY_INSTALL);
			case '2':   (void) putchar('\n'); 
				    return(MANUAL_INSTALL);
			case 'Q':  
			case 'q':  
			case 'E':  
			case 'e':  (void) putchar('\n'); 
				    return(EXIT_INSTALL);
		       }
	}
}

