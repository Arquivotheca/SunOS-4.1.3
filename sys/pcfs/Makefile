#
# @(#)Makefile 1.1 92/07/30 SMI
#
HFILES= pc_dir.h pc_fs.h pc_label.h pc_node.h

HDIR=$(DESTDIR)/usr/include/pcfs

all: $(HFILES)

clean:

install: $(HFILES)

install_h: $(HFILES)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
