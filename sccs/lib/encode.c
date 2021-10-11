# include       "../hdr/defines.h"

static char sccsid[]= "@(#)@(#)encode.c 1.1 92/07/30 SMI";

/* ENC is the basic 1 character encoding function to make a char printing */
#define ENC(c) (((c) & 077) + ' ')

/* single character decode */
#define DEC(c)	(((c) - ' ') & 077)

encode(infile,outfile)
FILE *infile;
FILE *outfile;
{
	char buf[80];
	int i,n;

	for (;;) 
	{
		/* 1 (up to) 45 character line */
		n = fr(infile, buf, 45);
		putc(ENC(n), outfile);
		for (i=0; i<n; i += 3)
			e_outdec(&buf[i], outfile);

		putc('\n', outfile);
		if (n <= 0)
			break;
	}
}

/*
 * output one group of 3 bytes, pointed at by p, on file f.
 */
e_outdec(p, f)
char *p;
FILE *f;
{
        int c1, c2, c3, c4;
 
  
	c1 = *p >> 2;
	c2 = (*p << 4) & 060 | (p[1] >> 4) & 017;
	c3 = (p[1] << 2) & 074 | (p[2] >> 6) & 03;
	c4 = p[2] & 077;
	putc(ENC(c1), f);
	putc(ENC(c2), f);
	putc(ENC(c3), f);
	putc(ENC(c4), f);
}

decode(istr,outfile)
char *istr;
FILE *outfile;
{
	char *bp;
	int n;

	n = DEC(istr[0]);
	if (n <= 0)
		return;

	bp = &istr[1];
	while (n > 0) {
		d_outdec(bp, outfile, n);
		bp += 4;
		n -= 3;
	}
}

/*
 * output a group of 3 bytes (4 input characters).
 * the input chars are pointed to by p, they are to
 * be output to file f.  n is used to tell us not to
 * output all of them at the end of the file.
 */
d_outdec(p, f, n)
char *p;
FILE *f;
int n;
{
	int c1, c2, c3;

	c1 = DEC(*p) << 2 | DEC(p[1]) >> 4;
	c2 = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
	c3 = DEC(p[2]) << 6 | DEC(p[3]);
	if (n >= 1)
		putc(c1, f);
	if (n >= 2)
		putc(c2, f);
	if (n >= 3)
		putc(c3, f);
}

/* fr: like read but stdio */
int
fr(fd, buf, cnt)
FILE *fd;
char *buf;
int cnt;
{
        int c, i;
 
        for (i=0; i<cnt; i++) {
                c = getc(fd);
                if (c == EOF)
                        return(i);
                buf[i] = c;
        }
        return (cnt);
}
