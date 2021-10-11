static char sccsid[] = "@(#)online_util.c	1.1  7/30/92  Copyright Sun Mircosystems Inc.";

/*************************************************************************
	This file contains the following routines:

	sun_arch() 	        - get the cpu architecture.
	sun_unix() 	        - get the unix version level.
	sun_cpu()	        - get the cpu type.
	check_superuser()       - check the superuser mode.
	format_line() 	        - format one long string to make indents of 
			          specified space for lines other than the first 
			          line.

	errmsg()                - get system error message string.
	get_test_id()           - get a test id number from  known name list.
	get_version_id()        - get an integer version number from provided string.
	get_sccs_version()      - get an integer number from SCCS version string.
	get_processors_mask()   - get processor affinity mask.
	get_number_processors() - get a number of processors.

*****************************************************************************/
#include <stdio.h>
#include <ctype.h>
#ifdef SVR4
#include <sys/cpu.h>
#include <sys/uadmin.h>
#include <sys/cpuvar.h>
#else  SVR4
#include <mon/idprom.h>
#endif SVR4
#include <sys/ioctl.h>
#include "libonline.h"

extern	unsigned  gethostid();
int     test_id = 999;          /* test_id default as 999 */
int     version_id = 99;
int     subtest_id = 999;
int     error_code = 999;       /* default value for error exit */
int     error_base;
int	cpu_num[32];		/* to save the kernel-assigned CPU number */
char    *versionid = "unknown_version";

/****************************************************************************
 NAME: sun_arch -  get cpu architecture.
 
 SYNOPSIS:
        int sun_arch()
 ARGUMENTS: none
 DESCRIPTION:
        This routine calls gethostid() to get cpu architecture by looking
        at only most significant byte of its output.
 RETURN VALUE:
        If succeeds, architecture code ARCH3, ARCH3X, ARCH4, ARCH4C, ARCH4M,
	or ARCH386 are returned. unknown codes are defaulted to ARCH4C.
 MACROS: none
 GLOBAL: gethostid()
*****************************************************************************/
int
sun_arch()
{
  unsigned  hostid;

  hostid = gethostid() >> 24;
  if (hostid == 0x20) return(ARCH4C);	/* for clones */

#ifdef	SVR4
    switch (hostid & CPU_ARCH) {
        case SUN4_ARCH:
	     return(ARCH4);
        case SUN4C_ARCH:
             return(ARCH4C);
	case SUN4M_ARCH:
	     return(ARCH4M);
        default:
             return(ARCH4C);		/* so that clone machines will work */
    }           
#else	SVR4
    switch (hostid & IDM_ARCH_MASK) {
        case IDM_ARCH_SUN4:
	     return(ARCH4);
        case IDM_ARCH_SUN4C:
             return(ARCH4C);
	case IDM_ARCH_SUN4M:
	     return(ARCH4M);
        default:
             return(ARCH4C);		/* so that clone machines will work */
    }           
#endif	SVR4
}
/****************************************************************************
 NAME: sun_cpu -  get the cpu type.
 
 SYNOPSIS:
        int sun_cpu()
 ARGUMENTS: none
 DESCRIPTION:
        This routine calls gethostid() to get cpu type by looking
        at only most significant byte of its output. It returns 
	all the possible values of the id_machine field (so far).
 RETURN VALUE:
	It returns the cpu type hex value according to those defined in
	/usr/include/mon/idprom.h.
 MACROS: none
 GLOBAL: gethostid()
*****************************************************************************/
int
sun_cpu()
{
    return (gethostid() >> 24);
}

/****************************************************************************
 NAME: sun_unix -  get the unix version level.
 
 SYNOPSIS:
        char *sun_unix()
 DESCRIPTION:
        This routine reads /etc/motd file to determin unix version level.
 RETURN VALUE:  
	If succeeds, a string name of the SunOS version level will 
	return, otherwise 0.
 MACROS: 
	none
 GLOBAL: fopen() 
*****************************************************************************/

char *
sun_unix()
{
    FILE *fp;
    static char outstr[20];
    char linbuf[256], *outstr_ptr= outstr;
    register int  i;

    if ((fp = fopen ("/etc/motd", "r")) == NULL)
	return(NULL);
     
    if (fgets(linbuf, 255, fp) != NULL)
    	for (i = 14; linbuf[i] != ' '; i++)
	    *outstr_ptr++ = linbuf[i];

    *outstr_ptr = NULL;
    return(outstr);
}

/*****************************************************************************
 NAME: check_superuser -  check the superuser mode.
 SYNOPSIS:
        check_superuser();
 RETURN VALUE: void if in superuser mode; or exit with error message.
 GLOBAL: geteuid()
*****************************************************************************/
void
check_superuser()
{
    if (geteuid()) {
        printf("ERROR: Must be super-user.\n");
	exit(1);
    }
}

/****************************************************************************
 NAME: format_line - format one long string to make indents of specified
	space for lines other than the first line. 
 
 SYNOPSIS:                                       
        void format_line(out_string, in_string, indent) 
 ARGUMENTS:
	input:
		char *in_string - input string that needs to be formatted.
		int   indent    - number of space indented for sencond line
				  and after.
	output:
		char *out_string -  output string after being formatted.
	
 DESCRIPTION:
*****************************************************************************/

void
format_line(out_string, in_string, indent)
    char *out_string;
    char *in_string;
    int   indent;
{
    int	firstline_len = MSG_LINELEN;/* let first line have length of 80 char */
    int secline_len;
    int first = TRUE;		/* always starts from first line */
    int i=0, j; 

	secline_len = firstline_len - indent;
        while(*in_string != '\0') {
		if(*in_string == '\n') { 
			i = 0;		/* meet a newline, reset char count */
			*out_string++ = *in_string++;
			continue;
		}
                i++;
                if( !first && (i-firstline_len-1)%secline_len == 0 ) {
                        *out_string++ = '\n';
                        for (j=0; j<indent; j++)
                                *out_string++ = ' ';
                        while(*in_string == ' ') in_string++;
                }
                else {
                    if( first && i%(firstline_len+1) == 0 ){
                        *out_string++ = '\n';
                        for (j=0; j<indent; j++)
                                *out_string++ = ' ';
                        while(*in_string == ' ') in_string++;
                        first = FALSE;
                    }
                }
                *out_string++ = *in_string++;
        }
	*out_string = '\0';
}


/****************************************************************************
 NAME:  errmsg - get system error message string. 
 SYNOPSIS: char * errmsg(errnum)
 ARGUMENTS:
        Input: int errnum - system error code 

 DESCRIPTION:

 RETURN VALUE:
	returns the string of the system error message, based on the 
	errnum, to your program, courtesy of Guy Harris.
 GLOBAL: sys_nerr, sys_errlist[]
*****************************************************************************/
char * errmsg(errnum)
int errnum;
{
        extern int sys_nerr;
        extern char *sys_errlist[];
        static char errmsgbuf[6+10+1];
        static char *errmsg1 = "Error %d";
        if (errnum >= 0 && errnum < sys_nerr)
                return (sys_errlist[errnum]);
        else {
                (void) sprintf(errmsgbuf, errmsg1, errnum);
                return (errmsgbuf);
        }
}

/****************************************************************************
 NAME: get_test_id -  get a test id number from  known name list.
 SYNOPSIS: int get_test_id(test_name, name_list)
 ARGUMENTS:
        Input: char *test_name - testname string to match. 
        Output: char *name_list[] - a known name list to search. 

 DESCRIPTION: Used for diagnostics standard message format.

 RETURN VALUE: if search successful, return test id number (integer);
		otherwise, return 999.
 GLOBAL: strcmp()
*****************************************************************************/
int
get_test_id(test_name, name_list)
char *test_name;
char *name_list[];
{
    register int i;

    for (i=0; name_list[i] != NULL; i++) {
        if (!strcmp (name_list[i], test_name))
                return (i);
    }
    return (999);               /* Couldn't get test id */
}


/****************************************************************************
 NAME: get_version_id - get an integer version number from provided string. 

 SYNOPSIS: int get_version_id(ptr)
 ARGUMENTS:
        Input:  char  *ptr - a string that indicates a version.

 DESCRIPTION:

 RETURN VALUE: return an integer for the input string:
		e.g.  return 120   for "1.2" or "1.20" or "1.2+" where '+'
			represents any non-digit character.
	       return 999 if string starts from an non-digit character.
		e.g  return 999    for  "a12" or "+cd

 GLOBAL: isdigit(), strlen(), strcat(), atoi().
*****************************************************************************/
int
get_version_id(ptr)
char  *ptr;
{
    static char  buf[20];
    char  *buf_ptr = buf;
 
    if(!isdigit(*ptr)) return (999);
    while (*ptr != '\0') {
	if (*ptr != '.')  
	    *buf_ptr++ = isdigit(*ptr)? *ptr: '0';
	ptr++;
    }

    *buf_ptr = '\0';
    while(strlen(buf) <3) 
	strcat(buf, "0");
    return (atoi(buf));
}


/****************************************************************************
 NAME: get_sccs_version - get an integer number from SCCS version string.

 SYNOPSIS: int get_sccs_version(vstring)
 ARGUMENTS:
        Input: char *vstring - SCCS version string format, e.g. "1.5" 

 DESCRIPTION:

 RETURN VALUE:  Return an integer number that only counts the portion right
		after '.', e.g.  return 5 for "1.5"; return 21 for "2.21".
		If no '.' is found in the inpur string, return 99.

 GLOBAL : atoi()
*****************************************************************************/
int
get_sccs_version(vstring)
char *vstring;
{
register int count;

    count = 0;
    while (*vstring++ != '.') 
	if (++count > 5) return(99);
    return (atoi(vstring));
}
 
/****************************************************************************
 NAME:  get_processors_mask()
 SYNOPSIS:
        int get_processors_mask()
 ARGUMENTS:
	None
 DESCRIPTION:
        This routine uses the processor affinity masks via ioctl call
        to the device driver for the kernel memory devices to find
        out the number of processors exist.  The normal device to
        use is /dev/null.  The ioctl call reads the processor affinity
        mask value.
 MACROS:
        none.

 RETURN VALUE:  unsigned
*****************************************************************************/
int	get_processors_mask()
{
unsigned mask_value;
#ifdef	SVR4
    int		i;
    unsigned	j;

    mask_value = 0;
    for (i=31; i >= 0; --i)
    {
      mask_value <<= 1;
      j = uadmin(A_STAT_CPU, i, 0);
      if (j != -1 && (j&CPU_READY))
	++mask_value;
    }
    return(mask_value);
#else	SVR4
    int		memfd;
    memfd = open("/dev/null", 0);
    mask_value = 0;
    ioctl(memfd, MIOCGPAM, &mask_value);
    close(memfd);

    if (mask_value == 0) return(1);	/* single processor */
    else return(mask_value);
#endif	SVR4
}
 
/****************************************************************************
 NAME:  get_number_processors()
 SYNOPSIS:
        int get_number_processors()
 ARGUMENTS:
        input:  none.
 DESCRIPTION:
        This routine get the number of processors.  It calls function
        get_processors_mask() to read the the processor affinity mask
        value.  Each bit of the processor affinity mask is set cor-
        responding to the existing processor.
 MACROS:
        none.
 
 RETURN VALUE:  int
*****************************************************************************/
int
get_number_processors(mask_value)
int mask_value;
{
 
    int i=0, j=0, bit;
 
    for (bit = 1; bit; bit <<= 1, ++j) 
    {
        if (mask_value & bit) 
	{
	    cpu_num[i] = j;	/* CPU number starts from 0 */
            ++i;
        }
    }
    return(i);
}
