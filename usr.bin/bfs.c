#ifndef lint
static	char sccsid[] = "@(#)bfs.c 1.1 92/07/30 SMI"; /* from S5R2 1.12 */
#endif

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
int setjmp();
void longjmp();
jmp_buf env;
char *sbrk();
void exit();
long lseek();
unsigned sleep();

#ifdef pdp11

#define BRKTYP	char
#define BRKSIZ	4096
#define BRKTWO	2
#define BFSAND	&0377
#define BFSLIM	254
#define BFSTRU	255
#define BFSBUF	256

#else

#define BRKTYP	short
#define BRKSIZ	8192
#define BRKTWO	4
#define BFSAND
#define BFSLIM	511
#define BFSTRU	511
#define BFSBUF	512
#endif

/* The following 6 defines are necessary for the regexp.h routines. */

#define INIT		register char *ptregx = instring;
#define GETC()		(*ptregx++)
#define PEEKC()		*ptregx
#define UNGETC(c)	(--ptregx)
#define RETURN(c)	return(c)
#define ERROR(c)	return(regerr(c))

char	*regerr();
void	colcomd();
void	jump();
void	ecomd();
void	fcomd();
void	gcomd();
void	kcomd();
void	pcomd();
void	qcomd();
void	xcomds();
void	xncomd();
void	xbcomd();
void	xccomd();
void	xfcomd();
void	xocomd();
void	xtcomd();
void	xvcomd();
void	wcomd();
void	nlcomd();
void	eqcomd();
void	excomd();

#include <regexp.h>

#define REBUF	256

struct Comd {
	int Cnumadr;
	int Cadr[2];
	char Csep;
	char Cop;
};

int Dot, Dollar;
int circf1, circf2;	/* Save global variable "circf" from regexp.h */
int markarray[26], *mark;
int fstack[15]={1, -1};
int infildes=0;
int outfildes=1;
char internal[512], *intptr;
char comdlist[100];
char *endds;		/* See brk(2) in manual. */
char charbuf='\n';
int peeked;
char currex[100];
int trunc=BFSTRU;
int crunch = -1;
int segblk[512], segoff[512], txtfd, prevblk, prevoff;
int oldfd=0;
int flag4=0;
int flag3=0;
int flag2=0;
int flag1=0;
int flag=0;
int lprev=1;
int status[1];

BRKTYP *lincnt;
char perbuf[REBUF];
char rebuf[REBUF];
char glbuf[REBUF];
char tty, *bigfile;
char fle[80];
char prompt=1;
char verbose=1;		/* 1=print # of bytes read in; 0=silent. */
char varray[10][100];	/* Holds xv cmd parameters. */
long outcnt;
char strtmp[32];

#define	equal(s1, s2)	(strcmp(s1, s2) == 0)

main(argc,argv)
int argc;
char *argv[];
{
	extern reset();
	struct Comd comdstruct, *p;

	if(argc < 2 || argc > 3) {
		(void) err(1,"arg count");
		quit();
	}
	mark = markarray-'a';
	if(argc == 3) verbose = 0;
	setbuf(stdout, (char *) 0);
	if(bigopen(bigfile=argv[argc-1])) quit();
	tty = isatty(0);

	p = &comdstruct;
		/* Look for 0 or more non-'%' char followed by a '%' */
	(void) compile("[^%]*%",perbuf,&perbuf[REBUF],'\0');
	circf1 = circf;		/* Save regexp.h flag */
	(void) setjmp(env);
 	(void) signal(SIGINT,reset);
	(void) err(0,"");
	(void) printf("\n");
	flag = 0;
	prompt = 0;
	while(1) begin(p);
}

reset()		/* for longjmp on signal */
{
	longjmp(env,1);
}
 

begin(p)
struct Comd *p;
{
	char line[256];

strtagn:	if(flag == 0) eat();
	if(infildes != 100) {
		if(infildes == 0 && prompt) (void) printf("*");
		flag3 = 1;
		(void) getstr(1,line,0,0,0);
		flag3 = 0;
		if(percent(line) < 0) goto strtagn;
		(void) newfile(1,"");
	}
	if(!(getcomd(p,1) < 0)) {

	switch(p->Cop) {

		case 'e':	if(!flag) ecomd();
				else (void) err(0,"");
				break;

		case 'f':	fcomd(p);
				break;

		case 'g':	if(flag == 0) gcomd(p,1);
				else (void) err(0,"");
				break;

		case 'k':	kcomd(p);
				break;

		case 'p':	pcomd(p);
				break;

		case 'q':	qcomd();
				break;

		case 'v':	if(flag == 0) gcomd(p,0);
				else (void) err(0,"");
				break;

		case 'x':	if(!flag) xcomds(p);
				else (void) err(0,"");
				break;

		case 'w':	wcomd(p);
				break;

		case '\n':	nlcomd(p);
				break;

		case '=':	eqcomd(p);
				break;

		case ':':	colcomd(p);
				break;

		case '!':	excomd();
				break;

		case 'P':	prompt = !prompt;
				break;


		default:	if(flag) (void) err(0,"");
				else (void) err(1,"bad command");
				break;
	}
	}
}


bigopen(file)
char file[];
{
	register int l, off, cnt;
	int blk, newline, n, s;
	char block[512];

	if((txtfd=open(file,0)) < 0) return(syserr(1,"can't open"));

	blk = -1;
	newline = 1;
	l = cnt = s = 0;
	off = 512;
	if((lincnt=(BRKTYP*)sbrk(BRKSIZ)) == (BRKTYP*)-1)
		return(err(1,"too many lines"));
	endds = (char *)lincnt;	/* Save initial data space address. */

	while((n=read(txtfd,block,512)) > 0) {
		blk++;
		for(off=0; off<n; off++) {
			if(newline) {
				newline = 0;
				if(l>0 && !(l&07777))
					if(sbrk(BRKSIZ) == (char *) -1)
					  return(err(1,"too many lines"));
				lincnt[l] = cnt;
				cnt = 0;
				if(!(l++ & 077)) {
					segblk[s] = blk;
					segoff[s++] = off;
				}
				if(l < 0 || l > 32767) return(err(1,"too many lines"));
			}
			if(block[off] == '\n') newline = 1;
			cnt++;
#ifdef pdp11
			if(cnt > 255)
				{(void) printf("Line %d too long\n",l);
				return(-1);
				}
#endif
		}
	}
	if(!(l&07777)) if(sbrk(BRKTWO) == (char *) -1)
		return(err(1,"too many lines"));
	lincnt[Dot = Dollar = l] = cnt;
	sizeprt(blk,off);
	return(0);
}


sizeprt(blk, off)
int blk, off;
{
	if(verbose) (void) printf("%ld",512L*blk+off);
}


int saveblk = -1;


bigread(l,rec)
int l;
char rec[];
{
	register int i;
	register char *r, *b;
	int off;
	static char savetxt[512];

	if((i=l-lprev) == 1) prevoff += lincnt[lprev]BFSAND;
	else if(i >= 0 && i <= 32)
		for(i=lprev; i<l; i++) prevoff += lincnt[i]BFSAND;
	else if(i < 0 && i >= -32)
		for(i=lprev-1; i>=l; i--) prevoff -= lincnt[i]BFSAND;
	else {
		prevblk = segblk[i=(l-1)>>6];
		prevoff = segoff[i];
		for(i=(i<<6)+1; i<l; i++) prevoff += lincnt[i]BFSAND;
	}

	prevblk += prevoff>>9;
	prevoff &= 0777;
	lprev = l;

	if(prevblk != saveblk) {
		if(lseek(txtfd,((long)(saveblk=prevblk))<<9,0) == -1L)
			return(syserr(1,"seek error"));
		if(read(txtfd,savetxt,512) < 0)
			return(syserr(1,"read error"));
	}

	r = rec;
	off = prevoff;
	while(1) {
		for(b=savetxt+off; b<savetxt+512; b++) {
			if((*r++ = *b) == '\n') {
				*(r-1) = '\0';
				return(0);
			}
			if(((unsigned)r - (unsigned)rec) > BFSLIM) {
#ifdef pdp11
				(void) printf("line too long\n");
				exit(1);
#else
				(void) printf("Line too long--output truncated\n");
				return(-1);
#endif
			}
		}
		if(read(txtfd,savetxt,512) < 0)
			return(syserr(1,"read error"));
		off = 0;
		saveblk++;
	}
	/*NOTREACHED*/
}

void
ecomd()
{
	register int i = 0;

	while(peekc() == ' ') (void) getch();
	while((fle[i++] = getch()) != '\n');
	fle[--i] = '\0';
	(void) close(txtfd);	/* Without this, ~20 "e" cmds gave "can't open" msg. */
	(void) brk(endds);	/* Reset data space addr. - mostly for 16-bit cpu's. */
	lprev=1; prevblk=0; prevoff=0; saveblk = -1;	/* Reset parameters. */
	if(bigopen(bigfile =fle)) quit();
	(void) printf("\n");
}

void
fcomd(p)
struct Comd *p;
{
	if(more() || defaults(p,1,0,0,0,0,0)) return;
	(void) printf("%s\n",bigfile);
}

void
gcomd(p,k)
int k;
struct Comd *p;
{
	register char d;
	register int i, end;
	char line[BFSBUF];

	if(defaults(p,1,2,1,Dollar,0,0)) return;

	if((d=getch()) == '\n') {
		(void) err(1,"syntax");
		return;
	}
	if(peekc() == d) (void) getch();
	else 
		if(getstr(1,currex,d,0,1)) return;
	if(compile(currex,glbuf,&glbuf[REBUF],'\0') == 0) {
		(void) err(1,"RE-syntax");
		return;
	}
	circf2 = circf;

	if(getstr(1,comdlist,0,0,0)) return;
	i = p->Cadr[0];
	end = p->Cadr[1];
	while (i<=end) {
		if(bigread(i,line) < 0)
			break;
		circf = circf2;		/* Restore circf for "step()" */
		if(!(step(line,glbuf))) {
			if(!k) {
				Dot = i;
				if (xcomdlist(p)) {
					(void) err(1,"bad comd list");
					break;
				}
			}
			i++;
		}
		else {
			if(k) {
				Dot = i;
				if (xcomdlist(p)) {
					(void) err(1,"bad comd list");
					break;
				}
			}
			i++;
		}
	}
}

void
kcomd(p)
struct Comd *p;
{
	register char c;

	if((c=peekc()) < 'a' || c > 'z') {
		(void) err(1,"bad mark");
		return;
	}
	(void) getch();
	if(more() || defaults(p,1,1,Dot,0,1,0)) return;

	mark[c] = Dot = p->Cadr[0];
}

void
xncomd(p)
struct Comd *p;
{
	register char c;

	if(more() || defaults(p,1,0,0,0,0,0)) return;

	for(c='a'; c<='z'; c++)
		if(mark[c]) (void) printf("%c\n",c);
}

void
pcomd(p)
struct Comd *p;
{
	register int i;
	char line[BFSBUF];

	if(more() || defaults(p,1,2,Dot,Dot,1,0)) return;

	for(i=p->Cadr[0]; i<=p->Cadr[1] && i>0; i++) {
		if(bigread(i,line) < 0)
			break;
		if(out(line) < 0)
			break;
	}
}

void
qcomd()
{
	if(!more())
		quit();
}

void
xcomds(p)
struct Comd *p;
{
	switch(getch()) {
		case 'b':	xbcomd(p);
		case 'c':	xccomd(p);
		case 'f':	xfcomd(p);
		case 'n':	xncomd(p);
		case 'o':	xocomd(p);
		case 't':	xtcomd(p);
		case 'v':	xvcomd();
		default:	(void) err(1,"bad command");
	}
}

void
xbcomd(p)
struct Comd *p;
{
	register int fail, n;
	register char d;
	char str[50];

	fail = 0;
	if(defaults(p,0,2,Dot,Dot,0,1)) fail = 1;
	else {
		if((d=getch()) == '\n') {
			(void) err(1,"syntax");
			return;
		}
		if(d == 'z') {
			if(status[0] != 0) return;
			(void) getch();
			if(getstr(1,str,0,0,0)) return;
			jump(1,str);
			return;
		}
		if(d == 'n') {
			if(status[0] == 0) return;
			(void) getch();
			if(getstr(1,str,0,0,0)) return;
			jump(1,str);
			return;
		}
		if(getstr(1,str,d,' ',0)) return;
		if((n=hunt(0,str,p->Cadr[0]-1,1,0,1)) < 0) fail = 1;
		if(getstr(1,str,0,0,0)) return;
		if(more()) {
			(void) err(1,"syntax");
			return;
		}
	}

	if(!fail) {
		Dot = n;
		jump(1,str);
	}
}

void
xccomd(p)
struct Comd *p;
{
	char arg[100];

	if(getstr(1,arg,0,' ',0) || defaults(p,1,0,0,0,0,0)) return;

	if(equal(arg,"")) crunch = -crunch;
	else if(equal(arg,"0")) crunch = -1;
	else if(equal(arg,"1")) crunch = 1;
	else (void) err(1,"syntax");
}

void
xfcomd(p)
struct Comd *p;
{
	char fl[100];
	register char *f;

	if(defaults(p,1,0,0,0,0,0)) return;

	while(peekc() == ' ') (void) getch();
	for(f=fl; (*f=getch()) != '\n'; f++);
	if(f==fl) {
		(void) err(1,"no file");
		return;
	}
	*f = '\0';

	(void) newfile(1,fl);
}

void
xocomd(p)
struct Comd *p;
{
	register int fd;
	char arg[100];

	if(getstr(1,arg,0,' ',0) || defaults(p,1,0,0,0,0,0)) return;

	if(!arg[0]) {
		if(outfildes == 1) {
			(void) err(1,"no diversion");
			return;
		}
		(void) close(outfildes);
		outfildes = 1;
	}
	else {
		if(outfildes != 1) {
			(void) err(1,"already diverted");
			return;
		}
		if((fd=creat(arg,0666)) < 0) {
			(void) syserr(1,"can't create");
			return;
		}
		outfildes = fd;
	}
}

void
xtcomd(p)
struct Comd *p;
{
	register int t;

	while(peekc() == ' ') (void) getch();
	if((t=rdnumb(1)) < 0 || more() || defaults(p,1,0,0,0,0,0))
		return;

	trunc = t;
}

void
xvcomd()
{
	register char c;
	register int i;
	int temp0, temp1, temp2;
	int fildes[2];

	if((c=peekc()) < '0' || c > '9') {
		(void) err(1,"digit required");
		return;
	}
	(void) getch();
	c -= '0';
	while(peekc() == ' ') (void) getch();
	if(peekc()=='\\') (void) getch();
	else if(peekc() == '!') {
		if(pipe(fildes) < 0) {
			(void) printf("Try again");
			return;
		}
		temp0 = dup(0);
		temp1 = dup(1);
		temp2 = infildes;
		(void) close(0);
		(void) dup(fildes[0]);
		(void) close(1);
		(void) dup(fildes[1]);
		(void) close(fildes[0]);
		(void) close(fildes[1]);
		(void) getch();
		flag4 = 1;
		excomd();
		(void) close(1);
		infildes = 0;
	}
	for(i=0;(varray[c][i] = getch()) != '\n';i++);
	varray[c][i] = '\0';
	if(flag4) {
		infildes = temp2;
		(void) close(0);
		(void) dup(temp0);
		(void) close(temp0);
		(void) dup(temp1);
		(void) close(temp1);
		flag4 = 0;
		charbuf = ' ';
	}
}

void
wcomd(p)
struct Comd *p;
{
	register int i, fd, savefd;
	int savecrunch, savetrunc;
	char arg[100], line[256];

	if(getstr(1,arg,0,' ',0) || defaults(p,1,2,1,Dollar,1,0)) return;
	if(!arg[0]) {
		(void) err(1,"no file name");
		return;
	}
	if(equal(arg,bigfile)) {
		(void) err(1,"no change indicated");
		return;
	}
	if((fd=creat(arg,0666)) <0) {
		(void) syserr(1,"can't create");
		return;
	}

	savefd = outfildes;
	savetrunc = trunc;
	savecrunch = crunch;
	outfildes = fd;
	trunc = BFSTRU;
	crunch = -1;

	outcnt = 0;
	for(i=p->Cadr[0]; i<=p->Cadr[1] && i>0; i++) {
		if(bigread(i,line) < 0)
			break;
		if(out(line) < 0)
			break;
	}
	if(verbose) (void) printf("%ld\n",outcnt);
	(void) close(fd);

	outfildes = savefd;
	trunc = savetrunc;
	crunch = savecrunch;
}

void
nlcomd(p)
struct Comd *p;
{
	if(defaults(p,1,2,Dot+1,Dot+1,1,0)) {
		(void) getch();
		return;
	}
	pcomd(p);
}

void
eqcomd(p)
struct Comd *p;
{
	if(more() || defaults(p,1,1,Dollar,0,0,0)) return;
	(void) printf("%d\n",p->Cadr[0]);
}

void
colcomd(p)
struct Comd *p;
{
	(void) defaults(p,1,0,0,0,0,0);
}


xcomdlist(p)
struct Comd *p;
{
	flag = 1;
	flag2 = 1;
	(void) newfile(1,"");
	while(flag2) begin(p);
	if(flag == 0) return(1);
	flag = 0;
	return(0);
}

void
excomd()
{
	register int i;

	if(infildes != 100) charbuf = '\n';
	while((i=fork()) < 0) (void) sleep(10);
	if(!i) {
		(void) signal(SIGINT, SIG_DFL); /*Guarantees child can be intr. */
		if(infildes == 100 || flag4) {
#ifdef S5EMUL
			(void) execl("/usr/5bin/sh","sh","-c",intptr,(char *) 0);
#else
			(void) execl("/bin/sh","sh","-c",intptr,(char *) 0);
#endif
			exit(0);
		}
		if(infildes != 0) {
			(void) close(0);
			(void) dup(infildes);
		}
		for(i=3; i<15; i++) (void) close(i);
#ifdef S5EMUL
		(void) execl("/usr/5bin/sh","sh","-t",(char *) 0);
#else
		(void) execl("/bin/sh","sh","-t",(char *) 0);
#endif
		exit(0);
	}
	(void) signal(SIGINT, SIG_IGN);
	while(wait(status) != i);
	status[0] = status[0] >> 8;
	(void) signal(SIGINT, reset);	/* Restore signal to previous status */

	if((infildes == 0 || (infildes  == 100 && fstack[fstack[0]] == 0))
				&& verbose && (!flag4)) (void) printf("!\n");
}


defaults(p,prt,max,def1,def2,setdot,errsok)
struct Comd *p;
int prt, max, def1, def2, setdot, errsok;
{
	if(!def1) def1 = Dot;
	if(!def2) def2 = def1;
	if(p->Cnumadr >= max) return(errsok?-1:err(prt,"adr count"));
	if(p->Cnumadr < 0) {
		p->Cadr[++p->Cnumadr] = def1;
		p->Cadr[++p->Cnumadr] = def2;
	}
	else if(p->Cnumadr < 1)
		p->Cadr[++p->Cnumadr] = p->Cadr[0];
	if(p->Cadr[0] < 1 || p->Cadr[0] > Dollar ||
	   p->Cadr[1] < 1 || p->Cadr[1] > Dollar)
		return(errsok?-1:err(prt,"range"));
	if(p->Cadr[0] > p->Cadr[1]) return(errsok?-1:err(prt,"adr1 > adr2"));
	if(setdot) Dot = p->Cadr[1];
	return(0);
}


getcomd(p,prt)
struct Comd *p;
int prt;
{
	register int r;
	register int c;

	p->Cnumadr = -1;
	p->Csep = ' ';
	switch(c = peekc()) {
		case ',':
		case ';':	p->Cop = getch();
				return(0);
	}

	if((r=getadrs(p,prt)) < 0) return(r);

	if((c=peekc()) < 0) return(err(prt,"syntax"));
	if(c == '\n') p->Cop = '\n';
	else p->Cop = getch();

	return(0);
}


getadrs(p,prt)
struct Comd *p;
int prt;
{
	register int r;
	register char c;

	if((r=getadr(p,prt)) < 0) return(r);

	switch(c=peekc()) {
		case ';':	Dot = p->Cadr[0];
		case ',':	(void) getch();
				p->Csep = c;
				return(getadr(p,prt));
	}

	return(0);
}


getadr(p,prt)
struct Comd *p;
int prt;
{
	register int r;
	register char c;

	r = 0;
	while(peekc() == ' ') (void) getch();	/* Ignore leading spaces */
	switch(c = peekc()) {
		case '\n':
		case ',':
		case ';':	return(0);

		case '\'':	(void) getch();
				r = getmark(p,prt);
				break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':	r = getnumb(p,prt);
				break;

		case '.':	(void) getch();
		case '+':
		case '-':	p->Cadr[++p->Cnumadr] = Dot;
				break;

		case '$':	(void) getch();
				p->Cadr[++p->Cnumadr] = Dollar;
				break;

		case '^':	(void) getch();
				p->Cadr[++p->Cnumadr] = Dot - 1;
				break;

		case '/':
		case '?':
		case '>':
		case '<':	(void) getch();
				r = getrex(p,prt,c);
				break;

		default:	return(0);
	}

	if(r == 0) r = getrel(p,prt);
	return(r);
}


getnumb(p,prt)
struct Comd *p;
int prt;
{
	register int i;

	if((i=rdnumb(prt)) < 0) return(-1);
	p->Cadr[++p->Cnumadr] = i;
	return(0);
}


rdnumb(prt)
int prt;
{
	char num[20], *n;
	int i;

	n = num;
	while((*n=peekc()) >= '0' && *n <= '9') {
		n++;
		(void) getch();
	}

	*n = '\0';
	if((i=patoi(num)) >= 0) return(i);
	return(err(prt,"bad num"));
}


getrel(p,prt)
struct Comd *p;
int prt;
{
	register int op, n;
	register char c;
	int j;

	n = 0;
	op = 1;
	while((c=peekc())=='+' || c=='-') {
		if(c=='+') n++;
		else n--;
		(void) getch();
	}
	j = n;
	if(n < 0) op = -1;
	if(c=='\n') p->Cadr[p->Cnumadr] += n;
	else {
		if((n=rdnumb(0)) > 0 && p->Cnumadr >= 0) {
			p->Cadr[p->Cnumadr] += op*n;
			(void) getrel(p,prt);
		}
		else {
			if(c=='-') p->Cadr[p->Cnumadr] += j;
			else p->Cadr[p->Cnumadr] += j;
		}
	}
	return(0);
}


getmark(p,prt)
struct Comd *p;
int prt;
{
	register char c;

	if((c=peekc()) < 'a' || c > 'z') return(err(prt,"bad mark"));
	(void) getch();

	if(!mark[c]) return(err(prt,"undefined mark"));
	p->Cadr[++p->Cnumadr] = mark[c];
	return(0);
}


getrex(p,prt,c)
struct Comd *p;
int prt;
char c;
{
	register int down, wrap, start;

	if(peekc() == c) (void) getch();
	else if(getstr(prt,currex,c,0,1)) return(-1);

	switch(c) {
		case '/':	down = 1; wrap = 1; break;
		case '?':	down = 0; wrap = 1; break;
		case '>':	down = 1; wrap = 0; break;
		case '<':	down = 0; wrap = 0; break;
	}

	if(p->Csep == ';') start = p->Cadr[0];
	else start = Dot;

	if((p->Cadr[++p->Cnumadr]=hunt(prt,currex,start,down,wrap,0)) < 0)
		return(-1);
	return(0);
}


hunt(prt,rex,start,down,wrap,errsok)
int prt, errsok;
char rex[];
int start, down, wrap;
{
	register int i, end1, incr;
	int start1, start2;
	char line[BFSBUF];

	if(down) {
		start1 = start + 1;
		end1 = Dollar;
		start2 = 1;
		incr = 1;
	}
	else {
		start1 = start  - 1;
		end1 = 1;
		start2 = Dollar;
		incr = -1;
	}

	if(compile(rex, rebuf,&rebuf[REBUF],'\0') == 0)
		return(errsok?-1:err(prt,"RE syntax"));

	for(i=start1; i != end1+incr; i += incr) {
		if(bigread(i,line) < 0)
			return(-1);
		if(step(line,rebuf)) {
			return(i);
		}
	}

	if(!wrap) return(errsok?-1:err(prt,"not found"));

	for(i=start2; i != start1; i += incr) {
		if(bigread(i,line) < 0)
			return(-1);
		if(step(line,rebuf)) {
			return(i);
		}
	}

	return(errsok?-1:err(prt,"not found"));
}

void
jump(prt,label)
int prt;
char label[];
{
	register char *l;
	char line[256];

	if(infildes == 0 && tty) {
		(void) err(prt,"jump on tty");
		return;
	}
	if(infildes == 100) intptr = internal;
	else (void) lseek(infildes,0L,0);

	(void) sprintf(strtmp, "^: *%s$", label);
	if(compile(strtmp, rebuf, &rebuf[REBUF], '\0') == 0)
		return;

	for(l=line; readc(infildes,l); l++) {
		if(*l == '\n') {
			*l = '\0';
			if(step(line, rebuf)) {
				charbuf = '\n';
				peeked = 0;
				return;
			}
			l = line - 1;
		}
	}

	(void) err(prt,"label not found");
}


getstr(prt,buf,strbrk,ignr,nonl)
int prt, nonl;
char buf[], strbrk, ignr;
{
	register char *b, c, prevc;

	prevc = 0;
	for(b=buf; c=peekc(); prevc=c) {
		if(c == '\n') {
			if(prevc == '\\' && (!flag3)) *(b-1) = getch();
			else if(prevc == '\\' && flag3) {
				*b++ = getch();
			}
			else if(nonl) break;
			else return(*b='\0');
		}
		else {
			(void) getch();
			if(c == strbrk) {
				if(prevc == '\\') *(b-1) = c;
				else return(*b='\0');
			}
			else if(b != buf || c != ignr) *b++ = c;
		}
	}
	return(err(prt,"syntax"));
}

char *
regerr(c)
int c;
{
	if(prompt)
		{switch(c) {
			case 11: (void) printf("Range endpoint too large.\n");
				break;
			case 16: (void) printf("Bad number.\n");
				break;
			case 25: (void) printf("\"\\digit\" out of range.\n");
				break;
			case 36: (void) printf("Illegal or missing delimiter.\n");
				break;
			case 41: (void) printf("No remembered search string.\n");
				break;
			case 42: (void) printf("\\(\\) imbalance.\n");
				break;
			case 43: (void) printf("Too many \\(.\n");
				break;
			case 44: (void) printf("More than 2 numbers given in \\{ \\}.\n");
				break;
			case 45: (void) printf("} expected after \\.\n");
				break;
			case 46: (void) printf("First number excceds second in \\{ \\}.\n");
				break;
			case 49: (void) printf("[] imbalance.\n");
				break;
			case 50: (void) printf("Regular expression overflow.\n");
				break;
			default: (void) printf("RE error.\n");
				break;
		}
	}
	if(!prompt)(void) printf("?\n");
	return(0);
}

err(prt,msg)
int prt;
char msg[];
{
	if(prt) {
		if(prompt)
			(void) printf("%s\n",msg);
		else
			(void) printf("?\n");
	}
	if(infildes != 0) {
		infildes = pop(fstack);
		charbuf = '\n';
		peeked = 0;
		flag3 = 0;
		flag2 = 0;
		flag = 0;
	}
	return(-1);
}

extern int errno, sys_nerr;
extern char *sys_errlist[];

syserr(prt,msg)
int prt;
char msg[];
{
	if(prt) {
		if(prompt)
			(void) printf("%s: %s\n", msg,
			    errno < sys_nerr ? sys_errlist[errno] : "Unknown error");
		else
			(void) printf("?\n");
	}
	return(err(0,""));
}


getch()
{
	if(!peeked) {
		while((!(infildes == oldfd && flag)) && (!flag1) && (!readc(infildes,&charbuf))) {
			if(infildes == 100 && (!flag)) flag1 = 1;
			if((infildes=pop(fstack)) == -1) quit();
			if((!flag1) && infildes == 0 && flag3 && prompt) (void) printf("*");
		}
		if(infildes == oldfd && flag) flag2 = 0;
		flag1 = 0;
	}
	else peeked = 0;
	return(charbuf);
}


readc(f,c)
int f;
char *c;
{
	if(f == 100) {
		if(!(*c = *intptr++)) {
			intptr--;
			charbuf = '\n';
			return(0);
		}
	}
	else if(read(f,c,1) != 1) {
		(void) close(f);
		charbuf = '\n';
		return(0);
	}
	return(1);
}


percent(line)
char line[256];
{
	register char *lp, *per, *var;
	char *front, c[2], *olp, p[2], fr[256], *strcpy();
	int i,j;

	per = p;
	var = c;
	front = fr;
	j = 0;
	circf = circf1;		/* Restore circf for step()  (regexp.h) */
	while(!j) {
		j = 1;
		olp = line;
		intptr = internal;
		while(step(olp,perbuf)) {
			while(loc1 < loc2) *front++ = *loc1++;
			*(--front) = '\0';
			front = fr;
			*per = '%';
			*var = *loc2;
			if((i = strlen(front)) >= 1 && fr[i-1] == '\\') {
				cat(front,"");
				--intptr;
				cat(per,"");
			}
			else {
				if(!(*var >= '0' && *var <= '9')) return(err(1,"usage: %digit"));
				cat(front,"");
				cat(varray[*var-'0'],"");
				j =0;
				loc2++;	/* Compensate for removing --lp above */
			}
			olp = loc2;
		}
		cat(olp,"");
		*intptr = '\0';
		if(!j) {
			intptr = internal;
			lp = line;
			(void) strcpy(lp,intptr);
		}
	}
	return(0);
}

cat(arg1,arg2)
char arg1[],arg2[];
{
	register char *arg;

	arg = arg1;
	while(*arg) *intptr++ = *arg++;
	if(*arg2) {
		arg = arg2;
		while(*arg) *intptr++ = *arg++;
	}
}


newfile(prt,f)
int prt;
char f[];
{
	register int fd;

	if(!*f) {
		if(flag != 0) {
			oldfd = infildes;
			intptr = comdlist;
		}
		else intptr = internal;
		fd = 100;
	}
	else if((fd=open(f,0)) < 0) {
		(void) sprintf(strtmp, "cannot open %s", f);
		return syserr(prt, strtmp);
	}

	push(fstack,infildes);
	if(flag4) oldfd = fd;
	infildes = fd;
	return(peeked=0);
}


push(s,d)
int s[], d;
{
	s[++s[0]] = d;
}


pop(s)
int s[];
{
	return(s[s[0]--]);
}


peekc()
{
	register char c;

	c = getch();
	peeked = 1;

	return(c);
}


eat()
{
	if(charbuf != '\n') while(getch() != '\n');
	peeked = 0;
}


more()
{
	if(getch() != '\n') return(err(1,"syntax"));
	return(0);
}


quit()
{
	exit(0);
}


out(ln)
char *ln;
{
	register char *rp, *wp, prev;
	char *untab();
	int lim;
	int cnt;

	if(crunch > 0) {

		ln = untab(ln);
		rp = wp = ln - 1;
		prev = ' ';

		while(*++rp) {
			if(prev != ' ' || *rp != ' ') *++wp = *rp;
			prev = *rp;
		}
		*++wp = '\n';
		lim = wp - ln;
		*++wp = '\0';

		if(*ln == '\n') return(0);
	}
	else ln[lim=strlen(ln)] = '\n';

	if(lim > trunc) ln[lim=trunc] = '\n';
	if((cnt = write(outfildes,ln,((unsigned) lim)+1)) < 0)
		return(syserr(1,"write error"));
	outcnt += cnt;
	return(0);
}




char *
untab(l)
char l[];
{
	static char line[BFSBUF];
	register char *q, *s;

	s = l;
	q = line;
	do {
		if(*s == '\t')
			do *q++ = ' '; while((q-line)%8);
		else *q++ = *s;
	} while(*s++);
	return(line);
}
/*
	Function to convert ascii string to integer.  Converts
	positive numbers only.  Returns -1 if non-numeric
	character encountered.
*/

patoi(b)
char *b;
{
	register int i;
	register char *a;

	a = b;
	i = 0;
	while(*a >= '0' && *a <= '9') i = 10 * i + *a++ - '0';

	if(*a) return(-1);
	return(i);
}
