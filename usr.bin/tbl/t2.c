#ifndef lint
static	char sccsid[] = "@(#)t2.c 1.1 92/07/30 SMI"; /* from UCB 4.1 83/02/12 */
#endif

 /* t2.c:  subroutine sequencing for one table */
# include "t..c"
tableput()
{
saveline();
savefill();
ifdivert();
cleanfc();
getcomm();
getspec();
gettbl();
getstop();
checkuse();
choochar();
maktab();
runout();
release();
rstofill();
endoff();
restline();
}
