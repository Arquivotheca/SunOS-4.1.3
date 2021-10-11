/* 
 * Installtxt - installs and maintains archives of message strings
 * which are subsequently used by the gettext() utility.
 * Version of /usr/group proposal 12th Oct 88.
 *
 */

#ifndef lint
static char *sccsid = "@(#)installtxt.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * installtxt. Utility to install and maintain archives of message 
 * text.
 * If any of the message archive data structures change here 
 * then they must change in gettext() also
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/file.h>
#include "gettext.h"

#define	ARMAG	"!<LC_MESSAGES>\n"
#define ARFMAG  "`\n"
#define	SARMAG	15
#define SKIP    1
#define IODD    2
#define OODD    4
#define HEAD    8
#define	MAXTAGLEN		127
#define MAXMSGLEN		255
#define CMD_DOMAIN		"domain"
#define CMD_DOMAIN_LEN		6
#define CMD_SEP			"separator"
#define CMD_SEP_LEN		9
#define CMD_QUOTE		"quote"
#define CMD_QUOTE_LEN		5
#define CMD_FORMAT		"format"
#define CMD_FORMAT_LEN		6
#define BUFSIZE			512

struct	stat	stbuf;
struct	ar_hdr	arbuf;
struct	lar_hdr {
	unsigned char	lar_name[16];
	long	lar_date;
	u_short	lar_uid;
	u_short	lar_gid;
	u_short	lar_mode;
	long	lar_size;
	unsigned char lar_sep;
	unsigned char lar_quote;
} larbuf;

char	*man	=	{ "cdrtx" };
char	*opt	=	{ "ouv" };

char 	linebuf[MAXTAGLEN+MAXMSGLEN+4];
char 	message_tag[MAXTAGLEN+1];
char 	message_text[MAXMSGLEN+1];
char 	domain[MAXDOMAIN+1];
char 	format[MAXFMTS+1] = "";
short 	format_type;
int	countofrecords;
long 	charsleft;
char 	pattern[15] = "%[^ ]%*c%[^\n]";
int	signum[] = {SIGHUP, SIGINT, SIGQUIT, 0};
int	sigdone();
long	lseek();
int	rcmd();
int	dcmd();
int	xcmd();
int	tcmd();
int	ccmd();
int	(*comfun)();
char	flg[26];
char	**namv;
int	namc;
char	*arnam;
char	*tmpfnam	=	{ "/tmp/vXXXXXX" };
char	*tmp1nam	=	{ "/tmp/v1XXXXXX" };
char	*tmp2nam	=	{ "/tmp/v2XXXXXX" };
char	*tfnam;
char	*tf1nam;
char	*tf2nam;
char	*file;
char	name[16];
int	af;
int	tf;
int	tf1;
int	tf2;
int	qf;
int	bastate;
char	buf[MAXBSIZE];
int	truncate;			/* ok to truncate argument filenames */
int	cur_address;

char	*trim();
char	*mktemp();
char	*ctime();
char	*strncpy();


/* This routine will fill up the output buffer pointed to by buff
 * with the escape expanded string, ready for human consumption
 * TODO : need notion of line length to do backslash-newline sequence
 *	if necessary on output
 */

short expandesc(buff, ch)
char *buff; unsigned short ch; 
{

	switch (ch) {
	case '\n':
	        *buff = '\\';
		*(buff+1) = 'n';
		return 2;
	case '\r':
	        *buff = '\\';
	        *(buff+1) = 'r';
		return 2;
	case '\b':
	        *buff = '\\';
	        *(buff+1) = 'b';
		return 2;
	case '\t':
	        *buff = '\\';
	        *(buff+1) = 't';
		return 2;
	case '\v':
	        *buff = '\\';
	        *(buff+1) = 'v';
		return 2;
	case '\f':
	        *buff = '\\';
	        *(buff+1) = 'f';
		return 2;
	case '\\':
	        *buff = '\\';
		*(buff+1) = '\\';
		return 2;
	case '\'':
	        *buff = '\\';
		*(buff+1) = '\'';
		return 2;
	case '\"':
	        *buff = '\\';
	        *(buff+1) = '\"';
		return 2;
	default:
		*buff = ch;
		return 1;
	}
}


setcom(fun)
int (*fun)();
{

	if(comfun != 0) {
		fprintf(stderr, "installtxt: only one of [%s] allowed\n", man);
		done(1);
	}
	comfun = fun;
}

rcmd()
{
	register f;

	truncate++;
	init();
	getaf();
	while(!getdir()) {
		if(namc == 0 || match()) {
			f = stats();
			if(f < 0) {
				if(namc)
					fprintf(stderr, "installtxt: cannot open %s\n", file);
				goto cp;
			}
			if(flg['u'-'a'])
				if(stbuf.st_mtime <= larbuf.lar_date) {
					close(f);
					goto cp;
				}
			mesg('r');
			copyfil(af, -1, SKIP);
			movefil(f);
			continue;
		}
	cp:
		mesg('c');
		copyfil(af, tf, HEAD);
	}
	cleanup();
}

ccmd() {
	rcmd();
}

dcmd()
{

	init();
	if(getaf())
		noar();
	while(!getdir()) {
		if(match()) {
			mesg('d');
			copyfil(af, -1, SKIP);
			continue;
		}
		mesg('c');
		copyfil(af, tf, HEAD);
	}
	install();
}

xcmd()
{
	register f;
	register short i;
	struct timeval tv[2];
	unsigned char buf[10];
	FILE *tempf;

	if(getaf())
		noar();
	while(!getdir()) {
		if(namc == 0 || match()) {
			f = open(file, O_RDWR | O_CREAT, larbuf.lar_mode & 0777);

			if(f < 0) {
				perror(file); 
				goto sk;
			}
			mesg('x');

			msg_extract(f);
			close(tf);
			close(f);
			if (flg['o'-'a']) {
				tv[0].tv_sec = tv[1].tv_sec = larbuf.lar_date;
				tv[0].tv_usec = tv[1].tv_usec = 0;
				utimes(file, tv);
			}
			continue;
		}
	sk:
		mesg('c');
		copyfil(af, -1, SKIP); 
		
		if (namc > 0  &&  !morefil())
			done(0);
	}
}


tcmd()
{

	if(getaf())
		noar();
	while(!getdir()) {
		if(namc == 0 || match()) {
			if(flg['v'-'a'])
				longt();
			printf("%s\n", trim(file, 0));
		}
		copyfil(af, -1, SKIP);
	}
}


init()
{

	tfnam = mktemp(tmpfnam);
	close(creat(tfnam, 0600));
	tf = open(tfnam, 2);
	if(tf < 0) {
		fprintf(stderr, "installtxt: cannot create temp file\n");
		done(1);
	}
	if (write(tf, ARMAG, SARMAG) != SARMAG)
		wrerr();
}

getaf()
{
	char mbuf[SARMAG];

	af = open(arnam, 0);
	if(af < 0)
		return(1);
	if (read(af, mbuf, SARMAG) != SARMAG || strncmp(mbuf, ARMAG, SARMAG)) {
		fprintf(stderr, "installtxt: %s not in message archive format\n", arnam);
		done(1);
	}
	return(0);
}

getqf()
{
	char mbuf[SARMAG];

	if ((qf = open(arnam, 2)) < 0) {
		if(!flg['c'-'a'])
			fprintf(stderr, "installtxt: creating %s\n", arnam);
		if ((qf = creat(arnam, 0666)) < 0) {
			fprintf(stderr, "installtxt: cannot create %s\n", arnam);
			done(1);
		}
		if (write(qf, ARMAG, SARMAG) != SARMAG)
			wrerr();
	} else if (read(qf, mbuf, SARMAG) != SARMAG
		|| strncmp(mbuf, ARMAG, SARMAG)) {
		fprintf(stderr, "installtxt: %s not in message archive format\n", arnam);
		done(1);
	}
}        
usage()
{
	fprintf(stderr, "usage: installtxt [%s][%s] source-message-files ...\n", man, opt);
	done(1);
}

noar()
{

	fprintf(stderr, "installtxt: %s does not exist\n", arnam);
	done(1);
}

sigdone()
{
	done(100);
}

done(c)
{

	if(tfnam)
		unlink(tfnam);
	if(tf1nam)
		unlink(tf1nam);
	if(tf2nam)
		unlink(tf2nam);
	exit(c);
}

notfound()
{
	register i, n;

	n = 0;
	for(i=0; i<namc; i++)
		if(namv[i]) {
			fprintf(stderr, "installtxt: %s not found\n", namv[i]);
			n++;
		}
	return(n);
}

morefil()
{
	register i, n;

	n = 0;
	for(i=0; i<namc; i++)
		if(namv[i])
			n++;
	return(n);
}

cleanup()
{
	register i, f;

	truncate++;
	for(i=0; i<namc; i++) {
		file = namv[i];
		if(file == 0)
			continue;
		namv[i] = 0;
		mesg('a');
		f = stats();
		if(f < 0) {
			fprintf(stderr, "installtxt: %s cannot open\n", file);
			continue;
		}
		movefil(f);
	}
	install();
}

install()
{
	register i;

	for(i=0; signum[i]; i++)
		signal(signum[i], SIG_IGN);
	if(af < 0)
		if(!flg['c'-'a'])
			fprintf(stderr, "installtxt: creating %s\n", arnam);
	close(af);
	af = creat(arnam, 0666);
	if(af < 0) {
		fprintf(stderr, "installtxt: cannot create %s\n", arnam);
		done(1);
	}
	if(tfnam) {
		lseek(tf, 0l, 0);
		while((i = read(tf, buf, MAXBSIZE)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	if(tf2nam) {
		lseek(tf2, 0l, 0);
		while((i = read(tf2, buf, MAXBSIZE)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	if(tf1nam) {
		lseek(tf1, 0l, 0);
		while((i = read(tf1, buf, MAXBSIZE)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
}

/*
 * insert the file 'file'
 * into the archive temporary file
 */
movefil(f)
{
	char buf[sizeof(arbuf)+1];

	sprintf(buf, "%-16s%-12ld%-6u%-6u%-8o%-10ld%2c%1c%-2s",
	   trim(file, 1),
	   stbuf.st_mtime,
	   (u_short)stbuf.st_uid,
	   (u_short)stbuf.st_gid,
	   stbuf.st_mode,
	   stbuf.st_size,
	   larbuf.lar_sep,
	   larbuf.lar_quote,
	   ARFMAG);
	memcpy((char *)&arbuf, buf, sizeof(arbuf));
	larbuf.lar_size = stbuf.st_size;
	msg_build(f);

	/* Copy the new binary archive format into place in the archive file 
	 * keep old statistics as if the source file is being represented.
	 */

	close(f); 
}

stats()
{
	register f;

	f = open(file, 0);
	if(f < 0)
		return(f);
	if(fstat(f, &stbuf) < 0) {
		close(f);
		return(-1);
	}
	return(f);
}

/*
 * copy next file
 * size given in arbuf
 */
copyfil(fi, fo, flag)
{
	register i, o;
	int pe;

	if(flag & HEAD) {
		for (i=sizeof(arbuf.ar_name)-1; i>=0; i--) {
			if (arbuf.ar_name[i]==' ')
				continue;
			else if (arbuf.ar_name[i]=='\0')
				arbuf.ar_name[i] = ' ';
			else
				break;
		}
		if (write(fo, (char *)&arbuf, sizeof arbuf) != sizeof arbuf)
			wrerr();
	}
	pe = 0;
	while(larbuf.lar_size > 0) {
		i = o = MAXBSIZE;
		if(larbuf.lar_size < i) {
			i = o = larbuf.lar_size;
			if(i&1) {
				buf[i] = '\n';
				if(flag & IODD)
					i++;
				if(flag & OODD)
					o++;
			}
		}
		if(read(fi, buf, i) != i)
			pe++;
		if((flag & SKIP) == 0)
			if (write(fo, buf, o) != o)
				wrerr();
		larbuf.lar_size -= MAXBSIZE;
	}
	if(pe)
		phserr();
}

getdir()
{
	register unsigned char *cp;
	register i;

	i = read(af, (char *)&arbuf, sizeof arbuf);
	if(i != sizeof arbuf) {
		if(tf1nam) {
			i = tf;
			tf = tf1;
			tf1 = i;
		}
		return(1);
	}
	if (strncmp(arbuf.ar_fmag, ARFMAG, sizeof(arbuf.ar_fmag))) {
		fprintf(stderr, "installtxt: malformed message archive (at %ld)\n", lseek(af, 0L, 1));
		done(1);
	}
	cp = arbuf.ar_name + sizeof(arbuf.ar_name);
	while (*--cp==' ')
		;
	if (*cp == '/')
		*cp = '\0';
	else
		*++cp = '\0';
	strncpy(name, arbuf.ar_name, sizeof(arbuf.ar_name));
	file = name;
	strncpy(larbuf.lar_name, name, sizeof(larbuf.lar_name));
	sscanf(arbuf.ar_date, "%ld", &larbuf.lar_date);
	sscanf(arbuf.ar_uid, "%hd", &larbuf.lar_uid);
	sscanf(arbuf.ar_gid, "%hd", &larbuf.lar_gid);
	sscanf(arbuf.ar_mode, "%ho", &larbuf.lar_mode);
	sscanf(arbuf.ar_size, "%ld", &larbuf.lar_size);
	larbuf.lar_sep = arbuf.ar_sep;
	larbuf.lar_quote = arbuf.ar_quote;
	return(0);
}

     
match()
{
	register i;

	for(i=0; i<namc; i++) {
		if(namv[i] == 0)
			continue;
		if(strcmp(trim(namv[i], 0), file) == 0) {
			file = namv[i];
			namv[i] = 0;
			return(1);
		}
	}
	return(0);
}


phserr()
{

	fprintf(stderr, "installtxt: phase error on %s\n", file);
}

mesg(c)
{

	if(flg['v'-'a'])
		if(c != 'c' || flg['v'-'a'] > 1)
			printf("%c - %s\n", c, file);
}

char *
trim(s, verb)
char *s;
{
	register char *p1, *p2;
	static char trimmedname[sizeof(arbuf.ar_name)];

	/* Strip trailing slashes */
	for(p1 = s; *p1; p1++)
		;
	while(p1 > s) {
		if(*--p1 != '/')
			break;
		*p1 = 0;
	}

	/* Find last component of path; do not zap the path */
	p2 = s;
	for(p1 = s; *p1; p1++)
		if(*p1 == '/')
			p2 = p1+1;

	/*
	 * Truncate name if too long, only if we are doing an 'add'
	 * type operation. We only allow 15 cause rest of ar
	 * isn't smart enough to deal with non-null terminated
	 * names.  Need an exit status convention...
	 * Need yet another new archive format...
	 */
	if (truncate && strlen(p2) > sizeof(arbuf.ar_name) - 1) {
		if (verb) fprintf(stderr, "installtxt: filename %s truncated to ", p2);
		p2 = strncpy(trimmedname,p2,sizeof(arbuf.ar_name) - 1);
		*(p2 + sizeof(arbuf.ar_name) - 1) = '\0';
		if (verb) fprintf(stderr, "%s\n", p2);
	}
	return(p2);
}

#define	IFMT	060000
#define	ISARG	01000
#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000

longt()
{
	register char *cp;

	pmode();
	printf("%3d/%1d", larbuf.lar_uid, larbuf.lar_gid);
	printf("%7ld", larbuf.lar_size);
	cp = ctime(&larbuf.lar_date);
	printf(" %-12.12s %-4.4s ", cp+4, cp+20);
}

int	m1[] = { 1, ROWN, 'r', '-' };
int	m2[] = { 1, WOWN, 'w', '-' };
int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int	m4[] = { 1, RGRP, 'r', '-' };
int	m5[] = { 1, WGRP, 'w', '-' };
int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int	m7[] = { 1, ROTH, 'r', '-' };
int	m8[] = { 1, WOTH, 'w', '-' };
int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

pmode()
{
	register int **mp;

	for (mp = &m[0]; mp < &m[9];)
		select(*mp++);
}

select(pairp)
int *pairp;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n>=0 && (larbuf.lar_mode&*ap++)==0)
		ap++;
	putchar(*ap);
}

wrerr()
{
	perror("installtxt: write error");
	done(1);
}
/*
 * Scan Source message file and output to a temporary file
 */

msg_build(fd)
int fd;
{
	register int i, j;
	unsigned char c;
	int linecount;
	char *obj1name, *obj2name;
	FILE *inputf;
	char tmparr[1]; 
	struct msg_header msg_header;
	char buf[BUFSIZE];	
	tmparr[0] = '\0';

	/* Open up two temporary files. tf1 will contain the member file header 
	 * information that gettext() will actually use in its runtime
	 * structures. tf2 will contain the concatenated target strings, either 
	 * strings or both tag and string depending on current value 
	 * of format string. When source file is exhausted we will merge
	 * back these two files.
	 */

	obj1name = mktemp(tmp1nam);
	close(creat(obj1name, 0600));
	tf1 = open(obj1name, 2);
	if(tf1 < 0) {
		fprintf(stderr, "installtxt: cannot create temp file\n");
	        done(1);
	}

	/* Put out a blank index header, which is overwritten with real
	 * values later one. 
	 */

	write(tf1, &msg_header, sizeof(struct msg_header)); 

	obj2name = mktemp(tmp2nam);
	close(creat(obj2name, 0600));
	tf2 = open(obj2name, 2);
	if(tf2 < 0) {
		fprintf(stderr, "installtxt: cannot create temp file\n");
	        done(1);
	}

	cur_address = 0; /* used from index section as relative offset 
			  * into target messages
			  */
	countofrecords = 0; /* output later in msg_header struct */

	inputf = fdopen(fd, "r");
	linecount = 0;

	/* The default separator is a space character */

	arbuf.ar_sep = pattern[3] = ' ';

	/* The default format of message tag is a regular string */

	format_type = STR;
	arbuf.ar_quote = 0;
	while (fgets(linebuf, MAXTAGLEN+MAXMSGLEN+4, inputf) != 0){
		linecount++;
		if ((i = skip_blanks(linebuf, 0)) == -1)
			continue;
	
		if (linebuf[i] == '$'){
			i++;
			/*
	       		 * Handle commands or comments
	       		 */
			if (strncmp(&linebuf[i], CMD_DOMAIN, CMD_DOMAIN_LEN) == 0){
				i += CMD_DOMAIN_LEN+1;
	
				/*
	         		 * Change domain
	         		 */

				strcpy(domain, &linebuf[i]);
				continue;
			}
			if (strncmp(&linebuf[i], CMD_SEP, CMD_SEP_LEN) == 0){
				i += CMD_SEP_LEN;
	
				/*
	         		 * Use a new separator character
	         		 */
	
				if ((c =linebuf[i+1]) == '\n')
					c = ' ';
				arbuf.ar_sep = pattern[3] = c;
				continue;
			}
			if (strncmp(&linebuf[i], CMD_QUOTE, CMD_QUOTE_LEN) == 0){
				i += CMD_QUOTE_LEN;
	
				/*
	         		 * Change quote character
	         		 */
	
				if ((i = skip_blanks(linebuf, i)) == -1)
					arbuf.ar_quote = 0;
				else
					arbuf.ar_quote  = linebuf[i];
				continue;
			}
			if (strncmp(&linebuf[i], CMD_FORMAT, CMD_FORMAT_LEN) == 0){
				i += CMD_FORMAT_LEN+1;
				while(linebuf[i] != '%' && linebuf[i] !=
0 )
					i++;
				
				if (linebuf[i] == 0) {
					fprintf(stderr, "Format string error on line %d\n", linecount);
					done(1);
				}
	
				if (strlen(&linebuf[i]) > MAXFMTS) {
					fprintf(stderr, "Format string too long at line %d\n", linecount);
					done(1);
				}	
				strcpy(format, &linebuf[i]);
				j=1;
				while (isdigit(format[j]))
					j++;
				if (format[j] != '[' & format[j] != 's')
					format_type = INT;
				else
					format_type = STR;
				
				continue;
			}
	
			/*
	       		 * Everything else is a comment
	       		 */
			continue;
		}
		if (linebuf[i] == '\\' && linebuf[i+1] == '$')
			/* We are escaping a message tag that actually uses
			 * $ as a first character !
			 */
			i++;

      

		/* Join up any newline escaped lines */

		while (strcmp(&linebuf[strlen(linebuf)-2],"\\\n") == 0) {
			linecount++;
			if (fgets(&linebuf[strlen(linebuf)-2], MAXTAGLEN+MAXMSGLEN+4, inputf) == 0) {
				fprintf(stderr, "line continuation expected on line %d\n",
				  	linecount);
				done(1);
			}


			if (strlen(linebuf) > MAXTAGLEN+MAXMSGLEN+4) {
				fprintf(stderr, "line too long: line %d\n", linecount);
				done(1);
			}
		}	
		if (arbuf.ar_quote != 0) {

			/*
			 * Quote character is the anchor point 
			 * for message text and message tag portion
			 */
			j=i+1;
			if (linebuf[i] == arbuf.ar_quote) {
				while(linebuf[j] != arbuf.ar_quote && linebuf[j] != 0) {
					if (linebuf[j] == '\\')
						j++;
					j++;
				}
			} else {
				while(!isspace(linebuf[j]))
					j++; 
				--i;
			}
			linebuf[j] = 0;
			strcpy(message_tag, &linebuf[i+1]);
#ifdef DEBUG
			fprintf(stderr,"just copied TAG %s \n", message_tag);
#endif
			i=j+1;
			while(linebuf[i] != arbuf.ar_quote && linebuf[i] !=
0)
                                i++;
                        j=i+1;
                        while(linebuf[j] != arbuf.ar_quote && linebuf[j] !=
0) {
				if (linebuf[j] == '\\')
					j++;
                                j++;
			}
			linebuf[j] = 0;
                        strcpy(message_text, &linebuf[i+1]);
#ifdef DEBUG
                        fprintf(stderr,"just copied TEXT %s \n", message_text);	
#endif
		} else 
			if (sscanf(&linebuf[i], pattern, message_tag, message_text) != 2) {
				fprintf(stderr, "Bad message-tag syntax at line %d\n", linecount);
				done(1);
			}
	
		/* Now do the real work of compiling in the new
		 * line into the message archive file. This work is done
	 	 * inside a member archive file.
		 */
		
		addentry(tf1, tf2);
	}

	/*
	 * Get the real size of the archive member file in object
	 * form and replace this value in the member file header
	 */

	stbuf.st_size = lseek(tf1, 0, L_INCR) + lseek(tf2, 0, L_INCR);
	sprintf(arbuf.ar_size,"%-10ld", stbuf.st_size);
	
	/* Now reset size to be real, for copyfil */
	larbuf.lar_size = lseek(tf1, 0, L_INCR);

	/* rewind the object member file, ready for assembly into 
	 * the final archive. Copyfil will place this file in the 
	 * correct place within the archive
	 */

	msg_header.maxno = countofrecords;
	msg_header.ptr = lseek(tf1, 0, L_INCR);
	msg_header.format_type = format_type;
	msg_header.format[MAXFMTS-1] = '!';
	strcpy(msg_header.format, format);
	if (lseek(tf1, 0, L_SET) == -1) {
		fprintf("installtxt: Error in reading temp file\n");
		done(1);
	}
	write(tf1, &msg_header, sizeof(struct msg_header)); 
	if (lseek(tf1, 0, L_SET) == -1) {
		fprintf("installtxt: Error in reading temp file\n");
		done(1);
	}
	if (lseek(tf2, 0, L_SET) == -1) {
		fprintf("installtxt: Error in reading temp file\n");
		done(1);
	}
	/* Note this outputs the archive header too */
	copyfil(tf1, tf, HEAD);

	/* Dont put any fluff between hdr and tail here! */

	while ((i = read(tf2, buf, BUFSIZE)) > 0)
		write(tf, buf, i);

	/* get rid of temp files */

	close(tf1); unlink(obj1name);
	close(tf2); unlink(obj2name);
	fclose(inputf);
	
}


/*
 * Skip blanks in a line buffer
 */

skip_blanks(linebuf, i)
char *linebuf;
int i;
{
	while (linebuf[i] && isspace(linebuf[i]) && !iscntrl(linebuf[i]))
		i++;
	if (!linebuf[i] || linebuf[i] == '\n')
		return -1;
	return i;
}

addentry(hdr, tail)
int hdr, tail;

{
char 	str[MAXTAGLEN+MAXMSGLEN+4];
struct 	index	index;
short 	arrays;
char 	*op, *cp;
unsigned char 	c;

/* This is simple flat file, use hashing later */

op=str;
arrays = 0;
for (cp=message_tag; arrays < 2;cp=message_text) {

	arrays++;
	while ((c = *(cp++)) != 0) {
		switch(c) {
		case '\\':
			c = *(cp++);
			switch(c) {
			case 'n':  
				c = '\n'; 
				break;
			case 'r':  
				c = '\r'; 
				break;
			case 'b':  
				c = '\b'; 
				break;
			case 't':  
				c = '\t'; 
				break;
			case 'v':  
				c = '\v'; 
				break;
			case 'f':  
				c = '\f'; 
				break;
			case '\\': 
				c = '\\'; 
				break;
			case '\'': 
				c = '\''; 
				break;
			case '\"': 
				c = '\"'; 
				break;
			default:
				if (isdigit(c)) {
					c = (int) strtol((cp-1), &cp, 8); 
				}
			}
			
		/* a plain 'ole input character */
		
		default:
			*op++ = c;
		}
	}
	
	/* A null separates the message tag and message text here */

	*op++ = 0;
	if (arrays == 1) {
		/* output the index */
		countofrecords++;
		if (format_type == INT)
			sscanf(str, format, &index.msgnumber);
		else
			index.msgnumber = 0;
		index.rel_addr = cur_address;
#ifdef DEBUG
		fprintf(stderr, "writing %d bytes to temp1\n", write(hdr, &index, sizeof (struct index) ));
#else
		write(hdr, &index, sizeof(struct index));
#endif
	} else {
#ifdef DEBUG
		fprintf(stderr, "writing %d bytes to temp2\n", op-str);
#endif
		if (format_type == STR) {

			/* write both tag and target string */

			write(tail, str, op-str);
			cur_address += op-str;
		} else {

			/* only write the target string */

			op=str+strlen(str) + 1;
#ifdef DEBUG
			fprintf(stderr,"writing %d bytes for int lookup\n",strlen(op)+1);
#endif
			write(tail, op, strlen(op) + 1);
			cur_address += strlen(op) +1;
		}
	}
}
}



msg_extract(f)
register int f; 
{	
	FILE *tempf;
	register short i;
	register short ch;
	
	/* here we create a temp file, copy the 
	 * whole member file to it. then we remove init();
	 */
	init();
	copyfil(af, tf, 0);
	lseek(tf, 0, L_SET);
	tempf = fdopen(tf, "r");
	if (fseek(tempf, SARMAG, 0) == -1) {
		fprintf("installtxt: seek error in tmp file\n");
		done(1);
	}
	if (larbuf.lar_quote != 0) {
		sprintf(buf,"$quote %c\n", larbuf.lar_quote);
		write(f, buf, 9);
	}
	if (larbuf.lar_sep != ' ') {
		sprintf(buf,"$separator %c\n", larbuf.lar_sep);
		write(f, buf, 13);
	}
	if (domain[0] != 0) {
		sprintf(buf, "$domain %s\n\n", domain);
		write(f, buf, 11 + strlen(domain));
	}
	sscanf(arbuf.ar_size, "%ld", &charsleft);

	/* There are two spare nulls at end of member file entry in the 
	 * archive
	 */

	while (charsleft > 2) {
		i = 0;
		while ((ch = getc(tempf)) != 0 && ch != EOF) {
			i+= expandesc(message_tag+i, ch);
		}
		if (ch == -1)
			break;
		message_tag[i] = 0;
		i = 0;
		while ((ch = getc(tempf)) != 0 && ch != EOF) {
			i+= expandesc(message_text+i, ch);
		}
		if (ch == -1)
			break;
		message_text[i] = 0;
		sprintf(buf,"%s%c", message_tag, arbuf.ar_sep);
		write(f, buf, 1 + strlen(message_tag));
		if (larbuf.lar_quote != 0) {
			sprintf(buf,"%c%s%c\n", larbuf.lar_quote,
				message_text, larbuf.lar_quote);
			write(f, buf, 3 + strlen(message_text));
		} else {
			sprintf(buf,"%s\n", message_text);
			write(f, buf, 1 + strlen(message_text));
		}
	}
	fclose(tempf); unlink(tfnam);
}


main(argc, argv)
char *argv[];
{
	register i;
	register char *cp;

	setlocale(LC_CTYPE,""); 
	for(i=0; signum[i]; i++)
		if(signal(signum[i], SIG_IGN) != SIG_IGN)
			signal(signum[i], sigdone);
	if(argc < 3)
		usage();
	cp = argv[1];
	if (*cp == '-')	/* skip a '-', make ar more like other commands */
		cp++;
	for(; *cp; cp++)
	switch(*cp) {
	case 'o':
	case 'v':
	case 'u':
		flg[*cp - 'a']++;
		continue;

	case 'r':
		setcom(rcmd);
		continue;

	case 'd':
		setcom(dcmd);
		continue;

	case 'x':
		setcom(xcmd);
		continue;

	case 't':
		setcom(tcmd);
		continue;

	case 'c':
		setcom(ccmd);
		continue;

	default:
		fprintf(stderr, "installtxt: bad option `%c'\n", *cp);
		done(1);
	}
	arnam = argv[2];
	namv = argv+3;
	namc = argc-3;
	if(comfun == 0) {
		if(flg['u'-'a'] == 0) {
			fprintf(stderr, "installtxt: one of [%s] must be specified\n", man);
			done(1);
		}
		setcom(rcmd);
	}
	(*comfun)();
	done(notfound());
	/* NOTREACHED */
}
