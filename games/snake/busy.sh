#! /bin/sh
#
#	%Z%%M% %I% %E% SMI; from UCB
#
# This file must be here, or SNAKE will not run.
# It sets the increment to the priority level under which it runs.
set number=`who | wc -l`
echo "$number / 2" | bc
