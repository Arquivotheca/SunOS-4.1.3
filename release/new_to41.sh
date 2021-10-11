#! /bin/sh

#
# new_to41
#	This command change the SunOS40 environment to the environment
#	which is expected to build SunOS41
#
# See also
#	restore_40
#


V_BIN=/usr/5bin
BIN=/usr/bin
UCB_BIN=/usr/ucb

COMMANDS='echo cat ls m4'

FLAG=/NEW_ENVIRONMENT_IS_SUNOS4.1

#
# (1) Which environment am I now ?
#
if test -f ${FLAG}
then
	echo "The environment is SunOS41 already."
	cat ${FLAG}
	exit 0
fi

#
# (2) Create a symlink, /usr/ucblib
#
rm -f /usr/ucblib
ln -s /usr/lib /usr/ucblib

rm -f /usr/ucbinclude
ln -s /usr/include /usr/ucbinclude

#
# (3) Check for the existence of the commands
#		NOOP

#
# (4) Now copy the files. The order is VERY important
#
for i in ${COMMANDS}
do
	cp ${BIN}/${i} ${UCB_BIN}	#BIN --> UCB
done

rm -f 	/usr/ucb/cc
ln -s	/usr/lib/compile /usr/ucb/cc

#
# (5) Now /usr/5bin should be a symbolic lint to /usr/bin
#  rename /usr/5bin to /usr/5bin.SunOS40 and create a symbolic link
#		NOOP

#
# (6) Also /usr/5include will be renamed to /usr/5include.SunOS40
#  symbolic link will be made by installing header files
#		NOOP

#
# (7) Finished copy, create the FLAG
#
echo "The environment is set to  SunOS41(NEW, BACK to BSD PRE)" > ${FLAG}
date 				         >> ${FLAG}
exit 0

