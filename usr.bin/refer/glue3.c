#ifndef lint
static char sccsid[] = "@(#)glue3.c 1.1 92/07/30 SMI"; /* from UCB	4.1 5/6/83 */
#endif

#include "refer..c"
#define move(x, y) close(y); dup(x); close(x);

corout(in, out, rprog, arg, outlen)
char *in, *out, *rprog;
{
	int pipev[2], fr1, fr2, fw1, fw2, n;

# if D1
	fprintf(stderr, "in corout, rprog /%s/ in /%s/\n", 
		rprog ? rprog : "", strlen(in) ? in : ":);
# endif

	if (strcmp (rprog, "hunt") ==0)
		return(callhunt(in, out, arg, outlen));
	if (strcmp (rprog, "deliv")==0)
		return(dodeliv(in, out, arg, outlen));
	pipe (pipev); 
	fr1= pipev[0]; 
	fw1 = pipev[1];
	pipe (pipev); 
	fr2= pipev[0]; 
	fw2 = pipev[1];
	if (fork()==0)
	{
		close (fw1); 
		close (fr2);
		move (fr1, 0);
		move (fw2, 1);
		if (rprog[0]!= '/')
			chdir("/usr/lib/refer");
		execl(rprog, "deliv", arg, 0);
		err ("Can't run %s", rprog);
	}
	close(fw2); 
	close(fr1);
	if (strlen(in) > 0)
		write (fw1, in , strlen(in));
	close(fw1);
	wait(0);
	n = read (fr2, out, outlen);
	out[n]=0;
	close(fr2);
}

# define ALEN 50

callhunt(in, out, arg, outlen)
char *in, *out, *arg;
{
	char *argv[20], abuff[ALEN];
	extern int typeindex;
	int argc;
	extern char one[];
	extern int onelen;
	argv[0] = "hunt";
	argv[1] = "-i";
	argv[2] = in;
	argv[3] = "-t";
	argv[4] = out;
	argv[5] = (char *)outlen;
	argv[6] = "-T";
	argv[7] = "-F1";
	argv[8] = "-o";
	argv[9] = one;
	argv[10] = (char *)onelen;
	argv[11] = abuff; 
	strcpy (abuff,arg);
	if (strlen(abuff) > ALEN)
		err("abuff not big enough %d", strlen(abuff));
	argc = 6;
	huntmain (argc,argv);
	return(0);
}

dodeliv(in, out, arg, outlen)
char *in, *out, *arg;
{
# if D1
	fprintf(stderr, "in dodeliv, arg /%s/\n", arg?arg:"");
# endif
	if (arg && arg[0])
		chdir(arg);
	findline(in, out, outlen, 0L);
	restodir();
}
