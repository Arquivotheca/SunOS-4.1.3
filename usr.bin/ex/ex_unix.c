/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1979 Regents of the University of California */

#ifndef lint
static	char *sccsid = "@(#)ex_unix.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.15 */
#endif

#include "ex.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"

#ifdef VFORK
#include <vfork.h>
#endif VFORK

/*
 * Unix escapes, filtering
 */

/*
 * First part of a shell escape,
 * parse the line, expanding # and % and ! and printing if implied.
 */
unix0(warn)
	bool warn;
{
	register char *up, *fp;
	register short c;
	char printub, puxb[UXBSIZE + sizeof (int)];
	printub = 0;
	CP(puxb, uxb);
	c = getchar();
	if (c == '\n' || c == EOF)
		error("Incomplete shell escape command@- use 'shell' to get a shell");
	up = uxb;
	do {
		switch (c) {

		case '\\':
			if (any(peekchar(), "%#!"))
				c = getchar();
		default:
			if (up >= &uxb[UXBSIZE]) {
tunix:
				uxb[0] = 0;
				error("Command too long");
			}
			*up++ = c;
			break;

		case '!':
			if ((up != uxb) && (peekchar() == '!')) {
				/*
				 * If the command has a '!!' discard the
				 * previous '!' and the one we peeked at
				 * and substitute the previous command.
				 */
				getchar();
				fp = puxb;
				if (*fp == 0) {
					uxb[0] = 0;
					error("No previous command@to substitute for !");
				}
				printub++;
				while (*fp) {
					if (up >= &uxb[UXBSIZE])
						goto tunix;
					*up++ = *fp++;
				}
			} else if (up == uxb) {
				/* If up = uxb it means we are on the first
				 * character inside the shell command.
				 * (i.e., after the ":!")
				 *
				 * The user has just entered ":!!" which
				 * means that though there is only technically
				 * one '!' we know he really meant ":!!!". So
				 * substitute the last command for him.
				 */
				fp = puxb;
				if (*fp == 0) {
					uxb[0] = 0;
					error("No previous command@to substitute for !");
				}
				printub++;
				while (*fp) {
					if (up >= &uxb[UXBSIZE])
						goto tunix;
					*up++ = *fp++;
				}
			} else {
				/*
				 * Treat a lone "!" as just a regular character
				 * so commands like "mail machine!login" will
				 * work as usual (i.e., the user doesn't need
				 * to dereference the "!" with "\!").
				 */
				if (up >= &uxb[UXBSIZE]) {
					uxb[0] = 0;
					error("Command too long");
				}
				*up++ = c;
			} 
			break;

		case '#':
			fp = altfile;
			if (*fp == 0) {
				uxb[0] = 0;
				error("No alternate filename@to substitute for #");
			}
			goto uexp;

		case '%':
			fp = savedfile;
			if (*fp == 0) {
				uxb[0] = 0;
				error("No filename@to substitute for %%");
			}
uexp:
			printub++;
			while (*fp) {
				if (up >= &uxb[UXBSIZE])
					goto tunix;
				*up++ = *fp++;
			}
			break;
		}
		c = getchar();
	} while (c == '"' || c == '|' || !endcmd(c));
	if (c == EOF)
		ungetchar(c);
	*up = 0;
	if (!inopen)
		resetflav();
	if (warn)
		ckaw();
	if (warn && hush == 0 && chng && xchng != chng && value(vi_WARN) && dol > zero) {
		xchng = chng;
		vnfl();
		printf(mesg("[No write]|[No write since last change]"));
		noonl();
		flush();
	} else
		warn = 0;
	if (printub) {
		if (uxb[0] == 0)
			error("No previous command@to repeat");
		if (inopen) {
			splitw++;
			vclean();
			vgoto(WECHO, 0);
		}
		if (warn)
			vnfl();
		if (hush == 0)
			lprintf("!%s", uxb);
		if (inopen && Outchar != termchar) {
			vclreol();
			vgoto(WECHO, 0);
		} else
			putnl();
		flush();
	}
}

/*
 * Do the real work for execution of a shell escape.
 * Mode is like the number passed to open system calls
 * and indicates filtering.  If input is implied, newstdin
 * must have been setup already.
 */
ttymode
unixex(opt, up, newstdin, mode)
	char *opt, *up;
	int newstdin, mode;
{
	int pvec[2];
	ttymode f;

	signal(SIGINT, SIG_IGN);
#ifdef SIGTSTP
	if (dosusp)
		signal(SIGTSTP, SIG_DFL);
#endif
	if (inopen)
		f = setty(normf);
	if ((mode & 1) && pipe(pvec) < 0) {
		/* Newstdin should be io so it will be closed */
		if (inopen)
			setty(f);
		error("Can't make pipe for filter");
	}
#ifndef VFORK
	pid = fork();
#else
	pid = vfork();
#endif
	if (pid < 0) {
		if (mode & 1) {
			close(pvec[0]);
			close(pvec[1]);
		}
		setrupt();
		if (inopen)
			setty(f);
		error("No more processes");
	}
	if (pid == 0) {
		if (mode & 2) {
			close(0);
			dup(newstdin);
			close(newstdin);
		}
		if (mode & 1) {
			close(pvec[0]);
			close(1);
			dup(pvec[1]);
			if (inopen) {
				close(2);
				dup(1);
			}
			close(pvec[1]);
		}
		if (io)
			close(io);
		if (tfile)
			close(tfile);
#ifndef VMUNIX
		close(erfile);
#endif
		signal(SIGHUP, oldhup);
		signal(SIGQUIT, oldquit);
		if (ruptible)
			signal(SIGINT, SIG_DFL);
	 	execlp(svalue(vi_SHELL), "sh", opt, up, (char *) 0);
		printf("Invalid SHELL value: %s\n", svalue(vi_SHELL));
		flush();
		error(NOSTR);
	}
	if (mode & 1) {
		io = pvec[0];
		close(pvec[1]);
	}
	if (newstdin)
		close(newstdin);
	return (f);
}

/*
 * Wait for the command to complete.
 * F is for restoration of tty mode if from open/visual.
 * C flags suppression of printing.
 */
unixwt(c, f)
	bool c;
	ttymode f;
{

	waitfor();
#ifdef SIGTSTP
	if (dosusp)
		signal(SIGTSTP, onsusp);
#endif
	if (inopen)
		setty(f);
	setrupt();
	if (!inopen && c && hush == 0) {
		printf("!\n");
		flush();
		termreset();
		gettmode();
	}
}

/*
 * Setup a pipeline for the filtration implied by mode
 * which is like a open number.  If input is required to
 * the filter, then a child editor is created to write it.
 * If output is catch it from io which is created by unixex.
 */
vi_filter(mode)
	register int mode;
{
	static int pvec[2];
	ttymode f;	/* was register */
	register int nlines = lineDOL();
	struct stat statb;
	jmp_buf save_vreslab;
	int save_vcatch;

	mode++;
	if (mode & 2) {
		signal(SIGINT, SIG_IGN);
		if (pipe(pvec) < 0)
			error("Can't make pipe");
		pid = fork();
		io = pvec[0];
		if (pid < 0) {
			setrupt();
			close(pvec[1]);
			error("No more processes");
		}
		if (pid == 0) {
			setrupt();
			io = pvec[1];
			close(pvec[0]);
			putfile(1);
			exit(errcnt);
		}
		close(pvec[1]);
		io = pvec[0];
		setrupt();
	}
	f = unixex("-c", uxb, (mode & 2) ? pvec[0] : 0, mode);
	bcopy((char *)vreslab, (char *)save_vreslab, sizeof(jmp_buf));
	save_vcatch = vcatch;
	CATCH
		if (mode == 3) {
			delete(0);
			addr2 = addr1 - 1;
		}
		if (mode == 1)
			deletenone();
		if (mode & 1) {
			if(FIXUNDO)
				undap1 = undap2 = addr2+1;
			if (fstat(io, &statb) < 0)
				bsize = LBSIZE;
			else {
				bsize = min(statb.st_blksize, MAXBSIZE);
				if (bsize <= 0)
					bsize = LBSIZE;
			}
			ignore(append(getfile, addr2));
#ifdef UNDOTRACE
			if (trace)
				vudump("after append in filter");
#endif
	ONERR
		close(io);
		io = -1;
		unixwt(!inopen, f);
		bcopy((char *)save_vreslab, (char *)vreslab, sizeof(jmp_buf));
		vcatch = save_vcatch;
		if (vcatch)
			longjmp(vreslab, 1);	/* linked exception handlers */
	ENDCATCH
	}
	close(io);
	io = -1;
	unixwt(!inopen, f);
	netchHAD(nlines);
}

/*
 * Set up to do a recover, getting io to be a pipe from
 * the recover process.
 */
recover()
{
	static int pvec[2];

	if (pipe(pvec) < 0)
		error(" Can't make pipe for recovery");
	pid = fork();
	io = pvec[0];
	if (pid < 0) {
		close(pvec[1]);
		error(" Can't fork to execute recovery");
	}
	if (pid == 0) {
		char cryptkey[19];
		close(2);
		dup(1);
		close(1);
		dup(pvec[1]);
	        close(pvec[1]);
			execlp(EXRECOVER, "exrecover", svalue(vi_DIRECTORY), file, (char *) 0);
		close(1);
		dup(2);
		error(" No recovery routine");
	}
	close(pvec[1]);
}


/*
 * Wait for the process (pid an external) to complete.
 */
waitfor()
{

        do
                rpid = wait(&status);
        while (rpid != pid && rpid != -1);
        if ((status & 0377) == 0) {
                status = (status >> 8) & 0377;
        } else {
                printf("%d: terminated with signal %d", pid, status & 0177);
                if (status & 0200)
                        printf(" -- core dumped");
                putchar('\n');
        }
}


/*
 * The end of a recover operation.  If the process
 * exits non-zero, force not edited; otherwise force
 * a write.
 */
revocer()
{

	waitfor();
	if (pid == rpid && status != 0)
		edited = 0;
	else
		change();
}
