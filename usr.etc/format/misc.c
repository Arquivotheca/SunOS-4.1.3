
#ifndef lint
static  char sccsid[] = "@(#)misc.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file contians miscellaneous routines.
 */
#include "global.h"
#include <mntent.h>
#include <signal.h>
#include <sgtty.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sun/autoconf.h>
#include "misc.h"
#include "analyze.h"

extern	char *calloc();
extern	char *realloc();

struct	env *current_env;		/* ptr to current environment */
int	stop_pending;			/* ctrl-Z is pending */
struct	ttystate ttystate;		/* tty info */
/*
 * This is the list of legal inputs for all yes/no questions.
 */
char	*confirm_list[] = {
	"yes",
	"no",
	NULL,
};

/*
 * This routine is a wrapper for malloc.  It allocates pre-zeroed space,
 * and checks the return value so the caller doesn't have to.
 */
char *
zalloc(count)
	int	count;
{
	char	*ptr;

	if ((ptr = calloc(1, (unsigned)count)) == NULL) {
		eprint("Error: unable to malloc more space.\n");
		fullabort();
	}
	return (ptr);
}

/*
 * This routine is a wrapper for realloc.  It reallocates the given
 * space, and checks the return value so the caller doesn't have to.
 * Note that the any space added by this call is NOT necessarily
 * zeroed.
 */
char *
rezalloc(ptr, count)
	char	*ptr;
	int	count;
{

	if ((ptr = realloc(ptr, (unsigned)count)) == NULL) {
		eprint("Error: unable to malloc more space.\n");
		fullabort();
	}
	return (ptr);
}

/*
 * This routine is a wrapper for free.
 */
destroy_data(data)
	char	*data;
{

	free(data);
}

#ifdef	not
/*
 * This routine takes the space number returned by an ioctl call and
 * returns a mnemonic name for that space.
 */
char *
space2str(space)
	u_int	space;
{
	char	*name;

	switch (space&SP_BUSMASK) {
	    case SP_VIRTUAL:
		name = "virtual";
		break;
	    case SP_OBMEM:
		name = "obmem";
		break;
	    case SP_OBIO:
		name = "obio";
		break;
	    case SP_MBMEM:
		name = "mbmem";
		break;
	    case SP_MBIO:
		name = "mbio";
		break;
	    case SP_VME16D16:
		name = "vme16d16";
		break;
	    case SP_VME24D16:
		name = "vme24d16";
		break;
	    case SP_VME32D16:
		name = "vme32d16";
		break;
	    case SP_VME16D32:
		name = "vme16d32";
		break;
	    case SP_VME24D32:
		name = "vme24d32";
		break;
	    case SP_VME32D32:
		name = "vme32d32";
		break;
	    default:
		eprint("Error: unknown address space type encountered.\n");
		fullabort();
	}
	return (name);
}
#endif	not

/*
 * This routine asks the user the given yes/no question and returns
 * the response.
 */
int
check(question)
	char	*question;
{
	int	answer;

	/*
	 * If we are running out of a command file, assume a yes answer.
	 */
	if (option_f)
		return (0);
	/*
	 * Ask the user.
	 */
	answer = input(FIO_MSTR, question, '?', (char *)confirm_list,
	    (int *)NULL, DATA_INPUT);
	return (answer);
}

/*
 * This routine aborts the current command.  It is called by a ctrl-C
 * interrupt and also under certain error conditions.
 */
void
cmdabort()
{

	/*
	 * If there is no usable saved environment and we are running
	 * from a command file, we gracefully exit.  This allows the
	 * user to interrupt the program even when input is from a file.
	 * If there is no usable environment and we are running with a user,
	 * just ignore the interruption.
	 */
	if (current_env == NULL || !(current_env->flags & ENV_USE)) {
		if (option_f)
			fullabort();
		else
			return;
	}
	/*
	 * If we are in a critical zone, note the attempt and return.
	 */
	if (current_env->flags & ENV_CRITICAL) {
		current_env->flags |= ENV_ABORT;
		return;
	}
	/*
	 * All interruptions when we are running out of a command file
	 * cause the program to gracefully exit.
	 */
	if (option_f)
		fullabort();
	printf("\n");
	/*
	 * Clean up any state left by the interrupted command.
	 */
	cleanup();
	/*
	 * Jump to the saved environment.
	 */
	longjmp(current_env->env, 0);
}

/*
 * This routine implements the ctrl-Z suspend mechanism.  It is called
 * when a suspend signal is received.
 */
void
onsusp()
{
	int	fix_term = 0;

	/*
	 * If we are in a critical zone, note the attempt and return.
	 */
	if (current_env != NULL && current_env->flags & ENV_CRITICAL) {
		stop_pending = 1;
		return;
	}
	/*
	 * If the terminal is mucked up, note that we will need to
	 * re-muck it when we start up again.
	 */
	if (ttystate.modified)
		fix_term = 1;
	printf("\n");
	/*
	 * Clean up any state left by the interrupted command.
	 */
	cleanup();
	/*
	 * Stop intercepting the suspend signal, then send ourselves one
	 * to cause us to stop.
	 */
	(void) sigsetmask(0);
	(void) signal(SIGTSTP, SIG_DFL);
	(void) kill(0, SIGTSTP);
	/*
	 * PC stops here
	 */
	/*
	 * We are started again.  Set us up to intercept the suspend
	 * signal once again.
	 */
	(void) signal(SIGTSTP, onsusp);
	/*
	 * Re-muck the terminal if necessary.
	 */
	if (fix_term)
		echo_off();
}

/*
 * This routine implements the timing function used during long-term
 * disk operations (e.g. formatting).  It is called when an alarm signal
 * is received.
 */
void
onalarm()
{
	/*	printf("\nformatting..."); */
	return;
}


/*
 * This routine gracefully exits the program.
 */
fullabort()
{

	printf("\n");
	/*
	 * Clean up any state left by an interrupted command.
	 */
	cleanup();
	exit (-1);
}

/*
 * This routine cleans up the state of the world.  It is a hodge-podge
 * of kludges to allow us to interrupt commands whenever possible.
 */
cleanup()
{

	/*
	 * Lock out interrupts to avoid recursion.
	 */
	enter_critical();
	/*
	 * Fix up the tty if necessary.
	 */
	if (ttystate.modified) {
		charmode_off();
		echo_on();
	}

	/*
	 * If the defect list is dirty, write it out.
	 */
	if (cur_list.flags & LIST_DIRTY) {
		cur_list.flags = 0;
		write_deflist(&cur_list);
	}
	/*
	 * If the label is dirty, write it out.
	 */
	if (cur_flags & LABEL_DIRTY) {
		cur_flags &= ~LABEL_DIRTY;
		(void) write_label();
	}
	/*
	 * If we are logging and just interrupted a scan, print out
	 * some summary info to the log file.
	 */
	if (log_file && scan_cur_block >= 0) {
		pr_dblock(lprint, scan_cur_block);
		lprint("\n");
	}
	if (scan_blocks_fixed >= 0)
		printf("Total of %d defective blocks repaired.\n",
		    scan_blocks_fixed);
	scan_cur_block = scan_blocks_fixed = -1;
	exit_critical();
}

/*
 * This routine causes the program to enter a critical zone.  Within the
 * critical zone, no interrupts are allowed.  Note that calls to this
 * routine for the same environment do NOT nest, so there is not
 * necessarily pairing between calls to enter_critical() and exit_critical().
 */
enter_critical()
{

	/*
	 * If there is no saved environment, interrupts will be ignored.
	 */
	if (current_env == NULL)
		return;
	/*
	 * Mark the environment to be in a critical zone.
	 */
	current_env->flags |= ENV_CRITICAL;
}

/*
 * This routine causes the program to exit a critical zone.  Note that
 * calls to enter_critical() for the same environment do NOT nest, so
 * one call to exit_critical() will erase any number of such calls.
 */
exit_critical()
{

	/*
	 * If there is a saved environment, mark it to be non-critical.
	 */
	if (current_env != NULL)
		current_env->flags &= ~ENV_CRITICAL;
	/*
	 * If there is a stop pending, execute the stop.
	 */
	if (stop_pending) {
		stop_pending = 0;
		onsusp();
	}
	/*
	 * If there is an abort pending, execute the abort.
	 */
	if (current_env == NULL)
		return;
	if (current_env->flags & ENV_ABORT) {
		current_env->flags &= ~ENV_ABORT;
		cmdabort();
	}
}

/*
 * This routine is a minimum function.
 */
int
min(x, y)
	int x, y;
{

	return (x < y ? x : y);
}

/*
 * This routine turns off echoing on the controlling tty for the program.
 */
echo_off()
{

	/*
	 * Mark the tty mucked up so we know to fix it later.
	 */
	ttystate.modified++;
	/*
	 * Open the tty and store the file pointer for later.
	 */
	if ((ttystate.ttyfile = open("/dev/tty", O_RDWR | FNDELAY)) < 0) {
		eprint("Unable to open /dev/tty.\n");
		fullabort();
	}
	/*
	 * Get the parameters for the tty, turn off echoing and set them.
	 */
	if (ioctl(ttystate.ttyfile, TIOCGETP, &ttystate.ttystate) < 0) {
		eprint("Unable to get tty parameters.\n");
		fullabort();
	}
	ttystate.ttystate.sg_flags &= ~ECHO;
	if (ioctl(ttystate.ttyfile, TIOCSETP, &ttystate.ttystate) < 0) {
		eprint("Unable to set tty parameters.\n");
		fullabort();
	}
}

/*
 * This routine turns on echoing on the controlling tty for the program.
 */
echo_on()
{

	/*
	 * Using the saved parameters, turn echoing on and set them.
	 */
	ttystate.ttystate.sg_flags |= ECHO;
	if (ioctl(ttystate.ttyfile, TIOCSETP, &ttystate.ttystate) < 0) {
		eprint("Unable to set tty parameters.\n");
		fullabort();
	}
	/*
	 * Close the tty and mark it ok again.
	 */
	if (--ttystate.modified == 0) {
		(void) close(ttystate.ttyfile);
		ttystate.modified = 0;
	}
}

/*
 * This routine turns off single character entry mode for tty.
 */
charmode_on()
{

	/*
	 * Mark the tty mucked up so it can be fixed alter.  Also,
	 * If tty unopened, open the tty and store the file pointer for later.
	 */
	if (ttystate.modified++) {
		if ((ttystate.ttyfile = open("/dev/tty", O_RDWR | FNDELAY)) < 0) {
			eprint("Unable to open /dev/tty.\n");
			fullabort();
		}
	}
	/*
	 * Get the parameters for the tty, turn on char mode.
	 */
	if (ioctl(ttystate.ttyfile, TIOCGETP, &ttystate.ttystate) < 0) {
		eprint("Unable to get tty parameters.\n");
		fullabort();
	}
	ttystate.ttystate.sg_flags |= CBREAK;
	if (ioctl(ttystate.ttyfile, TIOCSETP, &ttystate.ttystate) < 0) {
		eprint("Unable to set tty parameters.\n");
		fullabort();
	}
}

/*
 * This routine turns on single character entry mode for tty.
 * Note, this routine must be called before echo_on.
 */
charmode_off()
{

	/*
	 * Using the saved parameters, turn char mode on.
	 */
	ttystate.ttystate.sg_flags &= ~CBREAK;
	if (ioctl(ttystate.ttyfile, TIOCSETP, &ttystate.ttystate) < 0) {
		eprint("Unable to set tty parameters.\n");
		fullabort();
	}
	/*
	 * Close the tty and mark it ok again.
	 */
	if (--ttystate.modified == 0) {
		(void) close(ttystate.ttyfile);
		ttystate.modified = 0;
	}
}

/*
 * This routine checks to see if there are mounted partitions overlapping
 * a given portion of a disk.  If the start parameter is < 0, it means
 * that the entire disk should be checked.
 */
int
checkmount(disk, unit, start, end)
	char	*disk;
	short	unit;
	daddr_t	start, end;
{
	FILE	*fp;
	struct	mntent *mnt;
	char	c1[DK_DEVLEN], c2[DK_DEVLEN];
	int	i, found = 0;
	struct	dk_map *part;

	/*
	 * If we are only checking part of the disk, the disk must
	 * have a partition map to check against.  If it doesn't,
	 * we hope for the best.
	 */
	if (cur_parts == NULL && start >= 0)
		return (0);
	if (cur_ctype->ctype_ctype != DKC_PANTHER)
		(void) sprintf(c1, "%s%d", disk, unit);
	else
		(void) sprintf(c1, "%s%03x", disk, unit);
	/*
	 * Lock out interrupts because of the mntent protocol.
	 */
	enter_critical();
	/*
	 * Open the mount table.
	 */
	fp = setmntent(MOUNTED, "r");
	if (fp == NULL) {
		eprint("Unable to open mount table.\n");
		fullabort();
	}
	/*
	 * Loop through the mount table until we run out of entries.
	 */
	while ((mnt = getmntent(fp)) != NULL) {
		/*
		 * If this entry is not a 4.2 file system, it doesn't apply.
		 */
		if (strcmp(mnt->mnt_type, MNTTYPE_42) != 0)
			continue;
		/*
		 * Figure out which disk it's mounted from.
		 */
		if (sscanf(mnt->mnt_fsname, "/dev/%s", c2) != 1)
			continue;
		/*
		 * If it's not the disk we're interested in, it doesn't apply.
		 */
		if (strcnt(c1, c2) < strlen(c1))
			continue;
		/*
		 * It's a mount on the disk we're checking.  If we are
		 * checking whole disk, then we found trouble.  We can
		 * quit searching.
		 */
		if (start < 0) {
			found = -1;
			break;
		}
		/*
		 * Extract the partition it's mounted from.
		 */
		i = (int)(c2[strlen(c1)] - 'a');
		/*
		 * If the partition overlaps the zone we're checking,
		 * then we found trouble.  We cans quit searching.
		 */
		part = &cur_parts->pinfo_map[i];
		if (end < part->dkl_cylno * spc())
			continue;
		if (start >= part->dkl_cylno * spc() + part->dkl_nblk)
			continue;
		found = -1;
		break;
	}
	/*
	 * Close down the mount table.
	 */
	(void) endmntent(fp);
	/*
	 * If we found trouble and we're running from a command file,
	 * quit before doing something we really regret.
	 */
	if (found && option_f) {
		eprint("Operation on mounted disks must be interactive.\n");
		exit_critical();
		cmdabort();
	}
	exit_critical();
	/*
	 * Return the result.
	 */
	return (found);
}

int
check_label_with_mount(disk, unit) 
	char	*disk;
	short	unit;
{
	FILE	*fp;
	struct	mntent *mnt;
	char	c1[DK_DEVLEN], c2[DK_DEVLEN];
	int	i, j, found = 0;
	struct	dk_map *n, *o;
	unsigned int bm_mounted=0;
	struct dk_allmap old_map;
	int start, end, status;
	int b_startcyl, b_endcyl;

	/*
	 * If we are only checking part of the disk, the disk must
	 * have a partition map to check against.  If it doesn't,
	 * we hope for the best.
	 */
	if (cur_parts == NULL)
		return (0);	/* Will be checked later */
	(void) sprintf(c1, "%s%d", disk, unit);
	/*
	 * Lock out interrupts because of the mntent protocol.
	 */
	enter_critical();
	/*
	 * Open the mount table.
	 */
	fp = setmntent(MOUNTED, "r");
	if (fp == NULL) {
		eprint("Unable to open mount table.\n");
		fullabort();
	}

	/*
	 * Loop through the mount table until we run out of entries.
	 */
	while ((mnt = getmntent(fp)) != NULL) {
		/*
		 * If this entry is not a 4.2 file system, it doesn't apply.
		 */
		if (strcmp(mnt->mnt_type, MNTTYPE_42) != 0)
			continue;
		/*
		 * Figure out which disk it's mounted from.
		 */
		if (sscanf(mnt->mnt_fsname, "/dev/%s", c2) != 1)
			continue;
		/*
		 * If it's not the disk we're interested in, it doesn't apply.
		 */
		if (strcnt(c1, c2) < strlen(c1))
			continue;
		/*
		 * It's a mount on the disk we're checking.  If we are
		 * checking whole disk, then we found trouble.  We can
		 * quit searching.
		 */
		/*
		 * Extract the partition it's mounted from.
		 */
		i = (int)(c2[strlen(c1)] - 'a');
		/* construct a bit map of the partitions that are mounted */
		bm_mounted |= (1 << i);	
	}
	/*
	 * Close down the mount table.
	 */
	(void) endmntent(fp);
	/* Now we need to check that the current partition list and the
	 * previous partition list (which there must be if we actually
	 * have partitions mounted) overlap  in any way on the mounted
	 * partitions
	 */

	/* Get the "real" (on-disk) version of the partition table */
	status = ioctl(cur_file, DKIOCGAPART, &old_map);
	if(status)
	{
		eprint("Unable to get current partition map.\n");
		return(-1);
	}
	exit_critical();
	for(i=0; i < NDKMAP; i++)
	{
		if(bm_mounted & (unsigned int)(1 << i)) /* This partition is mounted */
		{
			o= &old_map.dka_map[i];
			n = &cur_parts->pinfo_map[i];
#ifdef DEBUG
			printf("check_label_to_mount: checking partition '%c'",i+'a');
#endif
			/*
			 * If partition is identical, we're fine.
			 * If the partition grows, we're also fine, because
			 *	-> the routines in partition.c check for overflow
			 *	-> it will (ultimately) be up to the routines in
			 *	   partition.c to warn about creation of overlapping
			 *	   partitions
			 */

			if (o->dkl_cylno == n->dkl_cylno && o->dkl_nblk <= n->dkl_nblk) {
#ifdef	DEBUG
				if (o->dkl_nblk < n->dkl_nblk) {
					printf("- new partition larger by %d blocks",
						n->dkl_nblk-o->dkl_nblk);
				}
				printf("\n");
#endif
				continue;
			}
#ifdef DEBUG
			printf("- changes; old (%d,%d)->new (%d,%d)\n",
				o->dkl_cylno, o->dkl_nblk, n->dkl_cylno, n->dkl_nblk);
#endif
			found = -1;
		}
		if(found) break;
	}

	/*
	 * If we found trouble and we're running from a command file,
	 * quit before doing something we really regret.
	 */

	if (found && option_f) {
		eprint("Operation on mounted disks must be interactive.\n");
		cmdabort();
	}
	/*
	 * Return the result.
	 */
	return (found);
}
