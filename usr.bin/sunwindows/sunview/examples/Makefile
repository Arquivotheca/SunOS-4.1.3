#
#       @(#)Makefile 1.1 92/07/30 Copyr 1986 Sun Micro
#		
#	Internal Makefile -- does not get sent to customer.
#
CFILES= filer.c image_browser_2.c resize_demo.c \
	coloredit.c animatecolor.c tty_io.c font_menu.c \
	dctool.c typein.c bounce.c spheres.c \
	seln_demo.c confirm.c
OBJS= $(CFILES:%.c=%.o)
BINS_UNIQUE= spheres
BINS=   filer image_browser_2 resize_demo coloredit animatecolor \
	tty_io font_menu dctool typein bounce seln_demo canvas_input \
	addnewtest canvas_repaint image_browser_1 loopback \
	menugenproc showcolor simple_panel

.KEEP_STATE:

include ../../Makefile.master

all: ${BINS} ${BINS_UNIQUE}

${BINS}: $$@.c
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} $@.c -o $@ ${LIBS}
	
spheres: spheres.icon spheres.c 
	${CC} ${LDFLAGS} ${CFLAGS_NOT_ROI_DATA} -o spheres spheres.c ${LIBS}

fast_install:

install: install_sources

install_sources: Makefile.customer spheres.icon ${CFILES} 
	-install -d -m 755 ${DESTDIR}${SRCDIR}/examples
	-install -m 644 Makefile.customer ${DESTDIR}${SRCDIR}/examples/Makefile
	-@for i in ${CFILES} spheres.icon; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${SRCDIR}/examples/$$i ]; then \
			rm -f ${DESTDIR}${SRCDIR}/examples/$$i; \
		fi; \
		install -m 444 $$i ${DESTDIR}${SRCDIR}/examples/$$i; \
	done

clean:
	rm -f ${OBJS} ${BINS} ${BINS_UNIQUE}
