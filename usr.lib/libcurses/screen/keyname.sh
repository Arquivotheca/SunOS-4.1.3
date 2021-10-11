#! /bin/sh
#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
#	%Z%%M% %I% %E% SMI; from S5R3.1 1.3
#
rm -f keyname.c
echo "#include	\"curses_inc.h\"" > keyname.c
echo "" >>keyname.c
echo "static	char	*keystrings[] =" >>keyname.c
echo "		{" >> keyname.c
{
    grep -v 'KEY_F(' keycaps | awk '{ print $5, $4 }' | sed -e 's/,//g' -e 's/KEY_//'
    # These three aren't in keycaps
    echo '0401 BREAK
0530 SRESET
0531 RESET'
} |  sort -n | awk '
    {
	print "\t\t    \"" $2 "\",	/* " $1 " */"
    }
' >> keyname.c

LAST=`tail -1 keyname.c | awk -F'"' '{print $2}'`
cat << ! >> keyname.c
		};

char	*keyname(key)
int	key;
{
    static	char	buf[16];

    if (key >= 0400)
    {
	register	int	i;

	if ((key == 0400) || (key > KEY_${LAST}))
	    return ("UNKNOWN KEY");
	if (key > 0507)
	    i = key - (0401 + ((0507 - 0410) + 1));
	else
	    if (key >= 0410)
	    {
		(void) sprintf(buf, "KEY_F(%d)", key - 0410);
		goto ret_buf;
	    }
	    else
		i = key - 0401;
	(void) sprintf(buf, "KEY_%s", keystrings[i]);
	goto ret_buf;
    }

    if (key >= 0200)
    {
	if (SHELLTTY.c_cflag & CS8)
	    (void) sprintf(buf, "%c", key);
	else
	    (void) sprintf(buf, "M-%s", unctrl(key & 0177));
	goto ret_buf;
    }

    if (key < 0)
    {
	(void) sprintf(buf, "%d", key);
ret_buf:
	return (buf);
    }

    return (unctrl(key));
}
!
exit 0
