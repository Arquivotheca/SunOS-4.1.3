#! /bin/sh
#
#	%Z%%M% %I% %E% SMI
#
LOG=messages
cd /var/adm
test -f $LOG.2 && mv $LOG.2 $LOG.3
test -f $LOG.1 && mv $LOG.1 $LOG.2
test -f $LOG.0 && mv $LOG.0 $LOG.1
mv $LOG   $LOG.0
cp /dev/null $LOG
chmod 644    $LOG
#
LOGDIR=/var/log
LOG=syslog
if test -d $LOGDIR
then
	cd $LOGDIR
	if test -s $LOG
	then
		test -f $LOG.6 && mv $LOG.6  $LOG.7
		test -f $LOG.5 && mv $LOG.5  $LOG.6
		test -f $LOG.4 && mv $LOG.4  $LOG.5
		test -f $LOG.3 && mv $LOG.3  $LOG.4
		test -f $LOG.2 && mv $LOG.2  $LOG.3
		test -f $LOG.1 && mv $LOG.1  $LOG.2
		test -f $LOG.0 && mv $LOG.0  $LOG.1
		mv $LOG    $LOG.0
		cp /dev/null $LOG
		chmod 666    $LOG
		sleep 40
	fi
fi
#
kill -HUP `cat /etc/syslog.pid`
