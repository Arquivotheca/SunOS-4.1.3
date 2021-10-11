#! /bin/sh

#
# to_41
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

FLAG=/ENVIRONMENT_IS_SUNOS4.1

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
# (2) Create /usr/ucblib if it does not exist
#
if test ! -d /usr/ucblib
then
	mkdir /usr/ucblib
fi

#
# (3) Check for the existence of the commands
#
for i in ${COMMANDS}
do
	if  test ! -f ${BIN}/${i}	#test for UCB
	then
		echo "the original ${BIN}/${i} does not exist."
		exit 1
	fi

	if  test ! -f ${V_BIN}/${i}	#test for SYSTEM V
	then
		echo "the original ${V_BIN}/${i} does not exist."
		exit 1
	fi
done

#
# (4) Now copy the files. The order is VERY important
#
for i in ${COMMANDS}
do
	cp ${BIN}/${i} ${UCB_BIN}	#BIN --> UCB
	mv ${BIN}/${i} ${BIN}/${i}.UCB	#save,,,
	cp ${V_BIN}/${i} ${BIN}		#5BIN --> BIN
					#don't save, leave them as they are,,
done

mv	${BIN}/cc	${UCB_BIN}	#BIN --> UCB
mv	${V_BIN}/cc	${BIN}		#5BIN --> BIN

#
# (5) Now /usr/5bin should be a symbolic lint to /usr/bin
#  rename /usr/5bin to /usr/5bin.SunOS40 and create a symbolic link
#
mv /usr/5bin	/usr/5bin.SunOS40	#rename /usr/5bin
ln -s /usr/bin	/usr/5bin		#create a symlink

#
# (6) Also /usr/5include will be renamed to /usr/5include.SunOS40
#  symbolic link will be made by installing header files
#
mv /usr/5include /usr/5include.SunOS40	#rename /usr/5include

#
# (7) Finished copy, create the FLAG
#
echo "The environment is set to  SunOS41" > ${FLAG}
date 				         >> ${FLAG}
exit 0

