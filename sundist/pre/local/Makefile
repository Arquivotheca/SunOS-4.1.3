#
# @(#)Makefile 1.1 92/07/30
#

DESTDIR=
SCRIPTS = disk_build ft_build sat_build tape_build
UTILS = Copyright_sun4c Copyright_sun3x

all: $(SCRIPTS) $(UTILS)

install: all
	install -o root -g daemon $(SCRIPTS) $(DESTDIR)

clean:
	rm -f $(SCRIPTS) $(UTILS)
