#! /bin/sh
# D. DiGiacomo
#%Z%%M% %I% %E% SMI

# install conveniences on release machines

if [ `whoami` != root ] ; then
	echo "must be root"
	exit 1
fi

# man pages

if [ ! -f /usr/man/Makefile -a ! -d /usr/man/SCCS/. ] ; then
	echo "installing link from /usr/man to /usr/src/man"
	mv /usr/man /usr/man-
	ln -s src/man /usr/man
fi

if [ ! -x /usr/adm/manup ] ; then
	echo "installing /usr/adm/manup"

cat > /usr/adm/manup << 'blart'
#! /bin/sh
#
# update man pages
#
PATH="$PATH:/etc:/usr/etc" export PATH

for dir in `echo ${@-${MANPATH-/usr/man}} | tr ':' ' '`
do
	if cd $dir ; then
		if [ -f Makefile -o -f makefile ] ; then
			make -k
		fi

		# delete obsolete cat files
		for mdir in man?
		do
			cdir=cat`expr $mdir : 'man\(.*\)'`
			(cd $cdir && find . -xdev -type f -print) |
			while read page
			do
				if [ ! -f $mdir/$page ] ; then
					echo rm $cdir/$page
					rm $cdir/$page
				fi
			done
		done

		catman -M $dir
	fi
done
blart

	chmod 755 /usr/adm/manup
	echo >> /usr/adm/manup.errs
	chmod 666 /usr/adm/manup.errs
fi

tab=/tmp/cron.$$
crontab -l > $tab
update=n

if grep -sv manup $tab ; then
	echo "adding manup to crontab"
	cat >> $tab <<- 'blart'
	#
	# update man pages
	11 05 * * 7 umask 2; /usr/adm/manup >> /usr/adm/manup.errs 2>&1
	blart
	update=y
fi

# fast find

echo >> /usr/adm/find.errs
chmod 666 /usr/adm/find.errs

if grep -sv updatedb $tab ; then
	echo "adding updatedb to crontab"
	cat >> $tab <<- 'blart'
	#
	# rebuild fast find database
	03 06 * * * /usr/lib/find/updatedb -n /usr/src/SCCS_DIRECTORIES /usr/src /usr/release >> /usr/adm/find.errs 2>&1
	blart
	update=y
fi

if [ $update = y ] ; then
	echo "updating root crontab"
	crontab $tab
fi

rm -f $tab

# initial run
echo "The rest of this script takes a long time... run in background"
echo ""

echo "building fast find database..."
(nice /usr/lib/find/updatedb -n /usr/src/SCCS_DIRECTORIES \
	/usr/src /usr/release >> /usr/adm/find.errs 2>&1)

echo "updating man pages..."
(umask 2; nice /usr/adm/manup >> /usr/adm/manup.errs 2>&1)

echo "done"

