#ifndef lint
static	char sccsid[] = "@(#)makekey.c 1.1 92/07/30 SMI"; /* from UCB 4.1 10/01/80 */
#endif
/*
 * You send it 10 bytes.
 * It sends you 13 bytes.
 * The transformation is expensive to perform
 * (a significant part of a second).
 */

char	*crypt();

main()
{
	char key[8];
	char salt[2];
	
	read(0, key, 8);
	read(0, salt, 2);
	write(1, crypt(key, salt), 13);
	return(0);
}
