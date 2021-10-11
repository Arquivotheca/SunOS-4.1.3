# @(#)Makefile 1.1 92/07/30 SMI

FILES=	tekfont0 tekfont1 tekfont2 tekfont3

all: $(FILES)

install: $(FILES)
	install -d -o bin -m 755 ${DESTDIR}/usr/lib/fonts/tekfonts
	install -m 644 $(FILES) ${DESTDIR}/usr/lib/fonts/tekfonts

