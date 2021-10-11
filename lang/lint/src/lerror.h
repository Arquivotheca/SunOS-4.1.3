/*	@(#)lerror.h 1.1 92/07/30 SMI; from S5R2 1.5	*/

/* defines for lint message buffering scheme 
 * be sure to include lerror.h before lmanifest.h
 */

/* number of chars in NAME, and filename */
#define LFNM 1024	/* SHOULD BE MAXPATHLEN */

#define	NUMBUF	24
#define	MAXBUF	100

# define PLAINTY	0
# define STRINGTY	01
# define DBLSTRTY	02
# define CHARTY		04
# define NUMTY		010

# define SIMPL		020
# define WERRTY		0100
# define UERRTY		0

# define TMPDIR	"/usr/tmp"
# define TMPLEN	sizeof( TMPDIR )

# define NOTHING	0
# define ERRMSG	01
# define FATAL		02
# define CCLOSE		04
# define HCLOSE		010

union str_union {
	long	su_strtab_offset;	/* offset in string table */
	char	*su_ptr;		/* pointer into string table */
};

struct hdritem {
	union str_union hname_su;
# define hname		hname_su.su_ptr
# define hoffset	hname_su.su_strtab_offset
	union str_union sname_su;
# define srcname	sname_su.su_ptr
# define soffset	sname_su.su_strtab_offset
	int	hcount;
};

# define HDRITEM	struct hdritem
# define NUMHDRS	100

struct crecord {
    int	code;
    int	lineno;
    union {
	char	*name1;
	char	char1;
	int	number;
    } arg1;
    char	*name2;
};

# define CRECORD	struct crecord
# define CRECSZ		sizeof ( CRECORD )

# define OKFSEEK	0
# define PERMSG		((long) CRECSZ * MAXBUF )

struct hrecord {
    int		msgndx;
    int		code;
    int		lineno;
    union {
	char	*name1;
	char	char1;
	int	number;
    } arg1;
    char	*name2;
};

# define HRECORD	struct hrecord
# define HRECSZ		sizeof( HRECORD )

enum boolean { true, false };

/* for pass2 in particular */

# define NUM2MSGS	12
# define MAX2BUF	100

struct c2record {
    char	*name;
    int		number;
    int		file1;
    int		line1;
    int		file2;
    int		line2;
};

# define C2RECORD	struct c2record
# define C2RECSZ	sizeof( C2RECORD )
# define PERC2SZ	((long) C2RECSZ * MAX2BUF )

# define NMONLY	1
# define NMFNLN	2
# define NM2FNLN	3
# define ND2FNLN	4

