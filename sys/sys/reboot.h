/*	@(#)reboot.h 1.1 92/07/30 SMI; from UCB 4.3 82/10/31	*/

/*
 * Arguments to reboot system call and flags to init.
 *
 * On the VAX, these are passed to boot program in r11,
 * and on to init.
 *
 * On the Sun, these are parsed from the boot command line
 * and used to construct the argument list for init.
 */

#ifndef _sys_reboot_h
#define _sys_reboot_h

#define	RB_AUTOBOOT	0	/* flags for system auto-booting itself */

#define	RB_ASKNAME	0x001	/* ask for file name to reboot from */
#define	RB_SINGLE	0x002	/* reboot to single user only */
#define	RB_NOSYNC	0x004	/* dont sync before reboot */
#define	RB_HALT		0x008	/* don't reboot, just halt */
#define	RB_INITNAME	0x010	/* name given for /etc/init */
#define	RB_NOBOOTRC	0x020	/* don't run /etc/rc.boot */
#define	RB_DEBUG	0x040	/* being run under debugger */
#define	RB_DUMP		0x080	/* dump system core */
#define	RB_WRITABLE	0x100	/* mount root read/write */
#define	RB_STRING	0x200	/* pass boot args to prom monitor */

/*
 * These flags take effect only on systems where
 * you have a choice.
 */
#define	RB_ASKMORE		0x00100000	/* ask about lots of things. */
#define	RB_COPYBACK		0x10000000	/* prefer copyback over writethru */
#define	RB_WRITETHRU		0x20000000	/* prefer writethru over copyback */
#define	RB_UNIPROCESSOR		0x40000000	/* prefer uni- over multi-processor */
#define	RB_MULTIPROCESSOR	0x80000000	/* prefer multi- over uni-processor */

#endif /*!_sys_reboot_h*/
