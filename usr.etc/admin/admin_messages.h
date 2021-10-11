/*  @(#)admin_messages.h 1.1 92/07/30 SMI  */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifndef _admin_messages_h
#define _admin_messages_h

/*
 * admin_messages.h - definitions of messages for administrative software
 */

#define	SYS_ERR_NO_ETHER0	"get_ether0name: Could not find an ethernet interface\n"
#define	SYS_ERR_NONET	"Network interface is not up\n"
#define	SYS_ERR_EXMETHOD	"%s: Failure executing %s method\n"
#define	SYS_ERR_PLS_RM	"Please remove %s\n"
#define	SYS_ERR_NO_HOSTNAME	"%s: No hostname specified\n"
#define	SYS_ERR_NO_DOMAIN	"%s: No domain specified\n"
#define	SYS_ERR_NO_IPADDR	"%s: No IP address specified\n"
#define	SYS_ERR_NO_TIMESERV	"%s: No timeserver specified\n"
#define	SYS_ERR_NO_TIMEZONE	"%s: No timezone specified\n"
#define	SYS_ERR_SET_HOSTNAME	"%s: Failure setting hostname\n"
#define	SYS_ERR_SET_DOMAIN	"%s: Failure setting NIS domain\n"
#define	SYS_ERR_SET_IPADDR	"%s: Failure setting IP address\n"
#define	SYS_ERR_SET_TZ	"%s: Failure setting timezone\n"
#define	SYS_ERR_SET_TIME	"%s: Failure setting system clock\n"
#define	SYS_ERR_ADD_ENTRY	"%s: Unable to add entry to %s.\n"
#define	SYS_ERR_SYS_CMD	"%s: System command %s failed, status was %d\n"
#define	SYS_ERR_ROOTPASSWD	"%s: Failure getting root passwd\n"
#define	SYS_ERR_IFCONFIG	"Error executing ifconfig for netmask\n"
#define	SYS_ERR_YPBIND		"Can't start ypbind.\n"

#define	OM_ERRNOOBJECT	"Could not find object class directory\n"
#define	OM_ERRNOCLASS	"Could not find class %s\n"
#define	OM_ERRNOMETHOD	"Could not find method %s for class %s\n"
#define	OM_ERROPENDIR	"Could not open directory %s\n"
#define OM_ERRCLOSEDIR	"Could not close directory %s\n"
#define OM_ERRREADDIR	"Could not read directory %s\n"


#define AMCL_ERR_BADMETHOD	"Requested class or method does not exist\n"
#define AMCL_ERR_ONLYLOCAL	"Non-LOCAL method invocation not supported\n"
#define AMCL_ERR_BADTYPE	"Invalid administrative argument type (%d)\n"
#define AMCL_ERR_BADOPTION	"Invalid option (%d)\n"
#define AMCL_ERR_OUTPIPE	"Could not create pipe to capture STDOUT.  Reason: errno %d\n"
#define AMCL_ERR_ERRPIPE	"Could not create pipe to capture STDERR.  Reason: errno %d\n"
#define AMCL_ERR_FORK		"Could not fork child method.  Reason: errno %d\n"
#define AMCL_ERR_STOPPED	"Child method stopped on signal %d\n"
#define AMCL_ERR_SIGNALED	"Child method terminated on signal %d\n"
#define AMCL_ERR_EXEC		"Could not execv() method.  Reason: errno %d\n"
#define AMCL_ERR_OUTDUP		"Could not dup() STDOUT of method to pipe.  Reason: errno %d\n"
#define AMCL_ERR_ERRDUP		"Could not dup() STDERR of method to pipe.  Reason: errno %d\n"
#define AMCL_ERR_WAIT		"Failure in waitpid() on child method. Reason: errno %d\n"
#define AMCL_ERR_BADREAD	"Read failure of ''%s'' pipe.  Reason: errno %d\n"

#define FAIL_MENU "\n\
___________________________________________________________________________\n\
\n\
		    INSTALLATION MESSAGES\n\
\n\
The automatic installation procedure did not find all of the\n\
information needed to set up this system on the network.\n\
\n\
You may:\n\
\n\
1.  Review the reasons the automatic installation was not\n\
    completed.\n\
\n\
2.  Continue the installation by supplying the needed\n\
    information manually.\n\
\n\
3.  Stop the installation procedure and halt the system.\n\
\n\
If you are installing a non-networked system or are attempting\n\
network installation without using the network information\n\
service, select option 2 to continue the installation.\n\n\n"

#define ENTER_CHOICE	"Enter choice (1, 2, or 3) and press the RETURN key. "

#define BAD_CHOICE	"\nInvalid choice.  You must enter 1, 2 or 3.\n"

#define BAD_CHOICE_2	"\nInvalid choice.  You must press RETURN or ESC.\n"

#define MANUAL_INTRO "\n\
___________________________________________________________________________\n\
\n\
				MANUAL SETUP\n\
\n\
You can set up this system manually by entering the following information\n\
as prompted:\n\
\n\
	o Terminal type (if this is not a bit-mapped Sun terminal)\n\
\n\
	o Hostname\n\
\n\
	o Time zone\n\
\n\
	o Date and time\n\
\n\
	o Internet address (if installing on a network)\n\
\n\
	o NIS domain name (if using NIS on this network)\n\
\n\n\
If you are installing your system on a network, but you do not have\n\
your Internet address or NIS domain name, contact your system\n\
administrator.\n\
\n\
Make sure your system administrator adds this system to the network\n\
databases so that this system can communicate with others on the\n\
network.\n\
\n\n\
Press RETURN to begin the manual setup or\n\
press the ESC key to return to the previous menu.\n\n"

#define	IP_FAIL_REASONS "\
___________________________________________________________________________\n\
\n\
			INTERNET ADDRESS NOT FOUND\n\
\n\
An Internet address could not be obtained from a server on the network due\n\
to one of the following reasons:\n\
\n\
	o The Ethernet cable is not connected correctly.\n\
\n\
	o The rarp daemon is not running on a server in this subnet.\n\
\n\
	o NIS is not running on the network.\n\
\n\
	o No NIS servers are functioning.\n\
\n\
	o There is no entry for this system in the NIS ethers and hosts\n\
	  databases.\n\
\n\n\
Press any key to return to the previous menu.\n\n"

#define	HOSTNAME_FAIL_REASONS "\
___________________________________________________________________________\n\
\n\
			HOSTNAME NOT FOUND\n\
\n\n\
A hostname could not be obtained from a server on the network due to one\n\
of the following reasons:\n\
\n\
	o The bootparams daemon is not running on a server in this\n\
	  subnet.\n\
\n\
	o NIS is not running on the network.\n\
\n\
	o No NIS servers are functioning.\n\
\n\
	o There is no entry for this system in the NIS bootparams database.\n\
\n\n\
Press any key to return to the previous menu.\n\n"

#define DOMAINNAME_FAIL_REASONS "\
_____________________________________________________________________\n\
\n\
			DOMAIN NAME NOT FOUND\n\
\n\n\
An NIS domain name could not be obtained from a server on the network\n\
due to one of the following reasons:\n\
\n\
	o The bootparams daemon is not running on a server in this\n\
	  subnet.\n\
\n\
	o NIS is not running on the network.\n\
\n\
	o No NIS servers are functioning.\n\
\n\
	o There is no entry for this system in the NIS bootparams database.\n\
\n\n\
Press any key to return to the previous menu.\n\n"

#define TIMEZONE_KEY_REASON "\
______________________________________________________________________\n\
\n\
			UNABLE TO SET TIME ZONE\n\
\n\n\
The time zone could not be obtained from a server on the network\n\
because there is no entry for this system or domain in the NIS\n\
timezone.byname map.\n\
\n\n\
Press any key to return to the previous menu.\n\n"

#define TIMEZONE_MAP_REASON "\
______________________________________________________________________\n\
\n\
			UNABLE TO SET TIME ZONE\n\
\n\n\
The time zone could not be obtained from a server on the network\n\
because the NIS 'timezone.byname' map does not exist.\n\
\n\n\
Press any key to return to the previous menu.\n\n"

#define TIMEZONE_FAIL_REASONS "\
______________________________________________________________________\n\
\n\
			UNABLE TO SET TIME ZONE\n\
\n\n\
The time zone could not be obtained from an NIS server on the network.\n\
The specific failure message returned by NIS was:\n\
\n\
%s\n\
\n\n\
Press any key to return to the previous menu.\n\n"

#define SETUP_FAIL_REASONS "\
________________________________________________________________________\n\
\n\
			UNABLE TO SET UP SYSTEM\n\
\n\n\
This system could not be set up automatically because of a hardware\n\
or software problem.  Contact your system administrator or service\n\
representative for assistance.  The error message from the system is:\n\
\n\
%s\n\
\n\n\
Press any key to return to the previous menu.\n\n"

#define	HALTING_MESSAGE	"\
\n\n\
The system is being halted.  In order to restart it:\n\
\n\
If you receive the 'ok ' prompt, type 'boot' and press RETURN.\n\
\n\
If you receive the '> ' prompt, type 'b' and press RETURN.\n\
\n\n"

#endif /* !_admin_messages_h */
