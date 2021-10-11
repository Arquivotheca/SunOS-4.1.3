#!/bin/csh -f
#	%Z%%M% %I% %E%
cd $1
if (-d SCCS) then
	sccs clean
endif
