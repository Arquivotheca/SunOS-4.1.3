#! /bin/sh
#
# %Z%%M% %I% %E% Copyr 1990 Sun Microsystems, Inc.  
#
# ypxfr_1perhour.sh - Do hourly NIS map check/updates
#

PATH=/bin:/usr/bin:/usr/etc:/usr/etc/yp:$PATH
export PATH

# set -xv
ypxfr passwd.byname
ypxfr passwd.byuid 
