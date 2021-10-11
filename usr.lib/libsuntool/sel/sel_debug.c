#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)sel_debug.c 1.1 92/07/30  Copyr 1985 Sun Micro";
#endif
#endif

#include <suntool/selection_impl.h>
#include <sys/stat.h>

/*
 *	seln_debug: dump routines for debugging selection service
 */

static void	seln_dump_access(),
		seln_dump_address(),
		seln_dump_fds(),
		seln_dump_function_key(),
		seln_dump_functions();
extern char *sprintf();

void
seln_dump_file_args(stream, ptr)
    FILE                 *stream;
    Seln_file_info       *ptr;
{
    (void)fprintf(stream, "\trank: ");
    seln_dump_rank(stream, &ptr->rank);
    (void)fprintf(stream, "; name: \"%s\"\n", ptr->pathname);
}

void
seln_dump_function_buffer(stream, ptr)
    FILE                 *stream;
    Seln_function_buffer *ptr;
{
    (void)fprintf(stream, "    function: ");
    seln_dump_function_key(stream, &ptr->function);
    (void)fprintf(stream, "; addressee-rank: ");
    seln_dump_rank(stream, &ptr->addressee_rank);
    (void)fprintf(stream, "\n    caret:\n");
    seln_dump_holder(stream, &ptr->caret);
    (void)fprintf(stream, "\n    primary:\n");
    seln_dump_holder(stream, &ptr->primary);
    (void)fprintf(stream, "\n    secondary:\n");
    seln_dump_holder(stream, &ptr->secondary);
    (void)fprintf(stream, "\n    shelf:\n");
    seln_dump_holder(stream, &ptr->shelf);
}

void
seln_dump_holder(stream, ptr)
    FILE                 *stream;
    Seln_holder          *ptr;
{
    (void)fprintf(stream, "      rank: ");
    seln_dump_rank(stream, &ptr->rank);
    (void)fprintf(stream, "  state: ");
    seln_dump_state(stream, &ptr->state);
    (void)fprintf(stream, "   access:\n");
    seln_dump_access(stream, &ptr->access);
}

void
seln_dump_inform_args(stream, ptr)
    FILE                 *stream;
    Seln_inform_args     *ptr;
{
    (void)fprintf(stream, "    function ");
    seln_dump_function_key(stream, &ptr->function);
    (void)fprintf(stream, " went %s as reported by holder:\n",
	    (ptr->down) ? "down" : "up");
    seln_dump_holder(stream, &ptr->holder);
}

void
seln_dump_rank(stream, ptr)
    FILE                 *stream;
    Seln_rank            *ptr;
{
    register char        *val;
    char                  ebuf[256];

    if (ptr == (Seln_rank *) NULL) {
	val = "<nil>";
    } else {
	switch (*ptr) {
	  case SELN_UNKNOWN:
	    val = "Unknown";
	    break;
	  case SELN_CARET:
	    val = "Caret";
	    break;
	  case SELN_PRIMARY:
	    val = "Primary";
	    break;
	  case SELN_SECONDARY:
	    val = "Secondary";
	    break;
	  case SELN_SHELF:
	    val = "Shelf";
	    break;
	  case SELN_UNSPECIFIED:
	    val = "Unspecified";
	    break;
	  default:
	    (void)sprintf(ebuf, "<invalid: 0x%lx>", *ptr);
	    val = ebuf;
	}
    }
    (void)fprintf(stream, val);
}

void
seln_dump_response(stream, ptr)
    FILE                 *stream;
    Seln_response        *ptr;
{
    register char        *val;
    char                  ebuf[256];

    if (ptr == (Seln_response *) NULL) {
	val = "<nil>";
    } else {
	switch (*ptr) {
	  case SELN_IGNORE:
	    val = "Ignore";
	    break;
	  case SELN_REQUEST:
	    val = "Request";
	    break;
	  case SELN_FIND:
	    val = "Find";
	    break;
	  case SELN_SHELVE:
	    val = "Shelve";
	    break;
	  case SELN_DELETE:
	    val = "Delete";
	    break;
	  default:
	    (void)sprintf(ebuf, "<invalid: 0x%lx>", *ptr);
	    val = ebuf;
	}
    }
    (void)fprintf(stream, val);
}

void
seln_dump_result(stream, ptr)
    FILE                 *stream;
    Seln_result          *ptr;
{
    char                 *val;
    char                  ebuf[256];

    if (ptr == (Seln_result *) NULL) {
	val = "<nil>";
    } else {
	switch (*ptr) {
	  case SELN_FAILED:
	    val = "Failure";
	    break;
	  case SELN_SUCCESS:
	    val = "Success";
	    break;
	  case SELN_NON_EXIST:
	    val = "Doesn't exist";
	    break;
	  case SELN_DIDNT_HAVE:
	    val = "Didn't have";
	    break;
	  case SELN_WRONG_RANK:
	    val = "Wrong rank";
	    break;
	  case SELN_CONTINUED:
	    val = "Continued request";
	    break;
	  case SELN_CANCEL:
	    val = "Cancelled request";
	    break;
	  case SELN_UNRECOGNIZED:
	    val = "Unrecognized request";
	    break;
	  default:
	    (void)sprintf(ebuf, "<invalid: 0x%lx>", *ptr);
	    val = ebuf;
	}
    }
    (void)fprintf(stream, val);
}

/*
 * Dump the service 
 */
Seln_result
seln_dump_service(stream, holder, rank)
    FILE                 *stream;
    Seln_holder           holder[];
    Seln_rank             rank;
{
    (void)fprintf(stream, "Service Dump\n");
    if (rank == SELN_CARET || rank == SELN_UNSPECIFIED) {
	seln_dump_holder(stream, &holder[ord(SELN_CARET)]);
    }
    if (rank == SELN_PRIMARY || rank == SELN_UNSPECIFIED) {
	seln_dump_holder(stream, &holder[ord(SELN_PRIMARY)]);
    }
    if (rank == SELN_SECONDARY || rank == SELN_UNSPECIFIED) {
	seln_dump_holder(stream, &holder[ord(SELN_SECONDARY)]);
    }
    if (rank == SELN_SHELF || rank == SELN_UNSPECIFIED) {
	seln_dump_holder(stream, &holder[ord(SELN_SHELF)]);
    }
    seln_dump_functions(stream);
    seln_dump_fds(stream);
    return SELN_SUCCESS;
}

void
seln_dump_state(stream, ptr)
    FILE                 *stream;
    Seln_state           *ptr;
{
    char                 *val;
    char                  ebuf[256];

    if (ptr == (Seln_state *) NULL) {
	val = "<nil>";
    } else {
	switch (*ptr) {
	  case SELN_NONE:
	    val = "No selection";
	    break;
	  case SELN_EXISTS:
	    val = "Exists";
	    break;
	  case SELN_FILE:
	    val = "File held by service";
	    break;
	  default:
	    (void)sprintf(ebuf, "<invalid: 0x%lx>", (int) *ptr);
	    val = ebuf;
	}
    }
    (void)fprintf(stream, val);
}

static void
seln_dump_access(stream, ptr)
    FILE                 *stream;
    Seln_access          *ptr;
{

    (void)fprintf(stream, "\tpid: %d; program # 0x%x; client: 0x%x;\n",
	    ptr->pid, ptr->program, ptr->client);
    (void)fprintf(stream, "\tTCP: ");
    seln_dump_address(stream, &ptr->tcp_address);
    (void)fprintf(stream, "\tUDP: ");
    seln_dump_address(stream, &ptr->udp_address);
}

static void
seln_dump_address(stream, ptr)
    FILE                 *stream;
    struct sockaddr_in   *ptr;
{
    (void)fprintf(stream, "family: %d; port %d; address 0x%x, sin_zero 0x%x\n",
	    ptr->sin_family, ptr->sin_port, ptr->sin_addr, ptr->sin_zero);
}

static void
seln_dump_function_key(stream, key)
    FILE                 *stream;
    Seln_function        *key;
{
    register char        *val;
    char                  ebuf[256];

    switch (*key) {
      case SELN_FN_STOP:
	val = "Stop";
	break;
      case SELN_FN_AGAIN:
	val = "Again";
	break;
      case SELN_FN_PROPS:
	val = "Props";
	break;
      case SELN_FN_UNDO:
	val = "Undo";
	break;
      case SELN_FN_FRONT:
	val = "Front";
	break;
      case SELN_FN_PUT:
	val = "Put";
	break;
      case SELN_FN_OPEN:
	val = "Open";
	break;
      case SELN_FN_GET:
	val = "Get";
	break;
      case SELN_FN_FIND:
	val = "Find";
	break;
      case SELN_FN_DELETE:
	val = "Delete";
	break;
      default:
	(void)sprintf(ebuf, "<invalid: 0x%lx>", (int) *key);
	val = ebuf;
    }
    (void)fprintf(stream, val);
}

static void
seln_dump_functions(stream)
    FILE                 *stream;
{
    Seln_functions_state   buf;
    unsigned             *bufp, *limit = (unsigned *) &buf +
					 sizeof (Seln_functions_state) /
					 sizeof (unsigned);

    buf = seln_functions_all();
    (void)fprintf(stream, "      function state: ");
    for (bufp = (unsigned *) &buf; bufp < limit; bufp++) {
	(void)fprintf(stream, " 0x%lx", *bufp);
    }
    (void)fprintf(stream, "\n");
}

static char           *header = "fd	dev: #,	type	inode\n";

static void
seln_dump_fds(stream)
    FILE                 *stream;
{
    register int          fd;
    struct stat           s;

    (void)fprintf(stream, header);
    for (fd = 0; fd < 32; fd++) {
	if (fstat(fd, &s) != -1) {
	    (void)fprintf(stream, "%2d\t%6d\t%4d\t%5d\n",
		    fd, s.st_dev, s.st_rdev, s.st_ino);
	}
    }
}
