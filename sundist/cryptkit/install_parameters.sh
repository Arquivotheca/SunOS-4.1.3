:! /bin/sh
:
: @(#)install_parameters.sh 1.1 92/07/30
:       Copyright (c) 1988, Sun Microsystems, Inc.  All Rights Reserved.
:       Sun considers its source code as an unpublished, proprietary
:       trade secret, and it is available only under strict license
:       provisions.  This copyright notice is placed here only to protect
:       Sun in the event the source is deemed a published work.  Dissassembly,
:       decompilation, or other means of reducing the object code to human
:       readable form is prohibited by the license agreement under which
:       this code is provided to the user or company in possession of this
:       copy.
:
:       RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
:       Government is subject to restrictions as set forth in subparagraph
:       (c)(1)(ii) of the Rights in Technical Data and Computer Software
:       clause at DFARS 52.227-7013 and in similar clauses in the FAR and
:       NASA FAR Supplement.
: 
: File: install_parameters.sh
: 
: Description:  This is file used to customize the installation scripts
: it is used by install_unbundled and 2.0_Template (renamed to reflect
: product).
:
:####################################################################
:  PRE-DETERMINED VARIABLES  (That means you set these up)          #
:####################################################################
: Name of product, including release number.
PROD="4.0.3 Domestic Encryption Kit"

:   This is what you renamed 2.0_Template to.  It should reflect
:   the product name.
:   format of script_name = releaseno_productname_release-level
:               note: for "fcs" release-level is left blank.
:		examples: 1.1_NeWS, 2.0_LISP_beta
SCRIPT_NAME="4.0.3_Cryptkit"

:#### The following three are needed for Language products. #####
:###  Otherwise leave blank.                                #####
: PROD_NAME is the plain product name, in all *lower* case letters;
:       e.g. c, lint, pascal, fortran, modula2.
PROD_NAME=""
: PROD_RELEASE is the product's release number.
PROD_RELEASE=""
: PROD_BASE_SOS_RELEASE is the basic SunOS release number (only one) upon
:       which this release of the product is intended to run.
PROD_BASE_SOS_RELEASE=""
:####################################################################

: Required login for installation
:       none = doesn't matter who it is, otherwise put logname here
REQ_LOGIN="root"
 
: Compatible Operating systems
: Only list major OS releases only, not "dot,dot"s, for Sys4-3.2 use Sys4
: More than one OS can be listed, just put blanks between each (e.g. 
: SOS_COMPAT="4.0 4.1"
SOS_COMPAT="4.0.3"
 
: Compatible Sun Architechures
: Allows product to be installed only on these architectures or servers
: supporting these architectures.
:
: Use "sun2" "sun3" "sun4" "sun386" exactly the way `arch` prints it
: New arch types: hydra="sun3x" SunRay="sun4"  campus="sun4c"
ARCH_COMPAT=_ARCH_
 
: List the Required Hardware
:           No testing is done this is only displayed to the user.
HARD_REQ=""
 
: List the Optional Hardware
:           No testing is done this is only displayed to the user.
HARD_OPT=""
 
: list the Required Software
:           No testing is done this is only displayed to the user.
SOFT_REQ=""
 
: List the Optional Software (separate products)
:           No testing is done this is only displayed to the user.
:           Software that complements the software to be installed
SOFT_OPT=""
 
: Can DESTDIR be changed?
:        Only put "Y" if the destination directory can be selected by the
:        user.
:        This will only be changable for standalone installations.
:
DEST_SETABLE="N"
 
: DESTINATION DIRECTORY
:        Fill in a default destination directory.  This directory should
:        already be present on the system as a partition,  since the tar
:        file will create any further directories that are needed.
:        For most unbundled products DESTDIR="/usr" is correct since /usr is
:        usually linked to /export/exec/sun#.
:
DESTDIR="/usr"

: CHANGE PERMISSION
:       To the directory that will contain symbolic links to the
:       product located in $DESTDIR (Sun386i)
:       No modification is necessary to this variable.
:
MAKE_DIR_RW="/etc/mount -o remount,rw /usr"

: Size of product software (approx. kbytes)
: This figure is used to test for available space on the installation partition.
SOFT_SIZE=3000

: Approximate Time for Installation
:       Display information only
INSTALL_TIME="5 minutes"

: Choose a major dir/file to test existence of.  This is to check for a previous#  installation of this software.
:  Start path relative to DESTDIR (ex: lib/modula2)
SOFT_PRETEST="/bin/des"

: Fill in description of what the script does.  This will be displayed to
: the installer (so keep it clean and concise).
SCRIPT_DESC="This script installs the U.S. Domestic version of the \
Unix utilities which use DES encryption."

:########################################
:  OPTIONAL SOFTWARE RELEASE VARIABLES  #
:########################################

:   Flag Y=Optional Software on tape N=no optional software
OPT_REL="N"

:  Comment out or remove this section if there is no optional software #

:  Can destination directory for optional software be changed (Y|N)?
:OPT_DEST_SETABLE="N"

:  Fill in default destination directory for optional software
:OPT_DESTDIR="/usr"

:  Name of optional software on release tape ( third file on tape )
:OPT_LIST="Option software"

:   Size of optional software (kbytes)
:OPT_SIZE=5
