/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)xec.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.28.1.1 */
#endif

/*
 *
 * UNIX shell
 *
 */


#include	"defs.h"
#include	<errno.h>
#include	"sym.h"
#include	"hash.h"
#include	<sys/types.h>
#include	<sys/times.h>
static int	parent;

/* ========	command execution	========*/


execute(argt, exec_link, errorflg, pf1, pf2)
struct trenod	*argt;
int	*pf1, *pf2;
{
	/*
	 * `stakbot' is preserved by this routine
	 */
	register struct trenod	*t;
	char		*sav = savstak();

	sigchk();
	if (!errorflg)
		flags &= ~errflg;

	if ((t = argt) && execbrk == 0)
	{
		register int	treeflgs;
		int 			type;
		register char	**com;
		short			pos;
		int 			linked;
		int 			execflg;

		linked = exec_link >> 1;
		execflg = exec_link & 01;

		treeflgs = t->tretyp;
		type = treeflgs & COMMSK;

		switch (type)
		{
		case TFND:
			{
				struct fndnod	*f = (struct fndnod *)t;
				struct namnod	*n = lookup(f->fndnam);

				exitval = 0;

				if (n->namflg & N_RDONLY)
					failed(n->namid, wtfailed);

				if (flags & rshflg && (n == &pathnod ||
					eq(n->namid, "SHELL")))
					failed(n->namid, restricted);
				if (n->namflg & N_FUNCTN)
					freefunc(n);
				else
				{
					free(n->namval);
					free(n->namenv);

					n->namval = 0;
					n->namflg &= ~(N_EXPORT | N_ENVCHG);
				}

				if (funcnt)
					f->fndval->tretyp++;

				n->namenv = (char *)f->fndval;
				attrib(n, N_FUNCTN);
				hash_func(n->namid);
				break;
			}

		case TCOM:
			{
				char	*a1;
				int	argn, internal;
				struct argnod	*schain = gchain;
				struct ionod	*io = t->treio;
				short 	cmdhash;
				short	comtype;

				exitval = 0;

				gchain = 0;
				argn = getarg(t);
				com = scan(argn);
				a1 = com[1];
				gchain = schain;

				if (argn != 0)
					cmdhash = pathlook(com[0], 1, comptr(t)->comset);

				if (argn == 0 || (comtype = hashtype(cmdhash)) == BUILTIN)
					setlist(comptr(t)->comset, 0);

				if (argn && (flags&noexec) == 0)
				{		
					/* print command if execpr */

					if (flags & execpr)
						execprint(com);

					if (comtype == NOTFOUND)
					{
						pos = hashdata(cmdhash);
						if (pos == 1)
							failure(*com, notfound);
						else if (pos == 2)
							failure(*com, badexec);
						else
							failure(*com, badperm);
						break;
					}

					else if (comtype == PATH_COMMAND)
					{
						pos = -1;
					}

					else if (comtype & (COMMAND | REL_COMMAND))
					{
						pos = hashdata(cmdhash);
					}

					else if (comtype == BUILTIN)
					{
						short index;

						internal = hashdata(cmdhash);
						index = initio(io, (internal != SYSEXEC));

						switch (internal)			
						{	
						case SYSDOT:	
							if (a1)	
							{	
								register int	f;	
				
								if ((f = pathopen(getpath(a1), a1)) < 0)	
									failed(a1, notfound);	
								else	
									execexp(0, f);	
							}	
							break;	
				
						case SYSTIMES:	
							{	
								struct tms t;	
				
								times(&t);	
								prt(t.tms_cutime);
								prc_buff(SP);	
								prt(t.tms_cstime);
								prc_buff(NL);	
							}	
							break;	
				
						case SYSEXIT:	
							flags |= forked;	/* force exit */	
							exitsh(a1 ? stoi(a1) : retval);
				
						case SYSNULL:	
							io = 0;	
							break;	
				
						case SYSCONT:	
							if (loopcnt)
							{
								execbrk = breakcnt = 1;
								if (a1)
									breakcnt = stoi(a1);
								if (breakcnt > loopcnt)
									breakcnt = loopcnt;
								else
									breakcnt = -breakcnt;
							}
							break;	

						case SYSBREAK:	
							if (loopcnt)
							{
								execbrk = breakcnt = 1;
								if (a1)
									breakcnt = stoi(a1);
								if (breakcnt > loopcnt)
									breakcnt = loopcnt;
							}
							break;	

						case SYSTRAP:	
							if (a1)	
							{	
								BOOL 	clear;	
		
								if ((clear = digit(*a1)) == 0)	
									++com;	
								while (*++com)	
								{	
									int	i;	
		
									if ((i = stoi(*com)) >= MAXTRAP || i < MINTRAP)	
										failed(*com, badtrap);	
									else if (clear)	
										clrsig(i);	
									else	
									{	
										replace(&trapcom[i], a1);	
										if (*a1)	
											getsig(i);	
										else	
											ignsig(i);	
									}	
								}	
							}	
							else	/* print out current traps */	
							{	
								int	i;	
				
								for (i = 0; i < MAXTRAP; i++)	
								{	
									if (trapcom[i])	
									{	
										prn_buff(i);	
										prs_buff(colon);	
										prs_buff(trapcom[i]);	
										prc_buff(NL);	
									}	
								}	
							}	
							break;	
				
						case SYSEXEC:	
							rmtemp(0);
							com++;	
							ioset = 0;	
							io = 0;	
							if (a1 == 0)	
							{	
								break;	
							}	
		
						case SYSLOGIN:	
							oldsigs();	
							execa(com, -1);
							done();	
			
						case SYSNEWGRP:	
							if (flags & rshflg)	
								failed(com[0], restricted);	
							else	
							{	
								flags |= forked;	/* force bad exec to terminate shell */	
								oldsigs();	
								execa(com, -1);
								done();	
							}	

						case SYSCD:	
							if (flags & rshflg)	
								failed(com[0], restricted);	
							else if ((a1 && *a1) || (a1 == 0 && (a1 = homenod.namval)))	
							{	
								char *cdpath;	
								char *dir;	
								int f;	
								char *cdmsg;
		
								if ((cdpath = cdpnod.namval) == 0 ||	
								     *a1 == '/' ||	
								     cf(a1, ".") == 0 ||	
								     cf(a1, "..") == 0 ||	
								     (*a1 == '.' && (*(a1+1) == '/' || *(a1+1) == '.' && *(a1+2) == '/')))	
									cdpath = nullstr;	
		
								do	
								{	
									cdmsg = baddir;
									dir = cdpath;	
									cdpath = catpath(cdpath,a1);	
									if ((f = chdir(curstak())) >= 0)
										break;
									if (errno != ENOTDIR
									    && errno != ENOENT)
										cdmsg = scerrmsg();
								}	
								while (cdpath);
		
								if (f)	
									failed(a1, cdmsg);
								else 
								{
									cwd(curstak());
									if (cf(nullstr, dir) &&	
									    *dir != ':' &&	
									 	any('/', curstak()) &&	
									 	flags & prompt)	
									{	
										prs_buff(curstak());	
										prc_buff(NL);	
									}
								}
								zapcd();
							}	
							else 
							{
								if (a1)
									error(nulldir);
								else
									error(nohome);
							}

							break;	
				
						case SYSSHFT:	
							{	
								int places;	
		
								places = a1 ? stoi(a1) : 1;	
		
								if ((dolc -= places) < 0)
								{
									dolc = 0;
									error(badshift);
								}
								else
									dolv += places;
							}				
		
							break;	
				
						case SYSWAIT:	
							await(a1 ? stoi(a1) : -1, 1);	
							break;	
				
						case SYSREAD:	
							if(argn < 2)
								failed(com[0],mssgargn);
							rwait = 1;	
							exitval = readvar(&com[1]);	
							rwait = 0;	
							break;	
			
						case SYSSET:	
							if (a1)	
							{	
								int	argc;	
		
								argc = options(argn, com);	
								if (argc > 1)	
									setargs(com + argn - argc);	
							}	
							else if (comptr(t)->comset == 0)	
							{
								/*	
								 * scan name chain and print	
								 */	
								namscan(printnam);
							}
							break;	
				
						case SYSRDONLY:	
							exitval = 0;
							if (a1)	
							{	
								while (*++com)	
									attrib(lookup(*com), N_RDONLY);
							}	
							else	
								namscan(printro);

							break;	

						case SYSXPORT:	
							{
								struct namnod 	*n;

								exitval = 0;
								if (a1)	
								{	
									while (*++com)	
									{
										n = lookup(*com);
										if (n->namflg & N_FUNCTN)
											error(badexport);
										else
											attrib(n, N_EXPORT);
									}
								}	
								else	
									namscan(printexp);
							}
							break;	
				
						case SYSEVAL:	
							if (a1)	
								execexp(a1, &com[2]);	
							break;	
		
#ifndef RES		
						case SYSULIMIT:	
							{	
								long int i;	
								long ulimit();	
								int command = 2;	
		
								if (a1 && *a1 == '-')	
								{	switch(*(a1+1))	
									{	
										case 'f':	
											command = 2;	
											break;	
		
		
										default:	
											error(badopt);	
									}	
									a1 = com[2];	
								}	
								if (a1)	
								{	
									int c;	
										
									i = 0;	
									while ((c = *a1++) >= '0' && c <= '9')	
									{	
										i = (i * 10) + (long)(c - '0');	
										if (i < 0)	
											error(badulimit);	
									}	
									if (c || i < 0)	
										error(badulimit);	
								}	
								else	
								{	
									i = -1;	
									command--;	
								}	
									
								if ((i = ulimit(command,i)) < 0)	
									error(badulimit);	
		
								if (command == 1 || command == 4)	
								{	
									prl(i);	
									prc_buff('\n');	
								}	
								break;	
							}				
#endif	
									
						case SYSUMASK:	
							if (a1)	
							{ 	
								int c, i;	
		
								i = 0;	
								while ((c = *a1++) >= '0' && c <= '7')	
									i = (i << 3) + c - '0';	
								umask(i);	
							}	
							else	
							{	
								int i, j;	
		
								umask(i = umask(0));	
								prc_buff('0');	
								for (j = 6; j >= 0; j -= 3)	
									prc_buff(((i >> j) & 07) +'0');	
								prc_buff(NL);	
							}	
							break;	
		
		
						case SYSTST:
							exitval = test(argn, com);
							break;

						case SYSECHO:
							exitval = echo(argn, com);
							break;

						case SYSHASH:
							exitval = 0;
				
							if (a1)
							{
								if (a1[0] == '-')
								{
									if (a1[1] == 'r')
										zaphash();
									else
										error(badopt);
								}
								else
								{
									while (*++com)
									{
										if (hashtype(hash_cmd(*com)) == NOTFOUND)
											failed(*com, notfound);
									}
								}
							}
							else
								hashpr();

							break;

						case SYSPWD:
							{
								exitval = 0;
								cwdprint();
							}
							break;

						case SYSRETURN:
							if (funcnt == 0)
								error(badreturn);

							execbrk = 1;
							exitval = (a1 ? stoi(a1) : retval);
							break;
							
						case SYSTYPE:
							exitval = 0;
							if (a1)
							{
								while (*++com)
									what_is_path(*com);
							}
							break;

						case SYSUNS:
							exitval = 0;
							if (a1)
							{
								while (*++com)
									unset_name(*com);
							}
							break;

						case SYSGETOPT: {
							int getoptval;
							struct namnod *n;
							extern char numbuf[];	
							char *varnam = com[2];
							char c[2];
							if(argn < 3) {
								failure(com[0],mssgargn);
								break;
							}
							exitval = 0;
							n = lookup("OPTIND");
							optind = stoi(n->namval);
							if(argn > 3) {
								com[2] = dolv[0];
								getoptval = getopt(argn-2, &com[2], com[1]);
							}
							else
								getoptval = getopt(dolc+1, dolv, com[1]);
							if(getoptval == -1) {
								itos(optind);
								assign(n, numbuf);
								n = lookup(varnam);
								assign(n, nullstr);
								exitval = 1;
								break;
							}
							com[2] = varnam;
							itos(optind);
							assign(n, numbuf);
							c[0] = getoptval;
							c[1] = 0;
							n = lookup(varnam);
							assign(n, c);
							n = lookup("OPTARG");
							assign(n, optarg);
							}
							break;
						
						default:	
							prs_buff("unknown builtin\n");
						}	
					

						flushb();
						restore(index);
						chktrap();
						break;
					}

					else if (comtype == FUNCTION)
					{
						struct dolnod *olddolh;
						struct namnod *n;
						short index;
						char **olddolv = dolv;
						int olddolc = dolc;
						n = findnam(com[0]);
					/* save current positional parameters */
						olddolh = (struct dolnod *)savargs(funcnt);
						funcnt++;
						index = initio(io, 1);
						setargs(com);
						execute((struct trenod *)(n->namenv), exec_link, errorflg, pf1, pf2);
						execbrk = 0;
						restore(index);
						(void) restorargs(olddolh, funcnt);
						dolv = olddolv;
						dolc = olddolc;
						funcnt--;

						break;
					}
				}
				else if (t->treio == 0)
				{
					chktrap();
					break;
				}

			}

		case TFORK:
			exitval = 0;
			if (execflg && (treeflgs & (FAMP | FPOU)) == 0)
				parent = 0;
			else
			{
				int forkcnt = 1;

				if (treeflgs & (FAMP | FPOU))
				{
					link_iodocs(iotemp);
					linked = 1;
				}


				/*
				 * FORKLIM is the max period between forks -
				 * power of 2 usually.  Currently shell tries after
				 * 2,4,8,16, and 32 seconds and then quits
				 */
	
				while ((parent = fork()) == -1)
				{
					if ((forkcnt = (forkcnt * 2)) > FORKLIM)	/* 32 */
					{
						switch (errno)
						{
						case ENOMEM:
							error(noswap);
							break;
						default:
						case EAGAIN:
							error(nofork);
							break;
						}
					}
					sigchk();
					alarm(forkcnt);
					pause(); 
					alarm(0); /* remove alarm so alarm signal
						     doesn't kill shell later */
				}
			}
			if (parent)
			{
				/*
				 * This is the parent branch of fork;
				 * it may or may not wait for the child
				 */
				if (treeflgs & FPRS && flags & ttyflg)
				{
					prn(parent);
					newline();
				}
				if (treeflgs & FPCL)
					closepipe(pf1);
				if ((treeflgs & (FAMP | FPOU)) == 0)
					await(parent, 0);
				else if ((treeflgs & FAMP) == 0)
					post(parent);
				else
					assnum(&pcsadr, parent);
				chktrap();
				break;
			}
			else	/* this is the forked branch (child) of execute */
			{
				flags |= forked;
				fiotemp  = 0;

				if (linked == 1)
				{
					swap_iodoc_nm(iotemp);
					exec_link |= 06;
				}
				else if (linked == 0)
					iotemp = 0;

#ifdef ACCT
				suspacct();
#endif

				postclr();
				settmp();
				/*
				 * Turn off INTR and QUIT if `FINT'
				 * Reset ramaining signals to parent
				 * except for those `lost' by trap
				 */
				oldsigs();
				if (treeflgs & FINT)
				{
					signal(SIGINT, SIG_IGN);
					signal(SIGQUIT, SIG_IGN);

#ifdef NICE
					nice(NICEVAL);
#endif

				}
				/*
				 * pipe in or out
				 */
				if (treeflgs & FPIN)
				{
					renamefd(pf1[INPIPE], 0);
					close(pf1[OTPIPE]);
				}
				if (treeflgs & FPOU)
				{
					close(pf2[INPIPE]);
					renamefd(pf2[OTPIPE], 1);
				}
				/*
				 * default std input for &
				 */
				if (treeflgs & FINT && ioset == 0)
					renamefd(chkopen(devnull), 0);
				/*
				 * io redirection
				 */
				initio(t->treio, 0);

				if (type != TCOM)
				{
					execute(forkptr(t)->forktre, exec_link | 01, errorflg);
				}
				else if (com[0] != ENDARGS)
				{
					eflag = 0;
					setlist(comptr(t)->comset, N_EXPORT);
					rmtemp(0);
					execa(com, pos);
				}
				done();
			}

		case TPAR:
			execute(parptr(t)->partre, exec_link, errorflg);
			done();

		case TFIL:
			{
				int pv[2];

				chkpipe(pv);
				if (execute(lstptr(t)->lstlef, 0, errorflg, pf1, pv) == 0)
					execute(lstptr(t)->lstrit, exec_link, errorflg, pv, pf2);
				else
					closepipe(pv);
			}
			break;

		case TLST:
			execute(lstptr(t)->lstlef, 0, errorflg);
			execute(lstptr(t)->lstrit, exec_link, errorflg);
			break;

		case TAND:
			if (execute(lstptr(t)->lstlef, 0, 0) == 0)
				execute(lstptr(t)->lstrit, exec_link, errorflg);
			break;

		case TORF:
			if (execute(lstptr(t)->lstlef, 0, 0) != 0)
				execute(lstptr(t)->lstrit, exec_link, errorflg);
			break;

		case TFOR:
			{
				struct namnod *n = lookup(forptr(t)->fornam);
				char	**args;
				struct dolnod *argsav = 0;

				if (forptr(t)->forlst == 0)
				{
					args = dolv + 1;
					argsav = useargs();
				}
				else
				{
					struct argnod *schain = gchain;

					gchain = 0;
					args = scan(getarg(forptr(t)->forlst));
					gchain = schain;
				}
				loopcnt++;
				while (*args != ENDARGS && execbrk == 0)
				{
					assign(n, *args++);
					execute(forptr(t)->fortre, 0, errorflg);
					if (breakcnt < 0)
						execbrk = (++breakcnt != 0);
				}
				if (breakcnt > 0)
						execbrk = (--breakcnt != 0);

				loopcnt--;
				if(argsav)
					argfor = (struct dolnod *)freeargs(argsav);
			}
			break;

		case TWH:
		case TUN:
			{
				int	i = 0;

				loopcnt++;
				while (execbrk == 0 && (execute(whptr(t)->whtre, 0, 0) == 0) == (type == TWH))
				{
					i = execute(whptr(t)->dotre, 0, errorflg);
					if (breakcnt < 0)
						execbrk = (++breakcnt != 0);
				}
				if (breakcnt > 0)
						execbrk = (--breakcnt != 0);

				loopcnt--;
				exitval = i;
			}
			break;

		case TIF:
			if (execute(ifptr(t)->iftre, 0, 0) == 0)
				execute(ifptr(t)->thtre, exec_link, errorflg);
			else if (ifptr(t)->eltre)
				execute(ifptr(t)->eltre, exec_link, errorflg);
			else
				exitval = 0;	/* force zero exit for if-then-fi */
			break;

		case TSW:
			{
				register char	*r = mactrim(swptr(t)->swarg);
				register struct regnod *regp;

				regp = swptr(t)->swlst;
				while (regp)
				{
					struct argnod *rex = regp->regptr;

					while (rex)
					{
						register char	*s;

						if (gmatch(r, s = macro(rex->argval)) || (trim(s), eq(r, s)))
						{
							execute(regp->regcom, 0, errorflg);
							regp = 0;
							break;
						}
						else
							rex = rex->argnxt;
					}
					if (regp)
						regp = regp->regnxt;
				}
			}
			break;
		}
		exitset();
	}
	sigchk();
	tdystak(sav);
	flags |= eflag;
	return(exitval);
}

execexp(s, f)
char	*s;
int	f;
{
	struct fileblk	fb;

	push(&fb);
	if (s)
	{
		estabf(s);
		fb.feval = (char **)(f);
	}
	else if (f >= 0)
		initf(f);
	execute(cmd(NL, NLFLG | MTFLG), 0, (int)(flags & errflg));
	pop();
}

execprint(com)
	char **com;
{
	register int 	argn = 0;

	prs(execpmsg);
	
	while(com[argn] != ENDARGS)
	{
		prs(com[argn++]);
		blank();
	}

	newline();
}
