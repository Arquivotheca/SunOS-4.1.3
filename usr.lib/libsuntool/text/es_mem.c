#ifndef lint
#ifdef sccs
static char  sccsid[] = "@(#)es_mem.c 1.1 92/07/30";
#endif
#endif

/*
 *  Copyright (c) 1986, 1987 by Sun Microsystems Inc.
 */

/*
 * Entity stream implementation for one block of virtual memory.
 */

#include <sys/types.h>
#include <varargs.h>
#include <suntool/primal.h>
#include <suntool/entity_stream.h>

#define MAS_ADDED_REALLOC

typedef struct es_mem_text {
	Es_status	 status;
	char		*value;
	u_int		 length;
	u_int		 position;
	u_int		 max_length;
#ifdef	MAS_ADDED_REALLOC
	u_int		 initial_max_length;
#endif
	caddr_t		 client_data;
} Es_mem_text;
typedef Es_mem_text *Es_mem_data;
#define	ABS_TO_REP(esh)	(Es_mem_data)LINT_CAST(esh->data)

extern char	*strncpy();

extern Es_handle es_mem_create();
static Es_status es_mem_commit();
static Es_handle es_mem_destroy();
static caddr_t   es_mem_get();
static Es_index  es_mem_get_length();
static Es_index  es_mem_get_position();
static Es_index  es_mem_set_position();
static Es_index  es_mem_read();
static Es_index  es_mem_replace();
static int       es_mem_set();

static struct es_ops es_mem_ops  =  {
	es_mem_commit,
	es_mem_destroy,
	es_mem_get,
	es_mem_get_length,
	es_mem_get_position,
	es_mem_set_position,
	es_mem_read,
	es_mem_replace,
	es_mem_set
};

extern Es_handle
es_mem_create(max, init)
	u_int	max;
	char   *init;
{
	extern char *calloc();
	extern char *malloc();
	Es_handle esh = NEW(Es_object);
	Es_mem_data private = NEW(Es_mem_text);

	if (esh == ES_NULL)  {
		goto AllocFailed;
	}
	if (private == 0)  {
		free((char *)esh);
		goto AllocFailed;
	}
#ifdef	MAS_ADDED_REALLOC
	private->initial_max_length = max;
	if (max == ES_INFINITY) {
		max = 10000;
	}
#endif
	private->value = malloc(max+1);
	if (private->value == 0)  {
		free((char *)private);
		free((char *)esh);
		goto AllocFailed;
	}

	(void) strncpy(private->value, init, (int)max);
	private->value[max] = '\0';
	private->length = strlen(private->value);
	private->position = private->length;
	private->max_length = max - 1;

	esh->ops = &es_mem_ops;
	esh->data = (caddr_t)private;
	return(esh);

AllocFailed:
	return(ES_NULL);
}

/* ARGSUSED */
static Es_status
es_mem_commit(esh)
	Es_handle esh;
{
	return(ES_SUCCESS);
}

static Es_handle
es_mem_destroy(esh)
	Es_handle esh;
{
	register Es_mem_data private = ABS_TO_REP(esh);

	free((char *)esh);
	free(private->value);
	free((char *)private);
	return(ES_NULL);
}

/* ARGSUSED */
static caddr_t
es_mem_get(esh, attribute, va_alist)
	Es_handle		esh;
	Es_attribute		attribute;
	va_dcl
{
	register Es_mem_data	private = ABS_TO_REP(esh);
#ifndef lint
	va_list			args;
#endif

	switch (attribute) {
	  case ES_CLIENT_DATA:
	    return((caddr_t)(private->client_data));
	  case ES_NAME:
	    return(0);
	  case ES_STATUS:
	    return((caddr_t)(private->status));
	  case ES_SIZE_OF_ENTITY:
	    return((caddr_t)sizeof(char));
	  case ES_TYPE:
	    return((caddr_t)ES_TYPE_MEMORY);
	  default:
	    return(0);
	}
}

static int
es_mem_set(esh, attrs)
	Es_handle	esh;
	Attr_avlist	attrs;
{
	register Es_mem_data	 private = ABS_TO_REP(esh);
	Es_status		 status_dummy = ES_SUCCESS;
	register Es_status	*status = &status_dummy;

	for (; *attrs && (*status == ES_SUCCESS); attrs = attr_next(attrs)) {
	    switch ((Es_attribute)*attrs) {
	      case ES_CLIENT_DATA:
		private->client_data = attrs[1];
		break;
	      case ES_STATUS:
		private->status = (Es_status)attrs[1];
		break;
	      case ES_STATUS_PTR:
		status = (Es_status *)LINT_CAST(attrs[1]);
		*status = status_dummy;
		break;
	      default:
		*status = ES_INVALID_ATTRIBUTE;
		break;
	    }
	}
	return((*status == ES_SUCCESS));
}

static Es_index
es_mem_get_length(esh)
	Es_handle esh;
{
	register Es_mem_data private = ABS_TO_REP(esh);
	return (private->length);
}

static Es_index
es_mem_get_position(esh)
	Es_handle esh;
{
	register Es_mem_data private = ABS_TO_REP(esh);
	return(private->position);
}

static Es_index
es_mem_set_position(esh, pos)
	Es_handle	esh;
	Es_index	pos;
{
	register Es_mem_data private = ABS_TO_REP(esh);

	if (pos > private->length) {
		pos = private->length;
	}
	return(private->position = pos);
}

static Es_index
es_mem_read(esh, len, bufp, resultp)
	Es_handle	 esh;
	u_int		 len,
			*resultp;
	register char	*bufp;
{
	register Es_mem_data private = ABS_TO_REP(esh);

	if (private->length - private->position < len)  {
		len = private->length - private->position;
	}
	bcopy(private->value + private->position, bufp, (int)len);
	*resultp = len;
	return (private->position += len);
}

static Es_index
es_mem_replace(esh, end, new_len, new, resultp)
	Es_handle  esh;
	int	   end,
		   new_len,
		  *resultp;
	char	  *new;
{
	int		   old_len, delta;
	char		  *start, *keep, *restore;
	register Es_mem_data private = ABS_TO_REP(esh);

	*resultp = 0;
	if (new == 0 && new_len != 0)  {
		private->status = ES_INVALID_ARGUMENTS;
		return ES_CANNOT_SET;
	}
	if (end > private->length) {
		end = private->length;
	} else if (end < private->position) {
		int tmp = end;
		end = private->position;
		private->position = tmp;
	}
	old_len = end - private->position;
	delta   = new_len - old_len;

	if (delta > 0 && private->length + delta > private->max_length) {
#ifdef	MAS_ADDED_REALLOC
	    extern char	*realloc();
	    char	*new_value = (char *)0;
	    if (private->initial_max_length == ES_INFINITY) {
		new_value = realloc(private->value,
				    private->max_length + delta + 10000 + 1);
		if (new_value) {
		    private->value = new_value;
		    private->max_length += delta + 10000;
		}
	    }
	    if (!new_value) {
#endif
		private->status = ES_SHORT_WRITE;
		return ES_CANNOT_SET;
#ifdef	MAS_ADDED_REALLOC
	    }
#endif
	}
	start = private->value + private->position;
	keep  = private->value + end;
	restore = start + new_len;
	if (delta != 0) {
		bcopy(keep, restore, (int)private->length - end + 1);
	}
	if (new_len > 0) {
		bcopy(new, start, new_len);
	}
	private->position = end + delta;
	private->length += delta;
	private->value[private->length] = '\0';
	*resultp = new_len;
	return restore - private->value;
}

#ifdef DEBUG
extern
es_mem_dump(fd, pdh)
	FILE		*fd;
	Es_mem_data	 pdh;
{
	(void) fprintf(fd, "\n\t\t\t\t\t\tmax length:  %ld", pdh->max_length);
	(void) fprintf(fd, "\n\t\t\t\t\t\tcurrent length:  %ld", pdh->length);
	(void) fprintf(fd, "\n\t\t\t\t\t\tposition:  %ld", pdh->position);
	(void) fprintf(fd, "\n\t\t\t\t\t\tvalue (%lx):  \"%s\"",
			pdh->value, (pdh->value ? pdh->value : "") );
}
#endif

