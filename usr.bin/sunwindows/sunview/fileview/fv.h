/*	@(#)fv.h 1.1 92/07/30 SMI	*/
/* USEFUL CONSTANTS */
#define BYTE			unsigned char
#define SBYTE			char
#define BOOLEAN			unsigned char
typedef int (*ROUTINE)();		/* Generic function definition */
#define SUCCESS			0	/* Function succeeded */
#define FAILURE			1	/* Function failed */
#define EMPTY			(-1)	/* Used in place of NULL for arrays.. */
#define BLACK			1	/* Colormap index */

/* MAXIMUMS: */
#define MAXPATH			1024	/* Maximum number of path characters */
#define MAXBIND 		128
#define MAXFILES		768
#define FV_MAXHISTORY		10	/* Max number of remembered folders */

/* MARGINS & GAPS & SCREEN DIMENSIONS: */
#define MARGIN			5
#define TOP_MARGIN		10
#define FV_WIDTH_COLS		60	/* Window width */
#define SCROLLBAR_WIDTH		14
#define SCROLLBAR_HEIGHT 	14
#define GLYPH_WIDTH		41
#define GLYPH_HEIGHT		41
#define TREE_GLYPH_TOP		7
#define TREE_GLYPH_HEIGHT	34
#define PROTO_TOP_MARGIN	28

/*KEYS: */
#define K_AGAIN			2
#define K_PROPS			3
#define K_UNDO			4
#define K_PUT			6
#define K_GET			8
#define K_FIND			9
#define K_DELETE		10

/* OBJECTS: */
#define FV_IFOLDER		0	/* Ordinary folder */
#define FV_IDOCUMENT		1	/* Generic document */
#define FV_IAPPLICATION		2	/* Executable */
#define FV_ISYSTEM		3	/* Pipe, socket, /dev stuff... */
#define FV_IBROKENLINK		4	/* Broken symbolic link */
#define FV_IUNEXPLFOLDER	5	/* Explored folder */
#define FV_IOPENFOLDER		6	/* Open folder */
#define FV_MAXOBJECTS		7

/* TREE STATUS: */
#define PRUNE			001	/* Pruned? */
#define SYMLINK			010	/* Symlink? */

/* DISPLAY SORT: */
#define FV_SORTALPHA		0	/* Sort alphabetically */
#define FV_SORTTIME		1	/* Sort by modification time */
#define FV_SORTSIZE		2	/* Sort by file size */
#define FV_SORTTYPE		3	/* Sort name within type */
#define FV_SORTCOLOR		4	/* Sort name within color */
#define FV_NOSORT		5	/* Don't sort folder */

/* DISPLAY STYLE: */
#define FV_DICON		0001	/* Display Icons */
#define FV_DPERMISSIONS		0002	/* Display permissions */
#define FV_DLINKS		0004	/* Display links */
#define FV_DOWNER		0010	/* Display owner */
#define FV_DGROUP		0020	/* Display group */
#define FV_DSIZE		0040	/* Display size */
#define FV_DDATE		0100	/* Display date */

#define FV_TEXTEDIT		0	/* Use textedit editor */
#define FV_VI			1	/* or vi */
#define FV_OTHER_EDITOR		2	/* or other */

#define FV_BUILD_FOLDER		0	/* Read directory */
#define FV_STYLE_FOLDER		1	/* Display options have changed */
#define FV_DISPLAY_FOLDER	2	/* Just display it */
#define FV_SMALLER_FOLDER	3	/* Smaller folder */

#define FV_COPYCURSOR		1	/* Copy cursor */
#define FV_MOVECURSOR		2	/* Move cursor */

/* MESSAGES, WARNINGS, ERRORS... */
#define MEWIN		0
#define MENOMEMORY	1
#define MECHDIR		2
#define MPRINT		3
#define MECREAT		4
#define MEDELETE	5
#define MDELCURRENT	6
#define MEMAGIC		7
#define MESELFIRST	8
#define MEBINDENT	9
#define MEPATTERN	10
#define MEMUSTENTER1	11
#define MEMUSTENTER2	12
#define MEUNKNOWNMAGIC	13
#define MELOAD1ST	14
#define MEFINDCLIENT	15
#define MSUCCEED	16
#define MCOPY		17
#define MMOVE		18
#define MEREAD		19
#define MECOPY		20
#define MCONFIRMDEL	21
#define MEDELFAILED	22
#define MEUNDELFAILED	23
#define MEEXIST		24
#define MRENAME		25
#define MLINK		26
#define ME2MANYFILES	27
#define MEFIND		28
#define MERENAME	29
#define MEDATE		30
#define MEOWNER		31
#define MSEARCH		32
#define MEPTY		33
#define MECHAR		34
#define MELISTBOX	35
#define MAUTOUPDATE	36
#define MESELN		37
#define MHOWTOPASTE	38
#define MENOSELN	39
#define MMODIFIED	40
#define MEHIDDEN	41
#define MECMD		42
#define MEEXEC		43
#define MEXEC		44

/* DATA STRUCTURES: */

typedef struct fsnode 			/* Tree node structure */
{
	char *name;			/* Folder name */
	struct fsnode *parent;		/* Who owns it */
	struct fsnode *child;		/* Who it owns */
	struct fsnode *sibling;		/* Peers */
	time_t mtime;			/* Modified time (when expl'd) */
	short x, y;			/* Display coord */
	BYTE stack;			/* Display stack depth */
	BYTE status;			/* Status; pruned, symlink, etc */
} 
FV_TNODE;

typedef struct 				/* File structure (in folder pane) */
{
	char *name;			/* File name */
	short x;			/* X coordinate */
	short width;			/* Width of string in pixels */
	BYTE type;			/* Document, folder, etc */
	SBYTE icon;			/* Index into bind array */
	BYTE selected;			/* Selected? */
	SBYTE color;			/* Icon color */
	u_short mode;			/* Permissions */
	short nlink;			/* Number of links */
	short uid;			/* Owner id */
	short gid;			/* Group id */
	long size;			/* file size */
	time_t mtime;			/* Modify time */
}
FV_FILE;

typedef struct {
	Canvas canvas;			/* Canvas */
	PAINTWIN pw;			/* Pixwin or Paintwindow */
	Scrollbar hsbar;		/* Horizontal scroll bar */
	Scrollbar vsbar;		/* Vertical scroll bar */
	Rect r;				/* Visible canvas dimensions */
}
FV_TREE_WINDOW;

typedef struct {
	Canvas canvas;			/* Canvas */
	PAINTWIN pw;			/* Pixwin or Paintwindow */
	Scrollbar vsbar;		/* Vertical scrollbar */
	int height;			/* In pixels of window */
}
FV_FOLDER_WINDOW;

typedef struct
{
	long off;			/* Byte offset into file */
	char type;			/* String, byte, short, int... */
	long mask;			/* If non-zero, mask value */
	char opcode;			/* Test magic value */
	union {
		long	num;		/* if a numeric type */
		char	*str;		/* if a string type */
	} value;
	char *str;			/* Magic number description */
}
FV_MAGIC;

typedef struct
{
	char *buf;			/* Malloced buffer */
	char *pattern;			/* Filename regular expression */
	FV_MAGIC *magic;		/* Or magic number description */
	char *application;		/* Object's action */
	char *iconfile;			/* Icon filename */
	Pixrect *icon;			/* Icon */
}
FV_BIND;
