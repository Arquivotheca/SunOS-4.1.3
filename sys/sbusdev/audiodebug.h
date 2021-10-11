/*
 * @(#) audiodebug.h 1.1@(#) Copyright (c) 1991-92 Sun Microsystems, Inc.
 */

/* If AUDIOTRACE is defined, include debugging trace definitions */

#ifndef __audiodebug_h__
#define	__audiodebug_h__

#define	dprintf	if (Dbri_debug) (void) printf
#define	aprintf	if (Audio_debug) (void) printf

#ifdef AUDIOTRACE

#ifndef NAUDIOTRACE
#define	NAUDIOTRACE 1024
#endif

struct audiotrace {
	int	count;
	int	function;	/* address of function */
	enum trace_action {
		ATR_NONE	= '    ',
		ATR_ACTREQON	= 'act+',
		ATR_ACTREQOFF	= 'act-',
		ATR_SETPORT	= 'setp',
		ATR_RELPORT	= 'relp',

		/* activation state definitions */
		ATR_F8		= 'TEF8',
		ATR_F7		= 'TEF7',
		ATR_F6		= 'TEF6',
		ATR_F4		= 'F345',
		ATR_G1		= 'NTG1',
		ATR_G2		= 'NTG2',
		ATR_G3		= 'NTG3',

		/* interrupt code definitions */
		ATR_LATE_ERR	= 'LATE',
		ATR_BUS_GRANT	= 'BGNT',
		ATR_BURST_ERR	= 'BRST',
		ATR_BERR	= 'BERR',
		ATR_FERR	= 'FERR',
		ATR_BRDY	= 'BRDY',
		ATR_MINT	= 'MINT',
		ATR_IBEG	= 'IBEG',
		ATR_EOL		= 'EOL ',
		ATR_CMDI	= 'CMDI',
		ATR_XCMP	= 'XCMP',
		ATR_SBRI	= 'SBRI',
		ATR_FXDT	= 'FXDT',
		ATR_CHIL	= 'CHIL',
		ATR_DBYT	= 'DBYT',
		ATR_RBYT	= 'RBYT',
		ATR_LINT	= 'LINT',
		ATR_UNDR	= 'UNDR',
		ATR_UNKNOWN	= 'unkn',

		ATR_MFSB	= 'mfsb',
		ATR_TXOUT	= 'tout',
		ATR_NEVER	= '!!!!',
		ATR_NOTBUSY	= '!bsy',
		ATR_INTRON	= '+int',
		ATR_INTROFF	= '-int',
		ATR_FIRST	= '1st ',
		ATR_INTRNOP	= 'Eint',
		ATR_FLOWPLAY	= 'P-ER',
		ATR_SKIPPLAY	= 'P-SK',
		ATR_FIRSTPLAY	= 'P1st',
		ATR_CLOSEPLAY	= 'Pclz',
		ATR_DONEPLAY	= 'Pfin',
		ATR_STARTPLAY	= 'Pgo ',
		ATR_OPENPLAY	= 'Popn',
		ATR_STOPPLAY	= 'Pstp',
		ATR_FLOWREC	= 'R-ER',
		ATR_SKIPREC	= 'R-SK',
		ATR_FIRSTREC	= 'R1st',
		ATR_CLOSEREC	= 'Rclz',
		ATR_DONEREC	= 'Rfin',
		ATR_STARTREC	= 'Rgo ',
		ATR_OPENREC	= 'Ropn',
		ATR_STOPREC	= 'Rstp',
		ATR_STOP	= 'STOP',
		ATR_CMDALLOC	= 'allC',
		ATR_APPEND	= 'appC',
		ATR_BUSY	= 'busy',
		ATR_CLOSEDRAIN	= 'cldr',
		ATR_BEGINCLOSE	= 'cloz',
		ATR_CLOSED	= 'clzd',
		ATR_DELCMD	= 'delC',
		ATR_DRAIN	= 'drai',
		ATR_DROPPACKET	= 'drpk',
		ATR_EOF		= 'eof ',
		ATR_EXITCLOSE	= 'exit',
		ATR_FLUSH	= 'flsh',
		ATR_CMDFREE	= 'freC',
		ATR_START	= 'go  ',
		ATR_INTERRUPTS	= 'intr',
		ATR_IOCTL	= 'ioct',
		ATR_BAD_IOCTL	= 'bioc',
		ATR_IREGSET	= 'irle',
		ATR_IREGGET	= 'irge',
		ATR_LOST	= 'lost',
		ATR_MARK	= 'mark',
		ATR_NEWR	= 'newr',
		ATR_OPENFLAG	= 'open',
		ATR_OPENED	= 'opnd',
		ATR_PLAYINTR	= 'pint',
		ATR_PUTRECBUF	= 'putr',
		ATR_RECORDINTR	= 'rint',
		ATR_RQ		= 'rque',
		ATR_SIGCTL	= 'sigC',
		ATR_WHY		= 'why?',
		ATR_WQ		= 'wque',


	}	action;
	int	object;		/* object operated on */
};

struct audiotrace	audiotrace_buffer[NAUDIOTRACE+1];
struct audiotrace	*audiotrace_ptr;
int			audiotrace_count;

#define	ATRACEINIT() {						\
	if (audiotrace_ptr == NULL)				\
		audiotrace_ptr = audiotrace_buffer;		\
	}

#define	ATRACE(func, act, obj) {		\
	int _s = splhigh();			\
	int *_p = &audiotrace_ptr->count;	\
	*_p++ = ++audiotrace_count;		\
	*_p++ = (int)(func);			\
	*_p++ = (act);				\
	*_p++ = (int)(obj);			\
	if ((struct audiotrace *)(void *)_p >= &audiotrace_buffer[NAUDIOTRACE])\
		audiotrace_ptr = audiotrace_buffer;		\
	else							\
		audiotrace_ptr = (struct audiotrace *)(void *)_p;	\
	(void) splx(_s);					\
	}

#define	ATRACEI(func, act, obj) {		\
	register int *_p = &audiotrace_ptr->count;	\
	*_p++ = ++audiotrace_count;		\
	*_p++ = (int)(func);			\
	*_p++ = (act);				\
	*_p++ = (int)(obj);			\
	if ((struct audiotrace *)(void *)_p >= &audiotrace_buffer[NAUDIOTRACE])\
		audiotrace_ptr = audiotrace_buffer;		\
	else					\
		audiotrace_ptr = (struct audiotrace *)(void *)_p;	\
	}

#define	ATRACEPRINT(str)	(void) printf(str)

#else	/* !AUDIOTRACE */

/* If no tracing, define no-ops */
#define	ATRACEINIT()
#define	ATRACE(a, b, c)
#define	ATRACEI(a, b, c)
#define	ATRACEPRINT(str)

#endif	/* !AUDIOTRACE */
#endif /* __audiodebug_h__ */
