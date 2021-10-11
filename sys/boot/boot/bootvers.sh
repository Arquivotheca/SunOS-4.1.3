# called by:  bootvers.sh <release_file> <arch>

PATH=/usr/ucb:/bin:/usr/bin

if [ ! -r version ]; then echo 0 > version; fi
touch version

VERS=`expr \`cat version\` + 1`

echo $VERS `basename \`pwd\`` `cat $1` | \
awk '	{	version = $1; system = $2; release = $3; }\
END	{	printf "char version[] = \"Boot Release %s (%s) #%d: ", release, system, version > "vers.c";\
		printf "%d\n", version > "version"; }' 
echo `date`'\nCopyright (c) 1983-1990, Sun Microsystems, Inc.\n";' >> vers.c

echo >> vers.c


