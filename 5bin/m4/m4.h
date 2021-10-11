/*	@(#)m4.h 1.1 92/07/30 SMI; from S5R2 1.3	*/

#include	<ctype.h>

#define EOS	'\0'
#define LOW7	0177
#define MAXSYM	5
#define PUSH	1
#define NOPUSH	0
#define OK	0
#define NOT_OK	1

#define	putbak(c)	(ip < ibuflm? (*ip++ = (c)): error2(pbmsg,bufsize))
#define	stkchr(c)	(op < obuflm? (*op++ = (c)): error2(aofmsg,bufsize))
#define sputchr(c,f)	(putc(c,f)=='\n'? lnsync(f): 0)
#define putchr(c)	(Cp?stkchr(c):cf?(sflag?sputchr(c,cf):putc(c,cf)):0)

struct bs {
	int	(*bfunc)();
	char	*bname;
};

struct	call {
	char	**argp;
	int	plev;
};

struct	nlist {
	char	*name;
	char	*def;
	char	tflag;
	struct	nlist *next;
};

extern FILE	*cf;
extern FILE	*ifile[];
extern FILE	*ofile[];
extern FILE	*xfopen();
extern char	**Ap;
extern char	**argstk;
extern char	*Wrapstr;
extern char	*astklm;
extern char	*inpmatch();
extern char	*chkbltin();
extern char	*calloc();
extern char	*xcalloc();
extern char	*copy();
extern char	*mktemp();
extern char	*strcpy();
extern char	*fname[];
extern char	*ibuf;
extern char	*ibuflm;
extern char	*ip;
extern char	*ipflr;
extern char	*ipstk[10];
extern char	*obuf;
extern char	*obuflm;
extern char	*op;
extern char	*procnam;
extern char	*tempfile;
extern char	*token;
extern char	*toklm;
extern int	C;
extern int	getchr();
extern char	aofmsg[];
extern char	astkof[];
extern char	badfile[];
extern char	fnbuf[];
extern char	lcom[];
extern char	lquote[];
extern char	nocore[];
extern char	nullstr[];
extern char	pbmsg[];
extern char	rcom[];
extern char	rquote[];
extern char	type[];
extern int	bufsize;
extern int	catchsig();
extern int	fline[];
extern int	hshsize;
extern int	hshval;
extern int	ifx;
extern int	nflag;
extern int	ofx;
extern int	sflag;
extern int	stksize;
extern int	sysrval;
extern int	toksize;
extern int	trace;
extern long	ctol();
extern struct bs	barray[];
extern struct call	*Cp;
extern struct call	*callst;
extern struct nlist	**hshtab;
extern struct nlist	*install();
extern struct nlist	*lookup();
