#
# @(#)Makefile 1.11 88/02/08 SMI
#

CLEANFILES= cache.h cache_common.c cache_common.o dynamic.h ldconfig \
	ldconfig.c ldconfig.o reloc_info.h rtld.h verscmp.c verscmp.o

all:	xall

include ./MakeCommon

xall:	$(HDRS) $(BINS)


install: 
	install -s -m 755 ldconfig ${DESTDIR}/toolkit/ldconfig

clean:	
	for i in ${CLEANFILES}; do \
		rm -f $$i; done


