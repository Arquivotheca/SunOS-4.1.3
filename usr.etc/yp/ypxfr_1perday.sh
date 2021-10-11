#! /bin/sh
#
# %Z%%M% %I% %E% Copyr 1990 Sun Microsystems, Inc.  
#
# ypxfr_1perday.sh - Do daily NIS map check/updates
#

PATH=/bin:/usr/bin:/usr/etc:/usr/etc/yp:$PATH
export PATH

# set -xv
ypxfr group.byname
ypxfr group.bygid 
ypxfr protocols.byname
ypxfr protocols.bynumber
ypxfr networks.byname
ypxfr networks.byaddr
ypxfr services.byname
ypxfr ypservers
