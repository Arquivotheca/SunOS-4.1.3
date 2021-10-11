#ifndef lint
static  char sccsid[] = "@(#)edittoc.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
/*
 * edittoc - edit table of contents
 *
 */

#include <curses.h>
#include <signal.h>
#include "mktp.h"


#define	SELDELSTATE	0
#define	COPYMOVESTATE	1
#define	QUITSTATE	2
#define	ENDSTATE	3

#define	COPY	0
#define	MOVE	1
void calcsize();
static toc_entry *filesel;	/* current file being looked at */
static toc_entry *cmsel;	/* current slot to move or copy to */
static char *Size = " SIZE=size ";	/* makefile param to size archive */

static int nvols, maxent;

static char copyormove, volume_overflow, savechanges;


main(argc,argv)
int argc;
char **argv;
{
	extern toc_entry entries[], *ep;

	if(argc < 3)
	{
		(void) fprintf(stderr,"Usage: %s infile outfile\n",*argv);
		exit(1);
	}


	if(!get_tables(argv[1]))
		exit(-1);
	else if(dinfo.dstate&(FITTED|SIZED|PARSED) != (FITTED|SIZED|PARSED))
	{
		(void) fprintf(stderr,
			"Can't edit before sizing or initial fitting done\n");
		exit(-1);
	}

	setbuf(stdout,(char *) 0);

	if(edit_file() < 0)
		exit(-1);
	else if(savechanges != 0)
	{
		dinfo.dstate |= EDITTED;
		exit(dump_tables(argv[2])?0:1);
	}
	else
	{
		/*
		 * we're abandoning changes made
		 * in order to do this sanely, we'll reload from input
		 * and dump to output
		 */
		destroy_all_entries();
		if(!get_tables(argv[1]))
			exit(-1);
		exit(dump_tables(argv[2]));
	}
	/* NOTREACHED */
}

static int
edit_file()
{
	register int state;
	/*
	 * get/set sizes of things
	 */

	(void) gsizes();
	filesel = entries;
	screen_on();

	/*
	 * 3) Drop into action loop
	 */


	for(state = SELDELSTATE; state != ENDSTATE; )
	{
		display();
		if (state == SELDELSTATE )
		{
			display_entry(filesel,1);
			state = seldelstate();
		}
		else if(state == COPYMOVESTATE)
		{
			display_entry(filesel,1);
			if(cmsel != filesel)
				display_entry(cmsel,2);
			else
				display_entry(filesel,3);
			state = copymovestate();
		}
		else if(state == QUITSTATE)
			state = checkquit();
		resequence_all_entries();
	}

	screen_off();
	return(0);
}

static int
seldelstate()
{
	int rstate;
	toc_entry *leftright();

	/*
	 * Put up command lines 
	 */
	move(LINES-3,0);
	clrtobot();
	addstr("Use one of h,j,k,l (left,down,up,right) to select an entry.");
	mvaddstr(LINES-2,0,"Then use ");
	standout();
	addch('x');
	standend();
	addstr(" to kill,");
	standout();
	addch('m');
	standend();
	addch('/');
	standout();
	addch('c');
	standend();
	addstr(" to copy/move, ");
	standout();
	addch('b');
	standend();
	addstr(" to bump, ");
	standout();
	addch('f');
	standend();
	addstr(" to re-fit, ");
	move(LINES-1,0);
	standout();
	addch('s');
	standend();
	addstr(" to size, ");
	standout();
	addch('e');
	standend();
	addstr(" to look at, ");
	standout();
	addch('M');
	standend();
	addstr(" to modify, ");
	standout();
	addch('z');
	standend();
	addstr(" to exit");
	refresh();

	switch(getch()&0x7f)
	{
	case 'j':
		display_entry(filesel,0);
		if(++filesel == ep)
			filesel = entries;
		rstate = SELDELSTATE;
		break;
	case 'k':
		display_entry(filesel,0);
		if(--filesel < entries)
			filesel = ep-1;
		rstate = SELDELSTATE;
		break;
	case 'h':
		display_entry(filesel,0);
		filesel = leftright(filesel,1);
		rstate = SELDELSTATE;
		break;
	case 'l':
		display_entry(filesel,0);
		filesel = leftright(filesel,0);
		rstate = SELDELSTATE;
		break;
	case 's':
		move(LINES-3,0);
		clrtobot();
		screen_off();
		calcsize(filesel);
		screen_on();
		(void) gsizes();
		rstate = SELDELSTATE;
		break;
	case 'M':
		modify(filesel);
		rstate = SELDELSTATE;
		break;
	case 'e':
		examine(filesel);
		rstate = SELDELSTATE;
		break;
	case 'x':
		remove_entry(filesel);
		if(filesel >= ep)
			filesel = ep-1;
		(void) gsizes();
		erase();
		rstate = SELDELSTATE;
		break;
	case 'f':
		erase();
		refit();
		rstate = SELDELSTATE;
		break;
	case 'm':
		cmsel = filesel;
		copyormove = MOVE;
		rstate = COPYMOVESTATE;
		break;
	case 'c':
		cmsel = filesel;
		copyormove = COPY;
		rstate = COPYMOVESTATE;
		break;
	case 'b':
		bump(filesel);
		(void) gsizes();
		erase();
		rstate = SELDELSTATE;
		break;
	case 'z':
		rstate = QUITSTATE;
		break;
	default:
		bell();
		rstate = SELDELSTATE;
		break;
	}
	return(rstate);
}

static int
copymovestate()
{
	register toc_entry *ep1, *ep2;
	int newvol;
	toc_entry *leftright();

	/*
	 * Check to make sure that we have enough room for a copy
	 * or a move...
	 */

	if(ep == &entries[NENTRIES-1])
	{
		errprint("Sorry, Internal Table full at %d entries",NENTRIES);
		return(SELDELSTATE);
	}

	/*
	 * Put up command lines 
	 */

	move(LINES-3,0);
	clrtobot();
	(void) printw("Use h,j,k,l to select a place to %s selection to.",
		(copyormove == MOVE)?"move":"copy");
	move(LINES-1,0);
	addstr("Use ");
	standout();
	addch('a');
	standend();
	addstr(" place after, or ");
	standout();
	addch('b');
	standend();
	addstr(" place before, or ");
	standout();
	addch('z');
	standend();
	addstr(" to cancel operation");
	refresh();

	switch(getch()&0x7f)
	{
	case 'j':
		if(cmsel != filesel)
			display_entry(cmsel,0);
		else
			display_entry(filesel,1);
		if(++cmsel == ep)
			cmsel = entries;
		return(COPYMOVESTATE);
		break;
	case 'k':
		if(cmsel != filesel)
			display_entry(cmsel,0);
		else
			display_entry(filesel,1);
		if(--cmsel < entries)
			cmsel = ep-1;
		return(COPYMOVESTATE);
		break;
	case 'h':
		if(cmsel != filesel)
			display_entry(cmsel,0);
		else
			display_entry(filesel,1);
		cmsel = leftright(cmsel,1);
		return(COPYMOVESTATE);
		break;
	case 'l':
		if(cmsel != filesel)
			display_entry(cmsel,0);
		else
			display_entry(filesel,1);
		cmsel = leftright(cmsel,0);
		return(COPYMOVESTATE);
		break;
	case 'a':
		/*
		 * Copy/Move AFTER where cmsel points to.
		 * We accomplish this by shoving everything ABOVE cmsel
		 * up one slot. If filesel came AFTER cmsel, we have
		 * to bump filesel as well. Then we bump cmsel to
		 * the freshly opened slot.
		 *
		 */
		newvol = Volume_of_Entry(cmsel);
		if(cmsel != ep-1)
		{
			for(ep2 = ep, ep1 = ep-1; ep1 > cmsel; ep1--, ep2--)
			{
				bzero((char *) ep2, sizeof (*ep2));
				*ep2 = *ep1;
			}
			if(filesel > cmsel)
				filesel++;
		}
		cmsel++;
		break;
	case 'b':
		/*
		 * Copy/Move BEFORE where cmsel points to.
		 * We accomplish this by shoving everything ABOVE and
		 * INCLUDING cmsel up one slot. If filesel came AFTER or EQUAL
		 * to cmsel, we have to bump filesel as well.
		 * We leave cmsel ALONE, as it already points to
		 * the freshly opened slot.
		 *
		 */
		newvol = Volume_of_Entry(cmsel);
		for(ep2 = ep, ep1 = ep-1; ep1 >= cmsel; ep1--, ep2--)
		{
			bzero((char *) ep2, sizeof (*ep2));
			*ep2 = *ep1;
		}
		if(filesel >= cmsel)
			filesel++;
		break;
	case 'z':
		if(cmsel != filesel)
			display_entry(cmsel,0);
		else
			display_entry(filesel,1);
		return(SELDELSTATE);
		break;
	default:
		bell();
		return(COPYMOVESTATE);
		break;
	}

	/*
	 * reflect the fact that we shoved part or all of the table
	 * up one...
	 */

	ep++;

	/*
	 * now copy to new location...
	 */

	dup_entry(cmsel,filesel);


	/*
	 * Set the volume of the new entry
	 */

	if(Set_New_Vol(cmsel,newvol) < 0)
	{
		errprint("Fatal error in Set_New_Volume()");
		screen_off();
		exit(-1);
	}

	/*
	 * If it was a move, delete the old entry...
	 */

	if(copyormove == MOVE)
		remove_entry(filesel);

	/*
	 * set filesel to cmsel
	 */
	filesel = cmsel;
	/*
	 * recalculate volume and file numbers
	 */
	(void) gsizes();
	/*
	 * erase the screen...
	 */
	erase();
	/*
	 * and return new state
	 */
	return(SELDELSTATE);
}

static int
checkquit()
{

	if(volume_overflow)
	{
		move(LINES-3,0);
		clrtobot();
		standout();
		addstr("You must re-fit this distribution because of volume overflow");
		standend();
		refresh();
		sleep(3);
		move(LINES-3,0);
		clrtobot();
		refresh();
		return(SELDELSTATE);
	}
	else
	{
		for(;;)
		{
			move(LINES-3,0);
			clrtobot();
			addstr("Save any changes made [y/n] ? ");
			refresh();
			switch(getch()&0x7f)
			{
			case 'y':
				savechanges = 1;
				return(ENDSTATE);
			case 'n':
				savechanges = 0;
				return(ENDSTATE);
			}
		}
	}
}

/*
 * subs....
 */

static int
quit()
{
	move(LINES-1,0);
	refresh();
        endwin();
	exit(-1);
}

static
display()
{
	register toc_entry *eptr;
	register int i;

	move(0,0);

	/*
	 * Display header...
	 */

	if (entries[0].Where.dtype == TAPE)
		for(i = 0; i < nvols; i++)
			(void) mvprintw(0,i*16,"    Volume %1d",i+1);
	else /* DISK */
		(void) mvprintw(0,0," Entries    Size  Volume#");

	/*
	 * Display entries
	 *
	 */
	for(eptr = entries; eptr < ep; eptr++)
		display_entry(eptr,0);

	if (entries[0].Where.dtype == TAPE)
	{
	    /*
	     * Calculate and display percent utilization of each volume
	     *
	     */

	    volume_overflow = 0;
	    for(i = 0; i < nvols; i++)
	    {
#ifdef	busted_sparc
		register unsigned f;
		f = sumvolume(i+1) * 100;
		f /= Size_of_Volumes();
		(void) mvprintw(maxent+1,i*16+2,"%d%% utilized",f);

#else
		(void) mvprintw(maxent+1,i*16+2,"%2.0f%% utilized",
(((double) sumvolume(i+1))/(double) Size_of_Volumes())*100.0);
#endif
		if(sumvolume(i+1) > Size_of_Volumes())
			volume_overflow++;
    	    }
	}
	/*
	 * paint it...
	 *
	 */

	refresh();
}

/*
 * display a particular entry
 *
 *	hilite	== 0 -> no particular notation
 *		== 1 -> standout the entire name (main selection)
 *			also mark with a '>' in front of it.
 *		== 2 -> standout the first char of the name (a secondary
 *			selection). Also mark with a '+'.
 *		== 3 -> reverse the standout of the first char of the name.
 *			This is the case when the secondary selection is
 *			the same as the primary selection. Mark as in secondary
 *			selection.
 */
static
display_entry(eptr,hilite)
toc_entry *eptr;
{
	char *fitsize();
	int col, row;
	char markchr;

	markchr = (hilite == 0)?' ':((hilite == 1)?'>':'+');

	if (eptr->Where.dtype == TAPE)
		col = (Volume_of_Entry(eptr) - 1) * 16;
	else /* DISK */
		col = 0;
	row = Entry_in_Volume(eptr) + 1;
	if(hilite == 1 || hilite == 3)
		standout();
	if (eptr->Where.dtype == TAPE)
		(void) mvprintw(row,col,"%c%-8.8s%c%6s",markchr,eptr->Name,
			(strlen(eptr->Name) > 8)?'*':' ',
			fitsize((unsigned)(Size_of_Entry(eptr)))); 
	else /* DISK */
		(void) mvprintw(row,col,"%c%-8.8s%c%6s%4d",markchr,eptr->Name,
			(strlen(eptr->Name) > 8)?'*':' ',
			fitsize((unsigned)(Size_of_Entry(eptr))),
			Volume_of_Entry(eptr)); 
	if(hilite == 1 || hilite == 3)
		standend();
	if(hilite == 2 || hilite == 3)
	{
		move(row,col+1);
		if(hilite == 2)
			standout();
		addch(eptr->Name[0]);
		if(hilite == 2)
			standend();
	}
}

static char *
fitsize(amt)
unsigned int amt;
{
	extern char *sprintf();
	static char local[16];

	if(amt < 1024)
		return(sprintf(local,"%6d",amt));
	else if(amt < 1024*1024)
		return(sprintf(local,"%4dkb",(amt+((1<<10)-1))>>10));
	else
		return(sprintf(local,"%4dmb",(amt+((1<<20)-1))>>20));
}

static
bump(ent)
register toc_entry *ent;
{
	int newvol;
	if(Entry_in_Volume(ent) == 0)
		errprint("Would delete volume %d!",Volume_of_Entry(ent));
	else
	{
		newvol = Volume_of_Entry(ent) + 1;
		while(Volume_of_Entry(ent) != newvol)
		{
			if(Set_New_Vol(ent,newvol) < 0)
			{
				errprint("Fatal error in Set_New_Volume()");
				screen_off();
				sleep(1);
				exit(-1);
			}
			if(++ent >= ep)
				break;
		}
	}
}



/*
 *
 */

static void (*isav)(), (*qsav)();

static int
screen_on()
{
	register toc_entry *eptr = entries;

	/*
	 * Set up screen
	 *
	 */

	(void) initscr();

        isav = signal(SIGINT,quit);
        qsav = signal(SIGQUIT,quit);
        nonl();         /* turn off CR-LF remap */
        crmode();       /* set cbreak...        */
        noecho();       /* turn off echoing     */
	clear();
	move(0,0);
	refresh();
	if(LINES <= maxent-4 ||
		COLS < ((eptr->Where.dtype == TAPE) ? 16*nvols : 16))
	{
		screen_off();
		(void) fprintf(stderr,
			"Sorry, Information won't fit on screen\n");
		exit(-1);
	}
}

static int
screen_off()
{
	(void) signal(SIGINT,isav);
	(void) signal(SIGQUIT,qsav);
	move(LINES-1,0);
	refresh();
	endwin();
}

static int
gsizes()
{
	register i, j;
	register toc_entry *eptr = entries;
	nvols = getnvols();
	if (eptr->Where.dtype == TAPE)
	{
		for(maxent = i = 0; i < nvols; i++)
		{
			j = getnent(i+1);
			if(j > maxent)
				maxent = j;
			else if(j == 0)
				return(-1);
		}
	} else /* DISK */
		for(maxent = 0,eptr = entries; eptr < ep;maxent++, eptr++);
	return(0);
}

static toc_entry *
leftright(ptr,moveleft)
toc_entry *ptr;
{
	register toc_entry *eptr;
	int filnum, volno, ment;

	filnum = Entry_in_Volume(ptr);
	volno = Volume_of_Entry(ptr);

	/*
	 * calculate new volume. Remember, volumes
	 * are ordinal [1..n]
	 */
	if(moveleft)
	{
		if(--volno <= 0)
			volno = nvols;
	}
	else
	{
		if(++volno > nvols)
			volno = 1;
	}

	/*
	 * calculate filnum in new volume. Remember, file numbers
	 * are ordinal [0..n-1].
	 *
	 */

	if((ment = getnent(volno)) <= filnum)
		filnum = ment-1;

	if(filnum >= 0)
	{
		/*
		 * Now position ourselves at beginning of desired volume
		 */
		for(eptr = entries; eptr < ep; eptr++)
			if(Volume_of_Entry(eptr) == volno)
				break;
		/*
		 * find pointer to filnum...
		 */
		while(eptr < ep)
		{
			if(Entry_in_Volume(eptr) == filnum &&
					Volume_of_Entry(eptr) == volno)
				return(eptr);
			eptr++;
		}
	}
	errprint("Ulp! Lost myself. Sorry.....");
	return(entries);
}

/*
 * file fitting - essentially the same as fit_file, but without
 * concerning ourselves about copying table of contents stuff
 */

/*
 * re-fit files..
 */

static int volno, filnum;

static int
refit()
{
	register unsigned bytes_per_volume;
	register toc_entry *eptr = entries;
	register unsigned thisvolsize;
	int (*setaddress)(), (*setup_a_func())();

	screen_off();

	if(ep == entries)
	{
		(void) fprintf(stderr,"You deleted it all!\n");
		sleep(2);
		exit(-1);
	}

	/*
	 * Get value of number of bytes per volume
	 *	Check to make sure that each volume can
	 *	hold at least the TOC entries plus at least the
	 *	largest other entry...
	 *	...Note! not necessary for diskettes, 'cause 
	 *	they have multi-volume archive capability...
	 *
 	 *	Initialize a few values.
	 */

	bytes_per_volume = Size_of_Volumes();

	if((eptr->Where.dtype == TAPE) && (Check_Size(bytes_per_volume) < 0))
	{
		(void) fprintf(stderr,
			"Can't fit TOC and entries onto a single volume\n");
		sleep(2);
                exit(-1);
        }



	/*
	 * Volumes are counted from 1 on up....
	 */
	volno = 1;
	/*
	 * File numbers are counted from 0 on up
	 */
	filnum = 0;

	/*
	 * 2) Set up a pointer to the function that will actually stuff
	 * the appropriate values into each TOC entry. This function
	 * will work off of the external static values of 'thisvolume'
	 * and 'filnum', and return either the size contained in a
	 * 'new' volume, or else the increment of the size of the
	 * just added entry.
	 *
	 */

	setaddress = setup_a_func();

	/*
 	 * 3) Now walk through table of contents, assigning
	 * address information as we go. Remember: each time we go to a new
	 * volume, we have to duplicate the TOC
	 * as the first entries for that volume... for tapes - yes,
	 * but not necessary for diskettes.
	 */

	if(eptr->Where.dtype == TAPE)
	{
	    for(eptr = entries, thisvolsize = 0; eptr < ep; eptr++)
	    {
		if(thisvolsize + Size_of_Entry(eptr) > bytes_per_volume)
			thisvolsize = (*setaddress)(eptr,1);
		else
			thisvolsize += (*setaddress) (eptr,0);
	    }
	}
	else	/* DISKETTES */
	{
		volno = (*setaddress)(entries,bytes_per_volume);
	}
	(void) gsizes();
	filesel = entries;
	screen_on();
}

static int
(*setup_a_func())()
{
	/*
	 * FIX ME - need to put in (maybe) a routine that
	 * stuffs values for disk distributions..its been done
	 */
	register toc_entry *eptr = entries;
	int	tapeset();
	int	diskset();

	if (eptr->Where.dtype == TAPE)
		return(tapeset);
	else /* DISKETTE */
		return(diskset);
}

static int
tapeset(eptr,mknewvol)
register toc_entry *eptr;
int mknewvol;
{
	auto char local[16];
	register unsigned sz;
	register tapeaddress *ta;
	extern char *malloc(), *sprintf();

	/*
	 * If we have to make a new volume, now is the place to
	 * do it.
	 *
	 */

	sz = 0;
	if(mknewvol)
	{
		volno++;
		filnum = 0;
		/*
		 * MAYBE FIX ME - doesn't have "nodup" fixes that fit_file.c has
		 */
	}

	ta = &eptr->Where.Address_u.tape;
	eptr->Where.dtype = TAPE;

	/*
	 * Fix Me - We assume that each entry is <= size
	 * of a tape. This may not be true at some point.
	 *
	 */
	/*
	 * Stuff address info...
	 */

	ta->volno = volno;
	ta->label = newstring(sprintf(local,"VOLUME%u",volno));
	ta->file_number = filnum++;
	sz += Size_of_Entry(eptr);
	return(sz);
}

/*
 * see fit_file.c
 */
static int
diskset(eptr,bytes_per_volume)
register toc_entry *eptr;
register unsigned bytes_per_volume;
{
	auto char local[16];
	register unsigned sz;
	register diskaddress *da;
	extern char *sprintf();
	int volno;
	unsigned thisvolsize;
	unsigned offset;

	offset = 0;
	for( volno=1, thisvolsize = bytes_per_volume; eptr < ep; eptr++) {

		/*
		 * If this is a table of contents, skip it.
		 * If this is a data image, skip it. (shouldn't be any!)
		 */
		if( IS_TOC(eptr) || (eptr->Type == IMAGE) ) {
			continue;
		}
		if( thisvolsize < 0 ) {
			fprintf(stderr, "horrible error in diskette sizing\n");
				exit(-1);
		}
		if( thisvolsize == 0 ) {
			/* need to start a new volume */
			volno++;
			offset = 0;
			thisvolsize = bytes_per_volume;
		}

		/*
		 * Stuff address info...
		 */
		da = &eptr->Where.Address_u.disk;
		da->volno = volno;
		da->label = newstring(sprintf(local,"VOLUME%u",volno));
		da->offset = offset;	/* offset into this diskette */

		/*
		 * If entry overflows this diskette, we have to determine
		 * how many diskettes it adds to the running total
		 * else, we just add the size to this diskette.
		 */
		sz = Size_of_Entry(eptr);
		/* round up to 512 (hopefully compiler is smart) */
		sz = ((sz + 511)/512)*512;

		do {
			/*
			 * if this file takes all or more than all of rest of
			 * diskette....
			 */
			if( sz >= thisvolsize ) {
				volno++;
				sz -= thisvolsize;
				/* start all over again, leave space for */
				/* bar volume and file headers */
				thisvolsize = bytes_per_volume;
				offset = 0;
			} else {
				/* this file doesn`t fill this diskette */
				thisvolsize -= sz;
				offset += sz;
				sz = 0;
			}
		} while( sz );
	}
	return(volno);
}

/*
 * calc_size - recalculate an entry's size
 *
 *	code stolen from calc_sizes.c
 *
 */

static void
calcsize(eptr)
register toc_entry *eptr;
{
	extern void Set_Size_of_Entry();
	register u_long size;
	register FILE *ip;
	FILE *popen();
	char cmdstring[BUFSIZ];

	if(IS_TOC(eptr))
		return;
	if(eptr->Where.dtype == TAPE)
		(void) strcpy(cmdstring, eptr->Command);
	else /* dtype == DISK */
	{
		/* tell makefile we're just sizing */
		(void) strcpy(cmdstring, eptr->Command);
		(void) strcat(cmdstring, Size);
	}

	if((ip = popen(cmdstring,"r")) == NULL)
	{
		(void) fprintf(stdout,"calcsize- cannot invoke '%s'",
			eptr->Command);
		sleep(2);
		return;
	}

	(void) fprintf(stdout,"recalculating size for entry '%s' .. ",
			eptr->Name);

	size = 0;
	while(getc(ip) != EOF)
		size++;
	if(ferror(ip))
	{
		(void) fprintf(stdout,"error in reading input\n");
		sleep(1);
		return;
	}
	(void) pclose(ip);
	Set_Size_of_Entry(eptr,size);
	(void) fprintf(stdout,"%d bytes\n",Size_of_Entry(eptr));
	sleep(1);
	return;
}

/*
 * examine - look at an entry, in depth
 */

static
examine(eptr)
register toc_entry *eptr;
{
	examine_modify(eptr);
	examine_modify_finish(eptr);

}

/*
 * Modify - modify an entry
 */
static
modify(eptr)
register toc_entry *eptr;
{
	static char buf[72];
	static int getinput();

	examine_modify(eptr);
	if (!eptr->Command) {
		mvaddstr(LINES-2,0,"No fields are changable");
		examine_modify_finish(eptr);
		return;
	}
	mvaddstr(LINES-3,0,"Only \"Command\" is modifiable");
	mvaddstr(LINES-2,0,
		"Enter new \"Command\" string, terminated with <RETURN>");
	refresh();
	/*
	 * since we are still using BSD curses, getstr doesn't echo
	 * or use erase characters properly, so we have to do this ourselves
	 */
	if (getinput(1,9,80-8,buf) == 0) {
		move(LINES-3,0);
		clrtobot();
		addstr("You didn't change anything");
	} else {
		eptr->Command = malloc(strlen(buf)+1);
		strcpy(eptr->Command,buf);
	}
	examine_modify_finish();
}

static int
getinput(line,begcol,max,buf)
int line, begcol, max;
char buf[];
{
	int ch, i, done;


	move(line,begcol);
	clrtoeol();

	for(i = done = 0, refresh(); done == 0; refresh()) {
		ch = getch();
		if (ch == EOF || ch == ERR) {
			i = 0;
			done = 1;
		} else if (ch == erasechar()) {
			if (i > 0) {
				i -= 1;
				move (line, begcol+i);
				clrtoeol();
			}
		} else if (ch == killchar()) {
			move(line,begcol);
			clrtoeol();
			i = 0;
		} else if (ch == '\n' || ch == '\r') {
			done = 1;
		} else if (ch < ' ' || ch == 0177) {
			continue;
		} else if (i < max) {
			buf[i++] = ch;
			addch(ch);
		}
	}
	buf[i] = 0;
	return (i);
}

/*
 * common display code for examine and modify
 */

static
examine_modify_finish(eptr)
register toc_entry *eptr;
{
	mvaddstr(LINES-1,0,"Type any key to continue: ");
	clrtobot();
	refresh();

	(void) getch();

	clear();
}

static
examine_modify(eptr)
register toc_entry *eptr;
{
	register i, j;
	char *p, **v;

	erase();
	/*
	 * Put up info common to all entries
	 *
	 */

	(void) mvprintw(0,0,"Name: %s",eptr->Name);
	(void) mvprintw(0,COLS/2,"Size (on media): %6s",
		fitsize((unsigned)(Size_of_Entry(eptr))));

	(void) mvprintw(1,0,"Command: %s",
		(eptr->Command)?eptr->Command:"(internal)");

	switch(eptr->Type)
        {
        default:
        case UNKNOWN:
                p = "unknown";
		break;
        case TOC:
                p = "toc";
		break;
        case IMAGE:
                p = "image";
		break;
        case TAR:
                p = "tar";
		break;
        case CPIO:
                p = "cpio";
		break;
        }

	(void) mvprintw(2,0,"File Type: %s",p);
	(void) mvprintw(2,COLS/2,"Location: Volume #%d, File #%d",
		Volume_of_Entry(eptr),Entry_in_Volume(eptr));

	switch(eptr->Info.kind)
	{
	default:
	case UNDEFINED:
		(void) mvprintw(3,0,"File Kind: Unknown");
		break;
	case CONTENTS:
		(void) mvprintw(3,0,"File Kind: TOC (xdr table of contents)");
		break;
	case AMORPHOUS:
		(void) mvprintw(3,0,"File Kind: Amorphous");
		(void) mvprintw(3,COLS/2,"Installed Size: %u",
			eptr->Info.Information_u.File.size);
		break;
	case STANDALONE:
		(void) mvprintw(3,0,"File Kind: Standalone");
		(void) mvprintw(3,COLS/2,"Installed Size: %u",
			eptr->Info.Information_u.Standalone.size);
		mvaddstr(5,COLS/2-15,"Intended Architectures");
		i = eptr->Info.Information_u.Standalone.arch.string_list_len;
		v = eptr->Info.Information_u.Standalone.arch.string_list_val;
		j = 6;
		while(i-- > 0)
		{
			if(v && *v && **v)
			{
				mvaddstr(j++,0,*v++);
			}
		}
		break;
	case EXECUTABLE:
		(void) mvprintw(3,0,"File Kind: Executable");
		(void) mvprintw(3,COLS/2,"Installed Size: %u",
			eptr->Info.Information_u.Exec.size);
		mvaddstr(5,COLS/2-15,"Intended Architectures");
		i = eptr->Info.Information_u.Exec.arch.string_list_len;
		v = eptr->Info.Information_u.Exec.arch.string_list_val;
		j = 6;
		while(i-- > 0)
		{
			if(v && *v && **v)
			{
				mvaddstr(j++,0,*v++);
			}
		}
		break;
	case INSTALL_TOOL:
		(void) mvprintw(3,0,"File Kind: Install Tool");
		(void) mvprintw(3,COLS/2,"Installed Size: %u",
			eptr->Info.Information_u.Tool.size);
		(void) mvprintw(3,COLS-30,"Workspace Size: %u",
			eptr->Info.Information_u.Tool.workspace);
		(void) mvprintw(4,0,"Loadpoint: %s",
			eptr->Info.Information_u.Tool.loadpoint);
		(void) mvprintw(4,2*(COLS/3),"Moveable: %s",
			(eptr->Info.Information_u.Tool.moveable)?
				"true":"false");
		mvaddstr(6,COLS/3,"Who Tool Is Intended For");
		i = eptr->Info.Information_u.Tool.belongs_to.string_list_len;
		v = eptr->Info.Information_u.Tool.belongs_to.string_list_val;
		j = 7;
		while(i-- > 0)
		{
			if(v && *v && **v)
			{
				mvaddstr(j++,0,*v++);
			}
		}
		break;
	case INSTALLABLE:
		(void) mvprintw(3,0,"File Kind: Installable");
		(void) mvprintw(3,COLS/2,"Installed Size: %u",
			eptr->Info.Information_u.Install.size);
		(void) mvprintw(4,0,"Loadpoint: %s",
			eptr->Info.Information_u.Install.loadpoint);
		(void) mvprintw(4,2*(COLS/3),"Moveable: %s",
			(eptr->Info.Information_u.Install.moveable)?
				"true":"false");
		(void) mvprintw(5,0,"Required: %s",
			(eptr->Info.Information_u.Install.required)?
				"true":"false");
		(void) mvprintw(5,COLS/3,"Desirable: %s",
			(eptr->Info.Information_u.Install.desirable)?
				"true":"false");
		(void) mvprintw(5,2*(COLS/3),"Common: %s",
			(eptr->Info.Information_u.Install.common)?
				"true":"false");
		(void) mvprintw(6,0,"Pre_Install: %s",
			eptr->Info.Information_u.Install.pre_install);
		(void) mvprintw(6,COLS/2,"Install: %s",
			eptr->Info.Information_u.Install.install);

		mvaddstr(8,COLS/2-15,"Intended Architectures");
		i = eptr->Info.Information_u.Install.arch_for.string_list_len;
		v = eptr->Info.Information_u.Install.arch_for.string_list_val;
		j = 9;
		while(i-- > 0)
		{
			if(v && *v && **v)
			{
				mvaddstr(j++,0,*v++);
			}
		}
		mvaddstr(++j,COLS/2-10,"Depends On");
		i = eptr->Info.Information_u.Install.soft_depends.string_list_len;
		v = eptr->Info.Information_u.Install.soft_depends.string_list_val;
		while(i-- > 0)
		{
			if(v && *v && **v)
			{
				mvaddstr(j++,0,*v++);
			}
		}
		break;
	}
}



resequence_all_entries()
{
	register volno, filenum;
	register toc_entry *eptr;

	if(entries[0].Where.dtype != TAPE)
		return;

	volno = 1;
	filenum = 0;
	for(eptr = entries; eptr < ep; eptr++)
	{
		if(Volume_of_Entry(eptr) != volno)
		{
			volno++;
			filenum = 0;
		}
		eptr->Where.Address_u.tape.file_number = filenum++;
	}
}
