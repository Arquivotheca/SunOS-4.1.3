#! /bin/sh
# %Z%%M% %I% %E% SMI; from UCB 4.2
PATH=/bin:/usr/bin:/usr/etc:$PATH
export PATH
cp /dev/null /fastboot
halt $*
