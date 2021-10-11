/*
 * codeset information
 */

#ifndef lint
/* static  char *sccsid = "@(#)codeset.h 1.1 92/07/30 SMI"; */
#endif

#include <stdio.h>
#define NAME_SIZE 14		/* Length of the code set name */

struct _code_header {
	char code_name[NAME_SIZE+1];	/* code set name */
	int code_id;			/* code set id */
	int code_info_size;		/* size of code set info */
};

struct _code_set_info {
	char code_name[NAME_SIZE+1];	/* code set name */
	int code_id;			/* code ID */
	char *code_info;		/* code information */
	int open_flag;			/* Am I open library ? */
};

#define EUC_SET		"euc"
#define XCCS_SET	"xccs"
#define ISO2022_SET	"iso2022"

#define CODESET_NONE	0
#define CODESET_EUC	2
#define CODESET_XCCS	3
#define CODESET_ISO2022	4
#define CODESET_USER 	100

#define ERROR -1
#define ERROR_NO_LIB	-2		/* dlopen failed */
#define ERROR_NO_SYM	-3		/* dlsym failed */

#ifdef DEBUG
#define LIBRARY_PATH 	"/tmp/"
#else
#define LIBRARY_PATH 	"/usr/lib/"
#endif

void *_ml_open_library();
