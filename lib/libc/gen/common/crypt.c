#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)crypt.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

/*LINTLIBRARY*/
/* The real crypt is now _crypt.  This version performs automatic
 * authentication via pwauth for special password entries, or simply
 * calls _crypt for the usual case.
 */

char *
crypt(pw, salt)
char	*pw, *salt;
{
	static char *iobuf;
	extern char *_crypt();
	extern char *malloc();

	if (iobuf == 0) {
		iobuf = malloc((unsigned)16);
		if (iobuf == 0)
			return (0);
	}
	/* handle the case where the password is really in passwd.adjunct.
	 * In this case, the salt will start with "##".  We should call
	 * passauth to determine if pw is valid.  If so, we should return
	 * the salt, and otherwise return NULL.  If salt does not start with
	 * "##", crypt will act in the normal fashion.
	 */
	if (salt[0] == '#' && salt[1] == '#') {
		if (pwdauth(salt+2, pw) == 0)
			strcpy(iobuf, salt);
		else
			iobuf[0] = '\0';
		return(iobuf);
	}
	/* handle the case where the password is really in group.adjunct.
	 * In this case, the salt will start with "#$".  We should call
	 * grpauth to determine if pw is valid.  If so, we should return
	 * the salt, and otherwise return NULL.  If salt does not start with
	 * "#$", crypt will act in the normal fashion.
	 */
	if (salt[0] == '#' && salt[1] == '$') {
		if (grpauth(salt+2, pw) == 0)
			strcpy(iobuf, salt);
		else
			iobuf[0] = '\0';
		return(iobuf);
	}
	return (_crypt(pw, salt));
}
