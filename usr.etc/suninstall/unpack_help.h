/*
 *	unpack_help.h	"@(#)unpack_help.h 1.1 92/07/30 SMI"
 *
 *
 * help windows/messages for various pieces of information
 * in the upack form.  NOTE:  These strings must be "preformatted",
 * i.e., the newlines must be in the right spot and the lines the
 * right length; any centering should be done herein.
 *
 * These are split up because of printf's limit of 128 format string
 * limitation.
 */

#define INTRO_ORG \
"\n                               Welcome to SunInstall\n\
\n\
     Remember:  Always back up your disks before beginning an installation.\n\
\n\
  SunInstall provides two installation methods:\n"
#define INTRO \
"                               Welcome to SunInstall\n\
\n\
     Remember:  Always back up your disks before beginning an installation.\n\
\n\
  SunInstall provides three installation methods:\n"
#define METHOD_0 \
"\n\
     0. Re-Preinstalled\n\
\n\
        This option reinstalls your machine to its factory pre-installed \n\
        state. No configuration is done, and this is mainly used to \n\
        rebuild your disk to its pristine factory condition. \n\
\n"
#define METHOD_1 \
"     1. Quick installation\n\
\n\
        This option provides an automatic installation with a choice of \n\
        standard installations, and a minimum number of questions asked. \n\
\n"
#define METHOD_2 \
"     2. Custom installation\n\
\n\
        Choose this method if you want more freedom to configure your\n\
        system.  You must use this option if you are installing your\n\
        system as a server.\n"
#define METHOD_3 \
"\n\
        Your choice (or Q to quit) >> " 

#define HOSTNAME_HELP \
"\n\
   If this system is connected to the network, the hostname must be unique to \n\
   the local area network.  Confirm this with your system administrator.\n\n\
   Enter a name to assign to this workstation and press RETURN:\n"

#define HOSTNAME_HELPSCREEN \
"\n\
You can choose the Hostname which specifies the name \n\
associated with your workstation.\n\
\n\
Hostnames should start with a letter, followed by any\n\
combination of letters, numbers, underscores (_),\n\
hyphens (-), or periods.  A hostname can have no more than 63 total\n\
characters.\n\
\n"

#define TZ_HELPSCREEN \
"\n\
The Time Zone screen lists all time zone regions Sun supports.\n\
\n\
You must select a time zone before you can set your system\n\
clock.  When you choose a region, a submenu of time zones\n\
within the region will be displayed.\n\
\n\
Note that you don't have to choose between\n\
\"Standard\" or \"Daylight\" because this distinction is\n\
handled automatically.\n\
\n"

#define SUBTZ_HELPSCREEN \
"\n\
The Time Zone submenu lists the time zones supported within the\n\
region selected on the Time Zone screen.  Choose\n\
the time zone for this system.\n\
\n"

#define NETWORK_HELP \
"\n\
  Enter selection for whether workstation is attached to a network \n\
  and press RETURN:\n\n\
    1) Yes, prompt for additional network questions to configure into network.\n\
        (Verify Ethernet cable is attached to workstation.)\n\n\
    2) No, workstation is NOT attached to a network."

#define NETWORK_HELPSCREEN \
"\n\
At this point, you must know whether or not your system is on a network.\n\
If you have questions about your networking status, ask your system\n\
administrator for help.\n\
   If you choose selection (1), you'll be asked some additional questions\n\
   about your network.\n\
   The Ethernet cable maintains your connection to the network.\n\
   Make sure it is plugged into the proper port on the back of\n\
   your workstation.  If it is not properly installed, the network\n\
   installation will fail.\n\
\n\
   If you choose selection (2), your workstation will NOT be attached to \n\
   a network, but will be configured as a standalone machine.\n\
   In this case, you can still attach it to a network later.\n"

#define IPADDR_HELP \
"\n\
  You must assign a network address to your workstation.  Contact your\n\
  system administrator for the unique network address of your system.\n\
  Do not use the displayed address (192.9.200.1) if your workstation\n\
  is being connected to the Internet network.  Do not enter an address\n\
  unless you are sure it is correct.\n\n\
  Enter the network address to be assigned to this workstation and\n\
  press RETURN:"
  

#define IPADDR_HELPSCREEN \
"\n\
A unique network address, must be assigned to your workstation\n\
to distinguish it from other systems on the network.  The network\n\
address has the form:\n\
              A.B.C.D\n\
The numbers should be separated by three periods.  The specific\n\
numbers you use must be registered with an external organization,\n\
so do not enter an address unless you are sure it is correct.\n\
\n\
In a local network, A and D of the network address represents\n\
a number in the range of 1 to 254.  B and C represents a number in\n\
the range of 0 to 255.\n\n\
In an Internet network, all the values represent numbers\n\
in the range of 1 to 254.\n\
If you do not have this information or are not sure, ask your system\n\
administrator for help.\n"


#define NIS_HELPSCREEN \
"\n\
The network information service (NIS) provides access to important\n\
resources (such as other machines and services) throughout your network.\n\
   Choose selection (1) if your site uses NIS and you want to utilize it.\n\
   You need to confirm this with your system administrator.\n\
\n\
   Choose selection (2) to specify that your workstation will not be using NIS.\n"

#define DOMAIN_HELPSCREEN \
"\n\
The NIS domain is a part of the network.  It is used to\n\
distinquish it from other domains on the network.  The\n\
NIS domain is a set of NIS maps maintained on the NIS\n\
master server and distributed to that server's NIS slaves.\n\
A NIS map is a database-like entity that maintains information\n\
about machines on a local area network.  Programs that\n\
are part of the NIS service query these maps.\n\n\
If you are unsure of your NIS domain name, ask your system\n\
administrator for the name assigned to your domain in the\n\
network.\n"

#define CONFIRM_HELPSCREEN \
"\n\
The Confirmation screen lists the data you have entered during\n\
the installation session.  You can change these settings by\n\
scrolling (using Control-B) to the screen whose setting you want\n\
to change.\n\
\n\
If you answer \"n\" to the confirmation message, you will be sent\n\
back to the Hostname screen to begin re-entry of all the settings.\n\
\n\
If you are satisfied with the settings listed, answer \"y\" to the\n\
confirmation message.\n"

#define DOMAIN_HELP \
"\n\
  A domain name uniquely identifies your part of the network.  Contact\n\
  your system administrator for the domain name.\n\n\
  Enter the domain name and press RETURN:\n"

#define YP_GEN_HELP \
"\n\
  NIS is the network information service.\n\n\
  Enter the selection that applies and press RETURN:\n\n\
    1) Yes, the network uses NIS.  Confirm this with your system administrator.\n\
    2) No, the workstation will not be using NIS." 

#define DEF_HELP \
"\n\
  Select the standard installation that best suits your needs.  Press\n\
  <Return> or <Space> to move forward a choice or <Control-B> to move\n\
  backward.  Type 'x' to make a selection or '?' to display more\n\
  information about a choice.  The menu scrolls up if additional\n\
  choices are available.\n\
\n\
  Select <none of the above> if none of the installations apply."


#define TIME_HELP \
"\n\
  Enter selection and press RETURN:\n\n\
      1) Yes, accept that as current time.\n\
      2) No, prompt me to input current time.\n"

#define SETTIME_HELP \
"\n\
 Enter the current date and local time and press RETURN:\n"

#define SYSTIME_HELPSCR \
"\n\
The System Time screen asks for confirmation of the information\n\
used to set the internal system clock.  Your workstation's internal\n\
clock synchronizes all of the activity taking place within your system.\n\
It is important for this clock to be set accurately.  You are allowed\n\
to confirm or set the correct system time at this point.\n\
   When you choose selection (1), the system will accept the displayed\n\
   system time.\n\
   When you choose selection (2), the Set Time screen is displayed \n\
   to allow you to set the system time.\n"

#define SETTIME_HELPSCR \
"\n\
The Set Time screen allows you to enter the current date and local\n\
time.  You can enter the current year, month, and day using either\n\
the names or their numeric equivalents in the format:\n\
        month/day/year\n\
or      mm/dd/yy\n\
Enter the local time in the format:\n\
        hours:minutes:seconds\n\
or      hh:mm:ss\n\
24-hour notation should be used in entering the time of day.  Use\n\
either the name of the month or its first three letters to specify\n\
the month.\n"

#define REBOOT_HELP \
"\n\
  A system reboot is necessary to properly initialize the system."

#define NO_MATCH_MSG \
"  No appropriate default configuration exists.\n\
   You must use the long-form suninstall program."

#define NO_CHOICE_MSG \
"  You must either make a selection from the \"INSTALLATION\n\
   TYPES\" menu, or hit the 'a' key to abort this program,\n\
   then use the long-form suninstall program to continue\n\
   this installation."


#define NO_CHOICE_FINAL_MSG \
"  This program requires that a choice be made from the \"INSTALLATION TYPES\"\n\
  menu.  Use the long-form suninstall program to continue this installation."

#define DATETIME_MMDDYY "                                               \n \
Enter the current date and local time (e.g. 03/09/88 12:20:30); the date\n \
may be in one of the following formats:                                 \n \
        mm/dd/yy                                                        \n \
        mm/dd/yyyy                                                      \n \
        dd.mm.yyyy                                                      \n \
        dd-mm-yyyy                                                      \n \
        dd-mm-yy                                                        \n \
        month dd, yyyy                                                  \n \
        dd month yyyy                                                   \n \
and the time may be in one of the following formats:                    \n \
        hh am/pm                                                        \n \
        hh:mm am/pm                                                     \n \
        hh:mm                                                           \n \
        hh.mm am/pm                                                     \n \
        hh.mm                                                           \n \
        hh:mm:ss am/pm                                                  \n \
        hh:mm:ss                                                        \n \
        hh.mm.ss am/pm                                                  \n \
        hh.mm.ss                                                        \n \
\n\
Example: To set for March 19, 1989 at 12:20 pm you could type:\n\
\n\
         >> 03/19/89 12:20:30\n\
\n\
\n\
>> "

#define DATETIME_DDMMYY "                                               \n \
Enter the current date and local time (e.g. 09/03/88 12:20:30); the date\n \
may be in one of the following formats:                                 \n \
        dd/mm/yy                                                        \n \
        dd/mm/yyyy                                                      \n \
        dd.mm.yyyy                                                      \n \
        dd-mm-yyyy                                                      \n \
        dd-mm-yy                                                        \n \
        month dd, yyyy                                                  \n \
        dd month yyyy                                                   \n \
and the time may be in one of the following formats:                    \n \
        hh am/pm                                                        \n \
        hh:mm am/pm                                                     \n \
        hh.mm                                                           \n \
        hh:mm am/pm                                                     \n \
        hh.mm                                                           \n \
        hh:mm:ss am/pm                                                  \n \
        hh:mm:ss                                                        \n \
        hh.mm.ss am/pm                                                  \n \
        hh.mm.ss                                                        \n \
Example: To set for March 19, 1989 at 12:20 pm you could type:\n\
\n\
	 >> 19/03/89 12:20:30\n\
\n\
\n\
>> "

#define PRESERVE_HELP\
"    Quick Install gives you the option of saving data partitions that\n\
    already exist on the installation disk.  If additional disks are\n\
    attached to the system, Quick Install does not affect them.\n\
 \n\
    If you enter 'n', Quick Install will not preserve any partitions\n\
    on the installation disk.  The contents of the disk will be erased\n\
    before installation begins.\n\
 \n\
    If you enter 'y', Quick Install will install software in the a: and\n\
    g: partitions, but preserve the other partitions.  Only the contents\n\
    of the a: and g: partitions will be erased before installation begins.\n\
 \n\
    -  If your home directory is currently in the g: partition, it will\n\
       be lost.  This commonly applies to small disks (130 MB or less).\n\
       Back up your home directory so you can restore it from media later.\n\
 \n\
    -  If your home directory is currently in the h: partition, it will   \n\
       be preserved.  This commonly applies to large disks (over 130 MB)."

#define PRESERVE_HELPSCREEN\
"\n\
If you're installing a system with a brand new disk, enter\n\
\"n\" because the disk has no data that needs to be preserved.\n\
 \n\
If you're reinstalling a system and you don't care about\n\
preserving the old data on the disk, enter \"n\".  (You may,\n\
for example, be reinstalling an old system that was given\n\
to you by another user.)\n\
 \n\
If you're reinstalling a system that previously ran the\n\
SunOS operating system, you will probably want to enter \"y\".\n\
Quick Install will only install software in two partitions:\n\
 \n\
   Partition a:   Contains the root, or /, filesystem.\n\
   Partition g:   On large disks, contains only /usr.\n\
                  On small disks, contains /usr and /home.\n\
 \n\
If your home directory is in partition g:, you will need\n\
to restore it from the backup media after the installation.\n\
"

#define CONFIRM_QUESTION "\n  Enter 'n' if you want to change some information.\n" 

#define ROOTPW_HELP \
"\n\
   For security reasons it is important to give a password to\n\
   the superuser (root) account.  A password should be six to \n\
   eight characters long.  To give a password to the root account,\n\
   enter the password below.  The password will not appear on the\n\
   screen as you type it.\n\n\
   Enter superuser password and press RETURN, or press RETURN to continue:\n\
\n"

#define ROOTPW_HELPSCREEN \
"\n\
Each user of your workstation must have a user account.\n\
The superuser account is an account on your system that has\n\
special privileges.  The user name of this account is\n\
\"root\".  You should use the root account only when you are\n\
doing administrative tasks on your system that require\n\
those special privileges.\n\
\n\
For security reasons it is important to give a password to the\n\
root account.  A password should include a mix of upper\n\
and lower case letters, numbers, and punctuation characters\n\
and should be six to eight characters long.\n\n\
The password will not appear on the screen as you type it.\n\
You will be asked to re-type the same password to ensure that you\n\
have not made any typographical errors.\n\
\n"  

#define USERACCT_HELP\
"\n\
  To use the system, a user account must be set up.  You can set up the account\n\
  now or separately after configuration.\n\n\
  The following information is needed prior to setting up the account:\n\n\
     User full name - the common name of the user, e.g. John Doe \n\
     User name - the login name of the user, e.g. jdoe\n\
     User id - the system numerical id of the user\n" 

#define USERACCT_HELPSCREEN \
"\n\
You must prepare for setting the user name and user id before\n\
proceeding with setting up the user account.\n\
\n"

#define USERACCT_FN_HELP\
"\n\
  The User's full name is a more verbose form to be used by some programs like\n\
  mailers.\n\n\
  Enter the user's full name and press RETURN:"

#define USERACCT_FN_HELPSCREEN \
"\n\
You must enter the full name of the person associated with the user\n\
\n\
account."


#define USERACCT_UN_HELP\
"\n\
  The user name (up to 8 lower-case letters, no spaces) is the name the\n\
  is known by on the system.\n\n\
  Enter the user name and press RETURN:"


#define USERACCT_UN_HELPSCREEN \
"\n\
Each user account must have a user name.  Your user name is the name you\n\
will be known by on the system.  The name may have up to 8 lower-case\n\
letters, but no spaces are permitted.  User names often consist of the\n\
user's last name and first initial.  For example, EdgarAllanPoe's username\n\
might be \"epoe\".  Local networks require that user names be unique\n\
throughout the entire network.\n"

#define USERACCT_UID_HELP\
"\n\
  The user id is a number between 10 and 60000 that is unique in the network.\n\
  Your system administrator can help confirm its uniqueness.\n\n\
  Enter the user id number and press RETURN:"

#define USERACCT_UID_HELPSCREEN \
"\n\
Every user account has a user identification number, or user ID. This\n\
number must have a value between 10 and 60000.  It must be unique\n\
among all users of your workstation and among all systems attached to \n\
your network.\n"

#define USERPW_HELP\
"\n\
  Passwords are used for security reasons, but are not required. You can enter\n\
  a password now, or you can also add one later.\n\n\
  Enter the user password, and press RETURN or press RETURN to continue:"

#define USERPW_HELPSCREEN\
"\n\
A password may consist of upper-case and lower-case letters, numbers,\n\
underscires(_), hyphens(-) or punctuation characters.\n\n\
It should be six to eight characters long.\n\
For security reasons, the password will not appear on the\n\
screen as you type it.\n"


/*
 * N.B. help for timezone is provided somewhat differently
 */

#define DEF_BANNER "STANDARD INSTALLATIONS "
#define CONF_BANNER "CONFIRMATION"
#define CONF_SCREEN "SysConfF1		       SYSTEM CONFIGURATION"
#define NETWORK_BANNER "NETWORK"
#define NETWORK_SCREEN "SysConfE1		      SYSTEM CONFIGURATION"
#define HOSTNAME_BANNER "HOSTNAME"
#define HOSTNAME_SCREEN "SysConfA1		      SYSTEM CONFIGURATION"
#define IP_BANNER "NETWORK ADDRESS"
#define IP_SCREEN "SysConfE2		      SYSTEM CONFIGURATION"
#define YP_BANNER "NIS NAME SERVICE"
#define YP_SCREEN "SysConfE3		      SYSTEM CONFIGURATION"
#define DOMAIN_BANNER "DOMAIN NAME"
#define DOMAIN_SCREEN "SysConfE4		      SYSTEM CONFIGURATION"
#define INST_BANNER "INSTALLATION"
#define INST_SCREEN "SysConfG1		      SYSTEM CONFIGURATION"
#define TZ_BANNER "TIME ZONE"
#define TZ_SCREEN "SysConfB1		      SYSTEM CONFIGURATION"
#define REBOOT_BANNER "REBOOT"
#define TIME_BANNER "SYSTEM TIME "
#define TIME_SCREEN "SysConfD1		     SYSTEM CONFIGURATION"
#define SET_TIME_BANNER "SET TIME"
#define SET_TIME_SCREEN "SysConfD2		      SYSTEM CONFIGURATION"
#define PRESERVE_BANNER "PRESERVE DATA PARTITIONS"

#define ROOTPW_BANNER "SUPERUSER PASSWORD"
#define ROOTPW_SCREEN "SysConfH1		      SYSTEM CONFIGURATION"
#define USERACCT_BANNER "USER ACCOUNT"
#define USERACCT_SCREEN "SysConfI1		      SYSTEM CONFIGURATION"
#define USERACCT_FN_BANNER "USER ACCOUNT - FULL NAME"
#define USERACCT_FN_SCREEN "SysConfI2		      SYSTEM CONFIGURATION"
#define USERACCT_UN_BANNER "USER ACCOUNT - USER NAME"
#define USERACCT_UN_SCREEN "SysConfI3		      SYSTEM CONFIGURATION"
#define USERACCT_UID_BANNER "USER ACCOUNT - USER ID"
#define USERACCT_UID_SCREEN "SysConfI4		      SYSTEM CONFIGURATION"
#define USERPW_BANNER "USER PASSWORD"
#define USERPW_SCREEN "SysConfI5		       SYSTEM CONFIGURATION"

#define	HOST_HELP_BANNER "HELP ABOUT SYSTEM NAME"
#define	TZ_HELP_BANNER "HELP ABOUT TIME ZONE"
#define	TIME_HELP_BANNER "HELP FOR SYSTEM TIME"
#define	IP_HELP_BANNER "HELP FOR NETWORK ADDRESS"
#define	DOMAIN_HELP_BANNER "HELP FOR DOMAIN NAME"
#define	PRESERVE_HELP_BANNER "HELP FOR PRESERVING DATA PARTITIONS"

#define DONT_CENTER 1000

