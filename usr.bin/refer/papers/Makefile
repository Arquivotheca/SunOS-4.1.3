#
# @(#)Makefile 1.1 92/07/30 SMI
#
PAPERS= Rbstjissue Rv7man runinv Ind.ia Ind.ib Ind.ic 

.DEFAULT:
	sccs get $@

all: $(PAPERS)

install: all
	for i in $(PAPERS); do \
		(install -c -m 644 $$i ${DESTDIR}/usr/dict/papers); done
