#ifndef lint
#ifdef sccs
static char  sccsid[] = "@(#)sv_malloc.c 1.1 92/07/30";
#endif
#endif

/*
 *  Copyright (c) 1989 by Sun Microsystems Inc.
 */





 /*=====================
   File: sv_malloc.c
 =====================*/





/*===========================================================================
  Introduction

  This is a wrapper for malloc(3).  Its purpose is to centralize and
  standardize the error checking that ought to be done after all attempts
  to dynamically allocate memory.  If the allocation succeeds, sv_malloc()
  returns just what malloc() returned.  On failure, sv_malloc() logs an
  error message using syslog(3), and exits.

  This approach has several advantages.  First, the existence of this fn
  should encourage checking of the returned pointers.  Second, allocation
  errors will now leave permanent evidence in a file as well as possibly
  transient evidence on stderr.  (This assumes that /etc/syslog.conf
  continues to route error messages to someplace like /var/adm/messages.)
  Finally, we can shrink the code size slightly by removing all the strings
  printed to stderr in all the functions that are currently doing checking
  on their own.  I think that smaller code is more important to us just now
  than the extra function call overhead.

  But the main motivation for this function is to avoid unnecessary core-
  dumping 'bugs' when users run out of swap space.  They can be viciously
  difficult -- and expensive -- to track down, and they don't look very good.
  Eventually storage allocation should be further rationalized.  The system
  should behave in a predictable, intelligent way when it can't get more
  space.  In lieu of that extra engineering and revamping, sv_malloc()
  is a patch.
                                                        Mick Goulish
                                                        21 Aug 89
===========================================================================*/





/*========================================================================
  Naming Conventions

  "sv" as a prefix on the memory-allocation functions stands for SunView.
=========================================================================*/





/*================
  Include Files
=================*/
#include        <syslog.h>			/* Needed for calls to syslog(3).  */





/*=====================
  Constants
======================*/
#define		MESSAGE_LENGTH			128

#define		SV_MALLOC_FAILED_EXIT_CODE	1





/*=====================
  Global Variables
======================*/
/*-----------------------------------------------------------
  These are here to save a little space.  Otherwise, there
  would be several copies of each.
------------------------------------------------------------*/
static char
  * product_name        = "SunView1",
  * message_suffix      = "couldn't allocate memory. Swap space exhausted?";





/*================================================================
  Fn: sv_malloc

  Args:		int requested_size;

  Returns:	The (char *) returned by malloc(3).

  Side Effects:	Logs an error and exits if the malloc call fails.

  Overview:	Try the allocation call.  If it succeeds, just return what
		it returned.  Otherwise, construct an error message, log it,
		and lie down and die gracefully.
=================================================================*/
char *
sv_malloc(requested_size)
  int   requested_size;
{
  char  * allocated_space_ptr;
  char  message[MESSAGE_LENGTH];

  if(! (allocated_space_ptr = (char *) malloc(requested_size)))
  {
    sprintf(message, "%s: sv_malloc() %s", product_name, message_suffix);
    syslog(LOG_ERR, message);
    exit(SV_MALLOC_FAILED_EXIT_CODE);
  }

  return allocated_space_ptr;
}





/*================================================================
  Fn: sv_calloc

  Args:		int how_many_blocks, size_per_block;

  Returns:	The (char *) returned by calloc(3).

  Side Effects:	Logs an error and exits if the calloc call fails.

  Overview:	Try the allocation call.  If it succeeds, just return what
		it returned.  Otherwise, construct an error message, log it,
		and lie down and die gracefully.
=================================================================*/
char *
sv_calloc(how_many_blocks, size_per_block)
  int   how_many_blocks,
        size_per_block;
{
  char  * allocated_space_ptr;
  char  message[MESSAGE_LENGTH];

  if(! (allocated_space_ptr =
         (char *) calloc(how_many_blocks, size_per_block)
       )
    )   
  {  
    sprintf(message, "%s: sv_calloc() %s", product_name, message_suffix);
    syslog (LOG_ERR, message);
    exit(SV_MALLOC_FAILED_EXIT_CODE);
  }

  return allocated_space_ptr;
}





/*================================================================
  Fn: sv_realloc

  Args:		char	* ptr;
		int	requested_size;

  Returns:	The (char *) returned by realloc(3).

  Side Effects:	Logs an error and exits if the realloc call fails.

  Overview:	Try the allocation call.  If it succeeds, just return what
		it returned.  Otherwise, construct an error message, log it,
		and lie down and die gracefully.
=================================================================*/
char *
sv_realloc(ptr, requested_size)
  char          * ptr;
  unsigned int  requested_size;
{
  char  * allocated_space_ptr;
  char  message[MESSAGE_LENGTH];

  if(! (allocated_space_ptr =
         (char *) realloc(ptr, requested_size)
       )
    )   
  {  
    sprintf(message, "%s: sv_realloc() %s", product_name, message_suffix);
    syslog (LOG_ERR, message);
    exit(SV_MALLOC_FAILED_EXIT_CODE);
  }

  return allocated_space_ptr;
}





