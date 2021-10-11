#!/bin/sh
#
# %Z%%M% %I% %E% SMI
# from install_unbundled 2.1 10/7/88
#       Copyright (c) 1988-92 Sun Microsystems, Inc.  All Rights Reserved.
#       Sun considers its source code as an unpublished, proprietary
#       trade secret, and it is available only under strict license
#       provisions.  This copyright notice is placed here only to protect
#       Sun in the event the source is deemed a published work.  Dissassembly,
#       decompilation, or other means of reducing the object code to human
#       readable form is prohibited by the license agreement under which
#       this code is provided to the user or company in possession of this
#       copy.
#
#       RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
#       Government is subject to restrictions as set forth in subparagraph
#       (c)(1)(ii) of the Rights in Technical Data and Computer Software
#       clause at DFARS 52.227-7013 and in similar clauses in the FAR and
#       NASA FAR Supplement.
#
# File: install_unbundled
# 
# Description: 
# 	This script begins installation by calling the template script
#	and logging the session.
#	This can be executed directly or called from extract_unbundled.
#	No modification to this script is necessary.
#
#	SMAR
##########################################################################
# install_dir does not change
PATH=/usr/ucb:/bin:/usr/bin:/etc:/usr/etc
export PATH
install_dir="/var/tmp/unbundled"
export install_dir
SCRIPT_NAME="4.1.3_US_Encryption"
export SCRIPT_NAME

LOGFILE="/var/tmp/${SCRIPT_NAME}.log"
export LOGFILE

# execute script and redirect output to $script_name.log
echo "Invoking ${install_dir}/${SCRIPT_NAME}; log file is $LOGFILE"
> $LOGFILE
${install_dir}/${SCRIPT_NAME} $* 2>&1 | tee -a $LOGFILE
