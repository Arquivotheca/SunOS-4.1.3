#!/bin/csh -f
# 
#	%Z%%M% %I% %E%
#
echo
echo "*************************************************"
echo "* build started on `date` *"
echo "*************************************************"
echo
#
umask 002
cd /usr/src/sys
make -k clean
make -k all 
