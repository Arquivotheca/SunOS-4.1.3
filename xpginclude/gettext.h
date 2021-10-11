/* common between installtxt and gettext() */

/*	@(#)gettext.h 1.1 92/07/30 SMI; */

#define	ARMAG		"!<LC_MESSAGES>\n"
#define ARFMAG  	"`\n"
#define	SARMAG		15
#define INT             0 
#define STR             1

struct ar_hdr {
        unsigned char ar_name[16];
	unsigned char ar_date[12];
        unsigned char ar_uid[6];
        unsigned char ar_gid[6];
        unsigned char ar_mode[8];
        unsigned char ar_size[10];
	unsigned char ar_fill;
	unsigned char ar_sep;
	unsigned char ar_quote;
        unsigned char ar_fmag[2];
};

struct msg_header {
	int maxno;	/* number of message tags in following 
		 	 * domain structure */

	int ptr;	/* relative offset from msg_header to start 
			 * of target strings block */

	short format_type;
	char format[MAXFMTS];	/* Format string */

};

struct index {
	int msgnumber;	/* The value of mesasge tag expressed as an integer
*/
	int rel_addr;	/* The relative offset to the real target string
*/
};
