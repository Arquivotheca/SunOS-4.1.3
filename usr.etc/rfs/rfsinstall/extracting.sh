#! /bin/sh
#
#  extract tapedev fsf bs keywords tapeserver 
#
#	%Z%  %M% %I% %E%
#
#       Copyright (c) 1987 by Sun Microsystems, Inc.
#

cmdname=$0
tapedev=$1
fsf=$2
bs=$3
keywords=$4
tapeserver=$5

EXTRACTMSG="Extracting \"${keywords}\" files from \"${tapedev}\" release tape."

if [ $# -lt 4 -o $# -gt 5 ]; then
	echo Usage: $0 tapedev fsf bs keywords tapeserver 
	exit 1
fi
if [ "${tapeserver}" != "" ]; then
        remote="rsh ${tapeserver} -n"
fi

set +x
${remote} mt -f ${tapedev} fsf ${fsf}
echo; echo "${EXTRACTMSG}" 
if [ "${tapeserver}" = "" ]; then
	tar xpfb ${tapedev} ${bs}
else
	${remote} dd if=${tapedev} bs=${bs}b 2>/dev/null | tar xpfB - 2>/dev/null
fi
sync; sync
set -x
