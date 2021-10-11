# called by:  newvers.sh <release_file> <arch>

PATH=/usr/ucb:/bin:/usr/bin

if [ ! -r version ]; then echo 0 > version; fi
touch version

VERS=`expr \`cat version\` + 1`
OS="SunOS"
# Temporary workaround - partial initialization of uname "SunOS" string 
# because some install scripts assume single "SunOS" in kernel.
# X_OS to be replaced by OS later
X_OS="sunOS"
ARCH=$2
# Truncate release field to 8 chars in utsname
RELEASE=`expr \`cat $1\` : '\(..\{0,7\}\)'`

echo '#include <sys/utsname.h>' > vers.c
echo >> vers.c

echo $VERS `basename \`pwd\`` `cat $1` | \
awk '	{	version = $1; system = $2; release = $3; }\
END	{	printf "char version[] = \"SunOS Release %s (%s) #%d: ", release, system, version >> "vers.c";\
		printf "%d\n", version > "version"; }' 
echo `date`'\nCopyright (c) 1983-1992, Sun Microsystems, Inc.\n";' >> vers.c

echo >> vers.c

# Initialize the utsname structure
echo $X_OS  $RELEASE  $VERS $ARCH | \
awk '	{	sysname = $1; release = $2; version = $3; arch = $4; }\
END	{	printf "struct utsname utsname =\n\t{ \"%s\", \"\", \"\", \"%s\", \"%s\", \"%s\" };\n\n", \
sysname, release, version, arch >> "vers.c" ; } '

