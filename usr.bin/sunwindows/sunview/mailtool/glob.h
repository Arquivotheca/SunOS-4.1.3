/*      @(#)glob.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - global constants
 */

#define INCR_SIZE       200     /* storage is allocated incrementally in */
                                /* increments of 200                     */
#define	MAXMSGS		2000	/* maximum number of messages supported */
#define	MAXFOLDERS	240	/* maximum number of folders supported.
				 * Greater than this number would cause the
				 * maximum number of attributes (250) to be
				 * exceeded, and the tool to exit. This
				 * number can be increased if and when menu
				 * package lets you specify count of
				 * attributes */
#define	DEFMAXFILES	10	/* default max # of file names in file menu */
#define	LINE_SIZE	1024

/*
 * Mailtool - global variables
 */

extern int      mt_memory_maximum;	/* for text subwindows */
extern int      mt_aborting;	/* aborting, don't save messages */
extern char    *mt_cmdname;	/* our name */
extern int      mt_mailclient;	/* client handle */

extern int      mt_curmsg;	/* number of current message */
extern int      mt_prevmsg;	/* number of previous message */
extern int      mt_maxmsg;	/* highest numbered message */
extern int	mt_undelmsgs;	/* number of undeleted messages */
extern int      mt_scandir;	/* scan direction */
extern char    *mt_mailbox;	/* name of user's mailbox */
extern char    *mt_folder;	/* name of current folder */
extern char    *mt_info;	/* info from save, file, etc. */
extern char    *line;		/* temporary buffer used everywhere */

/* extern int      init_flag; */
/* extern char    *mt_glob_mailrc; */     /* global mailrc file  */
/* FILE           *mt_glob_mailrc_ptr; */  /* global mailrc pointer */

struct msg {
	int             m_start;	/* start char of header in headersw */
	int             m_lineno;	/* line number (1 based) in headersw,
					 * i.e., message number - number of
					 * deleted messages that precede it
					 * in mail file */
	char           *m_header;	/* text from header line */
	struct msg     *m_next;		/* linked list of deleted messages */
	int             m_deleted:1;	/* message has been deleted */
};

extern struct msg *mt_message;	/* all the messages */
extern struct msg *mt_delp;	/* pointer to linked list of deleted messages */
extern int      mt_del_count;	/* number of deleted messages */
extern int      current_size;   /* current size of mt_message (header array) */

extern char     mt_hdrfile[];
extern char     mt_msgfile[];
extern char     mt_printfile[];
extern char     mt_scratchfile[];
extern char     mt_dummybox[];
extern char     mt_clipboard[];

char           *getenv();
char           *index();
char           *mt_savestr();
char           *mt_value(), *mt_get_header();
char          **mt_get_folder_list();
u_long          mt_current_time();


void            mt_init_mail_storage(), mt_stop_mail();
int             mt_check_mail_box(), mt_copy_msg(), mt_copy_msgs(), mt_next_msg(), mt_set_folder(), mt_incorporate_mail();
void            mt_get_folder(), mt_get_vars(), mt_set_var();
void            mt_get_mailwd(), mt_set_mailwd();
int		mt_get_headers(), mt_start_mail(), mt_idle_mail(), mt_reply_msg(), mt_compose_msg(), mt_include_msg();
void		mt_load_msg();
void            mt_print_msg(), mt_preserve_msg(), mt_del_msg(), mt_undel_msg();
void            mt_del_folder(), mt_mail_cmd(), mt_mail_start_cmd(), mt_done();
void            mt_assign();
int             mt_deassign(), mt_mail_seln_exists(), mt_get_curselmsg();
int             mt_is_prefix(), mt_full_path_name();
/*char		*mt_mail_get_line();*/

#define MT_MAIL_GET_LINE (fgets(line, LINE_SIZE, mt_mailout)==NULL || strcmp(line, "\004\n") == 0)? (char *)(mt_talking_to_Mail = FALSE, NULL) : line
