#! /bin/sh
#
#	%Z%%M% %I% %E% SMI; from S5R2 1.2
#
PATH=/bin:/usr/bin
if test -z "$3" -o "$3" = "$1" -o "$3" = "$2"; then
	echo "usage: diffmk name1 name2 name3 -- name3 must be different"
	exit 1
fi
diff -e $1 $2 | (sed -n -e '
/[ac]$/{
	p
	a\
.mc |
: loop
	n
	/^\.$/b done1
	p
	b loop
: done1
	a\
.mc\
.
	b
}

/d$/{
	s/d/c/p
	a\
.mc *\
.mc\
.
	b
}'; echo '1,$p') | ed - $1| sed -e '
/^\.TS/,/.*\. *$/b pos
/^\.T&/,/.*\. *$/b pos
p
d
:pos
/^\.mc/d
' > $3
