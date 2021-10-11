#! /bin/sh
#     %Z%%M% %I% %E% SMI

USAGE="Usage: $0 [ -k | archname ]"

case $# in
0)      OP=major;;
1)      case $1 in
        -k)     OP=minor;;
        sun*)   OP=compat;;
        *)      echo $USAGE;
                exit 1;;
        esac;;
*)      echo $USAGE;
        exit 1;;
esac


if [ -f /bin/sun4c ] && /bin/sun4c; then
        MINOR=sun4c
elif [ -f /usr/kvm/sun4m ] && /usr/kvm/sun4m; then
        MINOR=sun4m
elif [ -f /bin/sun4 ] && /bin/sun4; then
        MINOR=sun4
elif [ -f /bin/sun3 ] && /bin/sun3; then
        MINOR=sun3
elif [ -f /bin/sun3x ] && /bin/sun3x; then
        MINOR=sun3x
elif [ -f /bin/sun386 ] && /bin/sun386; then
        MINOR=sun386
elif [ -f /bin/sun2 ] && /bin/sun2; then
        MINOR=sun2
else
        MINOR=unknown
fi


case $MINOR in
sun2)   MAJOR=sun2;;
sun386) MAJOR=sun386;;
sun3*)  MAJOR=sun3;;
sun4*)  MAJOR=sun4;;
*)      MAJOR=unknown;;
esac

case $OP in
major)  echo $MAJOR;;
minor)  echo $MINOR;;
compat) [ $1 = $MAJOR ] ; exit ;;
esac

exit 0
