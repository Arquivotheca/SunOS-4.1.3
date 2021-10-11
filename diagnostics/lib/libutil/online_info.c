static char sccsid[] = "@(#)online_info.c	1.1  7/30/92  Copyright Sun Mircosystems Inc.";
/**************************************************************************
 This file, online_info.c, contains the following functions:
        search             - search a pattern from a file.
        sort_file          - sort a file by alphabetical order. 
        get_date           - get a date stamp. 
        mail               - send an email. 
        print              - print a file. 
        remove_dir_or_file - remove a directory or a file. 
        remove_line        - delete a line. 
        remove_lines       - remove line/lines. 
        copy_file          - copy file or directory. 
        move_file          - move an old file to a new file. 
	spawn_sh	   - spawn off a shell for execution. 
        get_line           - get a line.
**************************************************************************/
	
#ifdef	 SVR4
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORD                200
#define MAX_LENGTH              1024
#define MAX_FILE_NAME_LEN       512

/****************************************************************************
 NAME: search -  search a pattern from a file.
 SYNOPSIS:  int search(search_key, file_name)
 ARGUMENTS:
        Input:  char *search_key, *file_name;
 DESCRIPTION:  This routine spawns a child to execute an 'grep" command for
               the searching purpose, and waits for the
               child to finish, exiting if any errors occur.
 RETURN VALUE: status.w_retcode, 0 indicates search pattern is found.
	       status.w_retcode, 1 indicates search pattern is not found.
               status.w_retcode, 2 indicates other errors occurs.
 GLOBAL: sprintf(), spawn_sh(). 
*****************************************************************************/ 

int search(search_key, file_name)
char *search_key, *file_name;
{
    char cmd[MAX_WORD];

    sprintf(cmd, "grep -si %s %s", "grep", "-si", search_key,
                    file_name); 
    return (spawn_sh(cmd));
} /* end search */


/****************************************************************************
 NAME: sort_file - sort a file by alphabetical order.
 SYNOPSIS: int sort_file(file_name, new_file)
 ARGUMENTS:
        Input: char *file_name, *new_file;
 DESCRIPTION:  This routine spawns a child to execute an
               "sort" command to sort a file in alphabetical order,
               and waits for the child to finish, exiting if any
               errors occur.
 RETURN VALUE: status.w_retcode 0, indicates update file success.
	       status.w_retcode 1, indicates update file fail.
 GLOBAL: sprintf(), spawn_sh().
*****************************************************************************/

int sort_file(file_name, new_file)
char *file_name, *new_file;
{
    char cmd[MAX_WORD];

    sprintf(cmd, "sort %s %s", file_name, new_file);
    return (spawn_sh(cmd));
}

/****************************************************************************
 NAME:  get_date - get a date stamp.
 SYNOPSIS:  int get_date(file_name)
 ARGUMENTS:
        Input:  char *file_name;
 DESCRIPTION: 	This routine spawns a child to execute an
                "date" command to get date, and waits for the child
                to finish, exiting if any errors occur.
 RETURN VALUE:  0 indicates get date success .
                1 indicates get date fail.
 GLOBAL:  sprintf(), spawn_sh().
*****************************************************************************/

int get_date(file_name)
char *file_name;
{
    char cmd[MAX_LENGTH];

    sprintf(cmd, "date >%s", file_name);
    return (spawn_sh(cmd));
}


/****************************************************************************
 NAME: mail - send an email.
 SYNOPSIS:  int mail(subject, admin_alias, file_name)
 ARGUMENTS:
        Input: 	char *subject;          
		char *admin_alias;     
		char *file_name;      
 DESCRIPTION:	This routine spawns a child to execute an
                "lpr" command for the mailing purpose, and waits
                for the child to finish, exiting if any errors occur.

 RETURN VALUE:	0 indicates mail success .
                1 indicates mail fail.
 GLOBAL:  fork(), execl(), exit(), wait(), fprintf(), sprintf()
*****************************************************************************/
int mail(subject, admin_alias, file_name)
char *subject;		/* SUBJECT header field */
char *admin_alias;	/* pointer to string of recipients */ 
char *file_name;	/* mail source file */
{
    int pid;
#   ifdef SVR4
    int status;
#   else
    union wait status;
#   endif

    char cmd[MAX_LENGTH];

    switch (pid = fork()) {
        case -1:
	    fprintf(stderr, "fork failed\n"); 
	    exit(1);
	case 0:
            sprintf(cmd,"/usr/ucb/mail -s \'Suninfo: %s\' %s <\'%s\'"
		,subject, admin_alias, file_name);
	    system(cmd);
	    exit(0);
	default:
#	    ifdef SVR4
	    (void) wait(&status);
	    return(WEXITSTATUS(status)) ;
#	    else
	    (void) wait(&status);
            return(status.w_retcode);
#	    endif
     }
}


/****************************************************************************
 NAME: print - print a file.
 SYNOPSIS:  int print(file_name, printer_name)
 ARGUMENTS:
        Input: char *file_name, *printer_name;
 DESCRIPTION: 	This routine spawns a child to execute an
              	"lpr" command for the printing purpose, and waits
              	for the child to finish, exiting if any errors occur.
 RETURN VALUE: 	status.w_retcode, 0 indicates print success.
		status.w_retcode, 1 indicates print fail.
 GLOBAL:  fork(), execl(), exit(), wait(), fprintf(), sprintf()
*****************************************************************************/

int print(file_name, printer_name)
char *file_name, *printer_name;
{
    int pid;
#   ifdef SVR4
    int status;
#   else
    union wait status;
#   endif

    char cmd[MAX_LENGTH];

    switch (pid = fork()) {
        case -1:
            fprintf(stderr, "fork failed\n");
            exit(1);
        case 0:
            sprintf(cmd,"-%s",printer_name);
            execl("/usr/ucb/lpr", "lpr", cmd, file_name, 0);
            fprintf(stderr, "execl failed\n");
            exit(2);
        default:
#	    ifdef SVR4
	    (void) wait(&status);
	    return(WEXITSTATUS(status)) ;
#	    else
	    (void) wait(&status);
            return(status.w_retcode);
#	    endif
     }
}

/****************************************************************************
 NAME:  remove_dir_or_file - remove a directory or a file.
 SYNOPSIS: int remove_dir_or_file(file_or_dir_name)
 ARGUMENTS: 
        Input: char *file_or_dir_name;
 DESCRIPTION:  This routine spawns a child to execute
               an "rm" command for the remove directory purpose,
               and waits for the child to finish, exiting if any
               errors occur.
 RETURN VALUE: status.w_recode, 0 indicates remove success.
	       1 indicates remove fail.
 GLOBAL:  sprintf(), access(), spawn_sh().
*****************************************************************************/
int remove_dir_or_file(file_or_dir_name)
char *file_or_dir_name;
{
    char cmd[MAX_LENGTH];

    if(access(file_or_dir_name, W_OK) !=0){
	return(1);
    }
    sprintf(cmd, "rm -r \'%s\'", file_or_dir_name);
    return (spawn_sh(cmd));
}

/****************************************************************************
 NAME: remove_line - delete a line.
 SYNOPSIS: int remove_line(file_update, key_word, new_file)
 ARGUMENTS:
        Input: char *file_update, *key_word, *new_file;
 DESCRIPTION: 	This routine spawns a child to execute an
                "sed" command for the delete a line, and waits
                for the child to finish, exiting if any errors occur.
 RETURN VALUE:  0 indicates update file success .
		1 indicates update file fail.
 GLOBAL: sprintf(), spawn_sh(). 
*****************************************************************************/

int remove_line(file_update, key_word, new_file)
char *file_update, *key_word, *new_file;
{
    char cmd[MAX_LENGTH];

    sprintf(cmd, "sed '/%s/d' %s %s", key_word, file_update, new_file);
    return (spawn_sh(cmd));
}

/****************************************************************************
 NAME: remove_lines - remove line/lines.
 SYNOPSIS: int remove_lines(from_line, from_file, to_line, to_file)
 ARGUMENTS:
        Input:  char *to_file, *from_file;
		int  from_line, to_line;
 DESCRIPTION:	This routine spawns a child to execute an "sed" command to
                remove lines, and waits for the child to finish,
                exiting if any errors occur.
 RETURN VALUE:  status.w_retcode, 0 indicates remove line/lines success.
		1 indicates remove line/lines fail.
 GLOBAL: sprintf(), spawn_sh(). 
*****************************************************************************/

int remove_lines(from_line, from_file, to_line, to_file)
char *to_file, *from_file;
int from_line, to_line;
{
    char cmd[MAX_LENGTH];

    sprintf(cmd, "sed '%d,%dd' %s%s", from_line, to_line, from_file, to_file);
    return (spawn_sh(cmd));
}

/****************************************************************************
 NAME: copy_file - copy file or directory.
 SYNOPSIS: int copy_file(file_or_directory_name, new_file_or_directory_name)
	   char *file_or_directory_name; 
	   char *new_file_or_directory_name;
 ARGUMENTS:
        Input:  char *file_or_directory_name, *new_file_or_directory_name;
 DESCRIPTION:	This routine spawns a child to execute an
                "cp" command for the copy purpose, and waits for the
                child to finish, exiting if any errors occur.
 RETURN VALUE:  status.w_retcode, 0 indicates copy success.
		1 indicates copy fail.
 GLOBAL:  sprintf(), spawn_sh().
*****************************************************************************/

int copy_file(file_or_directory_name, new_file_or_directory_name)
char *file_or_directory_name, *new_file_or_directory_name;
{
    char cmd[MAX_LENGTH];

    sprintf(cmd, "cp -rp \'%s\' \'%s\'", file_or_directory_name,
			new_file_or_directory_name);
    return (spawn_sh(cmd));
}

/****************************************************************************
 NAME:  move_file - move an old file to a new file.
 SYNOPSIS: int move_file(old_file_name, new_file_name)
	   char *old_file_name, *new_file_name;
 ARGUMENTS:
        Input: 	char *old_file_name;
		char *new_file_name;
 DESCRIPTION:   This routine spawns a child to execute an
                "mv" command to move files, and waits for the child
                to finish, exiting if any errors occur.
 RETURN VALUE:  status.w_retcode 0, indicates move file success.
		status.w_retcode 1, indicates move file fail.
 GLOBAL: sprintf(), spawn_sh(). 
*****************************************************************************/

int move_file(old_file_name, new_file_name)
char *old_file_name, *new_file_name;
{
    char cmd[MAX_LENGTH];

    sprintf(cmd, "mv %s %s", "mv", old_file_name, new_file_name);
    return (spawn_sh(cmd));
}


/****************************************************************************
 NAME:  spawn_sh - spawn off a shell for execution.
 SYNOPSIS: int spawn_sh(cmd_string)
           char *cmd_string;
 ARGUMENTS:
        Input:  char *cmd_string;
 DESCRIPTION:   This routine spawns a child to execute an
                "/bin/sh" program for running cmd_string. 
 RETURN VALUE:  status.w_retcode 0, indicates move file success.
                status.w_retcode 1, indicates move file fail.
 GLOBAL:  sprintf(), fprintf(), exit(), execl(), fork(), wait().
*****************************************************************************/

int spawn_sh(cmd_string)
char *cmd_string;
{
    int pid;
#   ifdef SVR4
    int status;
#   else
    union wait status;
#   endif

    char cmd[MAX_LENGTH];

    switch (pid = fork()) {
        case -1:
            fprintf(stderr, "fork failed\n");
            exit(1);
        case 0:
            execl("/bin/sh", "sh", "-c", cmd_string, 0);
            fprintf(stderr, "execl failed\n");
            exit(2);
        default:
#	    ifdef SVR4
	    (void) wait(&status);
	    return(WEXITSTATUS(status)) ;
#	    else
	    (void) wait(&status);
            return(status.w_retcode);
#	    endif
     }
}
/****************************************************************************
 NAME: get_line - get a line.
 SYNOPSIS: char *get_line (file_ptr)
	   FILE *file_ptr;
 ARGUMENTS:
        Input: FILE *file_ptr
 DESCRIPTION: This routine reads characters from the data
              file into the buffer, until n-1 characters are
              read, a NEWLINE character is read and
              transferred to buffer.  It skips empty line,
              and leading tab.
 RETURN VALUE: none
 GLOBAL: fgets()
*****************************************************************************/

char *get_line (file_ptr)
FILE *file_ptr;
{

    int  i;
    static char line_ptr[MAX_FILE_NAME_LEN];
    char buffer[MAX_LENGTH+1];
    char *buffer_ptr = buffer; 
    
    /* get one line */
    fgets(buffer, MAX_LENGTH, file_ptr);

    /* initialize the word array */
    for (i=0; i<MAX_FILE_NAME_LEN; i++)
	line_ptr[i] = '\0';

    i = 0;
    /* skip leading space */
    while (*buffer == ' ')  
        buffer_ptr++; 

    while (*buffer_ptr != '\n') { 
        /* read a file name */
        line_ptr[i] = *buffer_ptr;
        i++;
        buffer_ptr++;
    }
    return(line_ptr);

} /* end get_line */

