#ident  "@(#) aclone.c 1.1@(#) Copyright (c) 1989-92 Sun Microsystems, Inc."

/*
 * Audio clone device driver.  Forces a clone open of some other character
 * device.  Since its purpose in life is to force some other device to
 * clone itself, there's no need for anything other than the open routine
 * here.
 *
 * A registration routine is provided to allow drivers to register which
 * minor number should be used for the open.  NOTE: the audio open routine
 * must return a different minor number than is registered if the open
 * is a clone open.  Failure to do so will result in system panics if
 * an attempt to open the audio device is made when it is already open.
 *
 * Mappings:
 *
 * 	69, 0		audio		<-- default audio device
 * 	69, 1		audioctl	<-- default audio control device
 * 	69, 2		audioro
 * 	69, 3		unused
 * 	69, 4		audioamd	(CLONE)
 * 	69, 5		audioamdctl	(INDIRECT)
 *	69, 6		audioamdro	(CLONE)
 *	69, 7		unused
 *	69, 8		audiodbri0
 *	69, 9		audiodbri0ctl
 *	69, 10		audiodbri0ro
 * ..etc...
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/conf.h>

#if !defined(TRUE)
#define	TRUE	(1)
#define	FALSE	(0)
#endif

#define	ACLONE_MAXDEV	(64)

#define	MAPINDEX(dev)		((minor(dev)) >> 2)
#define	MINORINDEX(dev)		((minor(dev)) & 0x03)

#define	ISDEFAULTDEV(dev)	(((minor(dev)) & ~0x03) == 0)
#define	ISCONTROLDEV(dev)	(MINORINDEX(dev) == 1)

typedef struct maptab maptab_t;
struct maptab {
	struct streamtab *drvinfo;
	int	aminor;		/* aminor..aminor+3 for this device */
	int	aud_major;	/* major of real device */
	int	minors[4];
	int	lookup;		/* TRUE=need major lookup */
};

int Aclone_debug = 0;
static maptab_t maptab[ACLONE_MAXDEV];


/*
 * Do a clone open.  The (major number of the) device to be cloned is
 * specified by minor(dev).  We tell spec_open to do the work by
 * returning EEXIST after naming the device to clone.
 */
/* ARGSUSED */
int
acloneopen(dev, flag, newdevp)
	dev_t dev;
	int flag;
	dev_t *newdevp;
{
	maptab_t *mp;
	int i;

	if (ISDEFAULTDEV(dev)) {
		for (i = 0; i < ACLONE_MAXDEV; i++) {
			mp = &maptab[i];
			if (mp->drvinfo != NULL)
				break;
		}
	} else {
		if (MAPINDEX(dev) < ACLONE_MAXDEV)
			mp = &maptab[MAPINDEX(dev)];
		else
			return (ENODEV);
	}

	/*
	 * If the aclone "unit" is not registered, there is no device there.
	 */
	if (mp->drvinfo == NULL)
		return (ENODEV);

	/*
	 * Delayed binding of major number since the devices do not know
	 * it until *after* attach time...
	 */
	if (mp->lookup == TRUE) {
		mp->aud_major = aclone_findmajor(mp->drvinfo);
		if (mp->aud_major < 0)
			return (ENODEV);
		mp->lookup = FALSE;
	}

	if (Aclone_debug) {
		(void) printf("aclone: Opening aclone minor %d ", minor(dev));
		(void) printf("as %d, %d...\n", mp->aud_major,
			    mp->minors[MINORINDEX(dev)]);
	}

	/*
	 * Convert to the device to be cloned.  If what we are opening is
	 * actually the audio control device, we don't want a clone, so
	 * we return EAGAIN and spec_open will just iterate through its
	 * loop one more time and perform a non-CLONE open.
	 */
	*newdevp = makedev(mp->aud_major, mp->minors[MINORINDEX(dev)]);
	return (ISCONTROLDEV(dev) ? EAGAIN : EEXIST);
}


/*
 * Register a major/minor pair for a cloneable device.  Returns the base
 * minor number used for the mappings.  base...base+3 are the aclone
 * minors associated with this audio unit.
 */
int
aclone_register(cookie, unit, audminor, arominor, ctlminor)
	caddr_t cookie; 	/* streamtab of device */
	int unit;		/* aclone "unit" number */
	int audminor;		/* minor of audio device */
	int arominor;		/* minor of audioro device */
	int ctlminor;		/* minor of audioctl device */
{
	maptab_t *p;
	struct streamtab *drvinfo; /* streamtab of device */

	/*
	 * XXX - This is for lint as I am unable to remove the error any
	 * other way from the kernel.
	 */
	drvinfo = (struct streamtab *)cookie;

	if (unit < 0 || unit >= ACLONE_MAXDEV)
		return (-1);

	p = &maptab[unit];

	/* Check if unit is already in use */
	if (p->drvinfo != NULL)
		return (-1);

	p->drvinfo = drvinfo;
	p->minors[0] = audminor;
	p->minors[2] = arominor;
	p->minors[1] = ctlminor;
	p->minors[3] = -1;
	p->lookup = TRUE;

	return (unit);
}


/*
 * Unregister a mapping
 */
void
aclone_unregister(cookie)
	caddr_t	cookie;
{
	struct streamtab *drvinfo;
	int i;

	/*
	 * XXX - This is for lint as I am unable to remove the error any
	 * other way from the kernel.
	 */
	drvinfo = (struct streamtab *)cookie;

	for (i = 0; i < ACLONE_MAXDEV; i++) {
		if (maptab[i].drvinfo == drvinfo)
			maptab[i].drvinfo = NULL;
	}

	return;
}


/*
 */
int
aclone_findmajor(drvinfo)
	struct streamtab *drvinfo;
{
	int i;
	extern int nchrdev;
	extern struct cdevsw cdevsw[];

	for (i = 0; i <= nchrdev; i++) {
		if (cdevsw[i].d_str == drvinfo)
			break;
	}

	if (i > nchrdev)
		i = -1;

	return (i);
}
