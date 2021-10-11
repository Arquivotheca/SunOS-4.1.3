#! /bin/sh
#
# verify_tapevol_arch arch tapen tapedev tapeserver
#
#	%Z%  %M% %I% %E%
#
#       Copyright (c) 1986 by Sun Microsystems, Inc.
#

ARCH=$1
TAPEN=$2 
TAPEDEV=$3
TAPESERVER=$4
ARCH_TAPE=""
TAPE_NUM=""

if [ $# -lt 3 -o $# -gt 4 ]; then
        echo Usage: $0 arch tapen tapedev tapeserver
        exit 1
fi
if [ "${TAPESERVER}" != "" ]; then
        REMOTE="rsh ${TAPESERVER} -n"
fi

while true; do
	while true; do
		STRING=`${REMOTE} mt -f ${TAPEDEV} rew 2>&1`
		if [ -z "${STRING}" ]; then
	   		${REMOTE} mt -f ${TAPEDEV} fsf 1
			break
		else
	   		echo "${STRING}"
	   		echo "Tape drive ${TAPEDEV} not ready."
			echo
                	echo -n "Load release tape #${TAPEN} for architecture ${ARCH}: "
                	read x
		fi
	done
	#
	# get TOC from release tape(dd format)
	#
	rm -f /tmp/TOC
	if [ "${REMOTE}" = "" ]; then
		dd if=${TAPEDEV} > /tmp/TOC 2>/dev/null
	else
		${REMOTE} "dd if=${TAPEDEV}" > /tmp/TOC 2>/dev/null
	fi
	ARCH_TAPE=`awk '/^sun/ { print $1 }' < /tmp/TOC`
	TAPE_NUM=`awk '/^sun/ { print $3 }' < /tmp/TOC`
	if [ "${ARCH_TAPE}" != "${ARCH}" -o "${TAPE_NUM}" != "${TAPEN}" ]; then
               	echo
		echo -n "Load release tape #${TAPEN} for architecture ${ARCH}: "
                read x
	else 
		break
	fi
done
${REMOTE} mt -f ${TAPEDEV} rew 2>&1
sync; sync
