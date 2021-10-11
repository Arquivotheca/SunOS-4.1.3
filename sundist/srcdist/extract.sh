#!/bin/sh
#
# %Z%%M% %I% %E%
#

mt -f /dev/nrst8 asf 2
dd if=/dev/rst8 bs=20b | uncompress -c | tar xfp -
