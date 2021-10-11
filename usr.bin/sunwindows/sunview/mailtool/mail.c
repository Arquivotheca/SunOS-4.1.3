#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)mail.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - Mail subprocess handling
 */

#include <stdio.h>
#include <sunwindow/window_hs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <vfork.h>

#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/text.h>

#include <suntool/scrollbar.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include <suntool/selection_attributes.h>
#include <suntool/walkmenu.h>
#include <suntool/icon.h>

/* performance: global cache of getdtablesize() */
int dtablesize_cache;
#define GETDTABLESIZE() \
        (dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))

#include "glob.h"
#include "tool.h"

char           *strcpy();

static int      mt_mailpid;	/* pid of mail */
static int	mt_talking_to_Mail;
static FILE    *mt_mailin, *mt_mailout;	/* input to mail, output from mail */

int	 	current_size;   /* size of mt_message array allocated */
int             mt_curmsg;	/* number of current message */
int             mt_maxmsg;
int             mt_scandir;	/* scan direction, forward to start */
char           *mt_mailbox;	/* name of user's mailbox [256] */
char           *mt_folder;	/* name of current folder [256] */
char           *mt_info;	/* info from save, file, etc. [256] */
char           *line;		/* tmp line buffer, used everywhere
				 * [LINE_SIZE] */

static char   **folder_list;	/* pointers to folder names [MAXFOLDERS+2] */

static enum mstate {
	S_NOMAIL, S_MAIL
}               mt_mstate;

struct msg     *mt_message;	/* [MAXMSGS] */
struct msg     *mt_delp;	/* pointer to linked list of deleted messages */
int             mt_del_count;	/* number of deleted messages */

static void     mt_set_state(), mt_insert_msg();
static Notify_value mt_Mail_died(), mt_broken_pipe();

/*
 * Dynamically allocate all the large arrays for mailtool
 * so that when we are bound with suntools (using toolmerge)
 * we don't take up a lot of data space that will not be used
 * by the other tools merged into suntools.
 */
void
mt_init_mail_storage()
{
	extern char    *calloc();

	current_size = 0;
	mt_message = (struct msg *)(LINT_CAST(calloc(INCR_SIZE,
		sizeof (struct msg))));
	current_size += INCR_SIZE;
	mt_delp = NULL;
	mt_del_count = 0;
	line = calloc(LINE_SIZE, sizeof (char));
	/*
	 * (Bug# 1010094 fix) Increasing MAXFOLDERS by +2 allows
	 * "av[ac] = av[ac + 1] = 0" to work in mt_folder_menu_build()
	 * w/o trashing the next byte.
	 */
	folder_list = (char **)(LINT_CAST(calloc(MAXFOLDERS+2,
		sizeof (char *))));
	mt_mailbox = calloc(256, sizeof (char));
	mt_folder = calloc(256, sizeof (char));
	mt_info = calloc(256, sizeof (char));
	mt_scandir = 1;
	mt_talking_to_Mail = FALSE;
}

mt_release_mail_storage() {} /* No-op for now */

/*
 * Start the Mail subprocess.
 */
int
mt_start_mail()
{
	int             in[2], out[2];
	int             i;
	FILE           *f;
	static char    *args[6] = {"Mail", "-N", "-B", "-f", 0, 0};

	if(!(f = mt_fopen(mt_dummybox, "w"))) 
		return(FALSE);
	(void)chmod(mt_dummybox, 0600);
	args[4] = mt_dummybox;
	(void)fprintf(f, "From xxx Fri Jan  1 00:00:00 1970\n");
	(void)fclose(f);
	(void)pipe(in);	/* input to mail */
	(void)pipe(out);	/* output from mail */
	if ((mt_mailpid = vfork()) == 0) {
		(void)dup2(in[0], 0);
		(void)dup2(out[1], 1);
		(void)dup2(out[1], 2);
		for (i = GETDTABLESIZE(); i > 2; i--)
			(void)close(i);
		execvp("Mail", args, 0);
		exit(-1);
	}
	(void)close(in[0]);
	(void)close(out[1]);
	mt_mailin = fdopen(in[1], "w");
	mt_mailout = fdopen(out[0], "r");
	(void)strcpy(mt_folder, "[None]");
	(void)notify_set_wait3_func(&mt_mailclient, mt_Mail_died,
		mt_mailpid);
	(void)notify_set_signal_func(&mt_mailclient, mt_broken_pipe,
		SIGPIPE, NOTIFY_ASYNC);
	return(TRUE);
}

/* ARGSUSED */
static Notify_value
mt_Mail_died(client, sig)
	Notify_client   client;
	int             sig;
{

	fprintf(stderr, "Mail process died! Aborting mailtool!\n");
	(void)fclose(mt_mailin);
	(void)fclose(mt_mailout);
	(void)unlink(mt_dummybox);
	mt_done(1);
}

/* ARGSUSED */
static Notify_value
mt_broken_pipe(client, sig)
	Notify_client   client;
	int             sig;
{

	if (!mt_talking_to_Mail)
		return;
	fprintf(stderr, "Mailtool not able to communicate with Mail!\n(Perhaps Mail is not an executable appropriate for this machine.)\n");
	(void)fclose(mt_mailin);
	(void)fclose(mt_mailout);
	(void)unlink(mt_dummybox);
	fprintf(stderr, "Aborting mailtool!\n");
	mt_done(1);
}

int
mt_idle_mail()
{
	FILE           *f;

	if(!(f = mt_fopen(mt_dummybox, "w")))
		return(FALSE);
	(void)chmod(mt_dummybox, 0600);
	(void)fprintf(f, "From xxx Fri Jan  1 00:00:00 1970\n");
	(void)fclose(f);
	(void)mt_set_folder(mt_dummybox);
	return(TRUE);
}

void
mt_stop_mail(doabort)
	int             doabort;
{

	mt_mail_cmd(doabort ? "x" : "quit");
	(void)fclose(mt_mailin);
	(void)fclose(mt_mailout);
	(void)unlink(mt_dummybox);
}

void
mt_tool_is_busy(busy, s)
	int             busy;
	char           *s;
{
	if (busy) {
		mt_waitcursor();
		mt_set_namestripe_temp(s);
	} else {
		mt_restorecursor();
		mt_check_mail_box();
	}
}

/*
 * Flag new mail when it arrives and check to see if the mail has been seen
 * but not necessarily read or deleted. After mail is seen the flag goes
 * down. Returns TRUE if new mail, FALSE if not. 
 */
int
mt_check_mail_box()
{
	struct stat     stat_buf;
	static time_t   mbox_time;	/* time mailbox last changed state */

	/*
	 * Stat the mailbox, if it doesn't exist then we don't have any mail.
	 */
	if (stat(mt_mailbox, &stat_buf) < 0) {
		if (mt_debugging)
			printf("checking mailbox: stat = %d\nNo Mail\n\n",
				stat);
		mt_set_state(S_NOMAIL);
		return (FALSE);
	}

	if (mt_debugging)
		printf("checking mailbox:\nsize = %d,\nlast accessed = %d,\nlast modified = %d,\nlast checked = %d,\n",
			stat_buf.st_size, stat_buf.st_atime,
			stat_buf.st_mtime, mbox_time);
	/*
	 * If the mailbox has been accessed (read) but not modified (written)
	 * since the last time we have checked, or is empty, then there is
	 * no new mail.  Update mbox_time to the mailbox last accessed time.
	 */
	if ((stat_buf.st_atime > stat_buf.st_mtime &&
	    stat_buf.st_atime > mbox_time) || stat_buf.st_size == 0) {
		if (mt_debugging)
			printf("No Mail\n\n");
		mbox_time = stat_buf.st_atime;
		mt_set_state(S_NOMAIL);
	}

	/*
	 * If the mailbox size is non-zero (it has some mail in it) and
	 * has been modified (written) since the last time we checked then
	 * there is new mail.
	 * Update mbox_time to the mailbox last modified time.
	 */
	else if (stat_buf.st_mtime > mbox_time) {
		if (mt_debugging)
			printf("New Mail\n\n");
		mbox_time = stat_buf.st_mtime;
		mt_set_state(S_MAIL);
		mt_announce_mail();
	} else if (mt_debugging) 
		printf("No change\n\n");

	/*
	 * If none of the above, nothing has changed.
	 */
	return (mt_mstate == S_MAIL);
}

/*
 * Set the icon and window label to reflect the
 * internal state (mail or no mail).
 */
static void
mt_set_state(s)
	enum mstate     s;
{
	char           *new;
	Pixrect        *pr;

	pr = NULL;
	mt_mstate = s;
	switch (mt_mstate) {
	case S_NOMAIL:
		mt_icon_ptr = &mt_nomail_icon;
		new = "";
		pr = mt_nomail_pr;
		break;
	case S_MAIL:
		mt_icon_ptr = &mt_mail_icon;
		new = "[New Mail]";
		pr = mt_newmail_pr;
		break;
	}
	mt_set_icon(mt_icon_ptr);
	mt_set_namestripe_left(mt_folder, new, TRUE);
	if (mt_panel_style == mt_3DImages)
		panel_set(mt_state_item, PANEL_LABEL_IMAGE, pr,	0);
}

/*
 * Get the headers for all messages from the Mail subprocess.
 */
int
mt_get_headers(start)
     int             start;
{
  extern char     *realloc();
  int             n = 0, first = TRUE;
  register struct msg *mp;

  mt_mail_start_cmd("from %d-$", start);
  
  while (MT_MAIL_GET_LINE) 
    {
      if (first) 	/* check for error msg from /bin/mail */
	{
	  extern char   *index();
	  char		*p;
	  
	  p = index(line, ':');
	  if (p != NULL && (strcmp(p, ": Invalid message number\n") == 0)) 
	    {
	      while (MT_MAIL_GET_LINE) {};	/* ignore the rest */
	      return(FALSE);
	    }
	  first = FALSE;
	}

      n = atoi(&line[2]);
      if (n >= current_size)	/* if too many lines read, get more space */
	{
	  if((current_size+INCR_SIZE) <= MAXMSGS) 
	    { 
	      struct msg *old_address;
	      old_address = mt_message; /* save old address to calc offset*/

	      /* allocate more storage by moving array if needed */
	      if((mt_message=(struct msg *)realloc(
			     (struct msg *)mt_message, 
			     (unsigned)(sizeof(struct msg)*(current_size+
			     INCR_SIZE)))) == NULL)
		{
		  fprintf(stderr,"Realloc failed to increment storage\n");
		  exit(1);
		}
	      else
		{ 
		  int address_offset;

		  /*
		   * In this case, the msg list has possibly been moved
		   * to another address.  This means the reverse list
		   * of deleted msgs, maintained with the m_next field,
		   * will all point to the wrong addresses.  By calculating
		   * the address offset and incrementing the m_next field,
		   * this will be corrected.
		   */
		  current_size = current_size + INCR_SIZE;

		  /* 
		   * realloc() does not guarentee "clean" memory, so
		   * zero out the additional memory after the header msgs.
		   */
		  bzero((char *)&mt_message[current_size - INCR_SIZE], sizeof(struct msg)*INCR_SIZE);
		  /*
		   * the compiler scales the result according to the size of
		   * the objects pointed to.  If not cast as chars (bytes),
		   * it would return the size in terms of struct msg.
		   */
		  address_offset = (char *)mt_message - (char *)old_address;
		  if ((mt_delp != NULL) &&
		      (address_offset))      /* if array was actually moved */
		  {
		    struct msg *mt_curr;

		    /* set to last deleted msg in moved array */
		    (char *)mt_delp += address_offset; 

		    /* transverse delete list and fix the addresses */
		    for (mt_curr = mt_delp; mt_curr->m_next != NULL;
		         mt_curr = mt_curr->m_next)
		     
		      (char *)mt_curr->m_next += address_offset; 
		  }
		}
	    }
	  else	/* not enough space for messages */
	    {
	      mt_warn(mt_frame,
		      "Maximum number of messages exceeded.",
		      "Split this folder.", 
		      0);
	      n--;
	      while (MT_MAIL_GET_LINE) {};
	      break;
	    }
	}
      /*
       * if this message is the 'current' message according to
       * Mail, remember it 
       */
      if (line[0] == '>') 
	{
	  mt_curmsg = n;
	  line[0] = ' ';
	}
      mp = &mt_message[n];
      if (mp->m_header)
	free(mp->m_header);
      mp->m_header = mt_savestr(line);
      mp->m_next = NULL;
      mp->m_deleted = FALSE;
    }
  mt_maxmsg = n;
  mt_message[mt_maxmsg+1].m_deleted = TRUE;
  return(TRUE);
}

char		*
mt_get_header(msg)
	int		msg;
{

	mt_mail_cmd("from %d", msg);
	return (mt_info);
}

/*
 * Get the name of the current folder from Mail.
 */
void
mt_get_folder()
{
	char           *p;
	extern char    *index();

	mt_mail_cmd("folder");
	if (mt_info[0] == '"' && (p = index(&mt_info[1], '"'))) {
		*p = '\0';
		(void)strcpy(mt_folder, &mt_info[1]);
	}
}

/*
 * Get the names of all the user's folders.
 */
char **
mt_get_folder_list(path_string, fdir_name, strtab, acp)
	char           *path_string, *fdir_name, *strtab;
	int            *acp;
{
	char           *s, *strtab_max;
	char          **av;
	int             ac, n = 0;

	/* reserve the first slot for use by the folder menu code */
	av = &folder_list[1];
	ac = 0;

	strtab_max = strtab + 15*MAXFOLDERS - 1; 
	*strtab = '\0';
	if (*fdir_name != '\0' && mt_value("foldermenupullrights"))
						/*
						 * present subdirectories as
						 * pullright menu items.
						 * Note: executing this
						 * command is somewhat slower
						 * than simply executing
						 * "folder". This is left in
						 * for people who run new
						 * mailtool without new mail 
						 */
		mt_mail_start_cmd("!ls -F %s", fdir_name);
	else
		mt_mail_start_cmd("folders %s",
			((*path_string == '+') ? path_string+1 : path_string));
		/*
		 * Newer version of mail lets you specify an argument to
		 * folders command indicating a subdirectory in folders
		 * hierarchy. Much more efficient than performing '!ls -F'
		 */
	while (MT_MAIL_GET_LINE) {
		n++;
		if (n > MAXFOLDERS) 
		{
	  	  while (MT_MAIL_GET_LINE)
		   	n++;
		  sprintf(line,
		        "%d folders exceeds maximum number (%d) allowed.",
		        n, MAXFOLDERS);
		  mt_warn(mt_frame,
			line,
			"Additional folders will be left off of folder menu.",
			"Suggest you combine several folders, or move some",
			"folders to other folder subdirectories.", 
			0);
		  break;
		}
		if (strcmp(line, "!\n") == 0)
			continue;
		*av++ = strtab;
		ac++;
		if (*path_string == '\0' && strtab < strtab_max) 
			*strtab++ = '+';	/* only put + when at top
						 * level, i.e. in folders
						 * directory. This is
						 * backwards compatible
						 * behavior. */
		for (s = line; *s != '\n' && strtab < strtab_max;)
			*strtab++ = *s++;
		if (*(strtab - 1) == '@') 
		{
			char	buf[128];
			struct stat     stbuf;

			(void) strcpy(buf, fdir_name);
			line[strlen(line) - 2] = '\0';
			/* remove the @ and newline */
			(void)strcat((char *)buf,(char *)line);
			if (stat(buf, &stbuf) != -1	&&
			    ((stbuf.st_mode & S_IFMT) == S_IFDIR)
			   ) 
			{
				*(strtab - 1) = '/';
				*strtab++ = '@';
			}
    		}
		if (strtab < strtab_max)
			*strtab++ = '\0';
		else	
		{
		  sprintf(line,
			"Maximum number of characters (%d) in folder names exceeded.",
			15*MAXFOLDERS - 1);
		  mt_warn(mt_frame,
			line,
			"Suggest you delete or give some folders shorter names,",
			"or move some folders to other folder subdirectories.", 
			0);
			/*
			 * Note that this warning will only go up for top
			 * level folder. However, chances are that if
			 * somebody is using the pullrights, rather than a
			 * flat structure, then they probably have smaller
			 * subdirectories. 
			 */
			/*
			 * If this really turns out to happen often, the
			 * right solution is to change the code to reallocate
			 * the table. This would require passing in to
			 * mt_get_folder_list the address of strtab, along
			 * with its current size, so that the new strtab
			 * could be saved. 
			 */
			*strtab_max = '\0';
		  while (MT_MAIL_GET_LINE) {};
		  break;
		}
	}
	*av = NULL;
	*acp = ac;
	return (&folder_list[1]);
}

/*
 * Get the variables (and their values) that Mail knows about.
 */
void
mt_get_vars(forceset)
int forceset;
{
	char	*p, *s, *tmp_line;
	char 	mailrc_str[256];          
        FILE 	*mailrc_ptr;
  
	/*                 
         * Read the user's .mailrc file if possible 
         */   
        /* mailrc_str = (char *)calloc(256,sizeof(char));         */
         
        if (getenv("MAILRC")) 
                strcpy(mailrc_str,getenv("MAILRC")); 
        else                 
                sprintf(mailrc_str, "%s/.mailrc", getenv("HOME")); 
                 
        if ( (forceset != 0) ||
	     ((mailrc_ptr = fopen(mailrc_str,"r")) == NULL)
	   )
	{  	/* 
		 * .mailrc not found, so get the mail variables by 
		 * sending a set command to the mail  program
		 */
	     	mt_mail_start_cmd("set");
/*		while (mt_mail_get_line()) */
		while (MT_MAIL_GET_LINE) 
		{
			s = &line[strlen(line) - 1];
			*s-- = '\0';
			if ((p = index(line, '=')) != NULL) 
			{
				*p++ = '\0';
				if (*p == '"' && *s == '"') 
				{
					p++;
					*s = '\0';
				}
				mt_assign(line, p);
			} else
				mt_assign(line, "");
		}
	}
	else
	{ 	/* 
		 * set the mail variables by directly reading .mailrc
		 */
		while (fgets(line, LINE_SIZE, mailrc_ptr) != NULL) 
		{
			if (*line == '\n'  || *line == '\t' || *line == ' ' )
				continue;

	 		s = &line[strlen(line) - 1];
			*s-- = '\0';
	
			/* skip over 'set' command in .mailrc file */
			tmp_line = line;
			if ((line = index(line, ' ')) != NULL)
				*line++;
			else
				line = tmp_line;

			if ((p = index(line, '=')) != NULL) 
			{
				*p++ = '\0';
				if ((*p == '"'  && *s == '"') || 
				    (*p == '\'' && *s == '\'' )) 
				{
					p++;
					*s = '\0';
				}
				mt_assign(line, p);
			} else
				mt_assign(line, "");
		}

	}
}

/*
 * Set (or unset) a Mail variable.
 */
void
mt_set_var(var, val)
	char           *var, *val;
{

	if (val == 0)
		mt_mail_cmd("unset %s", var);
	else if (strlen(val) == 0)
		mt_mail_cmd("set %s", var);
	else
		mt_mail_cmd("set %s=\"%s\"", var, val);
}

/*
 * Get Mail's working directory.
 */
void
mt_get_mailwd(dir)
	char           *dir;
{
	mt_mail_cmd("!pwd");
	mt_info[strlen(mt_info) - 1] = '\0';
	(void)strcpy(dir, mt_info);
}

/*
 * Set Mail's working directory.
 */
void
mt_set_mailwd(dir)
	char           *dir;
{
	mt_mail_cmd("cd %s", dir);
}

/*
 * Reply to the specified message. Put reply in "file". If "all", reply to
 * all recipients. If "orig", include original message. Called from
 * mt_reply_proc 
 */
int
mt_reply_msg(msg, file, all, orig)
	int             msg;
	char           *file;
	int             all;
	int             orig;
{
	FILE           *replyf;

	if(!(replyf = mt_fopen(file, "w")))
		return(FALSE);
	(void)chmod(file, 0600);
	(void)mt_insert_msg(replyf, msg, all, 0, TRUE);

	if (mt_value("askcc"))
		(void) fprintf (replyf, "Cc: %s\n", 
		mt_use_fields ? "|>other recipients<|" : "");

        if (mt_value("askbcc"))
                (void) fprintf (replyf, "Bcc: %s\n", 
                mt_use_fields ? "|>blind carbon copies<|" : "");

        (void)fprintf(replyf, "\n");
        if (orig)
                (void)mt_insert_msg(replyf, 0, FALSE, msg, TRUE);
        if (mt_use_fields)
                (void)fprintf(replyf, "|>body of message<|\n");
	(void)fclose(replyf);
	return(TRUE);
}

/*
 * Compose a message.
 * Put in Cc: blank fields if "askcc" set.
 * Include original message if "orig" set.
 * Called from mt_comp_proc
 */
int
mt_compose_msg(msg, file, askcc, orig)
	int             msg;
	char           *file;
	int             askcc, orig;
{
	FILE           *replyf;

	if(!(replyf = mt_fopen(file, "w")))
		return(FALSE);
	(void)chmod(file, 0600);
	(void)fprintf(replyf, "To: %s\n",
		(mt_use_fields ? "|>recipients<|" : ""));
	if (mt_value("asksub"))
		(void)fprintf(replyf, "Subject: %s\n",
		(mt_use_fields ? "|>subject<|" : ""));
	if (askcc)
		(void)fprintf(replyf, "Cc: %s\n",
		(mt_use_fields ? "|>other recipients<|" : ""));
        if (mt_value("askbcc"))
                (void) fprintf (replyf, "Bcc: %s\n",  
                mt_use_fields ? "|>blind carbon copies<|" : "");
        (void)fprintf(replyf, "\n");
        if (mt_use_fields)
                (void)fprintf(replyf, "|>body of message<|\n");
        if (orig) {
                (void)fprintf(replyf,
                        "\n----- Begin %s Message -----\n\n",
                        (mt_3x_compatibility ? "Forwarded" : "Included"));
                (void)mt_insert_msg(replyf, 0, FALSE, msg, FALSE);
                (void)fprintf(replyf,
                        "\n----- End %s Message -----\n\n",
                        (mt_3x_compatibility ? "Forwarded" : "Included"));
        }
	(void)fclose(replyf);
	return(TRUE);
}

/*
 * Includes specified message.
 * Message is indented if "indent" is set.
 * Called from mt_include_proc. 
 */
int
mt_include_msg(msg, file, indented)
	int             msg;
	char           *file;
	int             indented;
{
	FILE           *replyf;

	if(!(replyf = mt_fopen(file, "w")))
		return(FALSE);
	(void)chmod(file, 0600);
        if (!indented)
                (void)fprintf(replyf,
                        "\n----- Begin %s Message -----\n\n", 
                        (mt_3x_compatibility ? "Forwarded" : "Included"));
	(void)mt_insert_msg(replyf, 0, FALSE, msg, indented);
        if (!indented) 
                (void)fprintf(replyf,
                        "\n----- End %s Message -----\n\n",  
                        (mt_3x_compatibility ? "Forwarded" : "Included"));
	(void)fclose(replyf);
	return(TRUE);
}

static void
mt_insert_msg(file, replyto, replyall, include, indented)
	FILE           *file;
	int             replyto, replyall, include, indented;

{
	char           *escp;
	int             newline;

	if (!(escp = mt_value("escape")))
		escp = "~";
	if (replyto != 0)  
                /* sends command Reply msg# or reply msg# to Mail */
		(void) fprintf(mt_mailin, "%s %d\n",
			(replyall ? "Reply" : "reply"), replyto);
	else
		(void) fprintf(mt_mailin, "m\n");

	if (include) {
                /*
                 * sends command "~m" to Mail. This inserts message, indented
                 * by one tab, or whatever is the user's indentprefix.
                 */
		(void) fprintf(mt_mailin, "%c%c %d\n\n",
			*escp, (indented ? 'm' : 'f'), include);
	}

	(void)fprintf(mt_mailin, "%cp\n%cq\n", *escp, *escp);
	/*
	 * sends command ~p to Mail, which prints the contents of the message
	 * buffer, thereby getting To, Subject, cc fields that Mail inserted,
	 * plus the included message, if any. Then sends command ~q, which
	 * cause message composition operation to be aborted. 
	 */
		(void)fflush(mt_mailin);
	while (fgets(line, LINE_SIZE,  mt_mailout))
		if (strcmp(line, "Message contains:\n") == 0)
			break;
	newline = 0;
	while (fgets(line, LINE_SIZE, mt_mailout)) {
		if (strcmp(line, "(continue)\n") == 0)
			break;
			/* copy lines from Mail into reply subwindow */
		else if (strcmp(line, "\n") == 0) 
			++ newline;	/* delay putting out blank lines
					 * until see some non blank lines */
		else {
			while (newline > 0) {
				fputs("\n", file);
				newline--;
			}
			fputs(line, file); 
		}
	}
	if (include)
		fputs("\n", file); 
	while (fgets(line, LINE_SIZE, mt_mailout))
		if (strcmp(line, "Interrupt\n") == 0)
			break;
}

/*
 * Reload the specified message from the specified file.
 */
void
mt_load_msg(msg, file)
	int             msg;
	char           *file;
{

	mt_mail_cmd("load %d %s", msg, file);
}

/*
 * Save a message in a file or folder.
 * Return FALSE on failure, TRUE on success.
 */
int
mt_copy_msg(msg, file)
	int             msg;
	char           *file;
{
	register char  *p;
	extern char    *index();

	mt_mail_cmd("copy %d %s", msg, file);

	/* look for '"file" [Appended]'  or '"file" [New file]' */
	if ((p = index(mt_info, '[')) && ((strncmp(++p, "Appended", 8) == 0) ||
					  (strncmp(p, "New file", 8) == 0)))
		return (TRUE);
	else
		return (FALSE);
}

/*
 * Save messages in a file or folder.
 * Return FALSE on failure, TRUE on success.
 */
int
mt_copy_msgs(from, to, file)
	int             from, to;
	char           *file;
{
	register char  *p;
	extern char    *index();

	mt_mail_cmd("copy %d-%d %s", from, to, file);

	/* look for '"file" [Appended]'  or '"file" [New file]' */
	if ((p = index(mt_info, '[')) && ((strncmp(++p, "Appended", 8) == 0) ||
					  (strncmp(p, "New file", 8) == 0)))
		return (TRUE);
	else
		return (FALSE);
}

/*
 * Print a message.
 */
void
mt_print_msg(msg, file, ign)
	int             msg;
	char           *file;
	int             ign;
{

	if (ign) {
		/*
		 * If "alwaysignore" isn't set, turn it on and off
		 * around the "copy" command so it will have the
		 * same effect as the "print" command.
		 */
		if (!mt_value("alwaysignore"))
			mt_mail_cmd(
			    "set alwaysignore\ncopy %d %s\nunset alwaysignore",
			    msg, file);
		else
			mt_mail_cmd("copy %d %s", msg, file);
	} else {
		/*
		 * If "alwaysignore" is set, turn it off and on
		 * around the "copy" command so it will have the
		 * same effect as the "Print" command.
		 */
		if (mt_value("alwaysignore"))
			mt_mail_cmd(
			    "unset alwaysignore\ncopy %d %s\nset alwaysignore",
			    msg, file);
		else
			mt_mail_cmd("copy %d %s", msg, file);
	}
	(void)chmod(file, 0600);
}

/*
 * Preserve the specified message.
 */
void
mt_preserve_msg(msg)
	int             msg;
{

	mt_mail_cmd("preserve %d", msg);
}

/*
 * Delete the specified message.
 */
void
mt_del_msg(msg)
	int             msg;
{
	int             i, len;

	(void)unlink(mt_msgfile);
	mt_mail_cmd("delete %d", msg);
	mt_message[msg].m_deleted = TRUE;
	len = mt_message[msg+1].m_start - mt_message[msg].m_start;
	for (i = msg + 1; i <= mt_maxmsg + 1; i++) {
		mt_message[i].m_start -= len;
		mt_message[i].m_lineno--;
	}
	mt_del_count++;
}

/*
 * Undelete the specified message.
 */
void
mt_undel_msg(msg)
	int             msg;
{
	int             i, len;

	mt_mail_cmd("undelete %d", msg);
	mt_message[msg].m_deleted = FALSE;
	len = strlen(mt_message[msg].m_header);
	for (i = msg + 1; i <= mt_maxmsg + 1; i++) {
		mt_message[i].m_start += len;
		mt_message[i].m_lineno++;

	}
	mt_del_count--;
}

/*
 * Switch Mail to the specified folder.
 * Return number of messages in folder, -1 on failure.
 */
int
mt_set_folder(file)
	char           *file;
{
	register char  *p;
	int             n = -1;

	mt_mail_start_cmd("file %s", file);
/*	while (mt_mail_get_line()) {*/
	while (MT_MAIL_GET_LINE) {
		/* look for '"file" complete' */
		if ((p = index(line, '\0') - strlen("complete\n")) >= line &&
		    strcmp(p, "complete\n") == 0)
			continue;
		/* look for '"file" removed' */
		if ((p = index(line, '\0') - strlen("removed\n")) >= line &&
		    strcmp(p, "removed\n") == 0)
			continue;
		/* look for '"file": n messages' */
		if (line[0] == '"'
			&& (p = index(&line[1], '"'))
			&& *++p == ':') 
			n = atoi(++p);
		(void)strcpy(mt_info, line);
	}
	return (n);
}

/* incorporates mail into system mail box */
int
mt_incorporate_mail()
{
	mt_mail_cmd("inc");
	if (strcmp(mt_info, "Unknown command: \"inc\"\n") == 0) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Find the next message after msg.  If none after,
 * use specified message if not deleted.  Otherwise,
 * find first one before msg.  If none, return 0.
 */
int
mt_next_msg(msg)
	int             msg;
{
	register int    i;

	for (i = msg + mt_scandir; i <= mt_maxmsg && i > 0; i += mt_scandir)
		if (!mt_message[i].m_deleted)
			return (i);
	if (!mt_message[msg].m_deleted && msg != mt_curmsg)
		return (msg);
	for (i = msg - mt_scandir; i <= mt_maxmsg && i > 0; i -= mt_scandir)
		if (!mt_message[i].m_deleted) {
			if (mt_value("allowreversescan"))
				mt_scandir = -mt_scandir;
			return (i);
		}
	return (0);
}

/*
 * Delete a folder.
 */
void
mt_del_folder(file)
	char           *file;
{

	mt_mail_cmd("echo %s", file);
	mt_info[strlen(mt_info) - 1] = '\0';
	(void)unlink(mt_info);
}

/*
 * Do a simple Mail command.
 */
/* VARARGS1 */
void
mt_mail_cmd(cmd, a1, a2, a3)
	char           *cmd, *a1, *a2, *a3;
{
	register int    first = TRUE;

	mt_talking_to_Mail = TRUE;	/* so mt_broken_pipe can distinguish
					 * SIGPIPEs that come from attempts
					 * to talk to Mail from spurious
					 * SIGPIPEs */
	(void)fprintf(mt_mailin, cmd, a1, a2, a3);
	(void)fprintf(mt_mailin, "\necho \004\n");
	(void)fflush(mt_mailin);
/***	produces copious output
	if (mt_debugging) {
		(void)printf("sent:\n");
		(void)printf(cmd, a1, a2, a3);
		(void)printf("\necho \\004\n");
		(void)printf("received:\n");
	}
 ***/
 	while (fgets(line, LINE_SIZE, mt_mailout)) {
		if (strcmp(line, "\004\n") == 0) {
/***
			if (mt_debugging) 
				(void)printf("\\004\n");
 ***/
			break;
		} else if (first) {
			(void)strcpy(mt_info, line);
			first = FALSE;
		}
/***
		if (mt_debugging) 
			(void)printf(line);
 ***/
	}
	mt_talking_to_Mail = FALSE;
}

/*
 * Start a long Mail command.
 */
/* VARARGS1 */
void
mt_mail_start_cmd(cmd,a1,a2,a3)
	char           *cmd, *a1, *a2, *a3;
{

	mt_talking_to_Mail = TRUE;	/* so mt_broken_pipe can distinguish
					 * SIGPIPEs that come from attempts
					 * to talk to Mail from spurious
					 * SIGPIPEs */
	(void)fprintf(mt_mailin, cmd, a1, a2, a3);
	(void)fprintf(mt_mailin, "\necho \004\n");
/***	produces copious output
	if (mt_debugging) {
		(void)printf("sent:\n");
		(void)printf(cmd, a1, a2, a3);
		(void)printf("\necho \\004\n");
	}
 ***/
	(void)fflush(mt_mailin);
/***
	if (mt_debugging)
		(void)printf("received:\n");
 ***/
}

/*
 * Read one line of the response from a long
 * Mail command, checking for end of output.
 */
/*
char *
mt_mail_get_line()
{

	if (fgets(line, LINE_SIZE, mt_mailout) == NULL) {
		mt_talking_to_Mail = FALSE;
		return (NULL);
	}
	if (strcmp(line, "\004\n") == 0) {
		mt_talking_to_Mail = FALSE;
		return (NULL);
	}
	return (line);
}
*/


