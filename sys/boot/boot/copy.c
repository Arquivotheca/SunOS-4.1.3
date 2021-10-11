#ifndef lint
static	char sccsid[] = "@(#)copy.c	1.1 92/07/30	SMI"; /* From ancient history */
#endif

#include <mon/sunromvec.h>
#include <stand/saio.h>

/*
 * XXX - Using resalloc() here is a bug, since it trashes any resource
 * allocation done by the calling program (tpboot). This is currently
 * an issue only for sun4c.
 * TODO: Make resalloc(), kmem_alloc(), etc. system calls?
 */
extern char *resalloc();

#define	BUFSIZ	(3*10*1024)	/* must be multiple of 10K, < 32K */
static int controller, unit, subunit;
/*
 * Copy from to in large units.
 * Intended for use in bootstrap procedure.
 */
main()
{
	int from, to, fid, tid;
	char fbuf[50], tbuf[50];
	register int i;
#ifdef OPENPROMS
	int tcon, tun, tsub;
	int dcon, dun, dsub;
	int nblks;
	register int leave = 0;
	static char buffer[BUFSIZ];
#else
	char *buffer; 
#endif
	register int count = 0;

	printf("Standalone Copy\n");
#ifdef OPENPROMS
retry:
#define	ZERO_STATICS	controller = unit = subunit = 0;
#define	TAPE_STATICS	controller = tcon; unit = tun; subunit = tsub;
#define	DISK_STATICS	controller = dcon; unit = dun; subunit = dsub;

	ZERO_STATICS;
	/*
	 * Until a library routine is written to handle device-type 
	 * romvec interface differences (call config info until you match
	 * with the open device name)
	 * Only tape file to disk partition transfers are handled
	 */

	/*
	 * The current OPENPROM scsi driver can't handle more than one open
	 * scsi device, thus we do this open/close dance
	 */
	getdev("From Tape Device",fbuf);
	tcon = controller; tun = unit; tsub = subunit;
	ZERO_STATICS;
	getdev("To Disk Device",tbuf);
	dcon = controller; dun = unit; dsub = subunit;
	TAPE_STATICS;
	if ((fid = romp->v_open(fbuf)) == 0) {
	    printf("copy: could not open %s\n",fbuf);
	    goto retry;
	}
	if (romp->v_seek(fid, subunit, 0) < 0) {
	    printf("copy: could not seek on %s\n",fbuf);
	    goto retry;
	}
	while (! leave) {
	    TAPE_STATICS;
	    if ((fid = romp->v_open(fbuf)) == 0) {
		printf("copy: could not open %s\n",fbuf);
		goto retry;
	    }
	    i = romp->v_read_bytes(fid,BUFSIZ,count,buffer);
	    if (i < 0) {
		printf("Error %d reading %s\n",i, fbuf);
		break;
	    }
	    if (i < BUFSIZ)
		leave++;
	    romp->v_close(fid);
	    DISK_STATICS;
	    if ((tid = romp->v_open(tbuf)) == 0) {
		printf("copy: could not open %s\n",tbuf);
		goto retry;
	    }
	    nblks = i/ROMVEC_BLKSIZE;
	    i = romp->v_write_blocks(tid,nblks,count/ROMVEC_BLKSIZE,buffer);
	    if (i < 0) {
		printf("Error %d writing %s\n",tbuf);
		break;
	    }
	    if (i < nblks)
		leave++;
	    romp->v_close(tid);
	    count += i*ROMVEC_BLKSIZE;
	}
#else OPENPROMS
	buffer = resalloc(RES_DMAMEM, BUFSIZ);
	from = getdev("From", fbuf, 0);
	to = getdev("To", tbuf, 1);
	for (;;) {
		i = read(from, buffer, BUFSIZ);
		if (i <= 0)
			break;
		if (write(to, buffer, i) != i) {
			printf("Write error\n");
			break;
		}
		count += i;
	}
	if (i < 0)
		printf("Read error\n");
#endif OPENPROMS
	printf("Copy completed - %d bytes\n", count);
}

#ifdef OPENPROMS
/*
 * OPENPROMS take names of the form:
 * device-name([subunit])  
 * subunit is the disk partition or tape file number, and defaults to 0
 * Because of the incomplete architecture implementation,
 * the sun4c takes names of the form:
 * device-name([controller,][unit,][subunit])
 * where device-name is a prom device, 
 * controller is the the device instance,
 * unit is the unit (target) #,
 * subunit is the disk partition or tape file number
 * these all default to 0
 */
getdev(prompt,name)
	char *prompt, *name;
{
	char *cp, *gethex();

retry:
	printf("%s: ", prompt);
	gets(name);
	for (cp = name; *cp && *cp != '('; cp++)
		;
	if (*cp++ != '(')
		goto badsyntax;
	cp = gethex(cp, &controller);
#ifdef sun4c
	cp = gethex(cp, &unit);
	cp = gethex(cp, &subunit);
#endif sun4c
	if (*cp != ')') {
badsyntax:
#ifdef sun4c
		printf("\nBad name syntax, try 'device-name([controller,][unit,][subunit])'\n");
#else 
		printf("\nBad name syntax, try 'device-name([subunit])'\n");
#endif 
		goto retry;
	}
}

#else OPENPROMS
getdev(prompt, buf, mode)
	char *prompt, *buf;
	int mode;
{
	register int i;

	do {
		printf("%s: ", prompt);
		gets(buf);
		i = open(buf, mode);
	} while (i <= 0);
	return (i);
}
#endif OPENPROMS
