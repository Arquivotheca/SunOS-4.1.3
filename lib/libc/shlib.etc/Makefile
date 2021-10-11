#
# @(#)Makefile 1.1 92/07/30 SMI
#


OBJSORT=objsort
AWKFILE=awkfile


libc.so:
	ld -assert pure-text `${OBJSORT} lorder-sparc tmp`
	/bin/ls /usr/lib/libc.so.* > TMP_FILE
	mv a.out libc.so.`cat TMP_FILE | awk -f ${AWKFILE}`
	echo libc.so.`cat TMP_FILE | awk -f ${AWKFILE}` built.

libcs5.so:
	ld -assert pure-text `${OBJSORT} lorder-sparc tmp`
	/bin/ls /usr/5lib/libc.so.* > TMP_FILE
	mv a.out libc.so.`cat TMP_FILE | awk -f ${AWKFILE}`
	echo libc.so.`cat TMP_FILE | awk -f ${AWKFILE}` built.



