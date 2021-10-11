#ifndef lint
static  char sccsid[] = "@(#)play.c 1.1 92/07/30 SMI";
#endif

#include "old.h"

play(f)
short f;
{
	short t, i;

	clock();
	if(f) goto first;
loop:
	intrp = 0;
	move();

first:
	if(manflg)
		goto loop;
	i = mantom;
	t = clktim[i];
	if(!bookm())
	if(!mate(mdepth, 1))
		xplay();
	if(intrp) {
		decrem();
		mantom? bremove(): wremove();
		goto loop;
	}
	if(!abmove) {
		printf("Resign\n");
		onhup();
	}
	makmov(abmove);
	i = clktim[i];
	t = i-t;
	if(i/moveno > 150) {
		if(depth > 1)
			goto decr;
		goto loop;
	}
	if(depth==3 && t>180)
		goto decr;
	if(depth==1 && t<60)
		goto incr;
	if(game==3 && t<60 && depth==2)
		goto incr;
	goto loop;

incr:
	depth++;
	goto loop;

decr:
	goto loop;
}

move()
{
	short a, *p, *p1;

loop:
	lmp = done();		/* is game over? */
	a = manual();
	p = done();
	p1 = p;
	while(p1 != lmp) {
		p1++;
		if(*p1++ == a) {
			lmp = p;
			makmov(a);
			return;
		}
	}
	printf("Illegal move\n");
	lmp = p;
	goto loop;
}

manual()
{
	short a, b, c, *ap;
	char *p1;
	extern out1;

loop:
	intrp = 0;
	stage();
	rline();
	sbufp = sbuf;
	if(match("save")) {
		save();
		goto loop;
	}
	if(match("test")) {
		testf = !testf;
		goto loop;
	}
	if(match("remove")) {
		if(amp[-1] != -1) {
			decrem();
			mantom? bremove(): wremove();
		}
		if(amp[-1] != -1) {
			decrem();
			mantom? bremove(): wremove();
		}
		goto loop;
	}
	if(match("exit"))
		exit(0);
	if(match("manual")) {
		manflg = !manflg;
		goto loop;
	}
	if(match("resign"))
		onhup();
	if(moveno == 1 && mantom == 0) {
		if(match("first"))
			play(1);
		if(match("alg")) {
			mfmt = 1;
			goto loop;
		}
		if(match("restore")) {
			restore();
			goto loop;
		}
	}
	if(match("clock")) {
		clktim[mantom] += clock();
		ptime("white", clktim[0]);
		ptime("black", clktim[1]);
		goto loop;
	}
	if(match("score")) {
		score();
		goto loop;
	}
	if(match("setup")) {
		setup();
		goto loop;
	}
	if(match("hshort")) {
		a = xplay();
		out(abmove);
		printf(" %d\n", a);
		goto loop;
	}
	if(match("repeat")) {
		if(amp[-1] != -1) {
			ap = amp;
			mantom? wremove(): bremove();
			decrem();
			posit(&out1, ap);
		}
		goto loop;
	}
	if(*sbufp == '\0') {
		pboard();
		goto loop;
	}
	if((a=algin()) != 0) {
		mfmt = 1;
		return(a);
	}
	if((a=stdin()) != 0) {
		mfmt = 0;
		return(a);
	}
	printf("eh?\n");
	goto loop;
}

algin()
{
	short from, to;

	from = cooin();
	to = cooin();
	if(*sbufp != '\0') return(0);
	return((from<<8)|to);
}

cooin()
{
	short a, b;

	a = sbufp[0];
	if(a<'a' || a>'h') return(0);
	b = sbufp[1];
	if(b<'1' || b>'8') return(0);
	sbufp += 2;
	a = (a-'a')+8*('8'-b);
	return(a);
}

match(s)
char *s;
{
	char *p1;
	short c;

	p1 = sbufp;
	while((c = *s++) != '\0')
		if(*p1++ != c) return(0);
	sbufp = p1;
	return(1);
}

short *
done()
{
	short *p;

	if(rept() > 3) {
		printf("Draw by repetition\n");
		onhup();
	}
	p = lmp;
	mantom? bagen(): wagen();
	if(p == lmp) {
		if(check())
			if(mantom)
				printf("White wins\n"); else
				printf("Black wins\n"); else
		printf("Stale mate\n");
		onhup();
	}
	return(p);
}

xplay()
{
	short a;

	stage();
	abmove = 0;
	a = mantom? bplay(): wplay();
	ivalue = a;
	return(a);
}

term()
{
	short p[2];

	if(bookp)
		exit(0);
	mfmt = 0;
	pipe(p);
	if(fork()) {
		close(1);
		dup(p[1]);
		close(2);
		score();
		exit(0);
	}
	close(0);
	dup(p[0]);
	close(p[1]);
	printf("done!\n");
	/*execl("/bin/mail", "mail", "chess", 0); */
	/*execl("/usr/bin/mail", "mail", "chess", 0); */
	exit(0);
}
