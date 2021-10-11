/* @(#) ssum.h 1.1@(#) 7/30/92 */

/* Copyright 1986 Sun Microsystems, Inc. */

/*
 * SCCS File Format Definitions (see SCCSFILE(5))
 */

#define CTLCHAR		'\001'	/* SCCS control lines begin with this char. */
#define HEAD		'h'	/* Indicates file header */

#define STATS		's'	/* Lines inserted/deleted/unchanged
				 * (begining of delta entry)
				 */
#define BDELTAB		'd'	/* Info about a delta */
#define INCLUDE		'i'	/* Insert list of deltas */
#define EXCLUDE		'x'	/* Exclude list of deltas */
#define IGNORE		'g'	/* Ignore list of deltas */
#define COMMENTS	'c'	/* Delta comments entered by user */
#define MRNUM		'm'	/* MR number */
#define EDELTAB		'e'	/* end of delta entry */

#define BUSERNAM	'u'
#define EUSERNAM	'U'

#define FLAG		'f'
#define BUSERTXT	't'
#define EUSERTXT	'T'

/* SCCS edit commands */
#define INSERT	'I'	/* Insert text into delta until END command */
#define DELETE	'D'	/* Delete text from delta until END command */
#define END	'E'	/* End of edit */

#define NOERROR	0
#define ERROR	-2	/* assumes that EOF is (-1) */

#define BADDELTA	((delta_t *)(-1))

/*
 * slist_t is a generic linked list structure
 */
typedef struct slist {
	struct slist	*next;
} slist_t;

/*
 * A sid_t holds all of the information from the STATS record for a particular
 * SCCS delta.
 */
typedef struct sid {
	int	s_rel,		/* release */
		s_lev,		/* level */
		s_br,		/* branch number */
		s_seq,		/* sequence number on branch */

		s_serial,	/* delta serial number in SCCS file */
		s_pred,		/* parent delta's serial number */

		s_added,	/* number of lines added */
		s_deleted,	/* lines deleted */
		s_same;		/* lines unchanged */
} sid_t;

/*
 * ixglist_t holds information on inserted, excluded, and ignored deltas
 */
typedef struct ixglist {
	struct ixglist	*next;
	char	action;
	short	delta;
	struct delta	*deltap;
} ixglist_t;

typedef struct queue {
	struct queue	*next;		/* queue sorted from high to low */
	short		delta,		/* delta that caused this entry */
			insert;		/* insert state */
} queue_t;

typedef struct delta {
	struct delta	*parent,	/* predecessor delta */
			*sibling,	/* branch deltas */
			*child;		/* child deltas */
	queue_t		*q;		/* insert state */
	unsigned	sum;		/* checksum */
	long		bytes;		/* number of bytes in delta */
	short		command;
	long		lines,
			maxlines,
			absline;
	sid_t		sid;
	ixglist_t	*ixglist,	/* included, excluded, & ignored deltas */
			*includes_me,	/* other deltas that include this one */
			*excludes_me,	/* other deltas that exclude this one */
			*ignores_me;	/* other deltas that ignore this one */
} delta_t;

#define CMD_NIL	0
#define CMD_INS	1
#define CMD_DEL	2

#define OFF	0
#define ON	1
#define POP	2

char	*sidstr();
delta_t	**buildtree(),
	*readhead();
int	readtext();
void	addsum(),
	changestate(),
	freearray(),
	freeslist(),
	printsums();
