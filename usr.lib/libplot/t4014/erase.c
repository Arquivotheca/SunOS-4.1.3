#ifndef lint
static	char sccsid[] = "@(#)erase.c 1.1 92/07/30 SMI"; /* from UCB 4.1 6/27/83 */
#endif

extern int ohiy;
extern int ohix;
extern int oloy;
extern int oextra;
erase(){
	int i;
		putch(033);
		putch(014);
		ohiy= -1;
		ohix = -1;
		oextra = -1;
		oloy = -1;
		sleep(2);
		return;
}
