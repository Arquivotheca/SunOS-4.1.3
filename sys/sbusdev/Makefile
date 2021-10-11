#
# @(#)Makefile 1.1 92/07/30 SMI
#
HFILES=aclone.h audio_79C30.h audiodebug.h audiovar.h cg12reg.h dbri_reg.h  \
	dbrivar.h dmaga.h fdreg.h fdvar.h gtmcb.h gtreg.h gtseg.h memfb.h  \
	mmcodec_reg.h bpp_io.h
	

HDIR=$(DESTDIR)/usr/include/sbusdev

all: $(HFILES)

clean:

install: $(HFILES)

install_h: $(HFILES)
	install -d -o bin -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)

