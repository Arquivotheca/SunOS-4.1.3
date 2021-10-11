#!/bin/sh
#%Z%%M% %I% %E% SMI
#
#
# prints todays or yesterdays date in the format mm_dd_yy
#

USAGE="usage: $0 <today | yesterday>"
yesterday () {
	date | awk '
	BEGIN{
		m["Jan"] = 1; m["Feb"] = 2; m["Mar"] = 3; m["Apr"] = 4
	  	m["May"] = 5; m["Jun"] = 6; m["Jul"] = 7; m["Aug"] = 8
	  	m["Sep"] = 9; m["Oct"] = 10; m["Nov"] = 11; m["Dec"] = 12
	  	d[1] = 31; d[2] = 28; d[3] = 31; d[4] = 30; d[5] = 31; d[6] = 30
	  	d[7] = 31; d[8] = 31; d[9] = 30; d[10] = 31; d[11] = 30; d[12] = 31
	}

	{
	yr = $6; mon = m[$2]; day = $3
	if (yr % 4 == 0 && (yr % 100 != 0 || yr % 400 == 0))
		d[2] += 1
	day -= 1
	if (day == 0)
	{
		mon -= 1;
		if (mon == 0)
		{
			mon = 12
			yr -= 1
		}
		day = d[mon]
	}
	printf("%d%d_%d%d_%d\n",mon / 10, mon % 10, \
		                day / 10, day % 10, yr % 100);
	}'
}

# Program:

[ -z "$1" ]  && {
	echo $USAGE
	exit 1
}

case "$1" in
	today)
		date '+%m_%d_%y';exit 0;;
	yesterday)
		yesterday;exit 0;;
	*)
		echo $USAGE;exit 1;;
esac
