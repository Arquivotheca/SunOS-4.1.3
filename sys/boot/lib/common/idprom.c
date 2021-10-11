/*
 * @(#)idprom.c 1.1 92/07/30 SMI
 */

#include <sys/types.h>
#include <mon/idprom.h>
#include <mon/sunromvec.h>

/*
 * Read the ID prom and check it.
 * Arguments are a format number and an address to store prom contents at.
 *
 * Result is format number if prom has the right format and good checksum.
 * Result is -1		   if prom has the right format and bad checksum.
 * Result is prom's format if prom has the wrong format.
 *
 * If the PROM is in the wrong format, the addressed area is not changed.
 *
 * This routine must know the size, and checksum algorithm, of each format.
 * (Currently there's only one.)
 */

/*
 * For OPENPROM machines, getidprom buffers the root node property, "idprom".
 * For non-OBP's, buffer the idprom.
 */

#ifdef	OPENPROMS
#define	getidprom	prom_getidprom
#else	OPENPROMS
static struct idprom idprombuf;
static unsigned char idpromformat;
static int idpromcalled = 0;
#endif	OPENPROMS

int
idprom(format, idp)
	unsigned char format;
	register struct idprom *idp;
{

#ifdef	OPENPROMS
	unsigned char idpromformat;
#else	OPENPROMS
	unsigned char *cp, sum=0;
	short i;

	if (idpromcalled == 0)
#endif	OPENPROMS
		getidprom(&idpromformat, 1);

	if (format != idpromformat) {
#ifdef DEBUG
		printf("Error: bad idprom format (%x s.b. %x)\n",
		    idpromformat, format);
#endif DEBUG
		return (idpromformat);
	}

#ifndef	OPENPROMS
	if (idpromcalled == 0)  {
#ifdef DEBUG
		printf("idprom.c: Reading idprom.\n");
#endif DEBUG
#endif	OPENPROMS
		getidprom((unsigned char *)idp, sizeof (*idp));
#ifndef	OPENPROMS
		/* OBP has already done the checksum */
		cp = (unsigned char *)idp;
		for (i=0; i<16; i++)
			sum ^= *cp++;
		if (sum != 0) {
#ifdef DEBUG
			printf("Error: bad idprom checksum\n");
#endif DEBUG
			return (-1);
		}
		bcopy((char *)idp, (char *)(&idprombuf), sizeof (idprombuf));
		++idpromcalled;
#endif	OPENPROMS
		return (idpromformat);
#ifndef	OPENPROMS
	}
#ifdef	DEBUG
	printf("idprom.c: Reading bufferred idprom.\n");
#endif	DEBUG
	bcopy((char *)(&idprombuf), (char *)idp, sizeof (idprombuf));
	return (idpromformat);
#endif	OPENPROMS
}
