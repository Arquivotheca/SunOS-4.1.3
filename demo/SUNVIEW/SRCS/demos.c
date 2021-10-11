#ifndef lint
static	char sccsid[] = "@(#)bdemos.c 1.7 84/01/19 SMI";
#endif
#include <stdio.h>
#include <sgtty.h>

/*
 * bdemos.c - a kitchen sink demo for SUN graphics
 *
 *
 *
 * November 1983        Now calls other user level programs
 *                      to do actual work.
 */

char c;
int param, hitkey;
int fildes; short ttyflags;		/* set tty for polling */
struct sgttyb ttypar, *argp;


main() 
 {   

    int quitting = 0;
    argp = &ttypar;  fildes = fileno( stdin);  gtty( fildes, argp);
    ttyflags = argp->sg_flags; 
	argp->sg_flags |= CBREAK;
	argp->sg_flags &= ~ECHO;
    stty( fildes, argp);
    hitkey = 0;
    c = '?';
    help();
    while (!quitting) 
     {
	read(0,&c,1);
        switch (c) 
        {
        case 'a':system("brotcube"); break;
        case 'b':system("bsuncube"); continue;
        case 'c':system("bjump"); break;
        case 'd':system("bbounce"); continue;
        case 'e':system("bballs"); continue;
	case 'q':quitting = 1; break;
        default: help(); continue;
        }
    }
    argp->sg_flags = ttyflags;  stty( fildes, argp);
}



help()
{
    printf ("    Please press: \n");
    printf ("    a for rotating cube demo \n");
    printf ("    b for sun cube \n");
    printf ("    c for jump to hyperspace \n");
    printf ("    d for bouncing square demo \n");
    printf ("    e for colliding balls demo \n");
    printf ("    q to quit \n");
}

