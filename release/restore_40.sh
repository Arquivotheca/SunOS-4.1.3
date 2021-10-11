#! /bin/sh

#
# restore_40
#	This command restores the SunOS40 environment modified by
#	to_41 command.
#
# See also
#	to_41
#

V_BIN=/usr/5bin
BIN=/usr/bin
UCB_BIN=/usr/ucb

COMMANDS='echo cat ls m4'
COMPILE=cc

FLAG=/ENVIRONMENT_IS_SUNOS4.1

#
# (1) Which environment ??
#
if  test ! -f ${FLAG}		# The environment is not SunOS41
then
	echo "The environment is SunOS40 already"
	exit 0
fi

#
# (2) Now restore the commands and compiler
#
for i in ${COMMANDS}
do
	rm -f ${UCB_BIN}/${i}		#remove from ucb
	mv ${BIN}/${i}.UCB ${BIN}/${i}	#rename
done

mv	${BIN}/cc	/usr/5bin.SunOS40
mv 	${UCB_BIN}/cc	${BIN}/cc	#Now restore the old one

#
# (3) Now restore /usr/5bin
#
rm -f /usr/5bin				#This is a symlink to /usr/bin
mv /usr/5bin.SunOS40	/usr/5bin	#rename

#
# (4) Now restore /usr/5include
#
rm -f /usr/5include			#This should be a symlink to /usr/include
mv /usr/5include.SunOS40 /usr/5include	#rename

#
# (5) Now remove the FLAG
#
rm -f ${FLAG}
exit 0

