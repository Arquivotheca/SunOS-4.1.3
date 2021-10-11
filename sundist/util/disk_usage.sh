#! /bin/sh
#
# %Z%%M% %I% %E% SMI
#
# report on disk usage in /proto for different exclude lists
#

TMPFILE=/tmp/disk.usage.$$
ROOTSUM=0
USRSUM=0
if [ $PROTO"x" = "x" ]; then
	PROTO=/proto
fi
cd /usr/src/sundist/exclude_lists

for LIST in $*; do

	if expr match "${LIST}" "usr." > /dev/null; then
		DIR=$PROTO/usr
	else
		DIR=$PROTO
	fi

	SUM=0
	while read FILE; do
		SIZE=`(cd ${DIR};  du -s $FILE) | sed 's/	.*//'` 
		SUM=`expr ${SUM} + ${SIZE}`
		echo "${LIST}:	${SUM} Kbytes"
	done < ${LIST} > ${TMPFILE}

	tail -1 ${TMPFILE}

	SUMLINE=`tail -1 ${TMPFILE}`
	if [ "${DIR}" = "$PROTO/usr" ]; then
                SUM=`expr match "${SUMLINE}" ".*:	\([0-9]*\) Kbytes"`
                USRSUM=`expr ${USRSUM} + ${SUM}`
	else
                SUM=`expr match "${SUMLINE}" ".*:	\([0-9]*\) Kbytes"`
                ROOTSUM=`expr ${ROOTSUM} + ${SUM}`
	fi

done

echo
echo "root total:	${ROOTSUM} Kbytes"
echo
echo "usr total:	${USRSUM} Kbytes"
echo

rm -fr ${TMPFILE}
