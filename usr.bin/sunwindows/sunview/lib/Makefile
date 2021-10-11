#
# @(#)Makefile 1.1 92/07/30 SMI
#
TEXT_FILES= text_extras_menu rootmenu rootmenu.old suntools textswrc

.KEEP_STATE:

all: ${TEXT_FILES}

include ../../Makefile.master

fast_install: install
install: ${TEXT_FILES}
	-for i in ${TEXT_FILES} ; do \
	(	install -m 644 $$i ${DESTDIR}${PRODLIBDIR}/.$$i; \
		rm -f ${DESTDIR}${PRODLIBDIR}/$$i; \
		ln -s .$$i ${DESTDIR}${PRODLIBDIR}/$$i; \
	); done
	-ln -s .suntools ${DESTDIR}${PRODLIBDIR}/.sunview

clean: 
	rm -f *.o *.pg core errs ${TEXT_FILES}
