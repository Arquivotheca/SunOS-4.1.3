#ifndef lint
#ifdef sccs
static char   sccsid[] = "@(#)get_selection.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sunwindow/rect.h>
#include <suntool/selection_svc.h>
#include <suntool/selection_attributes.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

static Seln_result	  read_proc();

static int		  data_read = 0;

static void		  quit(), seln_use_test_service(), seln_use_timeout();

struct get_sel_args {
	char	*arg_name;
	char	arg_case;
} get_sel_synonyms[] ={
	"primary", '1',
	"secondary", '2',
	"clipboard", '3'
};

#define NSYNONYMS (sizeof(get_sel_synonyms) / sizeof(struct get_sel_args))

#ifdef STANDALONE
main(argc, argv)
#else
int get_selection_main(argc, argv)
#endif STANDALONE
    int                   argc;
    char                **argv;
{
    Seln_holder           holder;
    Seln_rank             rank = SELN_PRIMARY;
    char                  context = 0;
    int			  i;

    while (--argc) {
	argv++;
	for (i=0; i < NSYNONYMS; i++) {
		if (strcmp(*argv, get_sel_synonyms[i].arg_name) == 0)
			**argv = get_sel_synonyms[i].arg_case;
	}
	switch (**argv) {
	  case '1':
	    rank = SELN_PRIMARY;
	    break;
	  case '2':
	    rank = SELN_SECONDARY;
	    break;
	  case '3':
	    rank = SELN_SHELF;
	    break;
	  case 'D':
	    seln_use_test_service();
	    break;
	  case 't':
	  case 'T':
	    seln_use_timeout(atoi(*(++argv)));
	    --argc;
	    break;
	  default:
	    quit("Usage: get_selection [D] [t seconds] [1 | 2 |3] [primary | secondary | clipboard]\n");
	}
    }
    holder = seln_inquire(rank);
    if (holder.state == SELN_NONE) {
	quit("Selection non-existent, or selection-service failure\n");
    }
    (void) seln_query(&holder, read_proc, &context,
		      SELN_REQ_CONTENTS_ASCII, 0, 0);
    if (data_read)
	EXIT(0); 
    else
	EXIT(1); 
}

static void
quit(str)
    char                 *str;
{
    (void)fprintf(stderr, str);
    exit(1);
}

static Seln_result
read_proc(buffer)
    Seln_request         *buffer;
{
    char                 *reply;

    if (buffer == (Seln_request *) NULL) {
	quit("Selection holder error -- NULL reply.\n");
    }
    if (*buffer->requester.context == 0) {
	/* First buffer: check reply's format. */
	if (*((Seln_attribute *) (LINT_CAST(buffer->data)))
			!= SELN_REQ_CONTENTS_ASCII) {
	    quit("Selection holder error -- unrecognized request\n");
	}
	reply = buffer->data + sizeof (Seln_attribute);
	*buffer->requester.context = 1;
    } else {
	reply = buffer->data;
    }
    fputs(reply, stdout);
    (void)fflush(stdout);
    data_read = 1;
    return SELN_SUCCESS;
}
