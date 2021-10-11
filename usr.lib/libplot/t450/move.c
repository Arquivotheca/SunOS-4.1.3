#ifndef lint
static	char sccsid[] = "@(#)move.c 1.1 92/07/30 SMI"; /* from UCB 4.1 6/27/83 */
#endif

move(xi,yi){
		movep(xconv(xsc(xi)),yconv(ysc(yi)));
		return;
}
