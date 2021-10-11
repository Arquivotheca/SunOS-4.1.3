#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)sys_info.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)sys_info.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		sys_info.c
 *
 *	Description:	This file contains the routines necessary for
 *		interfacing with the 'sys_info' structure file.  The
 *		layout of a 'sys_info' file is as follows (order is no
 *		longer important):
 *
 *			hostname=	(comma-separated list of hostnames)
 *			ether_name=	(comma-separated list of ethernet
 *						 interface names)
 *			ip=		(comma-separated list of internet
 *						 addresses)
 *			hostname0=	(only for backward compatibility)
 *					( replaced by hostname= above )
 *			hostname1=	(		"		)
 *			sys_type=
 *			ether_name0=	(		"		)
 *			ether_name1=	(		"		)
 *			ip0=		(		"		)
 *			ip0_minlab= (SunB1)
 *			ip0_maxlab= (SunB1)
 *			ip1=		(		"		)
 *			ip1_minlab= (SunB1)
 *			ip1_maxlab= (SunB1)
 *			yp_type=
 *			domainname=
 *			op_type=
 *			reboot=
 *			rewind=
 *			arch_str=
 *			root=
 *			user=
 *			termtype=
 *			timezone=
 *			server=
 *			server_ip=
 *			exec_path=
 *			kvm_path=
 */

#include <stdio.h>
#include <string.h>
#include "install.h"
#include "menu.h"


/*
 *	Local functions:
 */
static	char *	out_std();
static	char *	cv_op_to_str();
static	int	cv_str_to_op();
static	int	cv_str_to_sys();
static	char *	cv_sys_to_str();
static	int	inp_hostname();
static	int	inp_ethername();
static	int	inp_ip();
static	char	*out_hostname();
static	char	*out_ethername();
static	char	*out_ip();
static	struct key_xlat_t	*lookup();




/*
 *	Local variables:
 */
static	sys_info	sys;			/* local copy of sys_info */


/*
 *	Key translation table:
 */

/*
 * The meaning of this table has been modified slightly because of the
 * structure change of sys_info required by multiple ethernet requirements.
 * The first field is the keyword name appearing to the left of the equal
 * sign in the sys_info file.  The next field is the input handling routine,
 * called when reading the sys_info file -- if this is 0, data is copied as
 * characters to the data_p field (the last field).  The third field is the
 * output handling routine which returns a string to be inserted after the
 * "=" in the output sys_info file.  If this field is zero, nothing is
 * written (indicates backward compatibility keywords -- can be read, but
 * the save routine does not write it out.
 */

static	key_xlat	key_list[] = {
	{ "hostname",	inp_hostname,	out_hostname,	(char*) sys.ethers },
	{ "sys_type",	cv_str_to_sys,	cv_sys_to_str,	(char *)&sys.sys_type},
	{ "ether_name",	inp_ethername,	out_ethername,	(char *) sys.ethers },
	{ "ip",		inp_ip,		out_ip,		(char *) sys.ethers },
	{ "hostname0",	0,		0,		sys.hostname0 },
	{ "hostname1",	0,		0,		sys.hostname1 },
	{ "ether_name0",	0,	0,		sys.ether_name0 },
	{ "ether_name1",	0,	0,		sys.ether_name1 },
	{ "ip0",	0,		0,		sys.ip0 },
	{ "ip1",	0,		0,		sys.ip1 },

#ifdef SunB1
	{ "ip0_minlab",	cv_str_to_lab, cv_lab_to_str,(char *)&sys.ip0_minlab },
	{ "ip0_maxlab",	cv_str_to_lab, cv_lab_to_str,(char *)&sys.ip0_maxlab },
	{ "ip1_minlab",	cv_str_to_lab, cv_lab_to_str,(char *)&sys.ip1_minlab },
	{ "ip1_maxlab",	cv_str_to_lab, cv_lab_to_str,(char *)&sys.ip1_maxlab },
#endif /* SunB1 */

	{ "yp_type",	cv_str_to_yp,	cv_yp_to_str,	(char *)&sys.yp_type },
	{ "domainname",	0,		out_std,	sys.domainname },
	{ "op_type",	cv_str_to_op,	cv_op_to_str,	(char *)&sys.op_type },
	{ "reboot",	cv_str_to_ans,	cv_ans_to_str,	(char *) &sys.reboot },
	{ "rewind",	cv_str_to_ans,	cv_ans_to_str,	(char *) &sys.rewind },
	{ "arch_str",	0,		out_std,	sys.arch_str },
	{ "root",	0,		out_std,	sys.root },
	{ "user",	0,		out_std,	sys.user },
	{ "termtype",	0,		out_std,	sys.termtype },
	{ "timezone",	0,		out_std,	sys.timezone },
	{ "server",	0,		out_std,	sys.server },
	{ "server_ip",	0,		out_std,	sys.server_ip },
	{ "exec_path",	0,		out_std,	sys.exec_path },
	{ "kvm_path",	0,		out_std,	sys.kvm_path },
	{ "static_sizing", cv_str_to_ans, cv_ans_to_str, (char *) &sys.static_sizing},

	{ NULL }
};

static struct key_xlat_t *
lookup(p)
	char *p;
{
	struct key_xlat_t *sysx;

	for (sysx = key_list; sysx->key_name != 0; sysx++)
		if (strcmp(sysx->key_name, p) == 0)
			return sysx;
	return 0;
}


/*
 *	Name:		read_sys_info()
 *
 *	Description:	Read system information from 'name' into 'sys_p'.
 *		Returns 1 if the file was saved successfully, and -1 if
 *		there was an error.
 */

int
read_sys_info(name, sys_p)
	char *		name;
	sys_info *	sys_p;
{
	char		buf[BUFSIZ];		/* buffer for I/O */
	FILE *		fp;			/* file pointer for name */
	struct key_xlat_t *sysx;


	/*
	 *	Always zero out the information buffer.
	 */
	bzero((char *) sys_p, sizeof(*sys_p));

	fp = fopen(name, "r");
	if (fp == NULL)
		return(0);

	bzero((char *) &sys, sizeof(sys));
	bzero(buf, sizeof(buf));

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		char *cp;
		char *cp1;

		if ((cp = strchr(buf, '=')) == 0)
			continue;
		*cp++ = 0;
		if ((cp1 = strchr(cp,'\n'))  != 0) {
			*cp1 = 0;
		}
		if ((sysx = lookup(buf)) != 0) {
			if (sysx->key_func == 0) {
				(void) strcpy(sysx->data_p, cp);
			} else {
				(*sysx->key_func)(cp, sysx->data_p);
			}
		}
	}
			
	*sys_p = sys;				/* copy valid sys_info */

	(void) fclose(fp);

	return(1);
} /* end read_sys_info() */




/*
 *	Name:		save_sys_info()
 *
 *	Description:	Save system information pointed to by 'sys_p'
 *		into 'name'.
 *
 *		Returns 1 if the file was saved successfully, and -1 if
 *		there was an error.
 */

int
save_sys_info(name, sys_p)
	char *		name;
	sys_info *	sys_p;
{
	FILE *		fp;			/* file pointer for name */
	struct key_xlat_t *sysx;


	sys = *sys_p;				/* copy valid sys_info */

	fp = fopen(name, "w");
	if (fp == NULL) {
		menu_log("%s: %s: cannot open file for writing.", progname,
			 name);
		return(-1);
	}

	for (sysx = key_list; sysx->key_name != 0; sysx++) {
		if (sysx->code_func != 0) {
			(void) fprintf(fp, "%s=%s\n", sysx->key_name, (sysx->code_func)(sysx->data_p));
		}
	}

	(void) fclose(fp);

	return(1);
} /* end save_sys_info() */

static char*
out_std(p)
	char *p;
{
	return p;
}

static int
inp_hostname(name, e)
	char *name;
	struct ether_interface *e;
{
	int i = 0;

	while (name != 0 && i < MAX_ETHER_INTERFACES) {
		char *p = strchr(name,',');
		char *newname = 0;

		if (p != 0) {
			*p = 0;
			newname = p+1;
		}
		(void) strcpy(e->hostname,name);
		name = newname;
		i++;
		e++;
	}
	return 1;
}
		
static int
inp_ethername(name, e)
	char *name;
	struct ether_interface *e;
{
	int i = 0;

	while (name != 0 && i < MAX_ETHER_INTERFACES) {
		char *p = strchr(name,',');
		char *newname = 0;

		if (p != 0) {
			*p = 0;
			newname = p+1;
		}
		(void) strcpy(e->interface_name,name);
		name = newname;
		i++;
		e++;
	}
	return 1;
}

static int
inp_ip(name, e)
	char *name;
	struct ether_interface *e;
{
	int i = 0;

	while (name != 0 && i < MAX_ETHER_INTERFACES) {
		char *p = strchr(name,',');
		char *newname = 0;

		if (p != 0) {
			*p = 0;
			newname = p+1;
		}
		(void) strcpy(e->internet_addr,name);
		name = newname;
		i++;
		e++;
	}
	return 1;
}

static char buff[BUFSIZ];

static char *
out_hostname(e)
	struct ether_interface *e;
{
	char *cp = buff;
	int nbr_nulls = 0;
	int i;

	*cp = 0;
	for (i = 0; i < MAX_ETHER_INTERFACES; i++, e++) {
		if (*e->hostname != 0) {
			while (nbr_nulls > 0) {
				*cp++ = ',';
				nbr_nulls--;
			}
			(void) strcpy(cp, e->hostname);
			cp += strlen(cp);
		}
		nbr_nulls++;
	}
	return buff;
}

static char *
out_ethername(e)
	struct ether_interface *e;
{
	char *cp = buff;
	int nbr_nulls = 0;
	int i;

	*cp = 0;
	for (i = 0; i < MAX_ETHER_INTERFACES; i++, e++) {
		if (*e->interface_name != 0) {
			while (nbr_nulls > 0) {
				*cp++ = ',';
				nbr_nulls--;
			}
			(void) strcpy(cp, e->interface_name);
			cp += strlen(cp);
		}
		nbr_nulls++;
	}
	return buff;
}

static char *
out_ip(e)
	struct ether_interface *e;
{
	char *cp = buff;
	int nbr_nulls = 0;
	int i;

	*cp = 0;
	for (i = 0; i < MAX_ETHER_INTERFACES; i++, e++) {
		if (*e->internet_addr != 0) {
			while (nbr_nulls > 0) {
				*cp++ = ',';
				nbr_nulls--;
			}
			(void) strcpy(cp, e->internet_addr);
			cp += strlen(cp);
		}
		nbr_nulls++;
	}
	return buff;
}

static	conv		op_list[] = {
	"install",	OP_INSTALL,
	"upgrade",	OP_UPGRADE,

	(char *) 0,	0
};




/*
 *	Name:		cv_op_to_str()
 *
 *	Description:	Convert an operation code into a string.  If the
 *		operation code cannot be converted, then NULL is returned.
 */

static char *
cv_op_to_str(op_p)
	int *		op_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = op_list; cv->conv_text; cv++)
		if (cv->conv_value == *op_p)
			return(cv->conv_text);
	return("");
} /* end cv_op_to_str() */




/*
 *	Name:		cv_str_to_op()
 *
 *	Description:	Convert a string into an operation code.  If the
 *		string cannot be converted, then 0 is returned.
 */

static int
cv_str_to_op(str, data_p)
	char *		str;
	int *		data_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = op_list; cv->conv_text; cv++)
		if (strcmp(cv->conv_text, str) == 0) {
			*data_p = cv->conv_value;
			return(1);
		}

	*data_p = 0;
	return(0);
} /* end cv_str_to_op() */




static	conv		sys_list[] = {
	"standalone",	SYS_STANDALONE,
	"server",	SYS_SERVER,
	"dataless",	SYS_DATALESS,

	(char *) 0,	0
};




/*
 *	Name:		cv_sys_to_str()
 *
 *	Description:	Convert a system type into a string.  If the system
 *		type cannot be converted, then NULL is returned.
 */

static char *
cv_sys_to_str(sys_p)
	int *		sys_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = sys_list; cv->conv_text; cv++)
		if (cv->conv_value == *sys_p)
			return(cv->conv_text);
	return("");
} /* end cv_sys_to_str() */




/*
 *	Name:		cv_str_to_sys()
 *
 *	Description:	Convert a string into a system type.  If the string
 *		cannot be converted, then 0 is returned.
 */

static int
cv_str_to_sys(str, data_p)
	char *		str;
	int *		data_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = sys_list; cv->conv_text; cv++)
		if (strcmp(cv->conv_text, str) == 0) {
			*data_p = cv->conv_value;
			return(1);
		}

	*data_p = 0;
	return(0);
} /* end cv_str_to_sys() */


/*
 *	Name:		read_sys_release()
 *
 *	Description:	Read the system RELEASE file and put it into buffer.
 *
 *	Return Value: 	0 : if the reads went ok
 *			-1: if the reads failed
 */
int
read_sys_release(buffer)
	char *		buffer;		/* buffer to store realease in */
{
	FILE *	fp;			/* scratch file pointer */

	if ((fp = fopen(RELEASE, "r")) == (FILE *)NULL) {
		menu_mesg("%s: failed to open %s", progname, RELEASE);
		return(-1);
	}
	
	if (fgets(buffer, MEDIUM_STR, fp ) == (char *)NULL) {
		menu_mesg("%s: failed to read release from %s", progname,
			  RELEASE);
		(void) fclose(fp);
		return(-1);
	}
	
	/*
	 *	Strip the newline off the end of the string
	 */
	buffer[strlen(buffer) - 1] = '\0';

	(void) fclose(fp);
	return(0);

} /* end read_sys_release() */


