#ifndef lint
static	char sccsid[] = "@(#)lerror2.c 1.1 92/07/30 SMI"; /* from S5R2 1.5 */
#endif

/* lerror2.c
 *	This file contains routines for message and error handling for
 *	the second lint pass (lint2).
 *
 *	Functions:
 *	==========
 *		buffer		buffer a message
 *		catchsig	set up signals
 *		lerror		lint error message
 *		onintr		clean up after an interrupt
 *		tmpopen		open intermediate and temporary files
 *		un2buffer	dump second pass messages
 *		unbuffer	dump header messages from first pass
 */

# include	<stdio.h>
# include	"lerror.h"
# include	"mip.h"
# include	"lmanifest.h"
# include	"lpass2.h"
# include	"messages.h"
# include	<signal.h>
# include	<ctype.h>

extern int	sys5flag;

/* tmpopen
 *	open source message buffer file for writing
 *  open header message file for reading
 *    initialize header file name and count list from header message file
 */

static char	ctmpname[ TMPLEN + 16 ] = "";
char		*htmpname = NULL;

static FILE	*ctmpfile = NULL;
static FILE	*htmpfile = NULL;

static HDRITEM	hdrlist[ NUMHDRS ];
static char	*hstrtab;

tmpopen( )
{
    register long hstrtab_pos;
    register int hstrtab_len;
    register int i;

    sprintf( ctmpname, "%s/clint%d", TMPDIR, getpid( ) );

    catchsig( );
    if ( (ctmpfile = fopen( ctmpname, "w" )) == NULL )
	lerror( "cannot open message buffer file", FATAL | ERRMSG );

    if ( htmpname == NULL )
	return;
    if ( (htmpfile = fopen( htmpname, "r" )) == NULL )
		lerror( "cannot open header message buffer file",
		  CCLOSE | FATAL | ERRMSG );
    if(fread( (char *)hdrlist, sizeof(HDRITEM),NUMHDRS,htmpfile) != NUMHDRS ) 
		lerror( "cannot read header message buffer file",
		  CCLOSE | HCLOSE | FATAL | ERRMSG );
    hstrtab_len = getw( htmpfile );
    if ( ferror( htmpfile ) || feof( htmpfile ) )
	lerror( "cannot read header message buffer file",
	  CCLOSE | HCLOSE | FATAL | ERRMSG );
    hstrtab_pos = ftell( htmpfile );
    if (hstrtab_len != 0) {
	hstrtab = malloc( (unsigned) hstrtab_len );
	if ( hstrtab == NULL )
		lerror( "cannot allocate space for string table",
		  CCLOSE | HCLOSE | FATAL | ERRMSG );
	if ( fseek( htmpfile, (long) -hstrtab_len, 2 )
	  != OKFSEEK ) 
		lerror( "cannot seek in header message buffer file",
		  CCLOSE | HCLOSE | FATAL | ERRMSG );
	if ( fread( hstrtab, sizeof( char ), hstrtab_len, htmpfile)
	  != hstrtab_len )
		lerror( "cannot read header message buffer file",
		  CCLOSE | HCLOSE | FATAL | ERRMSG );
	for ( i = 0; i < NUMHDRS && hdrlist[i].soffset != 0; i++ ) {
		hdrlist[i].hname = hstrtab + hdrlist[i].hoffset;
		hdrlist[i].srcname = hstrtab + hdrlist[i].soffset;
	}
    }
    if ( fseek( htmpfile, hstrtab_pos, 0 ) != OKFSEEK ) 
	lerror( "cannot seek in header message buffer file",
	  CCLOSE | HCLOSE | FATAL | ERRMSG );
}
/* lerror - lint error message routine
 *  if code is [CH]CLOSE error close and unlink appropriate files
 *  if code is FATAL exit
 */

lerror( message, code ) char *message; int code;
{
    if ( code & ERRMSG )
		fprintf( stderr, "lint pass2 error: %s\n", message );

    if ( code & CCLOSE )
		if ( ctmpfile != NULL ) {
			fclose( ctmpfile );
			unlink( ctmpname );
		}
    if ( code & HCLOSE )
		if ( htmpfile != NULL ) {
			fclose( htmpfile );
			unlink( htmpname );
		}
    if ( code & FATAL ) (void) exit( FATAL );
}

/* gethstr - reads in a null terminated string from htmpfile and
*		returns a pointer to it.
*/

char *
gethstr()
{
	static char buf[BUFSIZ];
	register char *cp = buf;
	register int ch;

	while ( ( ch = getc( htmpfile ) ) != EOF )
	{
		*cp++ = ch;
		if ( ch == '\0' || !isascii( ch ) )
			break;
	}
	if ( ch != '\0' )
		lerror( "Name string format error in header msg buffer file",
			HCLOSE | FATAL | ERRMSG );
	return ( buf );
}

/* unbuffer - writes out information saved in htmpfile */

unbuffer( )
{
    int		i, j, stop;
    HRECORD	record;

    if (fseek( htmpfile, (long) sizeof ( hdrlist ) + 4, 0 ) != OKFSEEK )  {
		lerror( "cannot seek in header message buffer file", HCLOSE | ERRMSG );
		return;
    }

    for ( i = 0; ( i < NUMHDRS ) && ( hdrlist[ i ].hcount != 0 ); ++i ) {
		stop = hdrlist[ i ].hcount;
		printf( "\n%s  (as included in %s)\n==============\n",
			hdrlist[ i ].hname, hdrlist[ i ].srcname );
		for ( j = 0; j < stop; ++j ) {
			if ( fread( (char *) &record, HRECSZ, 1, htmpfile ) != 1 ) 
				lerror( "cannot read header message buffer file",
				  HCLOSE | FATAL | ERRMSG );

			printf( "(%d)  ", record.lineno );
			if ( record.code & WERRTY ) 
				printf( "warning: " );

			switch( record.code & ~( WERRTY | SIMPL ) ) {

			case DBLSTRTY:
				{
					char tmpstr[ BUFSIZ ];

					strcpy( tmpstr, gethstr() );
					record.name2 = tmpstr;
					record.arg1.name1 = gethstr();
					printf( msgtext[ record.msgndx ],
					    record.arg1.name1, record.name2 );
					break;
				}

			case STRINGTY:
				record.arg1.name1 = gethstr();
				printf( msgtext[ record.msgndx ], record.arg1.name1 );
				break;

			case CHARTY:
				printf( msgtext[ record.msgndx ], record.arg1.char1 );
				break;

			case NUMTY:
				printf( msgtext[ record.msgndx ], record.arg1.number );
				break;

			default:
				printf( msgtext[ record.msgndx ] );
				break;

			}
		printf( "\n" );
		}
    }
    fclose( htmpfile );
    unlink( htmpname );
}
/*  onintr - clean up after an interrupt
 *  ignores signals (interrupts) during its work
 */
onintr( )
{
    (void) signal(SIGINT, SIG_IGN);
    (void) signal(SIGHUP, SIG_IGN);
    (void) signal(SIGQUIT, SIG_IGN);
    (void) signal(SIGPIPE, SIG_IGN);
    (void) signal(SIGTERM, SIG_IGN);

	putc( '\n', stderr);
    lerror( "interrupt", CCLOSE | HCLOSE | FATAL );
    /* note that no message is printed */
}

/*  catchsig - set up signal handling */

catchsig( )
{
    if ((signal(SIGINT, SIG_IGN)) == SIG_DFL)
	(void) signal(SIGINT, onintr);

    if ((signal(SIGHUP, SIG_IGN)) == SIG_DFL)
	(void) signal(SIGHUP, onintr);

    if ((signal(SIGQUIT, SIG_IGN)) == SIG_DFL)
	(void) signal(SIGQUIT, onintr);

    if ((signal(SIGPIPE, SIG_IGN)) == SIG_DFL)
	(void) signal(SIGPIPE, onintr);

    if ((signal(SIGTERM, SIG_IGN)) == SIG_DFL)
	(void) signal(SIGTERM, onintr);
}

static int	msg2totals[ NUM2MSGS ];
# define nextslot(x)	(( PERC2SZ * (x) ) + ( C2RECSZ * msg2totals[(x)] ))

/* VARARGS2 */
buffer( msgndx, symptr, digit ) int	msgndx; SYMTAB *symptr; int digit;
{
    extern char		*strncpy( );

    extern char		*msg2nbtext[ ];
    extern short	msg2type[ ];
    extern char		**fnm;
    extern int		cfno;
    extern union rec	r;

    C2RECORD		record;

    if ( ( msgndx < 0 ) || ( msgndx > NUM2MSGS ) )
		lerror( "message buffering scheme flakey", CCLOSE | FATAL | ERRMSG );

    if ( !sys5flag ) {
		if ( msg2type[ msgndx ] == ND2FNLN ) {
			printf( "%s, arg. %d", symptr->name, digit);
		} else {
			if (msg2type[ msgndx ] == NMONLY
			    && (symptr->decflag & LDS))
				printf( "%s(%d):", fnm[symptr->fno],
				    symptr->fline );
			printf( "%s", symptr->name );
		}
		switch( msg2type[ msgndx ] ) {

	    case NM2FNLN:
	    case ND2FNLN:
		printf( "%s", msg2nbtext[ msgndx ] );
		printf( "\t%s(%d)  ::  %s(%d)\n",
		  fnm[ symptr->fno ], symptr->fline,
		  fnm[ cfno ], r.l.fline );
		break;

	    case NMFNLN:
		printf( msg2nbtext[ msgndx ],
		  fnm[ symptr->fno ], symptr->fline );
		printf("\n");
		break;

	    case NMONLY:
		printf( "%s", msg2nbtext[ msgndx ] );
		printf("\n");
		break;

	    default:
		printf("\n");
		break;
		}
	return;
    }
    if ( msg2totals[ msgndx ] < MAX2BUF ) {
		record.name = symptr->name;

		switch( msg2type[ msgndx ] ) {

	    case ND2FNLN:
		record.number = digit;
		/* no break */

	    case NM2FNLN:
		record.file2 = cfno;
		record.line2 = r.l.fline;
		/* no break */

	    case NMFNLN:
		record.file1 = symptr->fno;
		record.line1 = symptr->fline;
		break;

	    case NMONLY:
		if (symptr->decflag & LDS) {
			record.file1 = symptr->fno;
			record.line1 = symptr->fline;
		} else
			record.file1 = -1;
		break;

	    default:
		break;
		}

		if ( fseek( ctmpfile, nextslot( msgndx ), 0 ) == OKFSEEK ) {
			if ( fwrite( (char *) &record, C2RECSZ, 1, ctmpfile ) != 1 )
				lerror( "cannot write to message buffer file",
				  CCLOSE | FATAL | ERRMSG );
		}
		else
			lerror( "cannot seek in message buffer file",
			  CCLOSE | FATAL | ERRMSG );
    }
    ++msg2totals[ msgndx ];
}
/* un2buffer - dump the second pass messages */

un2buffer( )
{
    extern char		*msg2text[ ];
    extern short	msg2type[ ];
    extern char		**fnm;

    int		i, j, stop;
    int		toggle;
    enum boolean	codeflag;
    C2RECORD	record;

    fclose( ctmpfile );
    if ( (ctmpfile = fopen( ctmpname, "r" )) == NULL ) 
		lerror( "cannot open message buffer file for reading",
		  CCLOSE | FATAL | ERRMSG );

    codeflag = false;

		/* note: ( msgndx == NUM2MSGS ) --> dummy message */
    for ( i = 0; i < NUM2MSGS ; ++i ) 
		if ( msg2totals[ i ] != 0 ) {
			if ( codeflag == false ) {
				printf( "\n\n==============\n" );
				codeflag = true;
			}
			toggle = 0;

			if ( fseek( ctmpfile, (PERC2SZ * i), 0 ) != OKFSEEK )
				lerror( "cannot seek in message buffer file",
				  CCLOSE | FATAL | ERRMSG );
			stop = msg2totals[ i ];
			if ( stop > MAX2BUF ) stop = MAX2BUF;

			printf( "%s\n", msg2text[ i ] );
			for ( j = 0; j < stop; ++j ) {
				if ( fread( (char *) &record, C2RECSZ, 1, ctmpfile ) != 1 )
					lerror( "cannot read message buffer file",
					  CCLOSE | FATAL | ERRMSG );
				switch( msg2type[ i ] ) {

				case NM2FNLN:
					printf( "    %s   \t%s(%d) :: %s(%d)\n",
					  record.name, fnm[ record.file1 ], record.line1,
					  fnm[ record.file2 ], record.line2 );
					break;

				case NMFNLN:
					printf( "    %s   \t%s(%d)\n",
					  record.name,
					  fnm[ record.file1 ], record.line1 );
					break;

				case NMONLY:
					if (record.file1 >= 0)
					    printf( "    %s(%d):%s",
						fnm[ record.file1 ],
						record.line1, record.name );
					else
					    printf( "    %s", record.name );
					if ( ++toggle == 3 ) {
						printf( "\n" );
						toggle = 0;
					}
					else printf( "\t" );
					break;

				case ND2FNLN:
					printf( "    %s( arg %d )   \t%s(%d) :: %s(%d)\n",
					  record.name, record.number, fnm[ record.file1 ],
					  record.line1, fnm[ record.file2 ], record.line2 );
					break;

				default:
					break;
				}
			}
			if ( toggle != 0 ) printf( "\n" );
			if ( stop < msg2totals[ i ] ) 
				printf( "    %d messages suppressed for lack of space\n",
			msg2totals[ i ] - stop );
	    } /* end for, if */
    fclose( ctmpfile );
    unlink( ctmpname );
}

